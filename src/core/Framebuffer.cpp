#include "Framebuffer.h"

namespace ASIS {

Framebuffer::Framebuffer()
    : m_fbo(0), m_colorTexture(0), m_depthTexture(0), m_rbo(0),
      m_opaqueFBO(0), m_opaqueColorTexture(0), m_opaqueDepthTexture(0),
      m_msaaFBO(0), m_msaaColorRBO(0), m_msaaDepthRBO(0),
      m_width(1280), m_height(720), m_samples(4)
{
}

Framebuffer::~Framebuffer() {
    destroy();
}

bool Framebuffer::init(int width, int height, int samples) {
    if (width <= 0 || height <= 0) return false;

    m_width = width;
    m_height = height;
    m_samples = (samples > 1) ? samples : 1;

    // 1. Crear el FBO de Resolve (textura estándar 2D para ImGui Image y shaders)
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    // Usar GL_RGBA16F para soporte HDR (Bloom, ACES Tonemapping sin banding)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ASIS Framebuffer ERROR] Resolve FBO incompleto.\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    // 2. Si samples > 1, crear el FBO Multisample
    if (m_samples > 1) {
        glGenFramebuffers(1, &m_msaaFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFBO);

        glGenRenderbuffers(1, &m_msaaColorRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_msaaColorRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA16F, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msaaColorRBO);

        glGenRenderbuffers(1, &m_msaaDepthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_msaaDepthRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH_COMPONENT24, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[ASIS Framebuffer ERROR] MSAA FBO incompleto (samples: " << m_samples << ")\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
    }

    // 3. FBO auxiliar para captura de opacos (Agua, refracción, orillas y caústicas)
    glGenFramebuffers(1, &m_opaqueFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_opaqueFBO);

    glGenTextures(1, &m_opaqueColorTexture);
    glBindTexture(GL_TEXTURE_2D, m_opaqueColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_opaqueColorTexture, 0);

    glGenTextures(1, &m_opaqueDepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_opaqueDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_opaqueDepthTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "[ASIS Framebuffer] FBO creado (" << m_width << "x" << m_height << ", MSAA " << m_samples << "x)\n";
    return true;
}

void Framebuffer::resize(int width, int height, int samples) {
    if (width <= 0 || height <= 0) return;
    int targetSamples = (samples > 1) ? samples : 1;
    if (width == m_width && height == m_height && targetSamples == m_samples) return;

    destroy();
    init(width, height, targetSamples);
}

void Framebuffer::bind() const {
    if (m_samples > 1 && m_msaaFBO != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFBO);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() const {
    resolve();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resolve() const {
    if (m_samples > 1 && m_msaaFBO != 0 && m_fbo != 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);

        // 1. Resolver búfer de color con filtrado lineal
        glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // 2. Resolver búfer de profundidad con GL_NEAREST (estricto por especificación OpenGL 3.3)
        glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Framebuffer::copyOpaqueBuffers() const {
    if (m_opaqueFBO == 0) return;

    GLuint srcFBO = (m_samples > 1 && m_msaaFBO != 0) ? m_msaaFBO : m_fbo;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_opaqueFBO);

    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // Restaurar FBO de dibujo
    glBindFramebuffer(GL_FRAMEBUFFER, srcFBO);
}

void Framebuffer::destroy() {
    if (m_opaqueFBO != 0) {
        glDeleteFramebuffers(1, &m_opaqueFBO);
        m_opaqueFBO = 0;
    }
    if (m_opaqueColorTexture != 0) {
        glDeleteTextures(1, &m_opaqueColorTexture);
        m_opaqueColorTexture = 0;
    }
    if (m_opaqueDepthTexture != 0) {
        glDeleteTextures(1, &m_opaqueDepthTexture);
        m_opaqueDepthTexture = 0;
    }
    if (m_msaaFBO != 0) {
        glDeleteFramebuffers(1, &m_msaaFBO);
        m_msaaFBO = 0;
    }
    if (m_msaaColorRBO != 0) {
        glDeleteRenderbuffers(1, &m_msaaColorRBO);
        m_msaaColorRBO = 0;
    }
    if (m_msaaDepthRBO != 0) {
        glDeleteRenderbuffers(1, &m_msaaDepthRBO);
        m_msaaDepthRBO = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_colorTexture != 0) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthTexture != 0) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
    if (m_rbo != 0) {
        glDeleteRenderbuffers(1, &m_rbo);
        m_rbo = 0;
    }
}

} // namespace ASIS
