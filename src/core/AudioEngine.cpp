#ifndef NOMINMAX
#define NOMINMAX
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "AudioEngine.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace ASIS {

namespace {

// Helper para escribir un archivo WAV mono de 16-bit a 44100 Hz
bool writeWavFile(const std::string& path, const std::vector<int16_t>& pcm, int sampleRate = 44100) {
    std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path());

    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;

    uint32_t dataSize = static_cast<uint32_t>(pcm.size() * sizeof(int16_t));
    uint32_t chunkSize = 36 + dataSize;
    uint32_t subChunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1; // Mono
    uint32_t byteRate = sampleRate * numChannels * sizeof(int16_t);
    uint16_t blockAlign = numChannels * sizeof(int16_t);
    uint16_t bitsPerSample = 16;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&chunkSize), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    out.write(reinterpret_cast<const char*>(&subChunk1Size), 4);
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
    out.write(reinterpret_cast<const char*>(pcm.data()), dataSize);

    return true;
}

// Generador de número pseudo-aleatorio entre -1.0 y 1.0
float randomFloat() {
    return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
}

} // namespace

struct AudioEngine::AudioImpl {
    ma_engine engine;
    bool engineInitialized = false;

    // Sonidos pre-cargados
    std::unordered_map<std::string, std::string> soundPaths;

    // Emisores activos para fuego y ambiente
    ma_sound fireSound;
    bool fireSoundActive = false;

    ma_sound ambientSound;
    bool ambientSoundActive = false;

    // Pool de voces persistentes en el heap para evitar corrupción de memoria en hilos de miniaudio
    static const int MAX_VOICES = 32;
    struct Voice {
        ma_sound sound;
        bool inUse = false;
        bool initialized = false;
    };
    Voice voices[MAX_VOICES];
    int nextVoiceIndex = 0;

    ma_sound* acquireVoice() {
        int idx = nextVoiceIndex;
        nextVoiceIndex = (nextVoiceIndex + 1) % MAX_VOICES;
        Voice& v = voices[idx];
        if (v.initialized) {
            ma_sound_stop(&v.sound);
            ma_sound_uninit(&v.sound);
            v.initialized = false;
        }
        return &v.sound;
    }

    void markVoiceActive(ma_sound* pSound) {
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (&voices[i].sound == pSound) {
                voices[i].initialized = true;
                voices[i].inUse = true;
                break;
            }
        }
    }
};

AudioEngine& AudioEngine::get() {
    static AudioEngine instance;
    return instance;
}

AudioEngine::AudioEngine()
    : m_initialized(false),
      m_isMuted(false),
      m_masterVolume(1.0f),
      m_sfxVolume(0.9f),
      m_footstepsVolume(0.85f),
      m_musicVolume(0.6f),
      m_ambientEnabled(true),
      m_listenerPos(0.0f),
      m_listenerForward(0.0f, 0.0f, -1.0f),
      m_listenerUp(0.0f, 1.0f, 0.0f),
      m_impl(new AudioImpl())
{
}

AudioEngine::~AudioEngine() {
    shutdown();
    delete m_impl;
}

