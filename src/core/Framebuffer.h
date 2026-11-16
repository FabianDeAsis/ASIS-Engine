#pragma once

#include <glad/glad.h>
#include <iostream>

namespace ASIS {

class Framebuffer {
public:
    Framebuffer();
    ~Framebuffer();

    // Inicializar el FBO con las dimensiones y muestras MSAA
    bool init(int width, int height, int samples = 4);

    // Redimensionar el FBO si el panel de ImGui cambia de tamaño o de nivel MSAA
    void resize(int width, int height, int samples = 4);

    // Activar FBO para renderizar la escena 3D (se une al MSAA FBO si samples > 1)
    void bind() const;

    // Desactivar FBO y resolver automáticamente de MSAA a textura 2D si corresponde
    void unbind() const;

    // Resolver explícitamente el buffer multisample a la textura 2D
    void resolve() const;

    // Liberar recursos de GPU
    void destroy();

    // Copiar búferes de la escena opaca para efectos de refracción y profundidad de agua/vidrio
    void copyOpaqueBuffers() const;

    GLuint getColorTexture() const { return m_colorTexture; }
    GLuint getDepthTexture() const { return m_depthTexture; }
    GLuint getOpaqueColorTexture() const { return m_opaqueColorTexture; }
    GLuint getOpaqueDepthTexture() const { return m_opaqueDepthTexture; }
    GLuint getFBO() const { return m_fbo; }
    GLuint getMSAAFBO() const { return m_msaaFBO; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getSamples() const { return m_samples; }
    float getAspectRatio() const { return (m_height > 0) ? static_cast<float>(m_width) / static_cast<float>(m_height) : 16.0f / 9.0f; }

private:
    GLuint m_fbo;           // Resolve FBO (textura 2D para ImGui y Post-Process)
    GLuint m_colorTexture;  // Textura GL_RGBA16F (HDR)
    GLuint m_depthTexture;  // Textura de Profundidad GL_DEPTH24_STENCIL8 (para SSAO y DoF)
    GLuint m_rbo;           // Depth/Stencil para resolve si necesario

    // Búferes auxiliares para captura de escena opaca (agua y refracción)
    GLuint m_opaqueFBO;
    GLuint m_opaqueColorTexture;
    GLuint m_opaqueDepthTexture;

    // Recursos MSAA
    GLuint m_msaaFBO;       // FBO de renderizado con muestras
    GLuint m_msaaColorRBO;  // Renderbuffer de color con multimuestreo
    GLuint m_msaaDepthRBO;  // Renderbuffer de depth/stencil con multimuestreo

    int m_width;
    int m_height;
    int m_samples;
};

} // namespace ASIS
