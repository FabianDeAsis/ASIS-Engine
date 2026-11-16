#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

namespace ASIS {

/**
 * @brief Gestor de Sombras Omnidireccionales para Luces Puntuales (Point Light Shadows)
 * 
 * Utiliza un FBO con textura Depth Cubemap (GL_TEXTURE_CUBE_MAP) para capturar la profundidad
 * en 360 grados en las 6 direcciones espaciales (+X, -X, +Y, -Y, +Z, -Z).
 */
class OmniShadowMap {
public:
    OmniShadowMap();
    ~OmniShadowMap();

    bool init(unsigned int resolution = 1024);
    void bind();
    void unbind();
    void bindFace(unsigned int faceIndex); // 0 = +X, 1 = -X, 2 = +Y, 3 = -Y, 4 = +Z, 5 = -Z
    void bindTexture(unsigned int unit = 26) const;

    std::vector<glm::mat4> calculateShadowMatrices(const glm::vec3& lightPos) const;
    glm::mat4 getShadowProjection() const;
    glm::mat4 getShadowView(unsigned int faceIndex, const glm::vec3& lightPos) const;

    GLuint getFBO() const { return m_fbo; }
    GLuint getDepthCubemap() const { return m_depthCubemap; }
    unsigned int getResolution() const { return m_resolution; }
    float getNearPlane() const { return m_nearPlane; }
    float getFarPlane() const { return m_farPlane; }
    void setFarPlane(float f) { m_farPlane = f; }

    void destroy();

private:
    GLuint m_fbo;
    GLuint m_depthCubemap;
    unsigned int m_resolution;
    float m_nearPlane;
    float m_farPlane;
};

} // namespace ASIS
