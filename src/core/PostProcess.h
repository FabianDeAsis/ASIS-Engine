#pragma once

#include <glad/glad.h>
#include <memory>
#include <string>
#include "Shader.h"

namespace ASIS {

class PostProcess {
public:
    PostProcess();
    ~PostProcess();

    bool init(int width, int height);
    void resize(int width, int height);
    void destroy();

    // Aplicar Bloom HDR sobre la escena y retornar la textura resultante
    GLuint applyBloom(GLuint sceneTexture, float threshold, float intensity);

    // Aplicar Profundidad de Campo (Depth of Field / Bokeh) (Fase 13)
    GLuint applyDepthOfField(GLuint sceneTexture, GLuint depthTexture, float focusDist, float focusRange, float bokehRadius, float nearPlane, float farPlane);

    // Aplicar FXAA (Fast Approximate Anti-Aliasing) sobre la textura de entrada
    GLuint applyFXAA(GLuint inputTexture);

    // Aplicar Screen-Space Contact Shadows (Fase 20)
    GLuint applyContactShadows(GLuint sceneTexture, GLuint depthTexture, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& lightDir, float distance, int steps, float intensity, float nearPlane, float farPlane);

    // Aplicar Efectos Ópticos y Niebla Subacuática (Underwater Mode)
    GLuint applyUnderwater(GLuint sceneTexture, GLuint depthTexture, float time, float waterLevel, const glm::vec3& camPos, const glm::mat4& projection, const glm::mat4& view, float nearPlane, float farPlane);

    // Medir luminancia media geométrica / logarítmica de la escena para Auto-Exposición (Fase 17)
    float measureAverageLuminance(GLuint sceneTexture);

    // Capturar cualquier textura del motor y guardarla en disco como imagen PNG sin pérdidas
    static bool renderToImage(GLuint textureID, int width, int height, const std::string& outputPath);

    GLuint getQuadVAO() const { return m_quadVAO; }

private:
    int m_width;
    int m_height;
    bool m_initialized;

    // Recursos de geometría para pantalla completa
    GLuint m_quadVAO;
    GLuint m_quadVBO;

    // Recursos Bloom (ping-pong FBOs a media resolución para velocidad y radio amplio)
    int m_bloomWidth;
    int m_bloomHeight;
    GLuint m_bloomExtractFBO;
    GLuint m_bloomExtractTex;
    GLuint m_bloomBlurFBO[2];
    GLuint m_bloomBlurTex[2];
    GLuint m_bloomCompositeFBO;
    GLuint m_bloomCompositeTex;

    std::unique_ptr<Shader> m_extractShader;
    std::unique_ptr<Shader> m_blurShader;
    std::unique_ptr<Shader> m_compositeShader;

    // Recursos FXAA
    GLuint m_fxaaFBO;
    GLuint m_fxaaTex;
    std::unique_ptr<Shader> m_fxaaShader;

    // Recursos Depth of Field (DoF) (Fase 13)
    GLuint m_dofFBO;
    GLuint m_dofTex;
    std::unique_ptr<Shader> m_dofShader;

    // Recursos Screen-Space Contact Shadows (Fase 20)
    GLuint m_contactShadowsFBO;
    GLuint m_contactShadowsTex;
    std::unique_ptr<Shader> m_contactShadowsShader;

    // Recursos Subacuáticos (Underwater Mode)
    GLuint m_underwaterFBO;
    GLuint m_underwaterTex;
    std::unique_ptr<Shader> m_underwaterShader;

    // Recursos Medición de Luminancia y Auto-Exposición (Fase 17)
    GLuint m_lumFBO;
    GLuint m_lumTex;
    std::unique_ptr<Shader> m_lumShader;

    void setupQuad();
    void renderQuad();
};

} // namespace ASIS
