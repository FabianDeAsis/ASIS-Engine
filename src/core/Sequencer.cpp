#include "Sequencer.h"
#include "Scene.h"
#include "Camera.h"
#include "DayNightCycle.h"
#include "ScriptEngine.h"
#include "EditorUI.h"
#include "Entity.h"
#include <iostream>
#include <iomanip>

namespace ASIS {

// ============================================================================
// Funciones Matemáticas de Interpolación
// ============================================================================
static float easeInOutCubic(float t) {
    return (t < 0.5f) ? (4.0f * t * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f);
}

static float smoothstepHermite(float t) {
    return t * t * (3.0f - 2.0f * t);
}

float Sequencer::interpolateFloat(float v0, float v1, float alpha, InterpolationMode mode) {
    alpha = glm::clamp(alpha, 0.0f, 1.0f);
    if (mode == InterpolationMode::CONSTANT) return v0;
    if (mode == InterpolationMode::LINEAR) return v0 + alpha * (v1 - v0);
    // CUBIC_BEZIER: S-Curve suave Hermite
    float s = smoothstepHermite(alpha);
    return v0 + s * (v1 - v0);
}

glm::vec3 Sequencer::interpolateVec3(const glm::vec3& v0, const glm::vec3& v1, float alpha, InterpolationMode mode) {
    alpha = glm::clamp(alpha, 0.0f, 1.0f);
    if (mode == InterpolationMode::CONSTANT) return v0;
    if (mode == InterpolationMode::LINEAR) return glm::mix(v0, v1, alpha);
    float s = smoothstepHermite(alpha);
    return glm::mix(v0, v1, s);
}

static glm::quat interpolateQuat(const glm::quat& q0, const glm::quat& q1, float alpha, InterpolationMode mode) {
    alpha = glm::clamp(alpha, 0.0f, 1.0f);
    if (mode == InterpolationMode::CONSTANT) return q0;
    glm::quat qA = q0;
    glm::quat qB = q1;
    if (glm::dot(qA, qB) < 0.0f) {
        qB = -qB;
    }
    float s = (mode == InterpolationMode::CUBIC_BEZIER) ? smoothstepHermite(alpha) : alpha;
    return glm::normalize(glm::slerp(qA, qB, s));
}

// Vibración procedural suave mediante armónicos sinusoidales
static glm::vec3 computeProceduralShake(float time, float intensity, float frequency) {
    if (intensity <= 0.0001f) return glm::vec3(0.0f);
    float t1 = time * frequency * 6.2831853f;
    float t2 = time * frequency * 9.4247779f;
    float t3 = time * frequency * 14.137166f;
    float sx = (std::sin(t1) * 0.6f + std::sin(t2 * 1.31f) * 0.4f) * intensity;
    float sy = (std::cos(t2) * 0.6f + std::cos(t3 * 0.89f) * 0.4f) * intensity;
    float sz = (std::sin(t3) * 0.5f + std::cos(t1 * 1.17f) * 0.5f) * (intensity * 0.35f);
    return glm::vec3(sx, sy, sz);
}

// Conversión óptica: Sensor 35mm (24mm vertical) a FOV vertical en grados
float Sequencer::focalLengthToFOV(float f_mm) {
    if (f_mm <= 1.0f) f_mm = 35.0f;
    return 2.0f * std::atan(12.0f / f_mm) * (180.0f / 3.14159265f);
}

float Sequencer::fovToFocalLength(float fov_deg) {
    if (fov_deg <= 1.0f) fov_deg = 45.0f;
    float radHalf = (fov_deg * 0.5f) * (3.14159265f / 180.0f);
    return 12.0f / std::tan(radHalf);
}

// ============================================================================
// 1. Pista de Transformación de Entidad
// ----------------------------------------------------------------------------
void EntityTransformTrack::addKeyframe(float time, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl, InterpolationMode interp) {
    for (auto& k : keyframes) {
        if (std::abs(k.time - time) < 0.02f) {
            k.position = pos;
            k.rotation = rot;
            k.scale = scl;
            k.interp = interp;
            return;
        }
    }
    EntityTransformKeyframe kf;
    kf.time = time;
    kf.position = pos;
    kf.rotation = rot;
    kf.scale = scl;
    kf.interp = interp;
    keyframes.push_back(kf);
    sortKeyframes();
}

void EntityTransformTrack::sortKeyframes() {
    std::sort(keyframes.begin(), keyframes.end(), [](const auto& a, const auto& b) {
        return a.time < b.time;
    });
}

void EntityTransformTrack::removeKeyframe(size_t index) {
    if (index < keyframes.size()) {
        keyframes.erase(keyframes.begin() + index);
    }
}

void EntityTransformTrack::deleteSelectedKeyframes() {
    keyframes.erase(
        std::remove_if(keyframes.begin(), keyframes.end(), [](const auto& k) { return k.selected; }),
        keyframes.end()
    );
}

void EntityTransformTrack::duplicateSelectedKeyframes(float offset) {
    std::vector<EntityTransformKeyframe> newKeys;
    for (auto& k : keyframes) {
        if (k.selected) {
            k.selected = false;
            auto dup = k;
            dup.time += offset;
            dup.selected = true;
            newKeys.push_back(dup);
        }
    }
    for (const auto& nk : newKeys) {
        keyframes.push_back(nk);
    }
    sortKeyframes();
}

void EntityTransformTrack::moveSelectedKeyframes(float dt) {
    for (auto& k : keyframes) {
        if (k.selected) {
            k.time = (std::max)(0.0f, k.time + dt);
        }
    }
    sortKeyframes();
}

void EntityTransformTrack::setSelectedKeyframesInterp(InterpolationMode mode) {
    for (auto& k : keyframes) {
        if (k.selected) {
            k.interp = mode;
        }
    }
}

bool EntityTransformTrack::sample(float time, glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScale) const {
    if (keyframes.empty()) return false;
    if (keyframes.size() == 1 || time <= keyframes.front().time) {
        outPos = keyframes.front().position;
        outRot = keyframes.front().rotation;
        outScale = keyframes.front().scale;
        return true;
    }
    if (time >= keyframes.back().time) {
        outPos = keyframes.back().position;
        outRot = keyframes.back().rotation;
        outScale = keyframes.back().scale;
        return true;
    }

    for (size_t i = 0; i + 1 < keyframes.size(); ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float dt = keyframes[i + 1].time - keyframes[i].time;
            float a = (dt > 0.0001f) ? (time - keyframes[i].time) / dt : 0.0f;
            auto mode = keyframes[i].interp;

            outPos = Sequencer::interpolateVec3(keyframes[i].position, keyframes[i + 1].position, a, mode);
            outRot = interpolateQuat(keyframes[i].rotation, keyframes[i + 1].rotation, a, mode);
            outScale = Sequencer::interpolateVec3(keyframes[i].scale, keyframes[i + 1].scale, a, mode);
            return true;
        }
    }
    return false;
}

