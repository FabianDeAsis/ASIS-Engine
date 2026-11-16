// ============================================================================
// ASIS Engine - Fase 13: VFX Partículas 3D, SSAO Bilateral y Rigging Assimp
// ============================================================================

#include "ParticleSystem.h"
#include "SSAO.h"
#include "Animation.h"
#include "PostProcess.h"

// 1. Emisión y Actualización de Partículas Instanciadas en GPU
void demoParticleEmitter(ASIS::ParticleSystem& vfx, const glm::vec3& pos) {
    // Añadir emisor de fuego con mezcla aditiva
    auto fire = vfx.addEmitter("Fuego_Magico", ASIS::EmitterType::FIRE, pos);
    fire->setEmissionRate(250.0f);
    fire->setMaxParticles(1500);
    fire->setStartColor(glm::vec4(1.0f, 0.6f, 0.1f, 1.0f));
    fire->setEndColor(glm::vec4(0.8f, 0.1f, 0.0f, 0.0f));
    fire->setStartSize(0.6f);
    fire->setEndSize(0.05f);
    fire->setVelocity(glm::vec3(0.0f, 2.5f, 0.0f));
    fire->setVelocitySpread(glm::vec3(0.8f, 0.4f, 0.8f));
    fire->setAdditive(true);
}

// 2. Cálculo de SSAO Bilateral y Composición
GLuint computeSSAO(ASIS::SSAO& ssao, GLuint depthTex, GLuint colorTex, const glm::mat4& proj, const glm::mat4& view) {
    // Hemisferio de 32 muestras orientadas en espacio de vista con derivadas analíticas
    GLuint ssaoBlur = ssao.compute(depthTex, proj, view, 0.75f, 0.025f, 1.6f);
    // Modular únicamente la iluminación ambiental preservando luces directas
    return ssao.apply(colorTex, ssaoBlur, 1.0f);
}

// 3. Profundidad de Campo Fotográfica (DoF / Bokeh Poisson Disc)
GLuint computeDoF(ASIS::PostProcess& postProcess, GLuint sceneTex, GLuint depthTex, float focusDist, float nearPlane, float farPlane) {
    return postProcess.applyDepthOfField(sceneTex, depthTex, focusDist, 3.0f, 4.0f, nearPlane, farPlane);
}

// 4. Skinning de Huesos por Hardware y Reproducción en GPU
void updateModelAnimation(ASIS::Model& model, float dt) {
    if (model.hasAnimation()) {
        ASIS::Animator* animator = model.getAnimator();
        animator->updateAnimation(dt);
        // Los uniforms u_BoneTransforms[100] y u_IsAnimated se cargan en model.vert
    }
}
