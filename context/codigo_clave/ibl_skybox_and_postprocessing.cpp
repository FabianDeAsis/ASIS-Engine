// ========================================================================
// ASIS Engine - FASE 12: Arquitectura de IBL / Skybox HDRI, MSAA y Post-Procesado
// ========================================================================

#include "Skybox.h"
#include "PostProcess.h"
#include "Framebuffer.h"

// 1. Doble Framebuffer con Multimuestreo en Hardware (MSAA FBO -> Resolve FBO HDR)
void setupMSAAFramebuffer() {
    // FBO Multisample con renderbuffers multirresueltos
    GLuint msaaFBO, msaaColorRBO, msaaDepthRBO;
    glGenFramebuffers(1, &msaaFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);

    glGenRenderbuffers(1, &msaaColorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA16F, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRBO);

    glGenRenderbuffers(1, &msaaDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaaDepthRBO);

    // Resolución automática vía Blit hacia textura GL_RGBA16F
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

// 2. Renderizado de Fondo Skybox con control de rotación y desenfoque (LOD)
void drawSkybox(const Camera& camera, float aspectRatio, float exposure, float rotDeg, float blurLOD) {
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    skyboxShader->use();
    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix())); // Sin traslación
    glm::mat4 proj = camera.getProjectionMatrix(aspectRatio);

    skyboxShader->setMat4("u_View", view);
    skyboxShader->setMat4("u_Projection", proj);
    skyboxShader->setFloat("u_SkyboxExposure", exposure);
    skyboxShader->setFloat("u_SkyboxRotation", glm::radians(rotDeg));
    skyboxShader->setFloat("u_SkyboxBlur", blurLOD);

    glBindTexture(GL_TEXTURE_2D, envTexture);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

// 3. Iluminación Basada en Imágenes (IBL) y Reflejos en Shader PBR (model.frag)
/*
    // Muestreo de reflexión especular con LOD proporcional a la rugosidad
    vec3 R = reflect(-V, N);
    vec3 rotatedR = rotateY(R, u_SkyboxRotation);
    vec2 envUV = dirToUV(normalize(rotatedR));
    float mipLevel = roughness * 7.0;
    vec3 prefilteredColor = textureLod(u_EnvMap, envUV, mipLevel).rgb;

    vec3 F_env = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 envSpecular = prefilteredColor * F_env * specular;

    // Irradiancia difusa ambiental
    vec3 rotatedN = rotateY(N, u_SkyboxRotation);
    vec2 irradianceUV = dirToUV(normalize(rotatedN));
    vec3 diffuseIrradiance = textureLod(u_EnvMap, irradianceUV, 6.0).rgb;
    vec3 kD_env = (vec3(1.0) - F_env) * (1.0 - metallic);
    vec3 envDiffuse = diffuseIrradiance * albedo * kD_env;

    color += (envDiffuse + envSpecular) * ao * u_IBLIntensity;
*/