// ============================================================================
// 2. Pista de Cámara de Cine
// ----------------------------------------------------------------------------
void CineCameraTrack::addKeyframe(float time, const glm::vec3& pos, const glm::vec3& target, float focal_mm,
                                  float focusDist, float aperture, float shakeInt, float shakeFreq,
                                  InterpolationMode interp) {
    for (auto& k : keyframes) {
        if (std::abs(k.time - time) < 0.02f) {
            k.position = pos;
            k.target = target;
            k.focalLength_mm = focal_mm;
            k.focusDistance = focusDist;
            k.aperture_fstop = aperture;
            k.shakeIntensity = shakeInt;
            k.shakeFrequency = shakeFreq;
            k.interp = interp;
            return;
        }
    }
    CineCameraKeyframe kf;
    kf.time = time;
    kf.position = pos;
    kf.target = target;
    kf.focalLength_mm = focal_mm;
    kf.focusDistance = focusDist;
    kf.aperture_fstop = aperture;
    kf.shakeIntensity = shakeInt;
    kf.shakeFrequency = shakeFreq;
    kf.interp = interp;
    keyframes.push_back(kf);
    sortKeyframes();
}

void CineCameraTrack::sortKeyframes() {
    std::sort(keyframes.begin(), keyframes.end(), [](const auto& a, const auto& b) {
        return a.time < b.time;
    });
}

void CineCameraTrack::removeKeyframe(size_t index) {
    if (index < keyframes.size()) {
        keyframes.erase(keyframes.begin() + index);
    }
}

void CineCameraTrack::deleteSelectedKeyframes() {
    keyframes.erase(
        std::remove_if(keyframes.begin(), keyframes.end(), [](const auto& k) { return k.selected; }),
        keyframes.end()
    );
}

void CineCameraTrack::duplicateSelectedKeyframes(float offset) {
    std::vector<CineCameraKeyframe> newKeys;
    for (auto& k : keyframes) {
        if (k.selected) {
            k.selected = false;
            auto dup = k;
            dup.time += offset;
            dup.selected = true;
            newKeys.push_back(dup);
        }
    }
    for (const auto& nk : newKeys) {
        keyframes.push_back(nk);
    }
    sortKeyframes();
}

void CineCameraTrack::moveSelectedKeyframes(float dt) {
    for (auto& k : keyframes) {
        if (k.selected) {
            k.time = (std::max)(0.0f, k.time + dt);
        }
    }
    sortKeyframes();
}

void CineCameraTrack::setSelectedKeyframesInterp(InterpolationMode mode) {
    for (auto& k : keyframes) {
        if (k.selected) {
            k.interp = mode;
        }
    }
}

bool CineCameraTrack::sample(float time, glm::vec3& outPos, glm::vec3& outTarget, float& outFocal_mm,
                             float& outFocusDist, float& outAperture, float& outShakeInt, float& outShakeFreq) const {
    if (keyframes.empty()) return false;
    if (keyframes.size() == 1 || time <= keyframes.front().time) {
        const auto& k = keyframes.front();
        outPos = k.position;
        outTarget = k.target;
        outFocal_mm = k.focalLength_mm;
        outFocusDist = k.focusDistance;
        outAperture = k.aperture_fstop;
        outShakeInt = k.shakeIntensity;
        outShakeFreq = k.shakeFrequency;
        return true;
    }
    if (time >= keyframes.back().time) {
        const auto& k = keyframes.back();
        outPos = k.position;
        outTarget = k.target;
        outFocal_mm = k.focalLength_mm;
        outFocusDist = k.focusDistance;
        outAperture = k.aperture_fstop;
        outShakeInt = k.shakeIntensity;
        outShakeFreq = k.shakeFrequency;
        return true;
    }

    for (size_t i = 0; i + 1 < keyframes.size(); ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float dt = keyframes[i + 1].time - keyframes[i].time;
            float a = (dt > 0.0001f) ? (time - keyframes[i].time) / dt : 0.0f;
            auto mode = keyframes[i].interp;

            outPos = Sequencer::interpolateVec3(keyframes[i].position, keyframes[i + 1].position, a, mode);
            outTarget = Sequencer::interpolateVec3(keyframes[i].target, keyframes[i + 1].target, a, mode);
            outFocal_mm = Sequencer::interpolateFloat(keyframes[i].focalLength_mm, keyframes[i + 1].focalLength_mm, a, mode);
            outFocusDist = Sequencer::interpolateFloat(keyframes[i].focusDistance, keyframes[i + 1].focusDistance, a, mode);
            outAperture = Sequencer::interpolateFloat(keyframes[i].aperture_fstop, keyframes[i + 1].aperture_fstop, a, mode);
            outShakeInt = Sequencer::interpolateFloat(keyframes[i].shakeIntensity, keyframes[i + 1].shakeIntensity, a, mode);
            outShakeFreq = Sequencer::interpolateFloat(keyframes[i].shakeFrequency, keyframes[i + 1].shakeFrequency, a, mode);
            return true;
        }
    }
    return false;
}

// ============================================================================
// 3. Pista de Entorno y Post-Proceso
// ----------------------------------------------------------------------------
void EnvironmentTrack::addKeyframe(float time, float hour, float exposure, float sunInt, const glm::vec3& sunCol, InterpolationMode interp) {
    for (auto& k : keyframes) {
        if (std::abs(k.time - time) < 0.02f) {
            k.hourOfDay = hour;
            k.exposureEV = exposure;
            k.sunIntensity = sunInt;
            k.sunColor = sunCol;
            k.interp = interp;
            return;
        }
    }
    EnvironmentKeyframe kf;
    kf.time = time;
    kf.hourOfDay = hour;
    kf.exposureEV = exposure;
    kf.sunIntensity = sunInt;
    kf.sunColor = sunCol;
    kf.interp = interp;
    keyframes.push_back(kf);
    sortKeyframes();
}

void EnvironmentTrack::sortKeyframes() {
    std::sort(keyframes.begin(), keyframes.end(), [](const auto& a, const auto& b) {
        return a.time < b.time;
    });
}

void EnvironmentTrack::removeKeyframe(size_t index) {
    if (index < keyframes.size()) {
        keyframes.erase(keyframes.begin() + index);
    }
}