bool AudioEngine::init() {
    if (m_initialized) return true;

    ma_result result = ma_engine_init(NULL, &m_impl->engine);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine ERROR] No se pudo inicializar miniaudio. Código: " << result << std::endl;
        return false;
    }
    m_impl->engineInitialized = true;

    // Directorio de almacenamiento para audios generados
    std::string audioDir = "assets/audio/generated/";
    std::filesystem::create_directories(audioDir);

    // 1. Generar SFX procedurales en disco si no existen
    auto genSound = [&](const std::string& name, const std::string& filename, auto generatorFunc) {
        std::string fullPath = audioDir + filename;
        if (!std::filesystem::exists(fullPath)) {
            auto pcm = generatorFunc();
            writeWavFile(fullPath, pcm);
        }
        m_impl->soundPaths[name] = fullPath;
    };

    genSound("footstep_grass", "footstep_grass.wav", [&]() { return generateWavFootstep(FootstepSurface::GRASS); });
    genSound("footstep_dirt", "footstep_dirt.wav", [&]() { return generateWavFootstep(FootstepSurface::DIRT); });
    genSound("footstep_rock", "footstep_rock.wav", [&]() { return generateWavFootstep(FootstepSurface::ROCK); });
    genSound("footstep_snow", "footstep_snow.wav", [&]() { return generateWavFootstep(FootstepSurface::SNOW); });
    genSound("footstep_concrete", "footstep_concrete.wav", [&]() { return generateWavFootstep(FootstepSurface::CONCRETE); });

    genSound("jump", "jump.wav", [&]() { return generateWavJump(); });
    genSound("land", "land.wav", [&]() { return generateWavLand(FootstepSurface::GRASS); });
    genSound("impact", "impact.wav", [&]() { return generateWavImpact(); });
    genSound("fire_crackle", "fire_crackle.wav", [&]() { return generateWavFireCrackle(); });
    genSound("ambient_drone", "ambient_drone.wav", [&]() { return generateWavAmbientTone(); });

    // Inicializar sonido de fuego 3D en bucle
    if (std::filesystem::exists(m_impl->soundPaths["fire_crackle"])) {
        if (ma_sound_init_from_file(&m_impl->engine, m_impl->soundPaths["fire_crackle"].c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &m_impl->fireSound) == MA_SUCCESS) {
            ma_sound_set_looping(&m_impl->fireSound, MA_TRUE);
            ma_sound_set_spatialization_enabled(&m_impl->fireSound, MA_TRUE);
            ma_sound_set_min_distance(&m_impl->fireSound, 2.0f);
            ma_sound_set_max_distance(&m_impl->fireSound, 25.0f);
            ma_sound_set_attenuation_model(&m_impl->fireSound, ma_attenuation_model_inverse);
            ma_sound_set_volume(&m_impl->fireSound, m_sfxVolume * m_masterVolume * 0.7f);
            ma_sound_start(&m_impl->fireSound);
            m_impl->fireSoundActive = true;
        }
    }

    // Inicializar sonido de ambiente en bucle
    if (std::filesystem::exists(m_impl->soundPaths["ambient_drone"])) {
        if (ma_sound_init_from_file(&m_impl->engine, m_impl->soundPaths["ambient_drone"].c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &m_impl->ambientSound) == MA_SUCCESS) {
            ma_sound_set_looping(&m_impl->ambientSound, MA_TRUE);
            ma_sound_set_volume(&m_impl->ambientSound, m_musicVolume * m_masterVolume * 0.45f);
            if (m_ambientEnabled) {
                ma_sound_start(&m_impl->ambientSound);
                m_impl->ambientSoundActive = true;
            }
        }
    }

    m_initialized = true;
    std::cout << "[AudioEngine] ✓ Sistema de Audio 3D Espacial inicializado (miniaudio).\n";
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;

    if (m_impl->engineInitialized) {
        for (int i = 0; i < AudioImpl::MAX_VOICES; ++i) {
            if (m_impl->voices[i].initialized) {
                ma_sound_stop(&m_impl->voices[i].sound);
                ma_sound_uninit(&m_impl->voices[i].sound);
                m_impl->voices[i].initialized = false;
            }
        }
        if (m_impl->fireSoundActive) {
            ma_sound_stop(&m_impl->fireSound);
            ma_sound_uninit(&m_impl->fireSound);
            m_impl->fireSoundActive = false;
        }
        if (m_impl->ambientSoundActive) {
            ma_sound_stop(&m_impl->ambientSound);
            ma_sound_uninit(&m_impl->ambientSound);
            m_impl->ambientSoundActive = false;
        }

        ma_engine_uninit(&m_impl->engine);
        m_impl->engineInitialized = false;
    }

    m_initialized = false;
}

