#include "CascadedShadowMap.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace ASIS {

CascadedShadowMap::CascadedShadowMap() = default;

CascadedShadowMap::~CascadedShadowMap() {
    shutdown();
}

bool CascadedShadowMap::init(unsigned int resolution, int cascadeCount) {
    shutdown();
    m_resolution = resolution;
    m_cascadeCount = std::clamp(cascadeCount, 1, 4);

    m_cascadeMatrices.resize(m_cascadeCount, glm::mat4(1.0f));
    m_cascadeSplits.resize(m_cascadeCount, 0.0f);

    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_depthArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_depthArray);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                 m_resolution, m_resolution, m_cascadeCount, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderCol[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderCol);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthArray, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[CSM ERROR] Fallo al crear FBO para Texture2DArray: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    std::cout << "[CSM] Sistema de Sombras en Cascada inicializado (" << m_cascadeCount 
              << " cascadas x " << m_resolution << "x" << m_resolution << ")\n";
    return true;
}

void CascadedShadowMap::shutdown() {
    if (m_depthArray != 0) {
        glDeleteTextures(1, &m_depthArray);
        m_depthArray = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

void CascadedShadowMap::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_resolution, m_resolution);
}

void CascadedShadowMap::bindCascade(int cascadeIndex) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthArray, 0, cascadeIndex);
    glViewport(0, 0, m_resolution, m_resolution);
}

void CascadedShadowMap::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CascadedShadowMap::bindTexture(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_depthArray);
}

void CascadedShadowMap::update(const Camera& camera, const glm::vec3& lightDir, float maxDistance, float aspectRatio) {
    float nearPlane = camera.getNearPlane();
    float farPlane = std::min(camera.getFarPlane(), maxDistance);

    // Esquema de partición logarítmica-lineal práctica (Practical Split Scheme)
    // Cascada 0: Cercana (0.1m - 18m) -> Centimétrica ultra-nítida
    // Cascada 1: Media   (18m - 75m)  -> Estructuras y terreno medio
    // Cascada 2: Lejana  (75m - max)  -> Paisaje distante
    float lambda = 0.85f;
    std::vector<float> cascadeLimits(m_cascadeCount + 1);
    cascadeLimits[0] = nearPlane;
    cascadeLimits[m_cascadeCount] = farPlane;

    for (int i = 1; i < m_cascadeCount; ++i) {
        float ratio = static_cast<float>(i) / static_cast<float>(m_cascadeCount);
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, ratio);
        float uniformSplit = nearPlane + (farPlane - nearPlane) * ratio;
        cascadeLimits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
    }

    for (int i = 0; i < m_cascadeCount; ++i) {
        m_cascadeSplits[i] = cascadeLimits[i + 1];
        m_cascadeMatrices[i] = calculateCascadeMatrix(camera, lightDir, cascadeLimits[i], cascadeLimits[i + 1], aspectRatio);
    }
}

glm::mat4 CascadedShadowMap::calculateCascadeMatrix(const Camera& camera, const glm::vec3& lightDir, float nearZ, float farZ, float aspectRatio) {
    // 1. Proyección del sub-frustum de la cascada
    float aspect = (aspectRatio > 0.05f) ? aspectRatio : (16.0f / 9.0f);
    glm::mat4 proj = glm::perspective(glm::radians(camera.getFOV()), aspect, nearZ, farZ);
    glm::mat4 invProjView = glm::inverse(proj * camera.getViewMatrix());

    // 2. Extraer los 8 vértices en coordenadas de mundo del sub-frustum
    std::vector<glm::vec3> corners;
    corners.reserve(8);
    for (unsigned int x = 0; x < 2; ++x) {
        for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
                glm::vec4 pt = invProjView * glm::vec4(
                    2.0f * static_cast<float>(x) - 1.0f,
                    2.0f * static_cast<float>(y) - 1.0f,
                    2.0f * static_cast<float>(z) - 1.0f,
                    1.0f
                );
                corners.push_back(glm::vec3(pt) / pt.w);
            }
        }
    }

    // 3. Calcular centro y radio de la esfera envolvente del sub-frustum
    glm::vec3 center(0.0f);
    for (const auto& c : corners) {
        center += c;
    }
    center /= static_cast<float>(corners.size());

    float radius = 0.0f;
    for (const auto& c : corners) {
        radius = std::max(radius, glm::distance(c, center));
    }
    radius = std::ceil(radius * 16.0f) / 16.0f; // Redondeo para estabilizar

    // 4. Snapping al grid de texels para eliminar vibración de sombras (Edge Shimmer)
    float worldUnitsPerTexel = (2.0f * radius) / static_cast<float>(m_resolution);
    glm::vec3 safeLightDir = glm::normalize(lightDir);
    if (glm::length(safeLightDir) < 0.001f) safeLightDir = glm::vec3(0.0f, -1.0f, 0.0f);

    glm::vec3 lightUp = (std::abs(safeLightDir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 lightView = glm::lookAt(center - safeLightDir * radius, center, lightUp);

    // Transformar centro a espacio de luz para alinear con texels
    glm::vec4 lightSpaceCenter = lightView * glm::vec4(center, 1.0f);
    lightSpaceCenter.x = std::floor(lightSpaceCenter.x / worldUnitsPerTexel) * worldUnitsPerTexel;
    lightSpaceCenter.y = std::floor(lightSpaceCenter.y / worldUnitsPerTexel) * worldUnitsPerTexel;
    center = glm::vec3(glm::inverse(lightView) * lightSpaceCenter);

    lightView = glm::lookAt(center - safeLightDir * radius, center, lightUp);
    glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, -radius * 3.0f, radius * 4.0f);

    return lightProj * lightView;
}

} // namespace ASIS
