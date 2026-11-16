#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include "OASISTerrain.h"

namespace ASIS {

class Camera;
class Shader;

enum class WaterPreset {
    CALM_LAKE = 0,
    TROPICAL_CARIBBEAN = 1,
    OPEN_OCEAN = 2,
    STORMY_SEA = 3
};

struct GerstnerWave {
    glm::vec2 direction = glm::vec2(1.0f, 0.0f);
    float amplitude = 0.35f;
    float wavelength = 12.0f;
    float speed = 1.8f;
    float steepness = 0.45f;
};

/**
 * @brief Descriptor de Chunk en el Quadtree de OASIS Océano
 */
struct WaterChunk {
    glm::vec2 offset;     // Esquina inferior izquierda (X, Z) en coordenadas de mundo
    float size;           // Dimensión en metros (ancho y largo)
    int lod;              // Nivel de detalle (0 = centimétrico ultra-detallado)
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

/**
 * @brief Telemetría y Estadísticas de Rendimiento en Tiempo Real de OASIS Océano
 */
struct WaterOASISTelemetry {
    int totalEvaluatedChunks = 0;
    int visibleChunksCount = 0;
    int culledChunksCount = 0;
    size_t renderedTriangles = 0;
    size_t totalUnoptimizedTriangles = 0;
    float triangleReductionPercent = 0.0f;
};

/**
 * @brief Sistema de Océano y Cuerpos de Agua 3D en Tiempo Real
 * 
 * Implementa una simulación física de olas Gerstner (GPU + CPU),
 * absorción de luz por profundidad de Beer-Lambert (turquesa a azul marino),
 * espuma de contacto con orillas y rocas basada en el Depth Buffer,
 * espuma en crestas de olas, reflejos Fresnel PBR, refracción subacuática
 * y flotabilidad física para rigid bodies.
 */
class Water {
public:
    Water();
    ~Water();

    bool init(int gridResolution = 128, float size = 700.0f);
    void shutdown();
    bool setGridResolution(int res);

    void update(float dt);
    void draw(Shader& shader, const Camera& camera, float aspectRatio,
              GLuint opaqueDepthTex, GLuint opaqueColorTex,
              const glm::vec3& sunDir, const glm::vec3& sunColor,
              const glm::vec3& ambientColor, GLuint skyboxEnvMap,
              bool ssrEnabled = false, int ssrMaxSteps = 64, float ssrStepSize = 0.15f,
              float ssrThickness = 0.05f, float ssrIntensity = 0.85f);

    // Consultas físicas para flotabilidad (Bullet Physics) y natación
    float getWaterHeightAt(float worldX, float worldZ) const;
    glm::vec3 getWaterNormalAt(float worldX, float worldZ) const;
    bool isUnderwater(const glm::vec3& camPos) const {
        return m_enabled && (camPos.y < (getWaterHeightAt(camPos.x, camPos.z) + 0.1f));
    }

    // Preajustes rápidos de agua
    void applyPreset(WaterPreset preset);

    // Getters y Setters
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool e) { m_enabled = e; }

    float getWaterLevel() const { return m_waterLevel; }
    void setWaterLevel(float y) { m_waterLevel = y; }

    float getWaveAmplitude() const { return m_waveAmplitude; }
    void setWaveAmplitude(float a) { m_waveAmplitude = (a >= 0.0f) ? a : 0.0f; }

    float getWaveSpeed() const { return m_waveSpeed; }
    void setWaveSpeed(float s) { m_waveSpeed = s; }

    glm::vec3 getShallowColor() const { return m_shallowColor; }
    void setShallowColor(const glm::vec3& c) { m_shallowColor = c; }

    glm::vec3 getDeepColor() const { return m_deepColor; }
    void setDeepColor(const glm::vec3& c) { m_deepColor = c; }

    float getClarity() const { return m_clarity; }
    void setClarity(float c) { m_clarity = (c > 0.001f) ? c : 0.001f; }

    glm::vec3 getFoamColor() const { return m_foamColor; }
    void setFoamColor(const glm::vec3& c) { m_foamColor = c; }

    float getFoamThreshold() const { return m_foamThreshold; }
    void setFoamThreshold(float t) { m_foamThreshold = (t >= 0.0f) ? t : 0.0f; }

