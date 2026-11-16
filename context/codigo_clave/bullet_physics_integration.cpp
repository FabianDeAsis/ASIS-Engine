// ========================================================================
// ASIS Engine - Integración de Bullet Physics 3 (Cuerpo Rígido y Sincronización)
// ========================================================================

#include "PhysicsWorld.h"
#include <btBulletDynamicsCommon.h>
#include <glm/gtc/matrix_transform.hpp>

namespace ASIS {

// Paso de simulación temporal a 60 Hz con sub-stepping
void PhysicsWorld::step(float deltaTime) {
    if (!m_simulationEnabled || !m_dynamicsWorld) return;
    
    // Sub-stepping de hasta 10 pasos fijos a 1/60s para máxima estabilidad
    m_dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f / 60.0f);
}

// Sincronización continua: Bullet Physics -> Entidades de la Escena
void PhysicsWorld::syncPhysicsToEntities(std::vector<std::shared_ptr<Entity>>& entities) {
    for (auto& entity : entities) {
        auto it = m_rigidBodies.find(entity->getName());
        if (it != m_rigidBodies.end() && it->second.body) {
            btRigidBody* body = it->second.body.get();
            
            // Si el cuerpo está activo y no es estático (masa > 0)
            if (body->getInvMass() > 0.0f && body->isActive()) {
                btTransform trans;
                body->getMotionState()->getWorldTransform(trans);

                // Posición
                btVector3 origin = trans.getOrigin();
                entity->setPosition(glm::vec3(origin.x(), origin.y(), origin.z()));

                // Rotación (Euler YPR a grados)
                btScalar yaw, pitch, roll;
                trans.getBasis().getEulerYPR(yaw, pitch, roll);
                entity->setRotation(glm::vec3(glm::degrees(pitch), glm::degrees(yaw), glm::degrees(roll)));
            }
        }
    }
}

// Sincronización inversa: Gizmo/Editor UI -> Bullet Physics
void PhysicsWorld::syncEntityToPhysics(const Entity& entity) {
    auto it = m_rigidBodies.find(entity.getName());
    if (it != m_rigidBodies.end() && it->second.body) {
        btRigidBody* body = it->second.body.get();

        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(entity.getPosition().x, entity.getPosition().y, entity.getPosition().z));

        // Rotación desde ángulos de Euler
        glm::vec3 rot = entity.getRotation();
        btQuaternion quat;
        quat.setEulerZYX(glm::radians(rot.y), glm::radians(rot.x), glm::radians(rot.z));
        trans.setRotation(quat);

        body->setWorldTransform(trans);
        body->getMotionState()->setWorldTransform(trans);

        // Limpiar velocidades residuales y reactivar cuerpo
        body->setLinearVelocity(btVector3(0, 0, 0));
        body->setAngularVelocity(btVector3(0, 0, 0));
        body->activate(true);
    }
}

} // namespace ASIS