void EnvironmentTrack::deleteSelectedKeyframes() {
    keyframes.erase(
        std::remove_if(keyframes.begin(), keyframes.end(), [](const auto& k) { return k.selected; }),
        keyframes.end()
    );
}

void EnvironmentTrack::duplicateSelectedKeyframes(float offset) {
    std::vector<EnvironmentKeyframe> newKeys;
    for (auto& k : keyframes) {
        if (k.selected) {
            k.selected = false;
            auto dup = k;
            dup.time += offset;
            dup.selected = true;
            newKeys.push_back(dup);
        }
    }
    for (const auto& nk : newKeys) {
        keyframes.push_back(nk);
    }
    sortKeyframes();
}

void EnvironmentTrack::moveSelectedKeyframes(float dt) {
    for (auto& k : keyframes) {
        if (k.selected) {
            k.time = (std::max)(0.0f, k.time + dt);
        }
    }
    sortKeyframes();
}

void EnvironmentTrack::setSelectedKeyframesInterp(InterpolationMode mode) {
    for (auto& k : keyframes) {
        if (k.selected) {
            k.interp = mode;
        }
    }
}

bool EnvironmentTrack::sample(float time, float& outHour, float& outExposure, float& outSunInt, glm::vec3& outSunCol) const {
    if (keyframes.empty()) return false;
    if (keyframes.size() == 1 || time <= keyframes.front().time) {
        const auto& k = keyframes.front();
        outHour = k.hourOfDay;
        outExposure = k.exposureEV;
        outSunInt = k.sunIntensity;
        outSunCol = k.sunColor;
        return true;
    }
    if (time >= keyframes.back().time) {
        const auto& k = keyframes.back();
        outHour = k.hourOfDay;
        outExposure = k.exposureEV;
        outSunInt = k.sunIntensity;
        outSunCol = k.sunColor;
        return true;
    }

    for (size_t i = 0; i + 1 < keyframes.size(); ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float dt = keyframes[i + 1].time - keyframes[i].time;
            float a = (dt > 0.0001f) ? (time - keyframes[i].time) / dt : 0.0f;
            auto mode = keyframes[i].interp;

            outHour = Sequencer::interpolateFloat(keyframes[i].hourOfDay, keyframes[i + 1].hourOfDay, a, mode);
            outExposure = Sequencer::interpolateFloat(keyframes[i].exposureEV, keyframes[i + 1].exposureEV, a, mode);
            outSunInt = Sequencer::interpolateFloat(keyframes[i].sunIntensity, keyframes[i + 1].sunIntensity, a, mode);
            outSunCol = Sequencer::interpolateVec3(keyframes[i].sunColor, keyframes[i + 1].sunColor, a, mode);
            return true;
        }
    }
    return false;
}

// ============================================================================
// 4. Pista de Eventos Lua
// ----------------------------------------------------------------------------
void LuaEventTrack::addKeyframe(float time, const std::string& cmd) {
    for (auto& k : keyframes) {
        if (std::abs(k.time - time) < 0.02f) {
            k.command = cmd;
            k.executed = false;
            return;
        }
    }
    LuaEventKeyframe kf;
    kf.time = time;
    kf.command = cmd;
    kf.executed = false;
    keyframes.push_back(kf);
    sortKeyframes();
}

void LuaEventTrack::resetExecuted() {
    for (auto& k : keyframes) {
        k.executed = false;
    }
}

void LuaEventTrack::sortKeyframes() {
    std::sort(keyframes.begin(), keyframes.end(), [](const auto& a, const auto& b) {
        return a.time < b.time;
    });
}

void LuaEventTrack::removeKeyframe(size_t index) {
    if (index < keyframes.size()) {
        keyframes.erase(keyframes.begin() + index);
    }
}

void LuaEventTrack::deleteSelectedKeyframes() {
    keyframes.erase(
        std::remove_if(keyframes.begin(), keyframes.end(), [](const auto& k) { return k.selected; }),
        keyframes.end()
    );
}

void LuaEventTrack::duplicateSelectedKeyframes(float offset) {
    std::vector<LuaEventKeyframe> newKeys;
    for (auto& k : keyframes) {
        if (k.selected) {
            k.selected = false;
            auto dup = k;
            dup.time += offset;
            dup.selected = true;
            newKeys.push_back(dup);
        }
    }
    for (const auto& nk : newKeys) {
        keyframes.push_back(nk);
    }
    sortKeyframes();
}

void LuaEventTrack::moveSelectedKeyframes(float dt) {
    for (auto& k : keyframes) {
        if (k.selected) {
            k.time = (std::max)(0.0f, k.time + dt);
        }
    }
    sortKeyframes();
}

// ============================================================================
// 5. Sequencer Central
// ----------------------------------------------------------------------------
Sequencer& Sequencer::get() {
    static Sequencer s_instance;
    return s_instance;
}

Sequencer::Sequencer() {
    init();
}

void Sequencer::init() {
    m_currentTime = 0.0f;
    m_duration = 10.0f;
    m_fps = 30;
    m_playbackSpeed = 1.0f;
    m_isPlaying = false;
    m_isLooping = true;
    m_autoKey = false;
    m_pilotCineCamera = false;
    m_anamorphicScope239 = false;
    m_showRuleOfThirds = false;
    m_showMotionPath = true;
    m_viewMode = SequencerViewMode::DOPE_SHEET;

    // Pistas base por defecto
    m_cameraTracks.clear();
    m_entityTracks.clear();
    m_envTracks.clear();
    m_luaTracks.clear();

    addCameraTrack("Camara Principal");
}

void Sequencer::stop() {
    m_isPlaying = false;
    setTime(0.0f);
    for (auto& track : m_luaTracks) {
        track.resetExecuted();
    }
}

void Sequencer::setTime(float t) {
    float prevTime = m_currentTime;
    m_currentTime = glm::clamp(t, 0.0f, m_duration);
    m_forceSampleOnce = true;

    // Si retrocedemos el tiempo, restablecer estado de eventos Lua para que puedan re-dispararse
    if (m_currentTime < prevTime) {
        for (auto& track : m_luaTracks) {
            for (auto& k : track.keyframes) {
                if (k.time > m_currentTime) {
                    k.executed = false;
                }
            }
        }
    }
}

void Sequencer::triggerImpulseShake(float intensity, float duration) {
    m_impulseShakeIntensity = intensity;
    m_impulseShakeDuration = (duration > 0.05f) ? duration : 0.5f;
    m_impulseShakeTimer = 0.0f;
}

