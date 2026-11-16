#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace ASIS {

class Frustum;
class Camera;
class Shader;
struct Vertex;

/**
 * @brief Descriptor de un Cluster / Meshlet en el sistema de Geometría Virtual OASIS
 * 
 * Cada Meshlet contiene un subconjunto compacto contiguo de triángulos (hasta 64-128)
 * con volúmenes envolventes (AABB y Esfera) para Frustum Culling
 * y un Cono de Normales para Backface Cluster Culling (descarte de caras posteriores en bloque).
 */
struct OASISMeshlet {
    uint32_t indexOffset = 0;   // Offset en el buffer de índices reorganizado
    uint32_t indexCount = 0;    // Cantidad de índices (3 por triángulo)
    uint32_t triangleCount = 0; // Cantidad de triángulos

    // Volúmenes de contorno en espacio local
    glm::vec3 aabbMin = glm::vec3(0.0f);
    glm::vec3 aabbMax = glm::vec3(0.0f);
    glm::vec3 sphereCenter = glm::vec3(0.0f);
    float sphereRadius = 0.0f;

    // Cono de Normales para Backface Cluster Culling
    glm::vec3 coneAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Eje promedio de normales
    float coneCutoff = 0.0f;                          // Coseno del semi-ángulo de dispersión

    // Color distintivo pseudo-aleatorio para el visualizador estilo Nanite
    glm::vec3 debugColor = glm::vec3(1.0f);
};

/**
 * @brief Telemetría global de rendimiento del Módulo 2 de OASIS para mallas y modelos
 */
struct OASISMeshletTelemetry {
    size_t totalMeshletsEvaluated = 0;
    size_t visibleMeshletsCount = 0;
    size_t frustumCulledMeshlets = 0;
    size_t backfaceCulledMeshlets = 0;
    size_t renderedTriangles = 0;
    size_t totalModelTriangles = 0;
    float triangleSavingsPercent = 0.0f;

    void reset() {
        totalMeshletsEvaluated = 0;
        visibleMeshletsCount = 0;
        frustumCulledMeshlets = 0;
        backfaceCulledMeshlets = 0;
        renderedTriangles = 0;
        totalModelTriangles = 0;
        triangleSavingsPercent = 0.0f;
    }
};

/**
 * @brief Opciones globales del Módulo 2 de OASIS para entidades y mallas 3D
 */
struct OASISMeshletConfig {
    bool enabled = true;
    bool frustumCulling = true;
    bool backfaceCulling = true;
    bool debugColors = false; // Nanite-style cluster color visualizer
};

/**
 * @brief Contenedor y Generador de Meshlets para una malla geométrica
 */
class OASISMeshletContainer {
public:
    OASISMeshletContainer() = default;
    ~OASISMeshletContainer() = default;

    // Generar clusters de Meshlets a partir de vértices e índices
    void buildFromMesh(const std::vector<Vertex>& vertices, 
                       std::vector<unsigned int>& inOutIndices, 
                       uint32_t maxTrianglesPerMeshlet = 64);

    // Evaluar culling por Frustum y Backface Cone Culling para una instancia
    void cullAndCollect(const glm::mat4& modelMatrix,
                        const glm::vec3& camPos,
                        const Frustum& frustum,
                        const OASISMeshletConfig& config,
                        std::vector<GLsizei>& outCounts,
                        std::vector<void*>& outOffsets,
                        std::vector<glm::vec3>& outColors,
                        OASISMeshletTelemetry& telemetry) const;

    const std::vector<OASISMeshlet>& getMeshlets() const { return m_meshlets; }
    size_t getMeshletCount() const { return m_meshlets.size(); }
    bool hasMeshlets() const { return !m_meshlets.empty(); }

private:
    std::vector<OASISMeshlet> m_meshlets;
};

} // namespace ASIS
