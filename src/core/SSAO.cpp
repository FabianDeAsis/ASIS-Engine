#include "SSAO.h"
#include <random>
#include <iostream>
#include <glm/gtc/matrix_inverse.hpp>

namespace ASIS {

SSAO::SSAO()
    : m_width(1280), m_height(720), m_initialized(false),
      m_ssaoFBO(0), m_ssaoColorTex(0),
      m_ssaoBlurFBO(0), m_ssaoBlurTex(0),
      m_ssaoCompositeFBO(0), m_ssaoCompositeTex(0),
      m_noiseTexture(0), m_quadVAO(0), m_quadVBO(0)
{
}

SSAO::~SSAO() {
    destroy();
}

bool SSAO::init(int width, int height) {
    if (width <= 0 || height <= 0) return false;
    m_width = width;
    m_height = height;

    try {
        m_ssaoShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/ssao.frag");
        m_blurShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/ssao_blur.frag");
        m_compositeShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/ssao_composite.frag");
    }
    catch (const std::exception& e) {
        std::cerr << "[ASIS SSAO ERROR] Fallo al compilar shaders de SSAO: " << e.what() << "\n";
        return false;
    }

    setupQuad();
    generateKernelAndNoise();

    // 1. FBO para cálculo de SSAO raw
    glGenFramebuffers(1, &m_ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);

    glGenTextures(1, &m_ssaoColorTex);
    glBindTexture(GL_TEXTURE_2D, m_ssaoColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoColorTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ASIS SSAO ERROR] SSAO FBO incompleto.\n";
        return false;
    }

    // 2. FBO para desenfoque bilateral SSAO
    glGenFramebuffers(1, &m_ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);

    glGenTextures(1, &m_ssaoBlurTex);
    glBindTexture(GL_TEXTURE_2D, m_ssaoBlurTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoBlurTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ASIS SSAO ERROR] SSAO Blur FBO incompleto.\n";
        return false;
    }

    // 3. FBO para composición con la escena HDR
    glGenFramebuffers(1, &m_ssaoCompositeFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoCompositeFBO);

    glGenTextures(1, &m_ssaoCompositeTex);
    glBindTexture(GL_TEXTURE_2D, m_ssaoCompositeTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoCompositeTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_initialized = true;
    std::cout << "[ASIS SSAO] Modulo SSAO inicializado (" << m_width << "x" << m_height << ", 32 muestras)\n";
    return true;
}

void SSAO::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == m_width && height == m_height) return;

    destroy();
    init(width, height);
}

void SSAO::destroy() {
    if (m_ssaoFBO != 0) { glDeleteFramebuffers(1, &m_ssaoFBO); m_ssaoFBO = 0; }
    if (m_ssaoColorTex != 0) { glDeleteTextures(1, &m_ssaoColorTex); m_ssaoColorTex = 0; }
    if (m_ssaoBlurFBO != 0) { glDeleteFramebuffers(1, &m_ssaoBlurFBO); m_ssaoBlurFBO = 0; }
    if (m_ssaoBlurTex != 0) { glDeleteTextures(1, &m_ssaoBlurTex); m_ssaoBlurTex = 0; }
    if (m_ssaoCompositeFBO != 0) { glDeleteFramebuffers(1, &m_ssaoCompositeFBO); m_ssaoCompositeFBO = 0; }
    if (m_ssaoCompositeTex != 0) { glDeleteTextures(1, &m_ssaoCompositeTex); m_ssaoCompositeTex = 0; }
    if (m_noiseTexture != 0) { glDeleteTextures(1, &m_noiseTexture); m_noiseTexture = 0; }
    if (m_quadVAO != 0) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO != 0) { glDeleteBuffers(1, &m_quadVBO); m_quadVBO = 0; }
    m_initialized = false;
}

void SSAO::setupQuad() {
    if (m_quadVAO != 0) return;

    float quadVertices[] = {
        // Posición    // TexCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void SSAO::renderQuad() {
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void SSAO::generateKernelAndNoise() {
    m_ssaoKernel.clear();
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator(1337);

    for (int i = 0; i < 32; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) // hemisferio positivo en Z
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // Distribución cuadrática (mayor densidad cerca de la superficie)
        float scale = float(i) / 32.0f;
        scale = 0.1f + scale * scale * (1.0f - 0.1f);
        sample *= scale;

        m_ssaoKernel.push_back(sample);
    }

    // Textura de rotación 4x4 (16 vectores tangentes)
    std::vector<glm::vec3> ssaoNoise;
    for (int i = 0; i < 16; ++i) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );
        ssaoNoise.push_back(noise);
    }

    if (m_noiseTexture != 0) {
        glDeleteTextures(1, &m_noiseTexture);
    }

    glGenTextures(1, &m_noiseTexture);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

GLuint SSAO::compute(GLuint depthTexture, const glm::mat4& projection, const glm::mat4& view, float radius, float bias, float power) {
    if (!m_initialized || depthTexture == 0) return 0;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // PASO 1: Calcular SSAO
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
    glViewport(0, 0, m_width, m_height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_ssaoShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_ssaoShader->setInt("u_DepthTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    m_ssaoShader->setInt("u_NoiseTexture", 1);

    for (int i = 0; i < 32; ++i) {
        m_ssaoShader->setVec3("u_Samples[" + std::to_string(i) + "]", m_ssaoKernel[i]);
    }

    glm::mat4 invProj = glm::inverse(projection);
    m_ssaoShader->setMat4("u_Projection", projection);
    m_ssaoShader->setMat4("u_InvProjection", invProj);
    m_ssaoShader->setVec2("u_NoiseScale", float(m_width) / 4.0f, float(m_height) / 4.0f);
    m_ssaoShader->setFloat("u_Radius", radius);
    m_ssaoShader->setFloat("u_Bias", bias);
    m_ssaoShader->setFloat("u_Power", power);

    renderQuad();

    // PASO 2: Desenfoque Bilateral para eliminar ruido
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_blurShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ssaoColorTex);
    m_blurShader->setInt("u_SSAOInput", 0);
    m_blurShader->setVec2("u_TexelSize", 1.0f / float(m_width), 1.0f / float(m_height));

    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return m_ssaoBlurTex;
}

GLuint SSAO::apply(GLuint sceneColorTexture, GLuint ssaoBlurTexture, float intensity) {
    if (!m_initialized || intensity <= 0.001f || ssaoBlurTexture == 0) return sceneColorTexture;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoCompositeFBO);
    glViewport(0, 0, m_width, m_height);

    m_compositeShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    m_compositeShader->setInt("u_SceneTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    m_compositeShader->setInt("u_SSAOTexture", 1);
    m_compositeShader->setFloat("u_Intensity", intensity);

    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return m_ssaoCompositeTex;
}

} // namespace ASIS