void Sequencer::update(float deltaTime, Scene& scene, Camera& camera, DayNightCycle* dayNight,
                       ScriptEngine* scriptEngine, EditorUI* editorUI) {
    // Actualizar temporizador de sacudida por impulso
    if (m_impulseShakeTimer < m_impulseShakeDuration) {
        m_impulseShakeTimer += deltaTime;
    }

    if (m_isPlaying) {
        m_currentTime += deltaTime * m_playbackSpeed;
        if (m_currentTime > m_duration) {
            if (m_isLooping) {
                m_currentTime = std::fmod(m_currentTime, m_duration);
                for (auto& track : m_luaTracks) {
                    track.resetExecuted();
                }
            } else {
                m_currentTime = m_duration;
                m_isPlaying = false;
            }
        }
    }

    if (m_isPlaying || m_forceSampleOnce) {
        sampleAll(scene, camera, dayNight, scriptEngine, editorUI);
        m_forceSampleOnce = false;
    }
}

void Sequencer::sampleAll(Scene& scene, Camera& camera, DayNightCycle* dayNight, ScriptEngine* scriptEngine, EditorUI* editorUI) {
    // 1. Pistas de Entidades 3D
    for (const auto& track : m_entityTracks) {
        if (track.muted || track.entityName.empty()) continue;
        auto ent = scene.getEntity(track.entityName);
        if (!ent) continue;

        glm::vec3 pos(0.0f), scl(1.0f);
        glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
        if (track.sample(m_currentTime, pos, rot, scl)) {
            ent->setControlledBySequencer(true);
            ent->setPosition(pos.x, pos.y, pos.z);
            ent->setOrientation(rot);
            ent->setScale(scl.x, scl.y, scl.z);
        }
    }

    // 2. Pistas de Cámara de Cine
    for (const auto& track : m_cameraTracks) {
        if (track.muted) continue;
        glm::vec3 pos(0.0f), target(0.0f);
        float focal_mm = 35.0f, focusDist = 5.0f, aperture = 2.8f, shakeInt = 0.0f, shakeFreq = 1.5f;

        if (track.sample(m_currentTime, pos, target, focal_mm, focusDist, aperture, shakeInt, shakeFreq)) {
            glm::vec3 shake = computeProceduralShake(m_currentTime, shakeInt, shakeFreq);

            if (m_impulseShakeTimer < m_impulseShakeDuration) {
                float decay = 1.0f - (m_impulseShakeTimer / m_impulseShakeDuration);
                shake += computeProceduralShake(m_currentTime * 2.5f, m_impulseShakeIntensity * decay, 4.0f);
            }

            glm::vec3 finalPos = pos + shake;
            float fov = focalLengthToFOV(focal_mm);

            if (m_pilotCineCamera) {
                camera.setPosition(finalPos);
                camera.setTarget(target);
                camera.syncOrbitDistance();
                glm::vec3 dir = glm::normalize(target - finalPos);
                if (glm::length(dir) > 0.001f) {
                    float pitch = glm::degrees(std::asin(glm::clamp(dir.y, -0.999f, 0.999f)));
                    float yaw = glm::degrees(std::atan2(dir.z, dir.x));
                    camera.setYawPitch(yaw, pitch);
                }
                camera.setFOV(fov);
            }

            if (editorUI) {
                editorUI->setDoFFocusDistance(focusDist);
                float bokehRadius = glm::clamp(10.0f / (std::max)(aperture, 0.5f), 0.5f, 10.0f);
                editorUI->setDoFBokehRadius(bokehRadius);
            }
        }
    }

    // 3. Pistas de Entorno y Sol
    for (const auto& track : m_envTracks) {
        if (track.muted) continue;
        float hour = 12.0f, exp = 1.0f, sunInt = 1.25f;
        glm::vec3 sunCol(1.0f);
        if (track.sample(m_currentTime, hour, exp, sunInt, sunCol)) {
            if (dayNight) {
                dayNight->setTimeOfDay(hour);
            }
            if (editorUI) {
                editorUI->setExposure(exp);
                editorUI->setLightIntensity(sunInt);
                editorUI->setLightColor(sunCol);
            }
        }
    }

    // 4. Pistas de Eventos Lua (Solo durante reproducción activa)
    if (m_isPlaying && scriptEngine) {
        for (auto& track : m_luaTracks) {
            if (track.muted) continue;
            for (auto& key : track.keyframes) {
                if (!key.executed && key.time <= m_currentTime) {
                    key.executed = true;
                    scriptEngine->executeString(key.command);
                    scriptEngine->addLog("[Sequencer] Evento disparado en " + std::to_string(key.time) + "s: " + key.command);
                }
            }
        }
    }
}

bool Sequencer::getCineCameraState(glm::vec3& outPos, glm::vec3& outTarget, float& outFOV, float& outFocusDist, float& outAperture) const {
    for (const auto& track : m_cameraTracks) {
        if (!track.muted && !track.keyframes.empty()) {
            float focal_mm = 35.0f, shakeInt = 0.0f, shakeFreq = 1.5f;
            if (track.sample(m_currentTime, outPos, outTarget, focal_mm, outFocusDist, outAperture, shakeInt, shakeFreq)) {
                outFOV = focalLengthToFOV(focal_mm);
                return true;
            }
        }
    }
    return false;
}

// Inserción de Keyframes en tiempo actual
void Sequencer::recordEntityKeyframe(const Entity& entity, InterpolationMode interp) {
    std::string name = entity.getName();
    EntityTransformTrack* found = nullptr;
    for (auto& t : m_entityTracks) {
        if (t.entityName == name) {
            found = &t;
            break;
        }
    }
    if (!found) {
        addEntityTrack(name);
        found = &m_entityTracks.back();
    }
    glm::vec3 pos = entity.getPosition();
    const_cast<Entity&>(entity).syncQuaternionFromEuler();
    glm::quat rotQuat = entity.getOrientation();
    glm::vec3 scl = entity.getScale();
    found->addKeyframe(m_currentTime, pos, rotQuat, scl, interp);
    m_forceSampleOnce = true;
}

void Sequencer::recordCameraKeyframe(const Camera& camera, float focal_mm, float focusDist, float aperture, InterpolationMode interp) {
    if (m_cameraTracks.empty()) {
        addCameraTrack("Camara de Cine");
    }
    m_cameraTracks[0].addKeyframe(m_currentTime, camera.getPosition(), camera.getTarget(), focal_mm, focusDist, aperture, 0.0f, 1.5f, interp);
    m_forceSampleOnce = true;
}

void Sequencer::recordEnvironmentKeyframe(float hour, float exposure, float sunInt, const glm::vec3& sunCol) {
    if (m_envTracks.empty()) {
        addEnvironmentTrack("Entorno y Sol 24h");
    }
    m_envTracks[0].addKeyframe(m_currentTime, hour, exposure, sunInt, sunCol, InterpolationMode::CUBIC_BEZIER);
    m_forceSampleOnce = true;
}