    float getFoamIntensity() const { return m_foamIntensity; }
    void setFoamIntensity(float i) { m_foamIntensity = (i >= 0.0f) ? i : 0.0f; }

    float getRefractionStrength() const { return m_refractionStrength; }
    void setRefractionStrength(float r) { m_refractionStrength = (r >= 0.0f) ? r : 0.0f; }

    float getRoughness() const { return m_roughness; }
    void setRoughness(float r) { m_roughness = std::clamp(r, 0.01f, 1.0f); }

    float getSpecular() const { return m_specular; }
    void setSpecular(float s) { m_specular = (s >= 0.0f) ? s : 0.0f; }

    // Efecto Horizonte Atmosférico
    float getHorizonDistance() const { return m_horizonDistance; }
    void setHorizonDistance(float d) { m_horizonDistance = (d > 10.0f) ? d : 10.0f; }

    float getHorizonBlend() const { return m_horizonBlend; }
    void setHorizonBlend(float b) { m_horizonBlend = std::clamp(b, 0.05f, 0.95f); }

    float getSize() const { return m_size; }
    int getResolution() const { return m_gridResolution; }
    float getTime() const { return m_time; }

    std::vector<GerstnerWave>& getWaves() { return m_waves; }
    const std::vector<GerstnerWave>& getWaves() const { return m_waves; }

    // Sistema OASIS Océano (Geometría Virtual y Streaming Adaptativo CDLOD)
    bool isOASISEnabled() const { return m_oasisEnabled; }
    void setOASISEnabled(bool e) { m_oasisEnabled = e; }

    bool isFrustumCullingEnabled() const { return m_frustumCulling; }
    void setFrustumCullingEnabled(bool e) { m_frustumCulling = e; }

    float getOASISLodFactor() const { return m_oasisLodFactor; }
    void setOASISLodFactor(float factor) { m_oasisLodFactor = std::clamp(factor, 0.1f, 3.0f); }

    int getMaxOASISLod() const { return m_maxOASISLod; }
    void setMaxOASISLod(int lod) { m_maxOASISLod = std::clamp(lod, 1, 6); }

    const WaterOASISTelemetry& getOASISTelemetry() const { return m_telemetry; }

private:
    void buildWaterMesh(int resolution, float size);
    void buildOASISPatch(int patchRes = 33);
    void evaluateQuadtree(const glm::vec2& offset, float size, int lod, const glm::vec3& camPos, const Frustum& frustum);
    void createProceduralTextures();

    bool m_enabled = true;
    float m_waterLevel = -3.5f; // Altitud Y de la superficie de agua
    float m_size = 700.0f;
    int m_gridResolution = 128;
    float m_time = 0.0f;

    // Parámetros OASIS Océano
    bool m_oasisEnabled = true;
    bool m_frustumCulling = true;
    float m_oasisLodFactor = 0.45f;
    int m_maxOASISLod = 4;
    GLuint m_patchVao = 0;
    GLuint m_patchVbo = 0;
    GLuint m_patchEbo = 0;
    GLsizei m_patchIndexCount = 0;
    std::vector<WaterChunk> m_visibleChunks;
    WaterOASISTelemetry m_telemetry;

    // Parámetros de oleaje
    float m_waveAmplitude = 0.45f;
    float m_waveSpeed = 1.0f;
    std::vector<GerstnerWave> m_waves;

    // Óptica y Sombras PBR
    glm::vec3 m_shallowColor = glm::vec3(0.08f, 0.58f, 0.65f); // Turquesa cristalino
    glm::vec3 m_deepColor = glm::vec3(0.01f, 0.08f, 0.28f);    // Azul marino oceánico
    float m_clarity = 0.22f;                                    // Coeficiente de absorción Beer-Lambert
    glm::vec3 m_foamColor = glm::vec3(0.96f, 0.98f, 1.0f);     // Espuma blanca
    float m_foamThreshold = 1.2f;                               // Alcance de espuma en orilla (m)
    float m_foamIntensity = 1.4f;
    float m_refractionStrength = 0.035f;
    float m_roughness = 0.06f;
    float m_specular = 1.0f;
    float m_horizonDistance = 330.0f;
    float m_horizonBlend = 0.40f;

    // Recursos OpenGL
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLsizei m_indexCount = 0;

    GLuint m_normalMapTex = 0;
    GLuint m_foamNoiseTex = 0;
};

} // namespace ASIS
