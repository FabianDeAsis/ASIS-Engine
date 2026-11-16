#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

namespace ASIS {

class Camera;

/**
 * @brief Gestor de Sombras en Cascada (Cascaded Shadow Maps - CSM)
 * 
 * Divide el Frustum de la cámara en cascadas por distancia de visión
 * para otorgar resolución milimétrica a objetos cercanos y alcance infinito
 * a montañas lejanas sin saturar la VRAM ni sufrir pixelación.
 */
class CascadedShadowMap {
public:
    CascadedShadowMap();
    ~CascadedShadowMap();

    bool init(unsigned int resolution = 2048, int cascadeCount = 3);
    void shutdown();

    void update(const Camera& camera, const glm::vec3& lightDir, float maxDistance, float aspectRatio = 16.0f / 9.0f);

    void bind();
    void bindCascade(int cascadeIndex);
    void unbind();
    void bindTexture(unsigned int unit = 25) const;

    const std::vector<glm::mat4>& getCascadeMatrices() const { return m_cascadeMatrices; }
    const std::vector<float>& getCascadeSplits() const { return m_cascadeSplits; }
    int getCascadeCount() const { return m_cascadeCount; }
    unsigned int getResolution() const { return m_resolution; }
    GLuint getTextureArray() const { return m_depthArray; }

private:
    glm::mat4 calculateCascadeMatrix(const Camera& camera, const glm::vec3& lightDir, float nearZ, float farZ, float aspectRatio = 16.0f / 9.0f);

    GLuint m_fbo = 0;
    GLuint m_depthArray = 0;
    unsigned int m_resolution = 2048;
    int m_cascadeCount = 3;

    std::vector<glm::mat4> m_cascadeMatrices;
    std::vector<float> m_cascadeSplits; // Distancias en espacio de vista
};

} // namespace ASIS