void AudioEngine::update(float deltaTime, const glm::vec3& listenerPos, const glm::vec3& listenerForward, const glm::vec3& listenerUp) {
    if (!m_initialized || !m_impl->engineInitialized) return;

    m_listenerPos = listenerPos;
    m_listenerForward = listenerForward;
    m_listenerUp = listenerUp;

    // Actualizar posición y orientación del listener en miniaudio
    ma_engine_listener_set_position(&m_impl->engine, 0, listenerPos.x, listenerPos.y, listenerPos.z);
    ma_engine_listener_set_direction(&m_impl->engine, 0, listenerForward.x, listenerForward.y, listenerForward.z);
    ma_engine_listener_set_world_up(&m_impl->engine, 0, listenerUp.x, listenerUp.y, listenerUp.z);
}

void AudioEngine::setMasterVolume(float vol) {
    m_masterVolume = std::clamp(vol, 0.0f, 2.0f);
    if (m_initialized && m_impl->engineInitialized) {
        ma_engine_set_volume(&m_impl->engine, m_isMuted ? 0.0f : m_masterVolume);
    }
}

void AudioEngine::setSFXVolume(float vol) {
    m_sfxVolume = std::clamp(vol, 0.0f, 2.0f);
    if (m_impl->fireSoundActive) {
        ma_sound_set_volume(&m_impl->fireSound, m_sfxVolume * m_masterVolume * 0.7f);
    }
}

void AudioEngine::setFootstepsVolume(float vol) {
    m_footstepsVolume = std::clamp(vol, 0.0f, 2.0f);
}

void AudioEngine::setMusicVolume(float vol) {
    m_musicVolume = std::clamp(vol, 0.0f, 2.0f);
    if (m_impl->ambientSoundActive) {
        ma_sound_set_volume(&m_impl->ambientSound, m_musicVolume * m_masterVolume * 0.45f);
    }
}

void AudioEngine::setMuted(bool mute) {
    m_isMuted = mute;
    if (m_initialized && m_impl->engineInitialized) {
        ma_engine_set_volume(&m_impl->engine, m_isMuted ? 0.0f : m_masterVolume);
    }
}

void AudioEngine::playFootstep(FootstepSurface surface, float volume) {
    if (!m_initialized || m_isMuted) return;

    std::string key = "footstep_grass";
    switch (surface) {
        case FootstepSurface::GRASS:    key = "footstep_grass"; break;
        case FootstepSurface::DIRT:     key = "footstep_dirt"; break;
        case FootstepSurface::ROCK:     key = "footstep_rock"; break;
        case FootstepSurface::SNOW:     key = "footstep_snow"; break;
        case FootstepSurface::CONCRETE: key = "footstep_concrete"; break;
    }

    auto it = m_impl->soundPaths.find(key);
    if (it != m_impl->soundPaths.end()) {
        ma_sound* pSound = m_impl->acquireVoice();
        if (ma_sound_init_from_file(&m_impl->engine, it->second.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, pSound) == MA_SUCCESS) {
            m_impl->markVoiceActive(pSound);
            float pitch = 0.94f + (static_cast<float>(rand() % 100) / 100.0f) * 0.12f; // Variación orgánica de tono
            ma_sound_set_pitch(pSound, pitch);
            ma_sound_set_volume(pSound, volume * m_footstepsVolume * m_masterVolume);
            ma_sound_start(pSound);
        }
    }
}

void AudioEngine::playJump(float volume) {
    if (!m_initialized || m_isMuted) return;

    auto it = m_impl->soundPaths.find("jump");
    if (it != m_impl->soundPaths.end()) {
        ma_sound* pSound = m_impl->acquireVoice();
        if (ma_sound_init_from_file(&m_impl->engine, it->second.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, pSound) == MA_SUCCESS) {
            m_impl->markVoiceActive(pSound);
            ma_sound_set_volume(pSound, volume * m_sfxVolume * m_masterVolume);
            ma_sound_start(pSound);
        }
    }
}

