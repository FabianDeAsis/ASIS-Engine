#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <algorithm>

namespace ASIS {

class Camera;
class Shader;

/**
 * @brief Plano 3D para extracción analítica de Frustum de Cámara (Gribb & Hartmann)
 */
struct FrustumPlane {
    glm::vec3 normal = glm::vec3(0.0f);
    float distance = 0.0f;

    void normalize() {
        float len = glm::length(normal);
        if (len > 0.00001f) {
            normal /= len;
            distance /= len;
        }
    }

    float distanceToPoint(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

/**
 * @brief Frustum de Cámara con prueba de intersección AABB ultra-rápida
 */
class Frustum {
public:
    void update(const glm::mat4& viewProj);
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max, float guardMargin = 0.0f) const;

private:
    FrustumPlane m_planes[6]; // Izquierda, Derecha, Abajo, Arriba, Cerca, Lejos
};

/**
 * @brief Descriptor de Chunk en el Quadtree de OASIS
 */
struct OASISChunk {
    glm::vec2 offset;     // Esquina inferior izquierda (X, Z) en coordenadas de mundo
    float size;           // Dimensión en metros (ancho y largo)
    int lod;              // Nivel de detalle (0 = centimétrico ultra-detallado, 1, 2, 3, 4)
    float morphFactor;    // Factor de Geomorphing continuo [0.0 - 1.0] para evitar popping
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

/**
 * @brief Telemetría y Estadísticas de Rendimiento en Tiempo Real de OASIS
 */
struct OASISTelemetry {
    int totalEvaluatedChunks = 0;
    int visibleChunksCount = 0;
    int culledChunksCount = 0;
    size_t renderedTriangles = 0;
    size_t totalUnoptimizedTriangles = 0;
    float triangleReductionPercent = 0.0f;
    float evalTimeMs = 0.0f;
};

/**
 * @brief OASIS: Occlusion and Adaptive Streaming of Infinite Surfaces (Módulo 1: Terreno)
 * 
 * Implementa una arquitectura CDLOD (Continuous Distance-Dependent Level of Detail)
 * con Quadtree de Chunks, Frustum Culling a nivel de bloque, Geomorphing sin rotura
 * y Desplazamiento Geométrico Real en GPU mediante Heightmaps flotantes de alta precisión (R32F).
 */
class OASISTerrain {
public:
    OASISTerrain();
    ~OASISTerrain();

    bool init(int patchGridResolution = 33);
    void shutdown();

    // Actualización de la textura de altura GPU (R32F)
    void updateHeightmap(const std::vector<float>& heights, int resolution);
    void updateHeightmapSubRegion(const std::vector<float>& heights, int resolution, int startX, int startZ, int width, int height);

    // Selección dinámica de LOD y Frustum Culling
    void update(const Camera& camera, float worldSize, float baseOffset, float minHeight, float maxHeight, float aspectRatio = 16.0f / 9.0f);

    // Renderizado de chunks seleccionados
    void render(Shader& shader, float worldSize, float baseOffset);

    // Configuración y Opciones
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool e) { m_enabled = e; }

    bool isFrustumCullingEnabled() const { return m_frustumCulling; }
    void setFrustumCulling(bool e) { m_frustumCulling = e; }

    bool isGeomorphingEnabled() const { return m_geomorphing; }
    void setGeomorphing(bool e) { m_geomorphing = e; }

    bool isDebugLODEnabled() const { return m_debugLOD; }
    void setDebugLOD(bool e) { m_debugLOD = e; }

    int getMaxLOD() const { return m_maxLOD; }
    void setMaxLOD(int lod) { m_maxLOD = std::clamp(lod, 1, 7); }

    float getLodDistanceFactor() const { return m_lodDistanceFactor; }
    void setLodDistanceFactor(float factor) { m_lodDistanceFactor = std::max(0.2f, factor); }

    int getPatchResolution() const { return m_patchResolution; }
    void setPatchResolution(int res); // 17, 33 o 65

    const OASISTelemetry& getTelemetry() const { return m_telemetry; }
    GLuint getHeightmapTexture() const { return m_heightmapTexture; }

private:
    void buildPatchMesh(int res);
    void evaluateQuadtree(const glm::vec2& offset, float size, int lod,
                          const glm::vec3& camPos, const Frustum& frustum,
                          float worldSize, float baseOffset, float minHeight, float maxHeight);

    bool m_enabled = true;
    bool m_frustumCulling = true;
    bool m_geomorphing = true;
    bool m_debugLOD = false;
    int m_maxLOD = 6;
    float m_lodDistanceFactor = 1.0f;
    int m_patchResolution = 33;

    GLuint m_patchVAO = 0;
    GLuint m_patchVBO = 0;
    GLuint m_patchEBO = 0;
    GLsizei m_patchIndexCount = 0;

    GLuint m_heightmapTexture = 0;
    int m_heightmapResolution = 0;

    Frustum m_frustum;
    std::vector<OASISChunk> m_visibleChunks;
    OASISTelemetry m_telemetry;
};

} // namespace ASIS
