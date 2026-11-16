#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Shader.h"

namespace ASIS {

class SSAO {
public:
    SSAO();
    ~SSAO();

    bool init(int width, int height);
    void resize(int width, int height);
    void destroy();

    // Calcular el mapa de oclusión ambiental SSAO y su desenfoque bilateral
    GLuint compute(GLuint depthTexture, const glm::mat4& projection, const glm::mat4& view, float radius = 0.75f, float bias = 0.025f, float power = 1.6f);

    // Modular la escena de color aplicando la oclusión ambiental con la intensidad deseada
    GLuint apply(GLuint sceneColorTexture, GLuint ssaoBlurTexture, float intensity = 1.0f);

    GLuint getSSAORawTexture() const { return m_ssaoColorTex; }
    GLuint getSSAOBlurTexture() const { return m_ssaoBlurTex; }

private:
    int m_width;
    int m_height;
    bool m_initialized;

    // FBOs y texturas (resolución completa o media para máximo rendimiento)
    GLuint m_ssaoFBO;
    GLuint m_ssaoColorTex;
    GLuint m_ssaoBlurFBO;
    GLuint m_ssaoBlurTex;
    GLuint m_ssaoCompositeFBO;
    GLuint m_ssaoCompositeTex;

    // Ruido y Kernel de muestras
    GLuint m_noiseTexture;
    std::vector<glm::vec3> m_ssaoKernel;

    // Shaders
    std::unique_ptr<Shader> m_ssaoShader;
    std::unique_ptr<Shader> m_blurShader;
    std::unique_ptr<Shader> m_compositeShader;

    // Geometría de Quad
    GLuint m_quadVAO;
    GLuint m_quadVBO;

    void generateKernelAndNoise();
    void setupQuad();
    void renderQuad();
};

} // namespace ASIS
