#include "ShadowMap.h"

namespace ASIS {

ShadowMap::ShadowMap()
    : m_fbo(0), m_depthTexture(0), m_width(2048), m_height(2048)
{
}

ShadowMap::~ShadowMap() {
    destroy();
}

bool ShadowMap::init(unsigned int width, unsigned int height) {
    m_width = width;
    m_height = height;

    // 1. Crear el Framebuffer Object para sombras
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // 2. Crear la textura de profundidad 2D de ultra alta precisión (32-bit Float)
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Configuración de filtrado de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Envolver bordes con color blanco (1.0f) para que fragmentos fuera del frustum solar no proyecten sombra
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Adjuntar la textura de profundidad al FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

    // Desactivar buffer de color (un mapa de sombras solo almacena profundidad)
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Verificar estado del Framebuffer
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ASIS ShadowMap ERROR] Framebuffer de sombras incompleto (codigo: " << status << ")\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "[ASIS ShadowMap] FBO de sombras 2K inicializado exitosamente (" << m_width << "x" << m_height << ")\n";
    return true;
}

void ShadowMap::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void ShadowMap::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bindTexture(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
}

bool ShadowMap::resize(unsigned int width, unsigned int height) {
    if (m_width == width && m_height == height && m_fbo != 0) return true;
    destroy();
    return init(width, height);
}

void ShadowMap::destroy() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_depthTexture != 0) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
}

} // namespace ASIS