void AudioEngine::playLand(FootstepSurface surface, float volume) {
    if (!m_initialized || m_isMuted) return;

    auto it = m_impl->soundPaths.find("land");
    if (it != m_impl->soundPaths.end()) {
        ma_sound* pSound = m_impl->acquireVoice();
        if (ma_sound_init_from_file(&m_impl->engine, it->second.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, pSound) == MA_SUCCESS) {
            m_impl->markVoiceActive(pSound);
            ma_sound_set_volume(pSound, volume * m_sfxVolume * m_masterVolume * 1.15f);
            ma_sound_start(pSound);
        }
    }
}

void AudioEngine::playImpact(const glm::vec3& worldPos, float impulse, float maxDistance) {
    if (!m_initialized || m_isMuted) return;

    float vol = std::clamp(impulse / 10.0f, 0.15f, 1.2f);
    playSound3D("impact", worldPos, vol, 0.9f + (rand() % 20) * 0.01f);
}

void AudioEngine::playSound3D(const std::string& soundName, const glm::vec3& worldPos, float volume, float pitch) {
    if (!m_initialized || m_isMuted) return;

    auto it = m_impl->soundPaths.find(soundName);
    if (it != m_impl->soundPaths.end()) {
        ma_sound* pSound = m_impl->acquireVoice();
        if (ma_sound_init_from_file(&m_impl->engine, it->second.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, pSound) == MA_SUCCESS) {
            m_impl->markVoiceActive(pSound);
            ma_sound_set_spatialization_enabled(pSound, MA_TRUE);
            ma_sound_set_position(pSound, worldPos.x, worldPos.y, worldPos.z);
            ma_sound_set_min_distance(pSound, 1.5f);
            ma_sound_set_max_distance(pSound, 40.0f);
            ma_sound_set_attenuation_model(pSound, ma_attenuation_model_inverse);
            ma_sound_set_pitch(pSound, pitch);
            ma_sound_set_volume(pSound, volume * m_sfxVolume * m_masterVolume);
            ma_sound_start(pSound);
        }
    }
}

void AudioEngine::updateFireAudio(const glm::vec3& firePos, bool isNear) {
    if (!m_initialized || !m_impl->fireSoundActive) return;

    ma_sound_set_position(&m_impl->fireSound, firePos.x, firePos.y, firePos.z);
    ma_sound_set_volume(&m_impl->fireSound, isNear ? (m_sfxVolume * m_masterVolume * 0.75f) : 0.0f);
}

void AudioEngine::toggleAmbientMusic(bool enable) {
    m_ambientEnabled = enable;
    if (m_impl->ambientSoundActive) {
        if (enable) {
            ma_sound_start(&m_impl->ambientSound);
        } else {
            ma_sound_stop(&m_impl->ambientSound);
        }
    }
}

bool AudioEngine::loadSound(const std::string& name, const std::string& filePath) {
    if (std::filesystem::exists(filePath)) {
        m_impl->soundPaths[name] = filePath;
        return true;
    }
    return false;
}

// -------------------------------------------------------------
// SÍNTESIS DE ONDAS PROCEDURALES EN MEMORIA (PCM 44.1kHz 16-bit)
// -------------------------------------------------------------

std::vector<int16_t> AudioEngine::generateWavFootstep(FootstepSurface surface) {
    const int sampleRate = 44100;
    const float duration = (surface == FootstepSurface::SNOW) ? 0.28f : 0.20f;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    float baseFreq = 90.0f;
    float noiseRatio = 0.5f;

    switch (surface) {
        case FootstepSurface::GRASS:
            baseFreq = 85.0f;
            noiseRatio = 0.70f;
            break;
        case FootstepSurface::DIRT:
            baseFreq = 75.0f;
            noiseRatio = 0.45f;
            break;
        case FootstepSurface::ROCK:
            baseFreq = 160.0f;
            noiseRatio = 0.25f;
            break;
        case FootstepSurface::SNOW:
            baseFreq = 110.0f;
            noiseRatio = 0.85f;
            break;
        case FootstepSurface::CONCRETE:
            baseFreq = 140.0f;
            noiseRatio = 0.35f;
            break;
    }

    float prevNoise = 0.0f;
    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float progress = t / duration;

        // Envolvente de decaimiento rápido percutivo
        float envelope = std::exp(-progress * 14.0f);

        // Componente tonal
        float tone = std::sin(2.0f * 3.14159f * baseFreq * t * (1.0f - progress * 0.3f));

        // Ruido filtrado paso-bajo (textura de paso)
        float rawNoise = randomFloat();
        float filteredNoise = prevNoise * 0.65f + rawNoise * 0.35f;
        prevNoise = filteredNoise;

        float sample = (tone * (1.0f - noiseRatio) + filteredNoise * noiseRatio) * envelope;
        pcm[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 24000.0f);
    }

    return pcm;
}

