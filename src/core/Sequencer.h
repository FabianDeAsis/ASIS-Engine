#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace ASIS {

class Scene;
class Camera;
class DayNightCycle;
class ScriptEngine;
class EditorUI;
class Entity;

/**
 * @brief Modo de interpolación entre fotogramas clave
 */
enum class InterpolationMode {
    LINEAR = 0,        // Interpolación lineal estándar
    CONSTANT = 1,      // Step / Mantener valor hasta siguiente clave (cortes de cámara)
    CUBIC_BEZIER = 2   // Curva suave Hermite / Bézier cúbica con tangentes continuas
};

/**
 * @brief Modo de visualización en el lienzo temporal
 */
enum class SequencerViewMode {
    DOPE_SHEET = 0,    // Claves en rombos (diamantes estilo Blender)
    GRAPH_EDITOR = 1   // Curvas continuas y canales F-Curve
};

// ----------------------------------------------------------------------------
// 1. Pista de Transformación de Entidades 3D
// ----------------------------------------------------------------------------
struct EntityTransformKeyframe {
    float time = 0.0f;
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    InterpolationMode interp = InterpolationMode::CUBIC_BEZIER;
    bool selected = false;
};

struct EntityTransformTrack {
    std::string entityName;
    bool muted = false;
    bool expanded = false;
    std::vector<EntityTransformKeyframe> keyframes;

    void addKeyframe(float time, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl, InterpolationMode interp = InterpolationMode::CUBIC_BEZIER);
    bool sample(float time, glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScale) const;
    void sortKeyframes();
    void removeKeyframe(size_t index);
    void deleteSelectedKeyframes();
    void duplicateSelectedKeyframes(float offset = 0.5f);
    void moveSelectedKeyframes(float dt);
    void setSelectedKeyframesInterp(InterpolationMode mode);
};

// ----------------------------------------------------------------------------
// 2. Pista de Cámara de Cine (Lentes Físicos, LookAt, DoF y Shake)
// ----------------------------------------------------------------------------
struct CineCameraKeyframe {
    float time = 0.0f;
    glm::vec3 position = glm::vec3(0.0f, 2.5f, 6.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.8f, 0.0f);
    float focalLength_mm = 35.0f;     // 18mm, 24mm, 35mm, 50mm, 85mm, 135mm
    float focusDistance = 5.0f;       // Distancia de enfoque fotográfico en metros
    float aperture_fstop = 2.8f;      // f/1.4, f/2.0, f/2.8, etc. (controla el Bokeh)
    float shakeIntensity = 0.0f;      // Vibración de cámara en mano (0.0 = trípode)
    float shakeFrequency = 1.5f;      // Frecuencia de vibración en Hz
    InterpolationMode interp = InterpolationMode::CUBIC_BEZIER;
    bool selected = false;
};

struct CineCameraTrack {
    std::string name = "Camara de Cine";
    bool muted = false;
    std::vector<CineCameraKeyframe> keyframes;

    void addKeyframe(float time, const glm::vec3& pos, const glm::vec3& target, float focal_mm = 35.0f,
                     float focusDist = 5.0f, float aperture = 2.8f, float shakeInt = 0.0f, float shakeFreq = 1.5f,
                     InterpolationMode interp = InterpolationMode::CUBIC_BEZIER);
    bool sample(float time, glm::vec3& outPos, glm::vec3& outTarget, float& outFocal_mm,
                float& outFocusDist, float& outAperture, float& outShakeInt, float& outShakeFreq) const;
    void sortKeyframes();
    void removeKeyframe(size_t index);
    void deleteSelectedKeyframes();
    void duplicateSelectedKeyframes(float offset = 0.5f);
    void moveSelectedKeyframes(float dt);
    void setSelectedKeyframesInterp(InterpolationMode mode);
};

// ----------------------------------------------------------------------------
// 3. Pista de Entorno y Post-Proceso (Ciclo Solar 24h, Exposición EV)
// ----------------------------------------------------------------------------
struct EnvironmentKeyframe {
    float time = 0.0f;
    float hourOfDay = 12.0f;          // 0.0h a 24.0h
    float exposureEV = 1.0f;          // Compensación de auto-exposición
    float sunIntensity = 1.25f;       // Intensidad del sol direccional
    glm::vec3 sunColor = glm::vec3(1.0f, 0.98f, 0.95f);
    InterpolationMode interp = InterpolationMode::CUBIC_BEZIER;
    bool selected = false;
};

