#include "OASISMeshlet.h"
#include "Mesh.h"
#include "OASISTerrain.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace ASIS {

static glm::vec3 hsvToRgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;
    glm::vec3 rgb(0.0f);
    if (h < 1.0f / 6.0f)      rgb = glm::vec3(c, x, 0.0f);
    else if (h < 2.0f / 6.0f) rgb = glm::vec3(x, c, 0.0f);
    else if (h < 3.0f / 6.0f) rgb = glm::vec3(0.0f, c, x);
    else if (h < 4.0f / 6.0f) rgb = glm::vec3(0.0f, x, c);
    else if (h < 5.0f / 6.0f) rgb = glm::vec3(x, 0.0f, c);
    else                      rgb = glm::vec3(c, 0.0f, x);
    return rgb + glm::vec3(m);
}

void OASISMeshletContainer::buildFromMesh(const std::vector<Vertex>& vertices, 
                                          std::vector<unsigned int>& inOutIndices, 
                                          uint32_t maxTrianglesPerMeshlet) {
    m_meshlets.clear();
    if (vertices.empty() || inOutIndices.size() < 3) return;

    if (maxTrianglesPerMeshlet < 16) maxTrianglesPerMeshlet = 16;
    if (maxTrianglesPerMeshlet > 128) maxTrianglesPerMeshlet = 128;

    size_t totalTriangles = inOutIndices.size() / 3;

    struct TriInfo {
        uint32_t i0, i1, i2;
        glm::vec3 centroid;
        glm::vec3 normal;
        bool assigned;
    };

    std::vector<TriInfo> triangles(totalTriangles);
    for (size_t t = 0; t < totalTriangles; ++t) {
        uint32_t i0 = inOutIndices[t * 3 + 0];
        uint32_t i1 = inOutIndices[t * 3 + 1];
        uint32_t i2 = inOutIndices[t * 3 + 2];

        const glm::vec3& p0 = vertices[i0].position;
        const glm::vec3& p1 = vertices[i1].position;
        const glm::vec3& p2 = vertices[i2].position;

        triangles[t].i0 = i0;
        triangles[t].i1 = i1;
        triangles[t].i2 = i2;
        triangles[t].centroid = (p0 + p1 + p2) * (1.0f / 3.0f);

        glm::vec3 crossNorm = glm::cross(p1 - p0, p2 - p0);
        float len = glm::length(crossNorm);
        triangles[t].normal = (len > 0.00001f) ? (crossNorm / len) : glm::vec3(0.0f, 1.0f, 0.0f);
        triangles[t].assigned = false;
    }

    std::vector<unsigned int> newIndices;
    newIndices.reserve(inOutIndices.size());

    uint32_t meshletIndex = 0;

    // Agrupación ávida en clusters compactos por proximidad y coherencia de normales
    for (size_t i = 0; i < totalTriangles; ++i) {
        if (triangles[i].assigned) continue;

        OASISMeshlet meshlet;
        meshlet.indexOffset = static_cast<uint32_t>(newIndices.size());

        glm::vec3 avgNormal = triangles[i].normal;
        glm::vec3 clusterCenter = triangles[i].centroid;
        uint32_t clusterCount = 0;

        triangles[i].assigned = true;
        newIndices.push_back(triangles[i].i0);
        newIndices.push_back(triangles[i].i1);
        newIndices.push_back(triangles[i].i2);
        clusterCount++;

        // Buscar triángulos vecinos cercanos con normales afines
        for (size_t j = i + 1; j < totalTriangles && clusterCount < maxTrianglesPerMeshlet; ++j) {
            if (triangles[j].assigned) continue;

            glm::vec3 diff = triangles[j].centroid - clusterCenter;
            float distSq = glm::dot(diff, diff);
            float normDot = glm::dot(triangles[j].normal, glm::normalize(avgNormal));

            // Si está espacialmente próximo y no de espaldas
            if (normDot > 0.2f || distSq < 4.0f) {
                triangles[j].assigned = true;
                newIndices.push_back(triangles[j].i0);
                newIndices.push_back(triangles[j].i1);
                newIndices.push_back(triangles[j].i2);

                avgNormal += triangles[j].normal;
                clusterCenter = (clusterCenter * static_cast<float>(clusterCount) + triangles[j].centroid) * (1.0f / static_cast<float>(clusterCount + 1));
                clusterCount++;
            }
        }

        meshlet.triangleCount = clusterCount;
        meshlet.indexCount = clusterCount * 3;

        // 1. Calcular AABB local
        glm::vec3 minP(1e9f);
        glm::vec3 maxP(-1e9f);
        glm::vec3 sumPos(0.0f);
        size_t vertCount = 0;

        for (uint32_t k = 0; k < meshlet.indexCount; ++k) {
            uint32_t idx = newIndices[meshlet.indexOffset + k];
            const glm::vec3& p = vertices[idx].position;
            minP = glm::min(minP, p);
            maxP = glm::max(maxP, p);
            sumPos += p;
            vertCount++;
        }

        meshlet.aabbMin = minP;
        meshlet.aabbMax = maxP;

        // 2. Calcular Bounding Sphere local
        meshlet.sphereCenter = (vertCount > 0) ? (sumPos * (1.0f / static_cast<float>(vertCount))) : ((minP + maxP) * 0.5f);
        float maxDistSq = 0.0f;
        for (uint32_t k = 0; k < meshlet.indexCount; ++k) {
            uint32_t idx = newIndices[meshlet.indexOffset + k];
            glm::vec3 diff = vertices[idx].position - meshlet.sphereCenter;
            float d2 = glm::dot(diff, diff);
            if (d2 > maxDistSq) maxDistSq = d2;
        }
        meshlet.sphereRadius = std::sqrt(maxDistSq);

        // 3. Cono de Normales para Backface Cluster Culling
        float normLen = glm::length(avgNormal);
        meshlet.coneAxis = (normLen > 0.0001f) ? (avgNormal / normLen) : glm::vec3(0.0f, 1.0f, 0.0f);

        float minCos = 1.0f;
        for (uint32_t k = 0; k < meshlet.indexCount; k += 3) {
            uint32_t i0 = newIndices[meshlet.indexOffset + k + 0];
            uint32_t i1 = newIndices[meshlet.indexOffset + k + 1];
            uint32_t i2 = newIndices[meshlet.indexOffset + k + 2];
            glm::vec3 n = glm::normalize(glm::cross(vertices[i1].position - vertices[i0].position,
                                                    vertices[i2].position - vertices[i0].position));
            float dotN = glm::dot(n, meshlet.coneAxis);
            if (dotN < minCos) minCos = dotN;
        }
        meshlet.coneCutoff = minCos;

        // 4. Color distributivo estilo Nanite con razón áurea
        float hue = std::fmod(static_cast<float>(meshletIndex) * 0.618033988749895f, 1.0f);
        meshlet.debugColor = hsvToRgb(hue, 0.85f, 0.95f);

        m_meshlets.push_back(meshlet);
        meshletIndex++;
    }

    // Reemplazar índices del Mesh con el nuevo orden clusterizado
    inOutIndices = std::move(newIndices);
}

