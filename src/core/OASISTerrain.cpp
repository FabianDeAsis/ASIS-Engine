#include "OASISTerrain.h"
#include "Camera.h"
#include "Shader.h"
#include <chrono>
#include <iostream>
#include <cmath>

namespace ASIS {

// ============================================================================
// Frustum de Cámara con prueba analítica de intersección AABB
// ============================================================================
void Frustum::update(const glm::mat4& vp) {
    // Extracción analítica de planos de corte según Gribb & Hartmann (OpenGL NDC: [-w, w])
    // Column-major: vp[col][row]
    glm::vec4 row0(vp[0][0], vp[1][0], vp[2][0], vp[3][0]);
    glm::vec4 row1(vp[0][1], vp[1][1], vp[2][1], vp[3][1]);
    glm::vec4 row2(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
    glm::vec4 row3(vp[0][3], vp[1][3], vp[2][3], vp[3][3]);

    // 0: Plano Izquierdo (Left: row3 + row0)
    glm::vec4 pLeft = row3 + row0;
    m_planes[0].normal = glm::vec3(pLeft.x, pLeft.y, pLeft.z);
    m_planes[0].distance = pLeft.w;
    m_planes[0].normalize();

    // 1: Plano Derecho (Right: row3 - row0)
    glm::vec4 pRight = row3 - row0;
    m_planes[1].normal = glm::vec3(pRight.x, pRight.y, pRight.z);
    m_planes[1].distance = pRight.w;
    m_planes[1].normalize();

    // 2: Plano Inferior (Bottom: row3 + row1)
    glm::vec4 pBottom = row3 + row1;
    m_planes[2].normal = glm::vec3(pBottom.x, pBottom.y, pBottom.z);
    m_planes[2].distance = pBottom.w;
    m_planes[2].normalize();

    // 3: Plano Superior (Top: row3 - row1)
    glm::vec4 pTop = row3 - row1;
    m_planes[3].normal = glm::vec3(pTop.x, pTop.y, pTop.z);
    m_planes[3].distance = pTop.w;
    m_planes[3].normalize();

    // 4: Plano Cercano (Near: row3 + row2)
    glm::vec4 pNear = row3 + row2;
    m_planes[4].normal = glm::vec3(pNear.x, pNear.y, pNear.z);
    m_planes[4].distance = pNear.w;
    m_planes[4].normalize();

    // 5: Plano Lejano (Far: row3 - row2)
    glm::vec4 pFar = row3 - row2;
    m_planes[5].normal = glm::vec3(pFar.x, pFar.y, pFar.z);
    m_planes[5].distance = pFar.w;
    m_planes[5].normalize();
}

bool Frustum::isBoxVisible(const glm::vec3& min, const glm::vec3& max, float guardMargin) const {
    for (int i = 0; i < 6; ++i) {
        // Encontrar el vértice más adelantado en la dirección de la normal del plano
        glm::vec3 p;
        p.x = (m_planes[i].normal.x > 0.0f) ? max.x : min.x;
        p.y = (m_planes[i].normal.y > 0.0f) ? max.y : min.y;
        p.z = (m_planes[i].normal.z > 0.0f) ? max.z : min.z;

        // Tolerancia negativa / Guard Band para evitar corte prematuro en bordes y esquinas
        if (m_planes[i].distanceToPoint(p) < -guardMargin) {
            return false; // La caja está completamente fuera de este plano
        }
    }
    return true; // La caja interseca o está totalmente dentro del Frustum
}

// ============================================================================
// OASISTerrain: Sistema CDLOD & Geometría Virtual Adaptativa
// ============================================================================
OASISTerrain::OASISTerrain()
    : m_enabled(true),
      m_frustumCulling(true),
      m_geomorphing(true),
      m_debugLOD(false),
      m_maxLOD(4),
      m_lodDistanceFactor(1.0f),
      m_patchResolution(33),
      m_patchVAO(0),
      m_patchVBO(0),
      m_patchEBO(0),
      m_patchIndexCount(0),
      m_heightmapTexture(0),
      m_heightmapResolution(0)
{
}

OASISTerrain::~OASISTerrain() {
    shutdown();
}

bool OASISTerrain::init(int patchGridResolution) {
    shutdown();
    buildPatchMesh(patchGridResolution);
    return true;
}

void OASISTerrain::shutdown() {
    if (m_patchVAO != 0) {
        glDeleteVertexArrays(1, &m_patchVAO);
        glDeleteBuffers(1, &m_patchVBO);
        glDeleteBuffers(1, &m_patchEBO);
        m_patchVAO = 0;
        m_patchVBO = 0;
        m_patchEBO = 0;
        m_patchIndexCount = 0;
    }
    if (m_heightmapTexture != 0) {
        glDeleteTextures(1, &m_heightmapTexture);
        m_heightmapTexture = 0;
        m_heightmapResolution = 0;
    }
}

void OASISTerrain::setPatchResolution(int res) {
    if (res < 9) res = 9;
    if (res > 129) res = 129;
    if (res % 2 == 0) res += 1; // Debe ser impar para alineación simétrica de quads
    m_patchResolution = res;
    buildPatchMesh(res);
}

void OASISTerrain::buildPatchMesh(int res) {
    if (m_patchVAO != 0) {
        glDeleteVertexArrays(1, &m_patchVAO);
        glDeleteBuffers(1, &m_patchVBO);
        glDeleteBuffers(1, &m_patchEBO);
    }

    m_patchResolution = res;

    struct PatchVertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    std::vector<PatchVertex> vertices;
    vertices.reserve(res * res);

    float step = 1.0f / static_cast<float>(res - 1);
    for (int z = 0; z < res; ++z) {
        for (int x = 0; x < res; ++x) {
            PatchVertex v;
            v.pos = glm::vec3(static_cast<float>(x) * step, 0.0f, static_cast<float>(z) * step);
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.uv = glm::vec2(v.pos.x, v.pos.z);
            vertices.push_back(v);
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve((res - 1) * (res - 1) * 6);
    for (int z = 0; z < res - 1; ++z) {
        for (int x = 0; x < res - 1; ++x) {
            uint32_t i0 = z * res + x;
            uint32_t i1 = (z + 1) * res + x;
            uint32_t i2 = (z + 1) * res + (x + 1);
            uint32_t i3 = z * res + (x + 1);

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    m_patchIndexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &m_patchVAO);
    glGenBuffers(1, &m_patchVBO);
    glGenBuffers(1, &m_patchEBO);

    glBindVertexArray(m_patchVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_patchVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(PatchVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Posición normalizada [0, 1] en Location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PatchVertex), (void*)offsetof(PatchVertex, pos));

    // Normal base en Location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PatchVertex), (void*)offsetof(PatchVertex, normal));

    // Coordenada UV normalizada en Location 2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PatchVertex), (void*)offsetof(PatchVertex, uv));

    glBindVertexArray(0);
}

void OASISTerrain::updateHeightmap(const std::vector<float>& heights, int resolution) {
    if (heights.empty() || resolution <= 0) return;

    m_heightmapResolution = resolution;

    if (m_heightmapTexture == 0) {
        glGenTextures(1, &m_heightmapTexture);
    }

    glBindTexture(GL_TEXTURE_2D, m_heightmapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, resolution, resolution, 0, GL_RED, GL_FLOAT, heights.data());

    // Filtrado bilineal en GPU para transición continua de alturas sin escalones
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OASISTerrain::updateHeightmapSubRegion(const std::vector<float>& heights, int resolution, 
                                           int startX, int startZ, int width, int height) {
    if (m_heightmapTexture == 0 || heights.empty()) {
        updateHeightmap(heights, resolution);
        return;
    }

    if (startX < 0) startX = 0;
    if (startZ < 0) startZ = 0;
    if (startX + width > resolution) width = resolution - startX;
    if (startZ + height > resolution) height = resolution - startZ;
    if (width <= 0 || height <= 0) return;

    std::vector<float> subData(width * height);
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            subData[z * width + x] = heights[(startZ + z) * resolution + (startX + x)];
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_heightmapTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, startX, startZ, width, height, GL_RED, GL_FLOAT, subData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OASISTerrain::update(const Camera& camera, float worldSize, float baseOffset, float minHeight, float maxHeight, float aspectRatio) {
    auto tStart = std::chrono::high_resolution_clock::now();

    // 1. Extraer Frustum analítico de la cámara con el FOV y Aspect Ratio real del Viewport
    float aspect = (aspectRatio > 0.05f) ? aspectRatio : (16.0f / 9.0f);
    glm::mat4 viewProj = camera.getProjectionMatrix(aspect) * camera.getViewMatrix();
    m_frustum.update(viewProj);

    m_visibleChunks.clear();
    m_telemetry.totalEvaluatedChunks = 0;
    m_telemetry.visibleChunksCount = 0;
    m_telemetry.culledChunksCount = 0;

    glm::vec3 camPos = camera.getPosition();

    // 2. Definir chunks raíz que cubren todo el terreno
    // Usamos una cuadrícula balanceada adaptada al tamaño del mundo
    int rootDivs = std::clamp(static_cast<int>(std::round(worldSize / 50.0f)), 4, 12);
    float rootSize = worldSize / static_cast<float>(rootDivs);
    float halfWorld = worldSize * 0.5f;

    for (int rz = 0; rz < rootDivs; ++rz) {
        for (int rx = 0; rx < rootDivs; ++rx) {
            glm::vec2 offset(-halfWorld + static_cast<float>(rx) * rootSize,
                             -halfWorld + static_cast<float>(rz) * rootSize);
            evaluateQuadtree(offset, rootSize, 0, camPos, m_frustum, worldSize, baseOffset, minHeight, maxHeight);
        }
    }

    // 3. Métricas de Telemetría
    auto tEnd = std::chrono::high_resolution_clock::now();
    m_telemetry.evalTimeMs = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
    m_telemetry.renderedTriangles = static_cast<size_t>(m_visibleChunks.size()) * (m_patchResolution - 1) * (m_patchResolution - 1) * 2;

    // Comparación con renderizado tradicional sin optimizar a la resolución del heightmap
    int fullRes = (m_heightmapResolution > 0) ? m_heightmapResolution : 256;
    m_telemetry.totalUnoptimizedTriangles = static_cast<size_t>(fullRes - 1) * (fullRes - 1) * 2;

    if (m_telemetry.totalUnoptimizedTriangles > 0) {
        float saved = 1.0f - (static_cast<float>(m_telemetry.renderedTriangles) / static_cast<float>(m_telemetry.totalUnoptimizedTriangles));
        m_telemetry.triangleReductionPercent = std::clamp(saved * 100.0f, 0.0f, 99.9f);
    } else {
        m_telemetry.triangleReductionPercent = 0.0f;
    }
}

void OASISTerrain::evaluateQuadtree(const glm::vec2& offset, float size, int lod,
                                    const glm::vec3& camPos, const Frustum& frustum,
                                    float worldSize, float baseOffset, float minHeight, float maxHeight) {
    m_telemetry.totalEvaluatedChunks++;

    // Alturas completas de mundo contemplando baseOffset y márgenes de seguridad para picos/escultura
    float absMaxH = std::max(std::abs(maxHeight), std::abs(minHeight));
    float minY = baseOffset - absMaxH - 20.0f;
    float maxY = baseOffset + absMaxH + 30.0f;

    // Bounding Box 3D del Chunk en coordenadas de mundo
    glm::vec3 aabbMin(offset.x, minY, offset.y);
    glm::vec3 aabbMax(offset.x + size, maxY, offset.y + size);

    // Margen de seguridad lateral (Guard Band) ampliado para garantizar cero popping en bordes
    float guardMargin = size * 1.75f * std::max(1.0f, m_lodDistanceFactor * 0.5f) + 15.0f;

    // Culling por Frustum con Guard Band
    if (m_frustumCulling && !frustum.isBoxVisible(aabbMin, aabbMax, guardMargin)) {
        m_telemetry.culledChunksCount++;
        return; // Descartado fuera de la visión de la cámara
    }

    // Distancia Euclidiana mínima desde la cámara al AABB del chunk
    float dx = std::max(0.0f, std::max(aabbMin.x - camPos.x, camPos.x - aabbMax.x));
    float dy = std::max(0.0f, std::max(aabbMin.y - camPos.y, camPos.y - aabbMax.y));
    float dz = std::max(0.0f, std::max(aabbMin.z - camPos.z, camPos.z - aabbMax.z));
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Criterio CDLOD robusto: subdividir si la distancia de la cámara a este bloque es menor que su radio de cobertura
    // Un chunk de dimensión `size` se subdivide cuando la cámara está dentro de (size * 2.2f * m_lodDistanceFactor)
    float subdivThreshold = size * 2.2f * m_lodDistanceFactor;

    bool shouldSubdivide = (lod < m_maxLOD) && (dist < subdivThreshold) && (size > 1.2f);

    if (shouldSubdivide) {
        float half = size * 0.5f;
        evaluateQuadtree(offset, half, lod + 1, camPos, frustum, worldSize, baseOffset, minHeight, maxHeight);
        evaluateQuadtree(offset + glm::vec2(half, 0.0f), half, lod + 1, camPos, frustum, worldSize, baseOffset, minHeight, maxHeight);
        evaluateQuadtree(offset + glm::vec2(0.0f, half), half, lod + 1, camPos, frustum, worldSize, baseOffset, minHeight, maxHeight);
        evaluateQuadtree(offset + glm::vec2(half, half), half, lod + 1, camPos, frustum, worldSize, baseOffset, minHeight, maxHeight);
    } else {
        // Chunk hoja a renderizar
        OASISChunk chunk;
        chunk.offset = offset;
        chunk.size = size;
        chunk.lod = lod;
        chunk.aabbMin = aabbMin;
        chunk.aabbMax = aabbMax;

        // Cálculo de Geomorphing exacto hacia el padre para evitar popping entre niveles contiguos
        chunk.morphFactor = 0.0f;
        if (m_geomorphing) {
            if (lod == 0) {
                chunk.morphFactor = 0.0f; // Chunks raíz no hacen morph hacia un padre inexistente
            } else {
                float parentSubdivThreshold = (size * 2.0f) * 2.2f * m_lodDistanceFactor;
                float morphRange = parentSubdivThreshold * 0.30f;
                float morphStart = parentSubdivThreshold - morphRange;
                if (dist > morphStart) {
                    chunk.morphFactor = std::clamp((dist - morphStart) / morphRange, 0.0f, 1.0f);
                }
            }
        }

        m_visibleChunks.push_back(chunk);
        m_telemetry.visibleChunksCount++;
    }
}

void OASISTerrain::render(Shader& shader, float worldSize, float baseOffset) {
    if (!m_enabled || m_visibleChunks.empty() || m_patchVAO == 0 || m_heightmapTexture == 0) {
        return;
    }

    // 1. Activar textura de Heightmap en la Unidad 24
    glActiveTexture(GL_TEXTURE24);
    glBindTexture(GL_TEXTURE_2D, m_heightmapTexture);

    shader.setInt("u_OASIS_Enabled", 1);
    shader.setInt("u_Heightmap", 24);
    shader.setFloat("u_WorldSize", worldSize);
    shader.setFloat("u_BaseOffset", baseOffset);
    shader.setInt("u_OASIS_DebugLOD", m_debugLOD ? 1 : 0);
    shader.setFloat("u_PatchGridSize", static_cast<float>(m_patchResolution - 1));

    glBindVertexArray(m_patchVAO);

    // 2. Renderizar los chunks seleccionados por el Quadtree
    for (const auto& chunk : m_visibleChunks) {
        shader.setVec2("u_ChunkOffset", chunk.offset);
        shader.setFloat("u_ChunkSize", chunk.size);
        shader.setInt("u_ChunkLOD", chunk.lod);
        shader.setFloat("u_MorphFactor", chunk.morphFactor);

        glDrawElements(GL_TRIANGLES, m_patchIndexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);

    // Restaurar estado de shader
    shader.setInt("u_OASIS_Enabled", 0);
    glActiveTexture(GL_TEXTURE0);
}

} // namespace ASIS
