#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <set>
#include <string>
#include <functional>

namespace ASIS {

class Entity;
class ScriptEngine;

enum class ColliderType {
    BOX,
    SPHERE,
    PLANE
};

struct RigidBodyComponent {
    btRigidBody* body = nullptr;
    btCollisionShape* shape = nullptr;
    btDefaultMotionState* motionState = nullptr;
    float mass = 1.0f;
    float restitution = 0.4f; // Rebote (0.0 = sin rebote, 1.0 = rebote elástico)
    float friction = 0.6f;    // Fricción superficial
    bool isKinematic = false;
    ColliderType colliderType = ColliderType::BOX;
    bool isTrigger = false;
    std::string triggerTag = "Default";
};

/**
 * @brief Gestor de Simulación de Física 3D basado en Bullet Physics
 * 
 * Administra el mundo dinámico de cuerpos rígidos, gravedad, colisiones,
 * fricción, restitución y sincronización bidireccional con las entidades de la escena.
 */
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    // Inicializar el mundo físico con gravedad y suelo estático
    void init();

    // Avanzar la simulación física (sub-stepping)
    void update(float deltaTime);

    // Liberar todos los recursos de Bullet Physics
    void shutdown();
    void clear();

    // Control de Gravedad
    void setGravity(const glm::vec3& gravity);
    glm::vec3 getGravity() const;

    // Control de Simulación (Play / Pause)
    bool isSimulationEnabled() const { return m_simulationEnabled; }
    void setSimulationEnabled(bool enabled) { m_simulationEnabled = enabled; }

    // Gestión de Cuerpos Rígidos por Entidad
    void addRigidBody(std::shared_ptr<Entity> entity, float mass = 1.0f, ColliderType type = ColliderType::BOX);
    void removeRigidBody(const std::string& entityName);
    bool hasRigidBody(const std::string& entityName) const;
    RigidBodyComponent* getRigidBodyComponent(const std::string& entityName);

    // Sincronización entre Bullet Physics y las entidades de Scene
    void syncPhysicsToEntities(std::unordered_map<std::string, std::shared_ptr<Entity>>& entities);
    void syncEntityToPhysics(std::shared_ptr<Entity> entity); // Llamado cuando se mueve con Gizmo

    // Aplicar fuerzas e impulsos
    void applyImpulse(const std::string& entityName, const glm::vec3& impulse);
    void resetVelocity(const std::string& entityName);

    // Suelo físico estático en Y = 0 (coincide con el plano y rejilla visual)
    void createGroundPlane(float size = 80.0f);
    void enableGroundPlane(bool enable);
    bool isGroundPlaneEnabled() const { return m_groundPlaneEnabled; }

    // Acceso al mundo dinámico de Bullet para formas personalizadas (Terreno)
    btDiscreteDynamicsWorld* getDynamicsWorld() { return m_dynamicsWorld; }
    void addRigidBodyDirect(btRigidBody* body);
    void removeRigidBodyDirect(btRigidBody* body);

    // Trazado de rayos físicos en el mundo (Ground check, spring arm de cámara)
    bool rayTest(const glm::vec3& from, const glm::vec3& to, glm::vec3& hitPoint, glm::vec3& hitNormal, const btRigidBody* ignoreBody = nullptr) const;

    size_t getBodyCount() const { return m_bodies.size(); }

    // Zonas de Trigger / Disparadores de Eventos (Fase 17)
    void setBodyTrigger(const std::string& entityName, bool isTrigger);
    bool isBodyTrigger(const std::string& entityName) const;
    void registerCustomBodyName(const btCollisionObject* body, const std::string& name);
    void unregisterCustomBodyName(const btCollisionObject* body);
    void checkTriggerEvents(ScriptEngine* scriptEngine);

    // Simulación de Flotabilidad Hidrodinámica (Principio de Arquímedes en Océano 3D - Fase 19)
    void applyBuoyancy(const std::function<float(float, float)>& getWaterHeight, float deltaTime);

    // Onda de choque / Impulso explosivo para barriles y granadas (Fase 17 / 20)
    void applyExplosionImpulse(const glm::vec3& center, float radius, float force);

private:
    btDefaultCollisionConfiguration* m_collisionConfig;
    btCollisionDispatcher* m_dispatcher;
    btBroadphaseInterface* m_broadphase;
    btSequentialImpulseConstraintSolver* m_solver;
    btDiscreteDynamicsWorld* m_dynamicsWorld;

    // Suelo estático
    btRigidBody* m_groundRigidBody;
    btCollisionShape* m_groundShape;
    btDefaultMotionState* m_groundMotionState;
    bool m_groundPlaneEnabled;

    // Mapa de cuerpos rígidos asociados a nombres de entidades
    std::unordered_map<std::string, RigidBodyComponent> m_bodies;
    std::unordered_map<const btCollisionObject*, std::string> m_bodyToName;
    std::set<std::pair<std::string, std::string>> m_activeTriggerOverlaps;
    bool m_simulationEnabled;
};

} // namespace ASIS
