#include "ParticleSystem.h"
#include <random>
#include <iostream>
#include <algorithm>

namespace ASIS {

ParticleEmitter::ParticleEmitter(const std::string& name, EmitterType type, const glm::vec3& position)
    : m_name(name), m_type(type), m_position(position), m_active(true),
      m_emissionRate(60.0f), m_maxParticles(1000), m_lifetime(1.5f),
      m_startColor(1.0f), m_endColor(0.0f), m_startSize(0.2f), m_endSize(0.6f),
      m_velocity(0.0f, 1.5f, 0.0f), m_velocitySpread(0.4f), m_gravity(0.0f),
      m_additiveBlend(true), m_accumulator(0.0f)
{
    applyPreset(type);
    m_particles.reserve(m_maxParticles);
}

void ParticleEmitter::setType(EmitterType type) {
    m_type = type;
    applyPreset(type);
}

void ParticleEmitter::setMaxParticles(int maxP) {
    m_maxParticles = std::max(10, maxP);
    if ((int)m_particles.size() > m_maxParticles) {
        m_particles.resize(m_maxParticles);
    }
}

void ParticleEmitter::reset() {
    m_particles.clear();
    m_accumulator = 0.0f;
}

void ParticleEmitter::applyPreset(EmitterType type) {
    switch (type) {
    case EmitterType::FIRE:
        m_emissionRate = 80.0f;
        m_maxParticles = 600;
        m_lifetime = 1.2f;
        m_startColor = glm::vec4(1.0f, 0.75f, 0.20f, 0.95f);
        m_endColor = glm::vec4(0.85f, 0.12f, 0.02f, 0.0f);
        m_startSize = 0.25f;
        m_endSize = 0.85f;
        m_velocity = glm::vec3(0.0f, 2.4f, 0.0f);
        m_velocitySpread = glm::vec3(0.45f, 0.6f, 0.45f);
        m_gravity = glm::vec3(0.0f, 0.4f, 0.0f); // Ascendente
        m_additiveBlend = true;
        break;

    case EmitterType::SPARKS:
        m_emissionRate = 70.0f;
        m_maxParticles = 500;
        m_lifetime = 0.9f;
        m_startColor = glm::vec4(1.0f, 0.98f, 0.85f, 1.0f);
        m_endColor = glm::vec4(1.0f, 0.45f, 0.05f, 0.0f);
        m_startSize = 0.12f;
        m_endSize = 0.02f;
        m_velocity = glm::vec3(0.0f, 4.2f, 0.0f);
        m_velocitySpread = glm::vec3(3.2f, 2.5f, 3.2f);
        m_gravity = glm::vec3(0.0f, -9.8f, 0.0f);
        m_additiveBlend = true;
        break;

    case EmitterType::SMOKE:
        m_emissionRate = 35.0f;
        m_maxParticles = 500;
        m_lifetime = 2.8f;
        m_startColor = glm::vec4(0.35f, 0.35f, 0.40f, 0.75f);
        m_endColor = glm::vec4(0.12f, 0.12f, 0.14f, 0.0f);
        m_startSize = 0.4f;
        m_endSize = 2.2f;
        m_velocity = glm::vec3(0.0f, 1.1f, 0.0f);
        m_velocitySpread = glm::vec3(0.35f, 0.4f, 0.35f);
        m_gravity = glm::vec3(0.0f, 0.15f, 0.0f);
        m_additiveBlend = false;
        break;

    case EmitterType::RAIN:
        m_emissionRate = 950.0f;
        m_maxParticles = 3000;
        m_lifetime = 1.1f;
        m_startColor = glm::vec4(0.72f, 0.85f, 1.0f, 0.85f);
        m_endColor = glm::vec4(0.80f, 0.92f, 1.0f, 0.25f);
        m_startSize = 0.14f;
        m_endSize = 0.08f;
        m_velocity = glm::vec3(-2.0f, -24.0f, 1.0f);
        m_velocitySpread = glm::vec3(22.0f, 2.0f, 22.0f);
        m_gravity = glm::vec3(0.0f, -9.81f, 0.0f);
        m_additiveBlend = false;
        break;

    case EmitterType::MAGIC:
        m_emissionRate = 65.0f;
        m_maxParticles = 800;
        m_lifetime = 2.0f;
        m_startColor = glm::vec4(0.65f, 0.15f, 1.0f, 0.95f);
        m_endColor = glm::vec4(0.10f, 0.95f, 0.85f, 0.0f);
        m_startSize = 0.35f;
        m_endSize = 0.08f;
        m_velocity = glm::vec3(0.0f, 1.6f, 0.0f);
        m_velocitySpread = glm::vec3(1.8f, 1.2f, 1.8f);
        m_gravity = glm::vec3(0.0f, 0.0f, 0.0f);
        m_additiveBlend = true;
        break;

    case EmitterType::CUSTOM:
    default:
        break;
    }
}

void ParticleEmitter::emit(int count) {
    static std::mt19937 gen(42);
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    std::uniform_real_distribution<float> dis01(0.0f, 1.0f);

    for (int i = 0; i < count; ++i) {
        if ((int)m_particles.size() >= m_maxParticles) {
            // Reutilizar partícula muerta si hay
            bool reused = false;
            for (auto& p : m_particles) {
                if (p.life <= 0.0f) {
                    p.position = m_position;
                    if (m_type == EmitterType::RAIN) {
                        p.position += glm::vec3(dis(gen) * m_velocitySpread.x, 8.0f, dis(gen) * m_velocitySpread.z);
                    }
                    p.velocity = m_velocity + glm::vec3(dis(gen) * m_velocitySpread.x, dis(gen) * m_velocitySpread.y, dis(gen) * m_velocitySpread.z);
                    p.color = m_startColor;
                    p.size = m_startSize;
                    p.maxLife = m_lifetime * (0.8f + dis01(gen) * 0.4f);
                    p.life = p.maxLife;
                    p.rotation = dis01(gen) * 6.28318f;
                    p.rotSpeed = dis(gen) * 3.0f;
                    reused = true;
                    break;
                }
            }
            if (!reused) break;
        } else {
            Particle p;
            p.position = m_position;
            if (m_type == EmitterType::RAIN) {
                p.position += glm::vec3(dis(gen) * m_velocitySpread.x, 8.0f, dis(gen) * m_velocitySpread.z);
            }
            p.velocity = m_velocity + glm::vec3(dis(gen) * m_velocitySpread.x, dis(gen) * m_velocitySpread.y, dis(gen) * m_velocitySpread.z);
            p.color = m_startColor;
            p.size = m_startSize;
            p.maxLife = m_lifetime * (0.8f + dis01(gen) * 0.4f);
            p.life = p.maxLife;
            p.rotation = dis01(gen) * 6.28318f;
            p.rotSpeed = dis(gen) * 3.0f;
            m_particles.push_back(p);
        }
    }
}

void ParticleEmitter::update(float dt) {
    if (!m_active) return;

    // 1. Actualizar partículas vivas
    for (auto& p : m_particles) {
        if (p.life > 0.0f) {
            p.life -= dt;
            if (p.life > 0.0f) {
                p.velocity += m_gravity * dt;
                p.position += p.velocity * dt;
                p.rotation += p.rotSpeed * dt;

                float t = 1.0f - (p.life / p.maxLife);
                p.color = glm::mix(m_startColor, m_endColor, t);
                p.size = glm::mix(m_startSize, m_endSize, t);
            }
        }
    }

    // 2. Emitir nuevas partículas
    if (m_emissionRate > 0.0f) {
        m_accumulator += dt;
        float interval = 1.0f / m_emissionRate;
        int spawnCount = static_cast<int>(m_accumulator / interval);
        if (spawnCount > 0) {
            emit(spawnCount);
            m_accumulator -= spawnCount * interval;
        }
    }
}

size_t ParticleEmitter::getAliveCount() const {
    size_t count = 0;
    for (const auto& p : m_particles) {
        if (p.life > 0.0f) count++;
    }
    return count;
}

void ParticleEmitter::fillInstanceData(std::vector<ParticleInstanceData>& outData) const {
    if (!m_active) return;
    for (const auto& p : m_particles) {
        if (p.life > 0.0f && p.size > 0.001f && p.color.a > 0.001f) {
            ParticleInstanceData inst;
            inst.position = p.position;
            inst.size = p.size;
            inst.color = p.color;
            inst.rotation = p.rotation;
            outData.push_back(inst);
        }
    }
}

// ============================================================================
// ParticleSystem
// ============================================================================

ParticleSystem::ParticleSystem()
    : m_initialized(false), m_quadVAO(0), m_quadVBO(0), m_instanceVBO(0)
{
}

ParticleSystem::~ParticleSystem() {
    destroy();
}

bool ParticleSystem::init() {
    try {
        m_particleShader = std::make_unique<Shader>("shaders/particle.vert", "shaders/particle.frag");
    }
    catch (const std::exception& e) {
        std::cerr << "[ASIS ParticleSystem ERROR] No se pudo compilar shader de partículas: " << e.what() << "\n";
        return false;
    }

    setupInstancedQuad();
    m_initialized = true;
    std::cout << "[ASIS Engine] Sistema de Partículas 3D (VFX Engine) inicializado con éxito.\n";
    return true;
}

void ParticleSystem::setupInstancedQuad() {
    float quadVertices[] = {
        // Posición    // UVs
        -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,

        -0.5f,  0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_instanceVBO);

    glBindVertexArray(m_quadVAO);

    // Vértices del quad
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // Datos por instancia
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    // Buffer dinámico predimensionado
    glBufferData(GL_ARRAY_BUFFER, 2000 * sizeof(ParticleInstanceData), nullptr, GL_DYNAMIC_DRAW);

    // 2: Posición (vec3)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)offsetof(ParticleInstanceData, position));
    glVertexAttribDivisor(2, 1);

    // 3: Tamaño (float)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)offsetof(ParticleInstanceData, size));
    glVertexAttribDivisor(3, 1);

    // 4: Color (vec4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)offsetof(ParticleInstanceData, color));
    glVertexAttribDivisor(4, 1);

    // 5: Rotación (float)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)offsetof(ParticleInstanceData, rotation));
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);
}