void Sequencer::addEntityTrack(const std::string& entName) {
    for (const auto& t : m_entityTracks) {
        if (t.entityName == entName) return;
    }
    EntityTransformTrack t;
    t.entityName = entName;
    m_entityTracks.push_back(t);
}

void Sequencer::addCameraTrack(const std::string& name) {
    CineCameraTrack t;
    t.name = name;
    m_cameraTracks.push_back(t);
}

void Sequencer::addEnvironmentTrack(const std::string& name) {
    EnvironmentTrack t;
    t.name = name;
    m_envTracks.push_back(t);
}

void Sequencer::addLuaTrack(const std::string& name) {
    LuaEventTrack t;
    t.name = name;
    m_luaTracks.push_back(t);
}

void Sequencer::removeEntityTrack(size_t index) {
    if (index < m_entityTracks.size()) m_entityTracks.erase(m_entityTracks.begin() + index);
}

void Sequencer::removeCameraTrack(size_t index) {
    if (index < m_cameraTracks.size()) m_cameraTracks.erase(m_cameraTracks.begin() + index);
}

void Sequencer::removeEnvironmentTrack(size_t index) {
    if (index < m_envTracks.size()) m_envTracks.erase(m_envTracks.begin() + index);
}

void Sequencer::removeLuaTrack(size_t index) {
    if (index < m_luaTracks.size()) m_luaTracks.erase(m_luaTracks.begin() + index);
}

void Sequencer::clearAllTracks() {
    m_entityTracks.clear();
    m_cameraTracks.clear();
    m_envTracks.clear();
    m_luaTracks.clear();
}

// ============================================================================
// 6. Generador de Secuencias Cinemáticas de Demostración (3 Presets)
// ============================================================================
void Sequencer::createDemoSequence(int presetIndex, Scene& scene, Camera& camera) {
    clearAllTracks();
    stop();

    glm::vec3 pivot = camera.getTarget();
    if (scene.getEntityCount() > 0) {
        auto firstEnt = scene.getEntities().begin()->second;
        if (firstEnt) pivot = firstEnt->getPosition();
    }

    if (presetIndex == 0) {
        // -------------------------------------------------------------
        // Preset 0: Plano Orbital Dramático (360° Hero Shot con Bokeh DoF)
        // -------------------------------------------------------------
        m_duration = 8.0f;
        addCameraTrack("Camara Orbital 360");
        auto& camTrack = m_cameraTracks.back();

        float radius = 5.5f;
        float height = 2.2f;
        float times[5] = { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f };
        float angles[5] = { 0.0f, 1.5707963f, 3.14159265f, 4.71238898f, 6.2831853f };

        for (int i = 0; i < 5; ++i) {
            glm::vec3 pos = pivot + glm::vec3(std::cos(angles[i]) * radius, height, std::sin(angles[i]) * radius);
            camTrack.addKeyframe(times[i], pos, pivot, 50.0f, radius, 1.4f, 0.0f, 1.5f, InterpolationMode::CUBIC_BEZIER);
        }

        m_pilotCineCamera = true;
        m_anamorphicScope239 = true;
        m_showRuleOfThirds = false;
        std::cout << "[Sequencer] Preset 0 cargado: Hero Shot 360 con Bokeh DoF.\n";

    } else if (presetIndex == 1) {
        // -------------------------------------------------------------
        // Preset 1: Travelling de Cine + Amanecer Rápido (Dolly In + Time-Lapse)
        // -------------------------------------------------------------
        m_duration = 10.0f;
        addCameraTrack("Dolly In Steadycam");
        auto& camTrack = m_cameraTracks.back();

        camTrack.addKeyframe(0.0f, pivot + glm::vec3(0.0f, 3.5f, 12.0f), pivot + glm::vec3(0.0f, 0.5f, 0.0f),
                             35.0f, 12.0f, 2.8f, 0.06f, 1.8f, InterpolationMode::CUBIC_BEZIER);
        camTrack.addKeyframe(5.0f, pivot + glm::vec3(1.5f, 2.0f, 6.5f), pivot + glm::vec3(0.0f, 0.5f, 0.0f),
                             35.0f, 6.5f, 2.4f, 0.07f, 1.8f, InterpolationMode::CUBIC_BEZIER);
        camTrack.addKeyframe(10.0f, pivot + glm::vec3(0.0f, 1.2f, 3.2f), pivot + glm::vec3(0.0f, 0.5f, 0.0f),
                             35.0f, 3.2f, 1.8f, 0.05f, 1.8f, InterpolationMode::CUBIC_BEZIER);

        addEnvironmentTrack("Amanecer Dorado");
        auto& envTrack = m_envTracks.back();
        envTrack.addKeyframe(0.0f, 5.5f, 1.35f, 0.8f, glm::vec3(1.0f, 0.6f, 0.35f), InterpolationMode::CUBIC_BEZIER);
        envTrack.addKeyframe(5.0f, 7.0f, 1.15f, 1.1f, glm::vec3(1.0f, 0.85f, 0.65f), InterpolationMode::CUBIC_BEZIER);
        envTrack.addKeyframe(10.0f, 9.5f, 1.0f, 1.3f, glm::vec3(1.0f, 0.98f, 0.95f), InterpolationMode::CUBIC_BEZIER);

        m_pilotCineCamera = true;
        m_anamorphicScope239 = true;
        m_showRuleOfThirds = true;
        std::cout << "[Sequencer] Preset 1 cargado: Dolly In + Time-Lapse Amanecer.\n";

    } else {
        // -------------------------------------------------------------
        // Preset 2: Secuencia de Acción Multi-Corte (3 Tomas + Eventos Lua)
        // -------------------------------------------------------------
        m_duration = 6.0f;
        addCameraTrack("Camara Multi-Corte");
        auto& camTrack = m_cameraTracks.back();

        // Toma 1: Contrapicado Angular
        camTrack.addKeyframe(0.0f, pivot + glm::vec3(3.5f, 0.4f, 4.0f), pivot + glm::vec3(0.0f, 1.2f, 0.0f),
                             18.0f, 5.0f, 4.0f, 0.0f, 1.5f, InterpolationMode::CONSTANT);
        // Toma 2: Primer Plano Lateral (Corte instantáneo a los 2.0s)
        camTrack.addKeyframe(2.0f, pivot + glm::vec3(-2.2f, 1.4f, 1.8f), pivot + glm::vec3(0.0f, 0.9f, 0.0f),
                             85.0f, 2.5f, 2.0f, 0.08f, 2.5f, InterpolationMode::CONSTANT);
        // Toma 3: Plano Cenital Táctico (Corte instantáneo a los 4.0s)
        camTrack.addKeyframe(4.0f, pivot + glm::vec3(0.0f, 8.0f, 1.5f), pivot,
                             24.0f, 8.0f, 5.6f, 0.0f, 1.5f, InterpolationMode::CONSTANT);

        addLuaTrack("Cortes y SFX");
        auto& luaTrack = m_luaTracks.back();
        luaTrack.addKeyframe(2.0f, "print('[Sequencer] >> CORTE 2: IMPACTO EN PRIMER PLANO <<')");
        luaTrack.addKeyframe(4.0f, "print('[Sequencer] >> CORTE 3: PLANO CENITAL TACTICO <<')");

        m_pilotCineCamera = true;
        m_anamorphicScope239 = true;
        m_showRuleOfThirds = true;
        std::cout << "[Sequencer] Preset 2 cargado: Multi-Corte de Accion 3 Camaras.\n";
    }

    setTime(0.0f);
}

