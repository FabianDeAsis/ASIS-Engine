#include "PostProcess.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <cstring>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace ASIS {

PostProcess::PostProcess()
    : m_width(1280), m_height(720), m_initialized(false),
      m_quadVAO(0), m_quadVBO(0),
      m_bloomWidth(640), m_bloomHeight(360),
      m_bloomExtractFBO(0), m_bloomExtractTex(0),
      m_bloomCompositeFBO(0), m_bloomCompositeTex(0),
      m_fxaaFBO(0), m_fxaaTex(0),
      m_dofFBO(0), m_dofTex(0),
      m_contactShadowsFBO(0), m_contactShadowsTex(0),
      m_underwaterFBO(0), m_underwaterTex(0),
      m_lumFBO(0), m_lumTex(0)
{
    m_bloomBlurFBO[0] = m_bloomBlurFBO[1] = 0;
    m_bloomBlurTex[0] = m_bloomBlurTex[1] = 0;
}

PostProcess::~PostProcess() {
    destroy();
}

void PostProcess::setupQuad() {
    if (m_quadVAO != 0) return;

    static const float quadVertices[] = {
        // Coordenadas (X, Y)   // UVs (U, V)
        -1.0f,  1.0f,          0.0f, 1.0f,
        -1.0f, -1.0f,          0.0f, 0.0f,
         1.0f, -1.0f,          1.0f, 0.0f,

        -1.0f,  1.0f,          0.0f, 1.0f,
         1.0f, -1.0f,          1.0f, 0.0f,
         1.0f,  1.0f,          1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void PostProcess::renderQuad() {
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool PostProcess::init(int width, int height) {
    if (width <= 0 || height <= 0) return false;

    m_width = width;
    m_height = height;
    m_bloomWidth = std::max(1, width / 2);
    m_bloomHeight = std::max(1, height / 2);

    setupQuad();

    // 1. Shaders
    try {
        m_extractShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/bloom_extract.frag");
        m_blurShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/bloom_blur.frag");
        m_compositeShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/bloom_composite.frag");
        m_fxaaShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/fxaa.frag");
        m_dofShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/dof.frag");
        m_contactShadowsShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/contact_shadows.frag");
        m_underwaterShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/underwater.frag");
        m_lumShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/lum_downsample.frag");
    } catch (const std::exception& e) {
        std::cerr << "[PostProcess] Error al compilar shaders de post-procesado: " << e.what() << std::endl;
        return false;
    }

    auto createTexFBO = [](GLuint& fbo, GLuint& tex, int w, int h, GLint internalFormat = GL_RGBA16F, GLenum type = GL_FLOAT) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    };

    // 2. FBOs para Bloom
    createTexFBO(m_bloomExtractFBO, m_bloomExtractTex, m_bloomWidth, m_bloomHeight);
    createTexFBO(m_bloomBlurFBO[0], m_bloomBlurTex[0], m_bloomWidth, m_bloomHeight);
    createTexFBO(m_bloomBlurFBO[1], m_bloomBlurTex[1], m_bloomWidth, m_bloomHeight);
    createTexFBO(m_bloomCompositeFBO, m_bloomCompositeTex, m_width, m_height);

    // 3. FBO para FXAA
    createTexFBO(m_fxaaFBO, m_fxaaTex, m_width, m_height, GL_RGBA8, GL_UNSIGNED_BYTE);

    // 4. FBO para Profundidad de Campo (DoF) (Fase 13)
    createTexFBO(m_dofFBO, m_dofTex, m_width, m_height, GL_RGBA16F, GL_FLOAT);

    // 4.5. FBO para Screen-Space Contact Shadows (Fase 20)
    createTexFBO(m_contactShadowsFBO, m_contactShadowsTex, m_width, m_height, GL_RGBA16F, GL_FLOAT);

    // 4.6. FBO para Efectos Subacuáticos (Underwater Mode)
    createTexFBO(m_underwaterFBO, m_underwaterTex, m_width, m_height, GL_RGBA16F, GL_FLOAT);

    // 5. FBO para Medición de Luminancia y Auto-Exposición (16x16) (Fase 17)
    createTexFBO(m_lumFBO, m_lumTex, 16, 16, GL_RGBA16F, GL_FLOAT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_initialized = true;
    std::cout << "[ASIS Engine] Pipeline de Post-procesado (Bloom HDR y FXAA) inicializado (" << m_width << "x" << m_height << ").\n";
    return true;
}

void PostProcess::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == m_width && height == m_height) return;

    destroy();
    init(width, height);
}

GLuint PostProcess::applyBloom(GLuint sceneTexture, float threshold, float intensity) {
    if (!m_initialized || intensity <= 0.001f) return sceneTexture;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // PASO 1: Extraer altas luces
    glBindFramebuffer(GL_FRAMEBUFFER, m_bloomExtractFBO);
    glViewport(0, 0, m_bloomWidth, m_bloomHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    m_extractShader->use();
    m_extractShader->setFloat("u_Threshold", threshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    m_extractShader->setInt("u_Scene", 0);
    renderQuad();

    // PASO 2: Desenfocar en dos direcciones (Gaussian Blur Ping-Pong)
    bool horizontal = true;
    bool first_iteration = true;
    int amount = 6; // 3 pasadas horizontales y 3 verticales

    m_blurShader->use();
    for (int i = 0; i < amount; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomBlurFBO[horizontal ? 1 : 0]);
        m_blurShader->setInt("u_Horizontal", horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? m_bloomExtractTex : m_bloomBlurTex[horizontal ? 0 : 1]);
        m_blurShader->setInt("u_Image", 0);

        renderQuad();
        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

    // PASO 3: Composición aditiva sobre la escena
    glBindFramebuffer(GL_FRAMEBUFFER, m_bloomCompositeFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_compositeShader->use();
    m_compositeShader->setFloat("u_Intensity", intensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    m_compositeShader->setInt("u_Scene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_bloomBlurTex[horizontal ? 0 : 1]);
    m_compositeShader->setInt("u_Bloom", 1);

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return m_bloomCompositeTex;
}

GLuint PostProcess::applyFXAA(GLuint inputTexture) {
    if (!m_initialized || !m_fxaaShader) return inputTexture;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fxaaFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_fxaaShader->use();
    m_fxaaShader->setVec2("u_Resolution", glm::vec2(m_width, m_height));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    m_fxaaShader->setInt("u_Texture", 0);

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return m_fxaaTex;
}

GLuint PostProcess::applyDepthOfField(GLuint sceneTexture, GLuint depthTexture, float focusDist, float focusRange, float bokehRadius, float nearPlane, float farPlane) {
    if (!m_initialized || !m_dofShader || bokehRadius <= 0.001f || depthTexture == 0) return sceneTexture;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, m_dofFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_dofShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    m_dofShader->setInt("u_ColorTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_dofShader->setInt("u_DepthTexture", 1);

    m_dofShader->setFloat("u_FocusDistance", focusDist);
    m_dofShader->setFloat("u_FocusRange", focusRange);
    m_dofShader->setFloat("u_BokehRadius", bokehRadius);
    m_dofShader->setFloat("u_Near", nearPlane);
    m_dofShader->setFloat("u_Far", farPlane);
    m_dofShader->setVec2("u_TexelSize", glm::vec2(1.0f / float(m_width), 1.0f / float(m_height)));

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return m_dofTex;
}

GLuint PostProcess::applyContactShadows(GLuint sceneTexture, GLuint depthTexture, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& lightDir, float distance, int steps, float intensity, float nearPlane, float farPlane) {
    if (!m_initialized || !m_contactShadowsShader || m_contactShadowsFBO == 0 || sceneTexture == 0 || depthTexture == 0 || intensity <= 0.001f) {
        return sceneTexture;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, m_contactShadowsFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_contactShadowsShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    m_contactShadowsShader->setInt("u_Scene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_contactShadowsShader->setInt("u_Depth", 1);

    m_contactShadowsShader->setMat4("u_Projection", projection);
    m_contactShadowsShader->setMat4("u_View", view);
    m_contactShadowsShader->setMat4("u_InvProjection", glm::inverse(projection));
    m_contactShadowsShader->setVec3("u_LightDir", lightDir);

    m_contactShadowsShader->setFloat("u_Distance", distance);
    m_contactShadowsShader->setInt("u_Steps", steps);
    m_contactShadowsShader->setFloat("u_Intensity", intensity);
    m_contactShadowsShader->setFloat("u_Near", nearPlane);
    m_contactShadowsShader->setFloat("u_Far", farPlane);

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return m_contactShadowsTex;
}

GLuint PostProcess::applyUnderwater(GLuint sceneTexture, GLuint depthTexture, float time, float waterLevel, const glm::vec3& camPos, const glm::mat4& projection, const glm::mat4& view, float nearPlane, float farPlane) {
    if (!m_initialized || !m_underwaterShader || m_underwaterFBO == 0 || sceneTexture == 0) {
        return sceneTexture;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, m_underwaterFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_underwaterShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    m_underwaterShader->setInt("u_Scene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_underwaterShader->setInt("u_Depth", 1);

    m_underwaterShader->setFloat("u_Time", time);
    m_underwaterShader->setFloat("u_WaterLevel", waterLevel);
    m_underwaterShader->setVec3("u_CamPos", camPos);
    m_underwaterShader->setMat4("u_InvProjection", glm::inverse(projection));
    m_underwaterShader->setMat4("u_InvView", glm::inverse(view));
    m_underwaterShader->setFloat("u_Near", nearPlane);
    m_underwaterShader->setFloat("u_Far", farPlane);

    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return m_underwaterTex;
}

float PostProcess::measureAverageLuminance(GLuint sceneTexture) {
    if (!m_initialized || !m_lumShader || m_lumFBO == 0 || sceneTexture == 0) return 0.18f;

    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, m_lumFBO);
    glViewport(0, 0, 16, 16);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);

    m_lumShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    m_lumShader->setInt("u_Scene", 0);
    renderQuad();

    float pixels[16 * 16 * 4];
    glReadPixels(0, 0, 16, 16, GL_RGBA, GL_FLOAT, pixels);

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    float sumLog = 0.0f;
    int validCount = 0;
    for (int i = 0; i < 16 * 16; ++i) {
        float logLum = pixels[i * 4 + 0];
        if (!std::isnan(logLum) && !std::isinf(logLum)) {
            sumLog += logLum;
            validCount++;
        }
    }

    if (validCount == 0) return 0.18f;
    float avgLog = sumLog / static_cast<float>(validCount);
    float avgLum = std::exp(avgLog);
    return std::clamp(avgLum, 0.0001f, 10000.0f);
}

bool PostProcess::renderToImage(GLuint textureID, int width, int height, const std::string& outputPath) {
    if (textureID == 0 || width <= 0 || height <= 0) return false;

    std::vector<unsigned char> pixels(width * height * 4);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Invertir verticalmente para formato estándar de imagen (OpenGL almacena origen abajo a la izquierda)
    std::vector<unsigned char> flipped(width * height * 4);
    for (int y = 0; y < height; ++y) {
        std::memcpy(&flipped[y * width * 4], &pixels[(height - 1 - y) * width * 4], width * 4);
    }

    try {
        std::filesystem::path p(outputPath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {}

    int res = stbi_write_png(outputPath.c_str(), width, height, 4, flipped.data(), width * 4);
    if (res != 0) {
        std::cout << "[Render a Imagen] ✓ Captura guardada en: " << outputPath << " (" << width << "x" << height << ")\n";
        return true;
    }
    return false;
}

void PostProcess::destroy() {
    auto delFBO = [](GLuint& fbo, GLuint& tex) {
        if (fbo != 0) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
        if (tex != 0) { glDeleteTextures(1, &tex); tex = 0; }
    };

    delFBO(m_bloomExtractFBO, m_bloomExtractTex);
    delFBO(m_bloomBlurFBO[0], m_bloomBlurTex[0]);
    delFBO(m_bloomBlurFBO[1], m_bloomBlurTex[1]);
    delFBO(m_bloomCompositeFBO, m_bloomCompositeTex);
    delFBO(m_fxaaFBO, m_fxaaTex);
    delFBO(m_dofFBO, m_dofTex);
    delFBO(m_contactShadowsFBO, m_contactShadowsTex);
    delFBO(m_underwaterFBO, m_underwaterTex);
    delFBO(m_lumFBO, m_lumTex);

    if (m_quadVAO != 0) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO != 0) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    m_initialized = false;
}

} // namespace ASIS
