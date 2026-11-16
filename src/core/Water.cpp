#include "Water.h"
#include "Camera.h"
#include "Shader.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace ASIS {

Water::Water() {
    // 4 trenes de ondas de Gerstner predeterminados (armónicos no repetitivos)
    m_waves.resize(4);

    // Onda 1: Oleaje oceánico principal (gran longitud de onda)
    m_waves[0].direction = glm::vec2(0.85f, 0.52f);
    m_waves[0].amplitude = 0.32f;
    m_waves[0].wavelength = 28.0f;
    m_waves[0].speed = 1.3f;
    m_waves[0].steepness = 0.40f;

    // Onda 2: Oleaje secundario angular
    m_waves[1].direction = glm::vec2(-0.55f, 0.83f);
    m_waves[1].amplitude = 0.20f;
    m_waves[1].wavelength = 15.0f;
    m_waves[1].speed = 1.7f;
    m_waves[1].steepness = 0.35f;

    // Onda 3: Choque transversal de viento
    m_waves[2].direction = glm::vec2(0.35f, -0.93f);
    m_waves[2].amplitude = 0.12f;
    m_waves[2].wavelength = 7.5f;
    m_waves[2].speed = 2.2f;
    m_waves[2].steepness = 0.30f;

    // Onda 4: Micro-ondulaciones rápidas
    m_waves[3].direction = glm::vec2(-0.90f, -0.43f);
    m_waves[3].amplitude = 0.06f;
    m_waves[3].wavelength = 3.8f;
    m_waves[3].speed = 2.8f;
    m_waves[3].steepness = 0.25f;
}

Water::~Water() {
    shutdown();
}

bool Water::init(int gridResolution, float size) {
    m_gridResolution = std::clamp(gridResolution, 32, 512);
    m_size = (size > 50.0f) ? size : 700.0f;

    buildWaterMesh(m_gridResolution, m_size);
    buildOASISPatch(33);
    createProceduralTextures();

    std::cout << "[Water] Sistema de Oceano 3D con OASIS inicializado (" << m_gridResolution << "x" << m_gridResolution << " vertices, " << m_size << "m)\n";
    return true;
}

void Water::shutdown() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        m_vao = m_vbo = m_ebo = 0;
    }
    if (m_patchVao != 0) {
        glDeleteVertexArrays(1, &m_patchVao);
        glDeleteBuffers(1, &m_patchVbo);
        glDeleteBuffers(1, &m_patchEbo);
        m_patchVao = m_patchVbo = m_patchEbo = 0;
    }
    if (m_normalMapTex != 0) {
        glDeleteTextures(1, &m_normalMapTex);
        m_normalMapTex = 0;
    }
    if (m_foamNoiseTex != 0) {
        glDeleteTextures(1, &m_foamNoiseTex);
        m_foamNoiseTex = 0;
    }
}

bool Water::setGridResolution(int res) {
    if (res < 32 || res > 1024) return false;
    if (res == m_gridResolution && m_vao != 0) return true;
    m_gridResolution = res;
    buildWaterMesh(m_gridResolution, m_size);
    return true;
}