void ParticleSystem::update(float dt) {
    for (auto& emitter : m_emitters) {
        if (emitter) {
            emitter->update(dt);
        }
    }
}

void ParticleSystem::draw(const Camera& camera, const glm::mat4& projection, const glm::mat4& view) {
    if (!m_initialized || m_emitters.empty()) return;

    m_additiveInstances.clear();
    m_alphaInstances.clear();

    for (const auto& emitter : m_emitters) {
        if (!emitter || !emitter->isActive()) continue;
        if (emitter->isAdditive()) {
            emitter->fillInstanceData(m_additiveInstances);
        } else {
            emitter->fillInstanceData(m_alphaInstances);
        }
    }

    if (m_additiveInstances.empty() && m_alphaInstances.empty()) return;

    m_particleShader->use();
    m_particleShader->setMat4("u_Projection", projection);
    m_particleShader->setMat4("u_View", view);
    m_particleShader->setVec3("u_CameraRight", camera.getRight());
    m_particleShader->setVec3("u_CameraUp", camera.getUp());

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE); // Partículas translúcidas no escriben en depth buffer
    glBindVertexArray(m_quadVAO);

    // 1. Dibujar partículas de mezcla Alpha (humo, lluvia)
    if (!m_alphaInstances.empty()) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, m_alphaInstances.size() * sizeof(ParticleInstanceData), m_alphaInstances.data(), GL_DYNAMIC_DRAW);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(m_alphaInstances.size()));
    }

    // 2. Dibujar partículas de mezcla Aditiva (fuego, chispas, magia)
    if (!m_additiveInstances.empty()) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, m_additiveInstances.size() * sizeof(ParticleInstanceData), m_additiveInstances.data(), GL_DYNAMIC_DRAW);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(m_additiveInstances.size()));
    }

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

std::shared_ptr<ParticleEmitter> ParticleSystem::addEmitter(const std::string& name, EmitterType type, const glm::vec3& position) {
    auto emitter = std::make_shared<ParticleEmitter>(name, type, position);
    m_emitters.push_back(emitter);
    return emitter;
}

void ParticleSystem::removeEmitter(size_t index) {
    if (index < m_emitters.size()) {
        m_emitters.erase(m_emitters.begin() + index);
    }
}

void ParticleSystem::removeEmitterByName(const std::string& name) {
    m_emitters.erase(std::remove_if(m_emitters.begin(), m_emitters.end(),
        [&](const std::shared_ptr<ParticleEmitter>& e) {
            return e && e->getName() == name;
        }), m_emitters.end());
}

void ParticleSystem::clear() {
    m_emitters.clear();
}

void ParticleSystem::destroy() {
    m_emitters.clear();
    if (m_quadVAO != 0) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO != 0) { glDeleteBuffers(1, &m_quadVBO); m_quadVBO = 0; }
    if (m_instanceVBO != 0) { glDeleteBuffers(1, &m_instanceVBO); m_instanceVBO = 0; }
    m_initialized = false;
}

} // namespace ASIS