std::vector<int16_t> AudioEngine::generateWavJump() {
    const int sampleRate = 44100;
    const float duration = 0.28f;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float progress = t / duration;
        float envelope = std::sin(progress * 3.14159f) * (1.0f - progress * 0.4f);

        // Barrido de frecuencia ascendente ("whoosh" de impulso vertical)
        float freq = 90.0f + progress * 240.0f;
        float tone = std::sin(2.0f * 3.14159f * freq * t);
        float noise = randomFloat() * 0.25f;

        float sample = (tone * 0.75f + noise * 0.25f) * envelope;
        pcm[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 22000.0f);
    }
    return pcm;
}

std::vector<int16_t> AudioEngine::generateWavLand(FootstepSurface surface) {
    const int sampleRate = 44100;
    const float duration = 0.32f;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    float prevNoise = 0.0f;
    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float progress = t / duration;
        float envelope = std::exp(-progress * 11.0f);

        // Golpe de baja frecuencia con impacto sordo
        float tone = std::sin(2.0f * 3.14159f * 65.0f * t * (1.0f - progress * 0.2f));
        float rawNoise = randomFloat();
        float filteredNoise = prevNoise * 0.70f + rawNoise * 0.30f;
        prevNoise = filteredNoise;

        float sample = (tone * 0.65f + filteredNoise * 0.35f) * envelope;
        pcm[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 26000.0f);
    }
    return pcm;
}

std::vector<int16_t> AudioEngine::generateWavImpact() {
    const int sampleRate = 44100;
    const float duration = 0.25f;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float progress = t / duration;
        float envelope = std::exp(-progress * 18.0f);

        float tone = std::sin(2.0f * 3.14159f * 120.0f * t);
        float noise = randomFloat() * 0.4f;

        float sample = (tone * 0.6f + noise) * envelope;
        pcm[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 25000.0f);
    }
    return pcm;
}

std::vector<int16_t> AudioEngine::generateWavFireCrackle() {
    const int sampleRate = 44100;
    const float duration = 1.8f;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    float lowNoise = 0.0f;
    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;

        // Ruido bajo continuo de llama
        lowNoise = lowNoise * 0.95f + randomFloat() * 0.05f;

        // Chispas y crujidos esporádicos
        float pop = 0.0f;
        if ((rand() % 1000) < 14) {
            pop = randomFloat() * 0.85f;
        }

        float sample = lowNoise * 0.6f + pop * 0.4f;
        pcm[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 16000.0f);
    }
    return pcm;
}

std::vector<int16_t> AudioEngine::generateWavAmbientTone() {
    const int sampleRate = 44100;
    const float duration = 3.5f;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;

        // Dos tonos suaves con modulación armónica que crean una atmósfera espaciosa y relajante
        float tone1 = std::sin(2.0f * 3.14159f * 55.0f * t);
        float tone2 = std::sin(2.0f * 3.14159f * 110.0f * t + std::sin(2.0f * 3.14159f * 0.2f * t));
        float wind = (randomFloat() * 0.08f);

        float sample = (tone1 * 0.45f + tone2 * 0.35f + wind) * 0.5f;
        pcm[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 12000.0f);
    }
    return pcm;
}

} // namespace ASIS