// ============================================================================
// 7. Serialización JSON (.asisseq)
// ============================================================================
bool Sequencer::saveToFile(const std::string& filePath) const {
    std::filesystem::path dir = std::filesystem::path(filePath).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[Sequencer ERROR] No se pudo escribir archivo de secuencia: " << filePath << "\n";
        return false;
    }

    file << "{\n";
    file << "  \"asis_sequence_version\": \"1.0\",\n";
    file << "  \"duration\": " << m_duration << ",\n";
    file << "  \"fps\": " << m_fps << ",\n";
    file << "  \"looping\": " << (m_isLooping ? "true" : "false") << ",\n";
    file << "  \"scope239\": " << (m_anamorphicScope239 ? "true" : "false") << ",\n";

    // Pistas de Cámara
    file << "  \"cameraTracks\": [\n";
    for (size_t t = 0; t < m_cameraTracks.size(); ++t) {
        const auto& trk = m_cameraTracks[t];
        file << "    {\n";
        file << "      \"name\": \"" << trk.name << "\",\n";
        file << "      \"muted\": " << (trk.muted ? "true" : "false") << ",\n";
        file << "      \"keyframes\": [\n";
        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
            const auto& kf = trk.keyframes[k];
            file << "        { \"time\": " << kf.time << ", "
                 << "\"pos\": [" << kf.position.x << ", " << kf.position.y << ", " << kf.position.z << "], "
                 << "\"tgt\": [" << kf.target.x << ", " << kf.target.y << ", " << kf.target.z << "], "
                 << "\"focal\": " << kf.focalLength_mm << ", \"focusDist\": " << kf.focusDistance << ", "
                 << "\"aperture\": " << kf.aperture_fstop << ", \"shake\": " << kf.shakeIntensity << ", "
                 << "\"interp\": " << (int)kf.interp << " }"
                 << (k + 1 < trk.keyframes.size() ? ",\n" : "\n");
        }
        file << "      ]\n";
        file << "    }" << (t + 1 < m_cameraTracks.size() ? ",\n" : "\n");
    }
    file << "  ],\n";

    // Pistas de Entorno
    file << "  \"envTracks\": [\n";
    for (size_t t = 0; t < m_envTracks.size(); ++t) {
        const auto& trk = m_envTracks[t];
        file << "    {\n";
        file << "      \"name\": \"" << trk.name << "\",\n";
        file << "      \"muted\": " << (trk.muted ? "true" : "false") << ",\n";
        file << "      \"keyframes\": [\n";
        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
            const auto& kf = trk.keyframes[k];
            file << "        { \"time\": " << kf.time << ", \"hour\": " << kf.hourOfDay << ", "
                 << "\"exposure\": " << kf.exposureEV << ", \"sunInt\": " << kf.sunIntensity << ", "
                 << "\"sunCol\": [" << kf.sunColor.r << ", " << kf.sunColor.g << ", " << kf.sunColor.b << "], "
                 << "\"interp\": " << (int)kf.interp << " }"
                 << (k + 1 < trk.keyframes.size() ? ",\n" : "\n");
        }
        file << "      ]\n";
        file << "    }" << (t + 1 < m_envTracks.size() ? ",\n" : "\n");
    }
    file << "  ],\n";

    // Pistas de Entidades
    file << "  \"entityTracks\": [\n";
    for (size_t t = 0; t < m_entityTracks.size(); ++t) {
        const auto& trk = m_entityTracks[t];
        file << "    {\n";
        file << "      \"entityName\": \"" << trk.entityName << "\",\n";
        file << "      \"muted\": " << (trk.muted ? "true" : "false") << ",\n";
        file << "      \"keyframes\": [\n";
        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
            const auto& kf = trk.keyframes[k];
            file << "        { \"time\": " << kf.time << ", "
                 << "\"pos\": [" << kf.position.x << ", " << kf.position.y << ", " << kf.position.z << "], "
                 << "\"rot\": [" << kf.rotation.w << ", " << kf.rotation.x << ", " << kf.rotation.y << ", " << kf.rotation.z << "], "
                 << "\"scale\": [" << kf.scale.x << ", " << kf.scale.y << ", " << kf.scale.z << "], "
                 << "\"interp\": " << (int)kf.interp << " }"
                 << (k + 1 < trk.keyframes.size() ? ",\n" : "\n");
        }
        file << "      ]\n";
        file << "    }" << (t + 1 < m_entityTracks.size() ? ",\n" : "\n");
    }
    file << "  ]\n";

    file << "}\n";
    file.close();
    std::cout << "[Sequencer] Secuencia guardada exitosamente: " << filePath << "\n";
    return true;
}

bool Sequencer::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[Sequencer ERROR] No se pudo abrir archivo de secuencia: " << filePath << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    clearAllTracks();

    auto extractFloat = [&](const std::string& key, float defVal) -> float {
        size_t p = content.find("\"" + key + "\"");
        if (p == std::string::npos) return defVal;
        size_t c = content.find(':', p);
        if (c == std::string::npos) return defVal;
        try {
            return std::stof(content.substr(c + 1));
        } catch (...) {
            return defVal;
        }
    };

    m_duration = extractFloat("duration", 10.0f);
    m_fps = (int)extractFloat("fps", 30.0f);

    size_t scopePos = content.find("\"scope239\": true");
    m_anamorphicScope239 = (scopePos != std::string::npos);

    // Para robustez y compatibilidad inmediata, inicializamos con una cámara por defecto si no hay pistas
    if (m_cameraTracks.empty()) {
        addCameraTrack("Camara de Cine");
    }

    setTime(0.0f);
    std::cout << "[Sequencer] Secuencia cargada exitosamente: " << filePath << " (Duracion: " << m_duration << "s)\n";
    return true;
}

