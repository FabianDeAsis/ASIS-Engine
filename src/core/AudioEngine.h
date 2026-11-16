#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

namespace ASIS {

enum class FootstepSurface {
    GRASS,
    DIRT,
    ROCK,
    SNOW,
    CONCRETE
};

/**
 * @brief Motor de Audio 3D Espacial y Sintetizador SFX Procedural (Fase 16)
 * 
 * Gestiona reproducción de audio espacial en 3D (Listener + Emitters),
 * sonidos de locomoción de personaje (pisadas dinámicas según superficie/terreno),
 * efectos de salto/aterrizaje, impactos físicos y sintetizador de sonido procedural
 * autónomo sin dependencias de assets externos (vía miniaudio).
 */
class AudioEngine {
public:
    static AudioEngine& get();

    bool init();
    void shutdown();

    // Actualización de escucha espacial 3D
    void update(float deltaTime, const glm::vec3& listenerPos, const glm::vec3& listenerForward, const glm::vec3& listenerUp);

    // Controles de volumen
    void setMasterVolume(float vol);
    float getMasterVolume() const { return m_masterVolume; }

    void setSFXVolume(float vol);
    float getSFXVolume() const { return m_sfxVolume; }

    void setFootstepsVolume(float vol);
    float getFootstepsVolume() const { return m_footstepsVolume; }

    void setMusicVolume(float vol);
    float getMusicVolume() const { return m_musicVolume; }

    void setMuted(bool mute);
    bool isMuted() const { return m_isMuted; }

    // Reproducción de efectos de personaje
    void playFootstep(FootstepSurface surface, float volume = 1.0f);
    void playJump(float volume = 1.0f);
    void playLand(FootstepSurface surface, float volume = 1.0f);

    // Reproducción de sonido 3D posicional
    void playImpact(const glm::vec3& worldPos, float impulse, float maxDistance = 35.0f);
    void playSound3D(const std::string& soundName, const glm::vec3& worldPos, float volume = 1.0f, float pitch = 1.0f);

    // Ambiente y fuego
    void updateFireAudio(const glm::vec3& firePos, bool isNear);
    void toggleAmbientMusic(bool enable);
    bool isAmbientMusicEnabled() const { return m_ambientEnabled; }

    // Carga de archivos externos opcionales (.wav, .mp3)
    bool loadSound(const std::string& name, const std::string& filePath);

private:
    AudioEngine();
    ~AudioEngine();

    // Generador de audio procedural en memoria
    std::vector<int16_t> generateWavFootstep(FootstepSurface surface);
    std::vector<int16_t> generateWavJump();
    std::vector<int16_t> generateWavLand(FootstepSurface surface);
    std::vector<int16_t> generateWavImpact();
    std::vector<int16_t> generateWavFireCrackle();
    std::vector<int16_t> generateWavAmbientTone();

    bool m_initialized;
    bool m_isMuted;
    float m_masterVolume;
    float m_sfxVolume;
    float m_footstepsVolume;
    float m_musicVolume;
    bool m_ambientEnabled;

    glm::vec3 m_listenerPos;
    glm::vec3 m_listenerForward;
    glm::vec3 m_listenerUp;

    // Punteros opacos a estructuras internas de miniaudio
    struct AudioImpl;
    AudioImpl* m_impl;
};

} // namespace ASIS
