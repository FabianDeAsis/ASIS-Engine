#pragma once

#include <glad/glad.h>
#include <iostream>

namespace ASIS {

/**
 * @brief Gestor de Framebuffer de Sombras (Shadow Map FBO)
 * 
 * Configura un FBO con una textura de profundidad de 2048x2048 para renderizar
 * el mapa de profundidad de la escena desde el punto de vista de la luz solar (Light Space).
 */
class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    // Inicializar el FBO y la textura de profundidad (por defecto 2K: 2048x2048)
    bool init(unsigned int width = 2048, unsigned int height = 2048);
    // Cambiar la resolución dinámica del mapa de sombras (1K, 2K, 4K)
    bool resize(unsigned int width, unsigned int height);

    // Activar el Framebuffer de sombras para el pase de profundidad
    void bind() const;

    // Desactivar el Framebuffer de sombras
    void unbind() const;

    // Enlazar la textura de profundidad a una unidad de textura OpenGL (por defecto GL_TEXTURE1)
    void bindTexture(unsigned int unit = 1) const;

    GLuint getDepthTexture() const { return m_depthTexture; }
    GLuint getFBO() const { return m_fbo; }
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }

    void destroy();

private:
    GLuint m_fbo;
    GLuint m_depthTexture;
    unsigned int m_width;
    unsigned int m_height;
};

} // namespace ASIS