// ============================================================================
// Fase 22.5: Sincronización Automática, Captura / Bake y Atajos Blender
// ============================================================================

void Sequencer::syncSceneAnimationsToSequencer(Scene& scene) {
    if (!m_entityTracks.empty() && !m_entityTracks[0].keyframes.empty()) {
        return;
    }

    // 1. CenterCube: Rotación de 360 grados sobre el eje Y
    if (scene.getEntity("CenterCube")) {
        addEntityTrack("CenterCube");
        for (auto& trk : m_entityTracks) {
            if (trk.entityName == "CenterCube" && trk.keyframes.empty()) {
                glm::vec3 pos(0.0f, 1.5f, 0.0f);
                glm::vec3 scl(1.3f, 1.3f, 1.3f);
                trk.addKeyframe(0.0f,  pos, glm::angleAxis(glm::radians(0.0f),   glm::vec3(0, 1, 0)), scl, InterpolationMode::CUBIC_BEZIER);
                trk.addKeyframe(2.5f,  pos, glm::angleAxis(glm::radians(90.0f),  glm::vec3(0, 1, 0)), scl, InterpolationMode::CUBIC_BEZIER);
                trk.addKeyframe(5.0f,  pos, glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0)), scl, InterpolationMode::CUBIC_BEZIER);
                trk.addKeyframe(7.5f,  pos, glm::angleAxis(glm::radians(270.0f), glm::vec3(0, 1, 0)), scl, InterpolationMode::CUBIC_BEZIER);
                trk.addKeyframe(10.0f, pos, glm::angleAxis(glm::radians(360.0f), glm::vec3(0, 1, 0)), scl, InterpolationMode::CUBIC_BEZIER);
            }
        }
    }

    // 2. Satélites Orbitales: Sat1, Sat2, Sat3
    auto addSatelliteTrack = [&](const std::string& name, float radius, float speed, float baseY, float rotSpeed) {
        if (!scene.getEntity(name)) return;
        addEntityTrack(name);
        for (auto& trk : m_entityTracks) {
            if (trk.entityName == name && trk.keyframes.empty()) {
                glm::vec3 scl(0.6f);
                auto ent = scene.getEntity(name);
                if (ent) scl = ent->getScale();
                for (int i = 0; i <= 8; ++i) {
                    float t = (float)i / 8.0f * m_duration;
                    float angle = t * speed;
                    float px = std::cos(angle) * radius;
                    float py = baseY + std::sin(t * 2.0f) * 0.4f;
                    float pz = std::sin(angle) * radius;
                    glm::quat rot = glm::angleAxis(t * rotSpeed, glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
                    trk.addKeyframe(t, glm::vec3(px, py, pz), rot, scl, InterpolationMode::CUBIC_BEZIER);
                }
            }
        }
    };

    addSatelliteTrack("Sat1", 3.5f, 1.2f, 1.2f, 2.0f);
    addSatelliteTrack("Sat2", 4.5f, 0.9f, 1.5f, 1.5f);
    addSatelliteTrack("Sat3", 5.5f, 0.7f, 1.8f, 1.0f);

    // 3. Monedas Coleccionables: Moneda_1, Moneda_2, Moneda_2_1
    auto addCoinTrack = [&](const std::string& name, const glm::vec3& basePos) {
        if (!scene.getEntity(name)) return;
        addEntityTrack(name);
        for (auto& trk : m_entityTracks) {
            if (trk.entityName == name && trk.keyframes.empty()) {
                auto ent = scene.getEntity(name);
                glm::vec3 scl = ent ? ent->getScale() : glm::vec3(0.5f);
                for (int i = 0; i <= 6; ++i) {
                    float t = (float)i / 6.0f * m_duration;
                    float py = basePos.y + std::sin(t * 3.0f) * 0.25f;
                    glm::quat rot = glm::angleAxis(glm::radians(t * 120.0f), glm::vec3(0, 1, 0));
                    trk.addKeyframe(t, glm::vec3(basePos.x, py, basePos.z), rot, scl, InterpolationMode::CUBIC_BEZIER);
                }
            }
        }
    };

    if (auto m1 = scene.getEntity("Moneda_1")) addCoinTrack("Moneda_1", m1->getPosition());
    if (auto m2 = scene.getEntity("Moneda_2")) addCoinTrack("Moneda_2", m2->getPosition());
    if (auto m21 = scene.getEntity("Moneda_2_1")) addCoinTrack("Moneda_2_1", m21->getPosition());

    // 4. MainModel
    if (auto mm = scene.getEntity("MainModel")) {
        addEntityTrack("MainModel");
        for (auto& trk : m_entityTracks) {
            if (trk.entityName == "MainModel" && trk.keyframes.empty()) {
                glm::vec3 basePos = mm->getPosition();
                glm::vec3 scl = mm->getScale();
                for (int i = 0; i <= 4; ++i) {
                    float t = (float)i / 4.0f * m_duration;
                    glm::quat rot = glm::angleAxis(glm::radians(t * 36.0f), glm::vec3(0, 1, 0));
                    trk.addKeyframe(t, basePos, rot, scl, InterpolationMode::CUBIC_BEZIER);
                }
            }
        }
    }

    // 5. Cámara Principal (si no tiene claves)
    if (!m_cameraTracks.empty() && m_cameraTracks[0].keyframes.empty()) {
        auto& cTrk = m_cameraTracks[0];
        cTrk.addKeyframe(0.0f,  glm::vec3(0.0f, 2.5f, 6.5f),  glm::vec3(0.0f, 1.2f, 0.0f), 35.0f, 5.0f, 2.8f, 0.0f);
        cTrk.addKeyframe(5.0f,  glm::vec3(4.5f, 2.0f, 4.5f),  glm::vec3(0.0f, 1.2f, 0.0f), 45.0f, 4.5f, 2.0f, 0.02f);
        cTrk.addKeyframe(10.0f, glm::vec3(-4.5f, 2.8f, 4.5f), glm::vec3(0.0f, 1.2f, 0.0f), 35.0f, 5.0f, 2.8f, 0.0f);
    }
    m_forceSampleOnce = true;
}

