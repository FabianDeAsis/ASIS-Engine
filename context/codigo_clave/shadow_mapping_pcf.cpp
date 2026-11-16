// ==============================================================================
// Snippet de Referencia: Shadow Mapping 2K con PCF y Plano Receptor en ASIS Engine
// Archivo de referencia para Fase 7: Iluminación Avanzada y Sombras Dinámicas
// ==============================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 1. Matriz de Espacio de Luz Solar
glm::mat4 computeLightSpaceMatrix(glm::vec3 lightDir, float sceneRadius = 22.0f) {
    glm::vec3 lightPos = -lightDir * 28.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 1.0f, 65.0f);
    return lightProj * lightView;
}

// 2. Pase 1: Renderizado de Profundidad desde la Luz
void renderShadowDepthPass(ASIS::ShadowMap& shadowMap, ASIS::Shader& depthShader, ASIS::Scene& scene, const glm::mat4& lightSpaceMatrix) {
    shadowMap.bind(); // glViewport(0, 0, 2048, 2048) y glBindFramebuffer
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.use();
    depthShader.setMat4("u_LightSpaceMatrix", lightSpaceMatrix);
    scene.draw(depthShader);

    shadowMap.unbind();
}

// 3. Fragment Shader GLSL: Cálculo de Sombra con PCF 3x3 y Bias Adaptativo
/*
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (u_ShadowsEnabled == 0) return 0.0;

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    vec3 toLight = normalize(-lightDir);
    float cosTheta = max(dot(normal, toLight), 0.0);
    float bias = max(0.005 * (1.0 - cosTheta), u_ShadowBias);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}
*/
