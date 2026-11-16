#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "Shader.h"
#include "Camera.h"

namespace ASIS {

enum class EmitterType {
    FIRE,       // Fuego ardiente ascendente con desvanecimiento a ceniza
    SPARKS,     // Chispas metálicas / eléctricas veloces con rebote y gravedad
    SMOKE,      // Columnas densas de humo que se expanden
    RAIN,       // Lluvia / tormenta cayendo desde el cielo
    MAGIC,      // Orbes místicos resplandecientes en espiral
    CUSTOM      // Emisor libre configurable por el usuario
};

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float size;
    float life;
    float maxLife;
    float rotation;
    float rotSpeed;
};

struct ParticleInstanceData {
    glm::vec3 position;
    float size;
    glm::vec4 color;
    float rotation;
};

class ParticleEmitter {
public:
    ParticleEmitter(const std::string& name, EmitterType type, const glm::vec3& position);
    ~ParticleEmitter() = default;

    void update(float dt);
    void emit(int count);
    void reset();

    // Getters y setters para la interfaz del editor
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    EmitterType getType() const { return m_type; }
    void setType(EmitterType type);

    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    float getEmissionRate() const { return m_emissionRate; }
    void setEmissionRate(float rate) { m_emissionRate = rate; }

    int getMaxParticles() const { return m_maxParticles; }
    void setMaxParticles(int maxP);

    float getLifetime() const { return m_lifetime; }
    void setLifetime(float lt) { m_lifetime = lt; }

    const glm::vec4& getStartColor() const { return m_startColor; }
    void setStartColor(const glm::vec4& col) { m_startColor = col; }

    const glm::vec4& getEndColor() const { return m_endColor; }
    void setEndColor(const glm::vec4& col) { m_endColor = col; }

    float getStartSize() const { return m_startSize; }
    void setStartSize(float size) { m_startSize = size; }

    float getEndSize() const { return m_endSize; }
    void setEndSize(float size) { m_endSize = size; }

    const glm::vec3& getVelocity() const { return m_velocity; }
    void setVelocity(const glm::vec3& v) { m_velocity = v; }

    const glm::vec3& getVelocitySpread() const { return m_velocitySpread; }
    void setVelocitySpread(const glm::vec3& vs) { m_velocitySpread = vs; }

    const glm::vec3& getGravity() const { return m_gravity; }
    void setGravity(const glm::vec3& g) { m_gravity = g; }

    bool isAdditive() const { return m_additiveBlend; }
    void setAdditive(bool add) { m_additiveBlend = add; }

    const std::vector<Particle>& getParticles() const { return m_particles; }
    size_t getAliveCount() const;

    void fillInstanceData(std::vector<ParticleInstanceData>& outData) const;

private:
    std::string m_name;
    EmitterType m_type;
    glm::vec3 m_position;
    bool m_active;

    float m_emissionRate;    // Partículas emitidas por segundo
    int m_maxParticles;
    float m_lifetime;

    glm::vec4 m_startColor;
    glm::vec4 m_endColor;
    float m_startSize;
    float m_endSize;

    glm::vec3 m_velocity;
    glm::vec3 m_velocitySpread;
    glm::vec3 m_gravity;
    bool m_additiveBlend;

    float m_accumulator;
    std::vector<Particle> m_particles;

    void applyPreset(EmitterType type);
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    bool init();
    void update(float dt);
    void draw(const Camera& camera, const glm::mat4& projection, const glm::mat4& view);
    void destroy();

    std::shared_ptr<ParticleEmitter> addEmitter(const std::string& name, EmitterType type, const glm::vec3& position);
    void removeEmitter(size_t index);
    void removeEmitterByName(const std::string& name);
    void clear();

    std::vector<std::shared_ptr<ParticleEmitter>>& getEmitters() { return m_emitters; }
    const std::vector<std::shared_ptr<ParticleEmitter>>& getEmitters() const { return m_emitters; }
    size_t getEmitterCount() const { return m_emitters.size(); }

    std::shared_ptr<ParticleEmitter> getEmitter(size_t index) {
        return (index < m_emitters.size()) ? m_emitters[index] : nullptr;
    }
    std::shared_ptr<ParticleEmitter> getEmitterByName(const std::string& name) {
        for (auto& em : m_emitters) {
            if (em && em->getName() == name) return em;
        }
        return nullptr;
    }

private:
    bool m_initialized;
    std::vector<std::shared_ptr<ParticleEmitter>> m_emitters;

    GLuint m_quadVAO;
    GLuint m_quadVBO;
    GLuint m_instanceVBO;
    std::unique_ptr<Shader> m_particleShader;

    std::vector<ParticleInstanceData> m_additiveInstances;
    std::vector<ParticleInstanceData> m_alphaInstances;

    void setupInstancedQuad();
};

} // namespace ASIS