struct EnvironmentTrack {
    std::string name = "Entorno y Sol 24h";
    bool muted = false;
    std::vector<EnvironmentKeyframe> keyframes;

    void addKeyframe(float time, float hour, float exposure = 1.0f, float sunInt = 1.25f,
                     const glm::vec3& sunCol = glm::vec3(1.0f, 0.98f, 0.95f),
                     InterpolationMode interp = InterpolationMode::CUBIC_BEZIER);
    bool sample(float time, float& outHour, float& outExposure, float& outSunInt, glm::vec3& outSunCol) const;
    void sortKeyframes();
    void removeKeyframe(size_t index);
    void deleteSelectedKeyframes();
    void duplicateSelectedKeyframes(float offset = 0.5f);
    void moveSelectedKeyframes(float dt);
    void setSelectedKeyframesInterp(InterpolationMode mode);
};

// ----------------------------------------------------------------------------
// 4. Pista de Eventos Lua (Trigger Keys)
// ----------------------------------------------------------------------------
struct LuaEventKeyframe {
    float time = 0.0f;
    std::string command;
    bool executed = false;
    bool selected = false;
};

struct LuaEventTrack {
    std::string name = "Eventos Lua";
    bool muted = false;
    std::vector<LuaEventKeyframe> keyframes;

    void addKeyframe(float time, const std::string& cmd);
    void resetExecuted();
    void sortKeyframes();
    void removeKeyframe(size_t index);
    void deleteSelectedKeyframes();
    void duplicateSelectedKeyframes(float offset = 0.5f);
    void moveSelectedKeyframes(float dt);
};

// ----------------------------------------------------------------------------
// 5. Motor Central de Animación Cinemática: Sequencer (Fase 22)
// ----------------------------------------------------------------------------
class Sequencer {
public:
    static Sequencer& get();

    void init();
    void update(float deltaTime, Scene& scene, Camera& camera, DayNightCycle* dayNight = nullptr,
                ScriptEngine* scriptEngine = nullptr, EditorUI* editorUI = nullptr);

    // Control de Reproducción
    void play() { m_isPlaying = true; }
    void pause() { m_isPlaying = false; }
    void stop();
    void setTime(float t);
    float getCurrentTime() const { return m_currentTime; }
    float getDuration() const { return m_duration; }
    void setDuration(float d) { m_duration = (d > 0.1f) ? d : 0.1f; }
    int getFPS() const { return m_fps; }
    void setFPS(int fps) { m_fps = (fps > 0) ? fps : 30; }
    float getPlaybackSpeed() const { return m_playbackSpeed; }
    void setPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
    bool isPlaying() const { return m_isPlaying; }
    bool isLooping() const { return m_isLooping; }
    void setLooping(bool loop) { m_isLooping = loop; }
    bool isAutoKeyEnabled() const { return m_autoKey; }
    void setAutoKeyEnabled(bool ak) { m_autoKey = ak; }

    // Opciones Cinematográficas de Dirección
    bool isPilotCineCamera() const { return m_pilotCineCamera; }
    void setPilotCineCamera(bool pilot) { m_pilotCineCamera = pilot; }
    bool isAnamorphicScope() const { return m_anamorphicScope239; }
    void setAnamorphicScope(bool scope) { m_anamorphicScope239 = scope; }
    bool isRuleOfThirds() const { return m_showRuleOfThirds; }
    void setRuleOfThirds(bool r) { m_showRuleOfThirds = r; }
    bool isMotionPathVisible() const { return m_showMotionPath; }
    void setMotionPathVisible(bool v) { m_showMotionPath = v; }

    SequencerViewMode getViewMode() const { return m_viewMode; }
    void setViewMode(SequencerViewMode mode) { m_viewMode = mode; }

    // Gestión de Pistas
    std::vector<EntityTransformTrack>& getEntityTracks() { return m_entityTracks; }
    const std::vector<EntityTransformTrack>& getEntityTracks() const { return m_entityTracks; }
    std::vector<CineCameraTrack>& getCameraTracks() { return m_cameraTracks; }
    const std::vector<CineCameraTrack>& getCameraTracks() const { return m_cameraTracks; }
    std::vector<EnvironmentTrack>& getEnvironmentTracks() { return m_envTracks; }
    const std::vector<EnvironmentTrack>& getEnvironmentTracks() const { return m_envTracks; }
    std::vector<LuaEventTrack>& getLuaTracks() { return m_luaTracks; }
    const std::vector<LuaEventTrack>& getLuaTracks() const { return m_luaTracks; }