void OASISMeshletContainer::cullAndCollect(const glm::mat4& modelMatrix,
                                          const glm::vec3& camPos,
                                          const Frustum& frustum,
                                          const OASISMeshletConfig& config,
                                          std::vector<GLsizei>& outCounts,
                                          std::vector<void*>& outOffsets,
                                          std::vector<glm::vec3>& outColors,
                                          OASISMeshletTelemetry& telemetry) const {
    if (m_meshlets.empty()) return;

    // Escala máxima del modelo en espacio de mundo
    float scaleX = glm::length(glm::vec3(modelMatrix[0]));
    float scaleY = glm::length(glm::vec3(modelMatrix[1]));
    float scaleZ = glm::length(glm::vec3(modelMatrix[2]));
    float maxScale = std::max(scaleX, std::max(scaleY, scaleZ));

    // Matriz de normales para transformar los conos de orientación
    glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

    for (const auto& meshlet : m_meshlets) {
        telemetry.totalMeshletsEvaluated++;
        telemetry.totalModelTriangles += meshlet.triangleCount;

        // 1. Transformar Bounding Sphere a mundo
        glm::vec3 worldSphereCenter = glm::vec3(modelMatrix * glm::vec4(meshlet.sphereCenter, 1.0f));
        float worldSphereRadius = meshlet.sphereRadius * maxScale;

        // 2. Frustum Culling
        if (config.frustumCulling) {
            // AABB de la esfera en espacio de mundo
            glm::vec3 aabbMin = worldSphereCenter - glm::vec3(worldSphereRadius);
            glm::vec3 aabbMax = worldSphereCenter + glm::vec3(worldSphereRadius);

            if (!frustum.isBoxVisible(aabbMin, aabbMax, worldSphereRadius * 0.5f)) {
                telemetry.frustumCulledMeshlets++;
                continue; // Descartado fuera del campo de visión
            }
        }

        // 3. Backface Cluster Cone Culling
        if (config.backfaceCulling) {
            glm::vec3 toCam = camPos - worldSphereCenter;
            float dist = glm::length(toCam);

            if (dist > worldSphereRadius && dist > 0.001f) {
                glm::vec3 viewDir = toCam / dist;
                glm::vec3 worldConeAxis = glm::normalize(normalMat * meshlet.coneAxis);

                // Ángulo subtendido por la esfera del cluster
                float sinSubtended = std::clamp(worldSphereRadius / dist, 0.0f, 1.0f);
                float angleSubtended = std::asin(sinSubtended);

                // Ángulo de apertura de normales del cluster
                float coneCutoffClamped = std::clamp(meshlet.coneCutoff, -1.0f, 1.0f);
                float coneHalfAngle = std::acos(coneCutoffClamped);

                // Si todas las normales miran en dirección opuesta a la cámara
                float thresholdAngle = coneHalfAngle + angleSubtended;
                if (thresholdAngle < 1.5707963f) { // Menor a 90 grados
                    float cosThreshold = std::cos(thresholdAngle);
                    float viewDotAxis = glm::dot(viewDir, worldConeAxis);

                    // Si el producto punto es negativo con margen, todas las caras están de espaldas
                    if (viewDotAxis < -cosThreshold) {
                        telemetry.backfaceCulledMeshlets++;
                        continue; // Descartado: todas las caras miran hacia atrás
                    }
                }
            }
        }

        // 4. Cluster Aprobado: Agregar a lista de dibujo
        outCounts.push_back(static_cast<GLsizei>(meshlet.indexCount));
        outOffsets.push_back(reinterpret_cast<void*>(static_cast<uintptr_t>(meshlet.indexOffset * sizeof(uint32_t))));
        outColors.push_back(meshlet.debugColor);

        telemetry.visibleMeshletsCount++;
        telemetry.renderedTriangles += meshlet.triangleCount;
    }

    if (telemetry.totalModelTriangles > 0) {
        float saved = 1.0f - (static_cast<float>(telemetry.renderedTriangles) / static_cast<float>(telemetry.totalModelTriangles));
        telemetry.triangleSavingsPercent = std::clamp(saved * 100.0f, 0.0f, 99.9f);
    }
}

} // namespace ASIS