void Water::buildWaterMesh(int resolution, float size) {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }

    struct WaterVertex {
        glm::vec3 pos;
        glm::vec2 uv;
    };

    std::vector<WaterVertex> vertices(resolution * resolution);
    std::vector<uint32_t> indices;
    indices.reserve((resolution - 1) * (resolution - 1) * 6);

    float halfSize = size * 0.5f;
    float step = size / static_cast<float>(resolution - 1);

    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            int idx = z * resolution + x;
            float nx = (static_cast<float>(x) / static_cast<float>(resolution - 1)) * 2.0f - 1.0f;
            float nz = (static_cast<float>(z) / static_cast<float>(resolution - 1)) * 2.0f - 1.0f;

            // Cuadrícula no lineal infinita: densa cerca de la cámara y proyectada al horizonte en los bordes
            float r = std::max(std::abs(nx), std::abs(nz));
            float expand = 1.0f;
            if (r > 0.65f) {
                float t = (r - 0.65f) / 0.35f;
                expand = 1.0f + t * t * 3.5f; // Los bordes exteriores se extienden hasta 2500m hacia el horizonte
            }

            float px = nx * halfSize * expand;
            float pz = nz * halfSize * expand;

            vertices[idx].pos = glm::vec3(px, 0.0f, pz);
            vertices[idx].uv = glm::vec2(
                static_cast<float>(x) / static_cast<float>(resolution - 1),
                static_cast<float>(z) / static_cast<float>(resolution - 1)
            );
        }
    }

    for (int z = 0; z < resolution - 1; ++z) {
        for (int x = 0; x < resolution - 1; ++x) {
            uint32_t i0 = z * resolution + x;
            uint32_t i1 = (z + 1) * resolution + x;
            uint32_t i2 = (z + 1) * resolution + (x + 1);
            uint32_t i3 = z * resolution + (x + 1);

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    m_indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(WaterVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Posicion (Location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void*)offsetof(WaterVertex, pos));

    // UV (Location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void*)offsetof(WaterVertex, uv));

    glBindVertexArray(0);
}

void Water::buildOASISPatch(int patchRes) {
    if (m_patchVao != 0) {
        glDeleteVertexArrays(1, &m_patchVao);
        glDeleteBuffers(1, &m_patchVbo);
        glDeleteBuffers(1, &m_patchEbo);
        m_patchVao = m_patchVbo = m_patchEbo = 0;
    }

    struct PatchVertex {
        glm::vec3 pos;
        glm::vec2 uv;
    };

    std::vector<PatchVertex> vertices(patchRes * patchRes);
    std::vector<uint32_t> indices;
    indices.reserve((patchRes - 1) * (patchRes - 1) * 6);

    for (int z = 0; z < patchRes; ++z) {
        for (int x = 0; x < patchRes; ++x) {
            int idx = z * patchRes + x;
            float u = static_cast<float>(x) / static_cast<float>(patchRes - 1);
            float v = static_cast<float>(z) / static_cast<float>(patchRes - 1);
            vertices[idx].pos = glm::vec3(u, 0.0f, v);
            vertices[idx].uv = glm::vec2(u, v);
        }
    }

    for (int z = 0; z < patchRes - 1; ++z) {
        for (int x = 0; x < patchRes - 1; ++x) {
            uint32_t i0 = z * patchRes + x;
            uint32_t i1 = (z + 1) * patchRes + x;
            uint32_t i2 = (z + 1) * patchRes + (x + 1);
            uint32_t i3 = z * patchRes + (x + 1);

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    m_patchIndexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &m_patchVao);
    glGenBuffers(1, &m_patchVbo);
    glGenBuffers(1, &m_patchEbo);

    glBindVertexArray(m_patchVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_patchVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(PatchVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PatchVertex), (void*)offsetof(PatchVertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(PatchVertex), (void*)offsetof(PatchVertex, uv));

    glBindVertexArray(0);
}

void Water::evaluateQuadtree(const glm::vec2& offset, float size, int lod, const glm::vec3& camPos, const Frustum& frustum) {
    m_telemetry.totalEvaluatedChunks++;

    float maxAmp = m_waveAmplitude * 1.5f + 1.0f;
    glm::vec3 aabbMin(offset.x, m_waterLevel - maxAmp, offset.y);
    glm::vec3 aabbMax(offset.x + size, m_waterLevel + maxAmp, offset.y + size);

    // Margen de seguridad lateral (Guard Band) para evitar desaparición de chunks en bordes
    float guardMargin = size * 1.5f * std::max(1.0f, m_oasisLodFactor * 0.5f) + 15.0f;

    if (m_frustumCulling && !frustum.isBoxVisible(aabbMin, aabbMax, guardMargin)) {
        m_telemetry.culledChunksCount++;
        return;
    }

    glm::vec2 chunkCenter = offset + glm::vec2(size * 0.5f);
    float distToCam = glm::length(glm::vec2(camPos.x, camPos.z) - chunkCenter);

    float splitDistance = size * m_oasisLodFactor * 2.8f;

    if (lod < m_maxOASISLod && distToCam < splitDistance) {
        float halfSize = size * 0.5f;
        evaluateQuadtree(offset, halfSize, lod + 1, camPos, frustum);
        evaluateQuadtree(offset + glm::vec2(halfSize, 0.0f), halfSize, lod + 1, camPos, frustum);
        evaluateQuadtree(offset + glm::vec2(0.0f, halfSize), halfSize, lod + 1, camPos, frustum);
        evaluateQuadtree(offset + glm::vec2(halfSize, halfSize), halfSize, lod + 1, camPos, frustum);
    } else {
        WaterChunk chunk;
        chunk.offset = offset;
        chunk.size = size;
        chunk.lod = lod;
        chunk.aabbMin = aabbMin;
        chunk.aabbMax = aabbMax;
        m_visibleChunks.push_back(chunk);
    }
}

void Water::createProceduralTextures() {
    // 1. Textura de Normal Map de Micro-ondas Seamless (256x256)
    const int W = 256;
    std::vector<uint8_t> normalData(W * W * 3);
    const float PI = 3.14159265359f;

    auto heightFunc = [&](float u, float v) -> float {
        float h = 0.0f;
        h += std::sin(u * 2.0f * PI * 3.0f + v * 2.0f * PI * 1.5f) * 0.5f;
        h += std::sin(u * 2.0f * PI * 6.0f - v * 2.0f * PI * 4.0f) * 0.25f;
        h += std::cos(u * 2.0f * PI * 12.0f + v * 2.0f * PI * 8.0f) * 0.125f;
        h += std::sin(u * 2.0f * PI * 24.0f - v * 2.0f * PI * 18.0f) * 0.0625f;
        return h;
    };

    float eps = 1.0f / static_cast<float>(W);
    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(W);
            float v = static_cast<float>(y) / static_cast<float>(W);

            float hL = heightFunc(u - eps, v);
            float hR = heightFunc(u + eps, v);
            float hD = heightFunc(u, v - eps);
            float hU = heightFunc(u, v + eps);

            glm::vec3 n = glm::normalize(glm::vec3((hL - hR) * 2.5f, (hD - hU) * 2.5f, 1.0f));
            // Mapear [-1, 1] a [0, 255]
            int idx = (y * W + x) * 3;
            normalData[idx + 0] = static_cast<uint8_t>(std::clamp((n.x * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            normalData[idx + 1] = static_cast<uint8_t>(std::clamp((n.y * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            normalData[idx + 2] = static_cast<uint8_t>(std::clamp((n.z * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
        }
    }

    glGenTextures(1, &m_normalMapTex);
    glBindTexture(GL_TEXTURE_2D, m_normalMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, W, W, 0, GL_RGB, GL_UNSIGNED_BYTE, normalData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // 2. Textura de Ruido Orgánico Celular (Voronoi F1/F2) para Espuma de Orilla (256x256)
    std::vector<uint8_t> foamData(W * W);
    auto hashCell = [](int x, int y) -> glm::vec2 {
        float fx = std::sin(static_cast<float>(x) * 127.1f + static_cast<float>(y) * 311.7f) * 43758.5453f;
        float fy = std::sin(static_cast<float>(x) * 269.5f + static_cast<float>(y) * 183.3f) * 43758.5453f;
        return glm::vec2(fx - std::floor(fx), fy - std::floor(fy));
    };

    const int numCells = 16;
    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(W);
            float v = static_cast<float>(y) / static_cast<float>(W);

            float px = u * numCells;
            float py = v * numCells;
            int ix = static_cast<int>(std::floor(px));
            int iy = static_cast<int>(std::floor(py));
            float fx = px - ix;
            float fy = py - iy;

            float minDist = 1.0f;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int cx = (ix + dx + numCells) % numCells;
                    int cy = (iy + dy + numCells) % numCells;
                    glm::vec2 r = hashCell(cx, cy);
                    glm::vec2 diff = glm::vec2(static_cast<float>(dx) + r.x - fx, static_cast<float>(dy) + r.y - fy);
                    minDist = std::min(minDist, glm::length(diff));
                }
            }

            float f = 1.0f - std::clamp(minDist, 0.0f, 1.0f);
            f = std::pow(f, 1.8f);
            foamData[y * W + x] = static_cast<uint8_t>(std::clamp(f * 255.0f, 0.0f, 255.0f));
        }
    }

    glGenTextures(1, &m_foamNoiseTex);
    glBindTexture(GL_TEXTURE_2D, m_foamNoiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, W, W, 0, GL_RED, GL_UNSIGNED_BYTE, foamData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Water::update(float dt) {
    if (!m_enabled) return;
    m_time += dt * m_waveSpeed;
}

float Water::getWaterHeightAt(float worldX, float worldZ) const {
    if (!m_enabled) return -1000.0f;

    float h = m_waterLevel;
    const float PI = 3.14159265359f;

    for (const auto& w : m_waves) {
        glm::vec2 dir = glm::length(w.direction) > 0.001f ? glm::normalize(w.direction) : glm::vec2(1.0f, 0.0f);
        float k = 2.0f * PI / std::max(w.wavelength, 0.2f);
        float speed = std::sqrt(9.81f / k) * w.speed * m_waveSpeed;
        float phase = k * (dir.x * worldX + dir.y * worldZ) - (k * speed) * m_time;
        float amp = w.amplitude * m_waveAmplitude;
        h += amp * std::sin(phase);
    }
    return h;
}

glm::vec3 Water::getWaterNormalAt(float worldX, float worldZ) const {
    if (!m_enabled) return glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 N(0.0f, 1.0f, 0.0f);
    const float PI = 3.14159265359f;
    float numWaves = static_cast<float>(std::max(1ULL, m_waves.size()));

    for (const auto& w : m_waves) {
        glm::vec2 dir = glm::length(w.direction) > 0.001f ? glm::normalize(w.direction) : glm::vec2(1.0f, 0.0f);
        float k = 2.0f * PI / std::max(w.wavelength, 0.2f);
        float speed = std::sqrt(9.81f / k) * w.speed * m_waveSpeed;
        float phase = k * (dir.x * worldX + dir.y * worldZ) - (k * speed) * m_time;
        float amp = w.amplitude * m_waveAmplitude;
        float Q = w.steepness / (k * std::max(amp, 0.001f) * numWaves);

        N.x -= dir.x * k * amp * std::cos(phase);
        N.z -= dir.y * k * amp * std::cos(phase);
        N.y -= Q * k * amp * std::sin(phase);
    }
    return glm::normalize(N);
}

void Water::applyPreset(WaterPreset preset) {
    switch (preset) {
        case WaterPreset::CALM_LAKE:
            m_waveAmplitude = 0.12f;
            m_waveSpeed = 0.7f;
            m_shallowColor = glm::vec3(0.12f, 0.65f, 0.55f);
            m_deepColor = glm::vec3(0.02f, 0.18f, 0.24f);
            m_clarity = 0.38f;
            m_foamIntensity = 0.6f;
            m_foamThreshold = 0.6f;
            m_roughness = 0.04f;
            break;

        case WaterPreset::TROPICAL_CARIBBEAN:
            m_waveAmplitude = 0.40f;
            m_waveSpeed = 1.0f;
            m_shallowColor = glm::vec3(0.08f, 0.65f, 0.72f);
            m_deepColor = glm::vec3(0.01f, 0.10f, 0.35f);
            m_clarity = 0.22f;
            m_foamIntensity = 1.4f;
            m_foamThreshold = 1.2f;
            m_roughness = 0.06f;
            break;

        case WaterPreset::OPEN_OCEAN:
            m_waveAmplitude = 0.85f;
            m_waveSpeed = 1.3f;
            m_shallowColor = glm::vec3(0.04f, 0.38f, 0.52f);
            m_deepColor = glm::vec3(0.008f, 0.04f, 0.20f);
            m_clarity = 0.14f;
            m_foamIntensity = 1.8f;
            m_foamThreshold = 1.8f;
            m_roughness = 0.09f;
            break;

        case WaterPreset::STORMY_SEA:
            m_waveAmplitude = 1.65f;
            m_waveSpeed = 1.8f;
            m_shallowColor = glm::vec3(0.12f, 0.22f, 0.26f);
            m_deepColor = glm::vec3(0.015f, 0.035f, 0.08f);
            m_clarity = 0.08f;
            m_foamIntensity = 2.4f;
            m_foamThreshold = 2.5f;
            m_roughness = 0.16f;
            break;
    }
}

void Water::draw(Shader& shader, const Camera& camera, float aspectRatio,
                 GLuint opaqueDepthTex, GLuint opaqueColorTex,
                 const glm::vec3& sunDir, const glm::vec3& sunColor,
                 const glm::vec3& ambientColor, GLuint skyboxEnvMap,
                 bool ssrEnabled, int ssrMaxSteps, float ssrStepSize,
                 float ssrThickness, float ssrIntensity) {
    if (!m_enabled || m_vao == 0 || m_indexCount == 0) return;

    shader.use();

    // 1. Matrices de Cámara y Centrado Continuo de Malla Océano
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
    glm::vec3 camPos = camera.getPosition();

    // Centrar la cuadrícula del océano en la posición de la cámara (snapped para evitar flickering)
    float gridStep = m_size / static_cast<float>(m_gridResolution - 1);
    float snapX = std::floor(camPos.x / gridStep) * gridStep;
    float snapZ = std::floor(camPos.z / gridStep) * gridStep;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(snapX, 0.0f, snapZ));

    shader.setMat4("u_Model", model);
    shader.setMat4("u_View", view);
    shader.setMat4("u_Projection", projection);
    shader.setVec3("u_ViewPos", camPos);

    // Screen-Space Reflections (SSR - Fase 20)
    shader.setInt("u_SSREnabled", ssrEnabled ? 1 : 0);
    shader.setInt("u_SSRMaxSteps", ssrMaxSteps);
    shader.setFloat("u_SSRStepSize", ssrStepSize);
    shader.setFloat("u_SSRThickness", ssrThickness);
    shader.setFloat("u_SSRIntensity", ssrIntensity);

    // Planos de profundidad
    shader.setFloat("u_NearPlane", camera.getNearPlane());
    shader.setFloat("u_FarPlane", camera.getFarPlane());

    // 2. Parámetros de Superficie y Oleaje de Gerstner
    shader.setFloat("u_WaterLevel", m_waterLevel);
    shader.setFloat("u_Time", m_time);
    shader.setFloat("u_WaveAmplitude", m_waveAmplitude);
    shader.setFloat("u_WaveSpeed", m_waveSpeed);

    for (int i = 0; i < 4 && i < static_cast<int>(m_waves.size()); ++i) {
        std::string prefix = "u_Waves[" + std::to_string(i) + "].";
        glm::vec2 dir = glm::length(m_waves[i].direction) > 0.001f ? glm::normalize(m_waves[i].direction) : glm::vec2(1.0f, 0.0f);
        shader.setVec2(prefix + "direction", dir);
        shader.setFloat(prefix + "amplitude", m_waves[i].amplitude);
        shader.setFloat(prefix + "wavelength", m_waves[i].wavelength);
        shader.setFloat(prefix + "speed", m_waves[i].speed);
        shader.setFloat(prefix + "steepness", m_waves[i].steepness);
    }

    // 3. Parámetros Ópticos y PBR
    shader.setVec3("u_ShallowColor", m_shallowColor);
    shader.setVec3("u_DeepColor", m_deepColor);
    shader.setFloat("u_Clarity", m_clarity);
    shader.setVec3("u_FoamColor", m_foamColor);
    shader.setFloat("u_FoamThreshold", m_foamThreshold);
    shader.setFloat("u_FoamIntensity", m_foamIntensity);
    shader.setFloat("u_RefractionStrength", m_refractionStrength);
    shader.setFloat("u_Roughness", m_roughness);
    shader.setFloat("u_Specular", m_specular);

    // Iluminación
    shader.setVec3("u_SunDir", sunDir);
    shader.setVec3("u_SunColor", sunColor);
    shader.setVec3("u_AmbientColor", ambientColor);

    // 4. Mapeo de Texturas
    // Unit 0: Textura de Color Opaco (Refracción)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, opaqueColorTex);
    shader.setInt("u_OpaqueColorTexture", 0);

    // Unit 1: Textura de Profundidad Opaca (Detección de orillas y absorción)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, opaqueDepthTex);
    shader.setInt("u_OpaqueDepthTexture", 1);

    // Unit 2: Micro-Normal Map procedimental
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_normalMapTex);
    shader.setInt("u_WaterNormalMap", 2);

    // Unit 3: Ruido de Espuma
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_foamNoiseTex);
    shader.setInt("u_FoamNoiseMap", 3);

    // Unit 4: Mapa de Entorno HDRI para reflejos de cielo
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, skyboxEnvMap);
    shader.setInt("u_EnvMap", 4);
    shader.setInt("u_HasEnvMap", (skyboxEnvMap != 0) ? 1 : 0);

    // 5. Configuración de Pipeline para Superficie Acuática Translúcida
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE); // Escribe profundidad para que el agua oculte objetos detrás de ella
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Permitir ver la superficie del agua tanto por arriba como desde abajo al nadar

    if (m_oasisEnabled && m_patchVao != 0) {
        m_visibleChunks.clear();
        m_telemetry.totalEvaluatedChunks = 0;
        m_telemetry.culledChunksCount = 0;

        Frustum frustum;
        frustum.update(projection * view);

        float oceanExt = m_size * 2.5f;
        float rootSize = oceanExt * 0.5f;

        float snapStep = 64.0f;
        float baseCamX = std::floor(camPos.x / snapStep) * snapStep;
        float baseCamZ = std::floor(camPos.z / snapStep) * snapStep;

        evaluateQuadtree(glm::vec2(baseCamX - oceanExt * 0.5f, baseCamZ - oceanExt * 0.5f), rootSize, 0, camPos, frustum);
        evaluateQuadtree(glm::vec2(baseCamX, baseCamZ - oceanExt * 0.5f), rootSize, 0, camPos, frustum);
        evaluateQuadtree(glm::vec2(baseCamX - oceanExt * 0.5f, baseCamZ), rootSize, 0, camPos, frustum);
        evaluateQuadtree(glm::vec2(baseCamX, baseCamZ), rootSize, 0, camPos, frustum);

        m_telemetry.visibleChunksCount = static_cast<int>(m_visibleChunks.size());
        m_telemetry.renderedTriangles = m_visibleChunks.size() * (32 * 32 * 2);
        size_t bruteTriangles = static_cast<size_t>(m_gridResolution - 1) * (m_gridResolution - 1) * 2;
        m_telemetry.totalUnoptimizedTriangles = bruteTriangles;
        if (bruteTriangles > 0) {
            float saved = 1.0f - (static_cast<float>(m_telemetry.renderedTriangles) / static_cast<float>(bruteTriangles));
            m_telemetry.triangleReductionPercent = std::clamp(saved * 100.0f, 0.0f, 99.9f);
        }

        shader.setInt("u_WaterOASIS_Enabled", 1);
        glBindVertexArray(m_patchVao);

        for (const auto& chunk : m_visibleChunks) {
            shader.setVec2("u_WaterChunkOffset", chunk.offset);
            shader.setFloat("u_WaterChunkSize", chunk.size);
            glDrawElements(GL_TRIANGLES, m_patchIndexCount, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
        shader.setInt("u_WaterOASIS_Enabled", 0);
    } else {
        shader.setInt("u_WaterOASIS_Enabled", 0);
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
}

} // namespace ASIS