    void addEntityTrack(const std::string& entName);
    void addCameraTrack(const std::string& name = "Camara de Cine");
    void addEnvironmentTrack(const std::string& name = "Entorno y Sol 24h");
    void addLuaTrack(const std::string& name = "Eventos Lua");

    void removeEntityTrack(size_t index);
    void removeCameraTrack(size_t index);
    void removeEnvironmentTrack(size_t index);
    void removeLuaTrack(size_t index);
    void clearAllTracks();

    // Inserción de Keyframes en el tiempo actual
    void recordEntityKeyframe(const Entity& entity, InterpolationMode interp = InterpolationMode::CUBIC_BEZIER);
    void recordCameraKeyframe(const Camera& camera, float focal_mm = 35.0f, float focusDist = 5.0f, float aperture = 2.8f, InterpolationMode interp = InterpolationMode::CUBIC_BEZIER);
    void recordEnvironmentKeyframe(float hour, float exposure = 1.0f, float sunInt = 1.25f, const glm::vec3& sunCol = glm::vec3(1.0f, 0.98f, 0.95f));

    // Sincronización Automática de Animaciones IA/Lua y Captura / Bake (Fase 22.5)
    void syncSceneAnimationsToSequencer(Scene& scene);
    void bakeScene(Scene& scene, float duration = 10.0f, float interval = 0.5f);

    // Manipulación de Keyframes Seleccionados (Atajos Blender: G, Shift+D, Supr/X, T)
    void deleteSelectedKeyframes();
    void duplicateSelectedKeyframes(float offset = 0.5f);
    void moveSelectedKeyframes(float dt);
    void setSelectedKeyframesInterp(InterpolationMode mode);
    void clearSelection();
    void selectAllKeyframes(bool select = true);
    bool hasSelectedKeyframes() const;
    int getSelectedKeyframeCount() const;
    void sortAllKeyframes();
    void requestSample() { m_forceSampleOnce = true; }

    // Vibración de cámara por impulso (para Lua o impactos físicos)
    void triggerImpulseShake(float intensity, float duration);

    // Presets Cinemáticos en 1 Clic
    void createDemoSequence(int presetIndex, Scene& scene, Camera& camera);

    // Serialización .asisseq (JSON)
    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);

    // Conversión de Lentes Físicos (Sensor 35mm Full-Frame: 24mm de alto)
    static float focalLengthToFOV(float f_mm);
    static float fovToFocalLength(float fov_deg);

    // Muestreo del estado de la cámara de cine
    bool getCineCameraState(glm::vec3& outPos, glm::vec3& outTarget, float& outFOV, float& outFocusDist, float& outAperture) const;

    // Interpolación matemática helper
    static float interpolateFloat(float v0, float v1, float alpha, InterpolationMode mode);
    static glm::vec3 interpolateVec3(const glm::vec3& v0, const glm::vec3& v1, float alpha, InterpolationMode mode);

private:
    Sequencer();
    ~Sequencer() = default;

    float m_currentTime = 0.0f;
    float m_duration = 10.0f;
    int m_fps = 30;
    float m_playbackSpeed = 1.0f;
    bool m_isPlaying = false;
    bool m_isLooping = true;
    bool m_forceSampleOnce = true;
    bool m_autoKey = false;
    bool m_pilotCineCamera = false;
    bool m_anamorphicScope239 = false;
    bool m_showRuleOfThirds = false;
    bool m_showMotionPath = true;
    SequencerViewMode m_viewMode = SequencerViewMode::DOPE_SHEET;

    // Vibración por impulso
    float m_impulseShakeIntensity = 0.0f;
    float m_impulseShakeDuration = 0.0f;
    float m_impulseShakeTimer = 0.0f;

    std::vector<EntityTransformTrack> m_entityTracks;
    std::vector<CineCameraTrack> m_cameraTracks;
    std::vector<EnvironmentTrack> m_envTracks;
    std::vector<LuaEventTrack> m_luaTracks;

    void sampleAll(Scene& scene, Camera& camera, DayNightCycle* dayNight, ScriptEngine* scriptEngine, EditorUI* editorUI);
};

} // namespace ASIS