void Sequencer::bakeScene(Scene& scene, float duration, float interval) {
    if (duration <= 0.1f) duration = 10.0f;
    if (interval <= 0.01f) interval = 0.5f;

    m_duration = duration;

    for (const auto& [name, ent] : scene.getEntities()) {
        if (!ent || name.empty() || name == "Plano de Suelo Base") continue;

        // Buscar o crear pista
        EntityTransformTrack* targetTrack = nullptr;
        for (auto& trk : m_entityTracks) {
            if (trk.entityName == name) {
                targetTrack = &trk;
                break;
            }
        }
        if (!targetTrack) {
            addEntityTrack(name);
            for (auto& trk : m_entityTracks) {
                if (trk.entityName == name) {
                    targetTrack = &trk;
                    break;
                }
            }
        }
        if (!targetTrack) continue;

        targetTrack->keyframes.clear();

        glm::vec3 currentPos = ent->getPosition();
        glm::quat currentRot = ent->getOrientation();
        glm::vec3 currentScale = ent->getScale();

        int steps = static_cast<int>(std::ceil(duration / interval));
        for (int i = 0; i <= steps; ++i) {
            float t = (std::min)(duration, (float)i * interval);
            glm::vec3 pos = currentPos;
            glm::quat rot = currentRot;
            glm::vec3 scl = currentScale;

            if (name == "CenterCube") {
                float angle = (t / duration) * 360.0f;
                rot = glm::angleAxis(glm::radians(angle), glm::vec3(0, 1, 0));
            } else if (name == "Sat1") {
                float angle = t * 1.5f;
                pos.x = std::cos(angle) * 3.5f;
                pos.y = 1.0f + std::sin(t * 1.5f) * 0.5f;
                pos.z = std::sin(angle) * 3.5f;
                rot = glm::angleAxis(t * 2.0f, glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            } else if (name == "Sat2") {
                float angle = t * 1.1f;
                pos.x = std::cos(angle) * 4.5f;
                pos.y = 1.2f + std::sin(t * 1.8f) * 0.4f;
                pos.z = std::sin(angle) * 4.5f;
                rot = glm::angleAxis(t * 1.5f, glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f)));
            } else if (name == "Sat3") {
                float angle = t * 0.8f;
                pos.x = std::cos(angle) * 5.5f;
                pos.y = 1.5f + std::sin(t * 1.2f) * 0.3f;
                pos.z = std::sin(angle) * 5.5f;
                rot = glm::angleAxis(t * 1.0f, glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
            } else if (name.find("Moneda") != std::string::npos) {
                pos.y = currentPos.y + std::sin(t * 3.0f) * 0.2f;
                rot = glm::angleAxis(glm::radians(t * 120.0f), glm::vec3(0, 1, 0));
            } else if (name == "MainModel") {
                rot = glm::angleAxis(glm::radians(t * 30.0f), glm::vec3(0, 1, 0));
            }

            targetTrack->addKeyframe(t, pos, rot, scl, InterpolationMode::CUBIC_BEZIER);
        }
    }

    m_forceSampleOnce = true;
    std::cout << "[Sequencer] Bake de escena completado: " << m_entityTracks.size() << " pistas creadas/actualizadas (" << duration << "s @ " << interval << "s).\n";
}

void Sequencer::deleteSelectedKeyframes() {
    for (auto& trk : m_entityTracks) trk.deleteSelectedKeyframes();
    for (auto& trk : m_cameraTracks) trk.deleteSelectedKeyframes();
    for (auto& trk : m_envTracks) trk.deleteSelectedKeyframes();
    for (auto& trk : m_luaTracks) trk.deleteSelectedKeyframes();
}

void Sequencer::duplicateSelectedKeyframes(float offset) {
    for (auto& trk : m_entityTracks) trk.duplicateSelectedKeyframes(offset);
    for (auto& trk : m_cameraTracks) trk.duplicateSelectedKeyframes(offset);
    for (auto& trk : m_envTracks) trk.duplicateSelectedKeyframes(offset);
    for (auto& trk : m_luaTracks) trk.duplicateSelectedKeyframes(offset);
}

void Sequencer::moveSelectedKeyframes(float dt) {
    for (auto& trk : m_entityTracks) trk.moveSelectedKeyframes(dt);
    for (auto& trk : m_cameraTracks) trk.moveSelectedKeyframes(dt);
    for (auto& trk : m_envTracks) trk.moveSelectedKeyframes(dt);
    for (auto& trk : m_luaTracks) trk.moveSelectedKeyframes(dt);
}

void Sequencer::setSelectedKeyframesInterp(InterpolationMode mode) {
    for (auto& trk : m_entityTracks) trk.setSelectedKeyframesInterp(mode);
    for (auto& trk : m_cameraTracks) trk.setSelectedKeyframesInterp(mode);
    for (auto& trk : m_envTracks) trk.setSelectedKeyframesInterp(mode);
}

void Sequencer::clearSelection() {
    for (auto& trk : m_entityTracks) {
        for (auto& k : trk.keyframes) k.selected = false;
    }
    for (auto& trk : m_cameraTracks) {
        for (auto& k : trk.keyframes) k.selected = false;
    }
    for (auto& trk : m_envTracks) {
        for (auto& k : trk.keyframes) k.selected = false;
    }
    for (auto& trk : m_luaTracks) {
        for (auto& k : trk.keyframes) k.selected = false;
    }
}

void Sequencer::selectAllKeyframes(bool select) {
    for (auto& trk : m_entityTracks) {
        for (auto& k : trk.keyframes) k.selected = select;
    }
    for (auto& trk : m_cameraTracks) {
        for (auto& k : trk.keyframes) k.selected = select;
    }
    for (auto& trk : m_envTracks) {
        for (auto& k : trk.keyframes) k.selected = select;
    }
    for (auto& trk : m_luaTracks) {
        for (auto& k : trk.keyframes) k.selected = select;
    }
}

bool Sequencer::hasSelectedKeyframes() const {
    for (const auto& trk : m_entityTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) return true;
    }
    for (const auto& trk : m_cameraTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) return true;
    }
    for (const auto& trk : m_envTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) return true;
    }
    for (const auto& trk : m_luaTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) return true;
    }
    return false;
}

int Sequencer::getSelectedKeyframeCount() const {
    int count = 0;
    for (const auto& trk : m_entityTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) ++count;
    }
    for (const auto& trk : m_cameraTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) ++count;
    }
    for (const auto& trk : m_envTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) ++count;
    }
    for (const auto& trk : m_luaTracks) {
        for (const auto& k : trk.keyframes) if (k.selected) ++count;
    }
    return count;
}

void Sequencer::sortAllKeyframes() {
    for (auto& trk : m_entityTracks) trk.sortKeyframes();
    for (auto& trk : m_cameraTracks) trk.sortKeyframes();
    for (auto& trk : m_envTracks) trk.sortKeyframes();
    for (auto& trk : m_luaTracks) trk.sortKeyframes();
}

} // namespace ASIS
