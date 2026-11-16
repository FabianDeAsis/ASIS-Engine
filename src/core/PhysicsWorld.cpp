#include "PhysicsWorld.h"
#include "Entity.h"
#include "ScriptEngine.h"
#include "Sequencer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

namespace ASIS {

static btVector3 toBt(const glm::vec3& v) {
    return btVector3(v.x, v.y, v.z);
}

static glm::vec3 toGlm(const btVector3& v) {
    return glm::vec3(v.x(), v.y(), v.z());
}

PhysicsWorld::PhysicsWorld()
    : m_collisionConfig(nullptr),
      m_dispatcher(nullptr),
      m_broadphase(nullptr),
      m_solver(nullptr),
      m_dynamicsWorld(nullptr),
      m_groundRigidBody(nullptr),
      m_groundShape(nullptr),
      m_groundMotionState(nullptr),
      m_groundPlaneEnabled(true),
      m_simulationEnabled(true)
{
}

PhysicsWorld::~PhysicsWorld() {
    shutdown();
}

void PhysicsWorld::init() {
    // 1. Configuración de detección de colisiones de Bullet
    m_collisionConfig = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfig);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();

    // 2. Crear mundo dinámico con gravedad estándar terrestre
    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfig);
    m_dynamicsWorld->setGravity(btVector3(0.0f, -9.81f, 0.0f));
    m_dynamicsWorld->getSolverInfo().m_numIterations = 20;
    m_dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;

    // 3. Crear suelo estático infinito en Y = 0
    createGroundPlane(100.0f);

    std::cout << "[ASIS Physics] Motor de Fisica Bullet 3D inicializado con gravedad (-9.81 m/s^2).\n";
}

void PhysicsWorld::createGroundPlane(float size) {
    // Suelo estático como caja gruesa cuya cara superior coincide exactamente con Y = 0.0f
    float halfThickness = 1.0f;
    m_groundShape = new btBoxShape(btVector3(size, halfThickness, size));

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0.0f, -halfThickness, 0.0f));

    m_groundMotionState = new btDefaultMotionState(groundTransform);

    // Masa = 0 hace que el cuerpo sea completamente estático e inamovible en Bullet
    btRigidBody::btRigidBodyConstructionInfo groundRbInfo(0.0f, m_groundMotionState, m_groundShape, btVector3(0, 0, 0));
    groundRbInfo.m_restitution = 0.5f; // Rebote moderado
    groundRbInfo.m_friction = 0.8f;    // Buena tracción superficial

    m_groundRigidBody = new btRigidBody(groundRbInfo);
    m_dynamicsWorld->addRigidBody(m_groundRigidBody);
    m_groundPlaneEnabled = true;
}

void PhysicsWorld::enableGroundPlane(bool enable) {
    if (!m_dynamicsWorld || !m_groundRigidBody) return;
    if (enable && !m_groundPlaneEnabled) {
        m_dynamicsWorld->addRigidBody(m_groundRigidBody);
        m_groundPlaneEnabled = true;
    } else if (!enable && m_groundPlaneEnabled) {
        m_dynamicsWorld->removeRigidBody(m_groundRigidBody);
        m_groundPlaneEnabled = false;
    }
}

void PhysicsWorld::addRigidBodyDirect(btRigidBody* body) {
    if (m_dynamicsWorld && body) {
        m_dynamicsWorld->addRigidBody(body);
    }
}

void PhysicsWorld::removeRigidBodyDirect(btRigidBody* body) {
    if (m_dynamicsWorld && body) {
        m_dynamicsWorld->removeRigidBody(body);
    }
}

bool PhysicsWorld::rayTest(const glm::vec3& from, const glm::vec3& to, glm::vec3& hitPoint, glm::vec3& hitNormal, const btRigidBody* ignoreBody) const {
    if (!m_dynamicsWorld) return false;

    btVector3 btFrom(from.x, from.y, from.z);
    btVector3 btTo(to.x, to.y, to.z);

    struct CustomRayCallback : public btCollisionWorld::ClosestRayResultCallback {
        const btRigidBody* m_ignore;
        CustomRayCallback(const btVector3& from, const btVector3& to, const btRigidBody* ignore)
            : ClosestRayResultCallback(from, to), m_ignore(ignore) {}

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override {
            if (rayResult.m_collisionObject == m_ignore) {
                return 1.0f;
            }
            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    CustomRayCallback rayCallback(btFrom, btTo, ignoreBody);
    m_dynamicsWorld->rayTest(btFrom, btTo, rayCallback);

    if (rayCallback.hasHit()) {
        hitPoint = glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());
        hitNormal = glm::vec3(rayCallback.m_hitNormalWorld.x(), rayCallback.m_hitNormalWorld.y(), rayCallback.m_hitNormalWorld.z());
        return true;
    }
    return false;
}

void PhysicsWorld::setGravity(const glm::vec3& gravity) {
    if (m_dynamicsWorld) {
        m_dynamicsWorld->setGravity(toBt(gravity));
    }
}

glm::vec3 PhysicsWorld::getGravity() const {
    if (m_dynamicsWorld) {
        return toGlm(m_dynamicsWorld->getGravity());
    }
    return glm::vec3(0.0f, -9.81f, 0.0f);
}

void PhysicsWorld::addRigidBody(std::shared_ptr<Entity> entity, float mass, ColliderType type) {
    if (!entity || !m_dynamicsWorld) return;

    std::string name = entity->getName();
    removeRigidBody(name); // Eliminar previo si ya existía

    glm::vec3 pos = entity->getPosition();
    glm::vec3 rot = entity->getRotation();
    glm::vec3 scale = entity->getScale();

    // 1. Determinar forma de colisión
    btCollisionShape* shape = nullptr;
    if (type == ColliderType::SPHERE) {
        float radius = std::max({scale.x, scale.y, scale.z}) * 0.5f;
        shape = new btSphereShape(radius);
    } else {
        // Caja por defecto
        shape = new btBoxShape(btVector3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f));
    }
    shape->setMargin(0.015f);

    // 2. Inercia local según masa
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, localInertia);
    }

    // 3. Transformación inicial
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(toBt(pos));

    btQuaternion quat;
    if (entity->usesQuaternion()) {
        const glm::quat& q = entity->getOrientation();
        quat = btQuaternion(q.x, q.y, q.z, q.w);
    } else {
        quat.setEulerZYX(glm::radians(rot.z), glm::radians(rot.y), glm::radians(rot.x));
    }
    startTransform.setRotation(quat);

    btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    rbInfo.m_restitution = 0.20f;
    rbInfo.m_friction = 0.85f;
    rbInfo.m_linearDamping = 0.05f;
    rbInfo.m_angularDamping = 0.15f;

    btRigidBody* body = new btRigidBody(rbInfo);

    // Evitar que los cuerpos entren en reposo absoluto inmediato mientras se manipulan
    body->setActivationState(DISABLE_DEACTIVATION);

    m_dynamicsWorld->addRigidBody(body);

    RigidBodyComponent comp;
    comp.body = body;
    comp.shape = shape;
    comp.motionState = motionState;
    comp.mass = mass;
    comp.restitution = 0.45f;
    comp.friction = 0.65f;
    comp.isKinematic = (mass == 0.0f);
    comp.colliderType = type;
    comp.isTrigger = entity->isTrigger();
    comp.triggerTag = entity->getTriggerTag();

    // Sincronizar propiedades físicas con el Entity para persistencia
    entity->setHasPhysics(true);
    entity->setMass(mass);
    entity->setColliderType(type == ColliderType::SPHERE ? "SPHERE" : (type == ColliderType::PLANE ? "PLANE" : "BOX"));
    entity->setFriction(comp.friction);
    entity->setRestitution(comp.restitution);
    entity->setKinematic(comp.isKinematic);

    m_bodies[name] = comp;
    m_bodyToName[body] = name;
    if (entity->isTrigger()) {
        setBodyTrigger(name, true);
    }
    std::cout << "[ASIS Physics] Cuerpo rigido anadido: '" << name << "' (Masa: " << mass << " kg" << (comp.isTrigger ? ", TRIGGER" : "") << ")\n";
}

void PhysicsWorld::removeRigidBody(const std::string& entityName) {
    auto it = m_bodies.find(entityName);
    if (it != m_bodies.end()) {
        if (it->second.body) {
            m_bodyToName.erase(it->second.body);
            if (m_dynamicsWorld) {
                m_dynamicsWorld->removeRigidBody(it->second.body);
            }
        }
        delete it->second.body;
        delete it->second.motionState;
        delete it->second.shape;
        m_bodies.erase(it);
    }
}

bool PhysicsWorld::hasRigidBody(const std::string& entityName) const {
    return m_bodies.find(entityName) != m_bodies.end();
}

RigidBodyComponent* PhysicsWorld::getRigidBodyComponent(const std::string& entityName) {
    auto it = m_bodies.find(entityName);
    if (it != m_bodies.end()) {
        return &it->second;
    }
    return nullptr;
}

void PhysicsWorld::setBodyTrigger(const std::string& entityName, bool isTrigger) {
    auto it = m_bodies.find(entityName);
    if (it != m_bodies.end() && it->second.body) {
        it->second.isTrigger = isTrigger;
        int flags = it->second.body->getCollisionFlags();
        if (isTrigger) {
            flags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
        } else {
            flags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;
        }
        it->second.body->setCollisionFlags(flags);
    }
}

bool PhysicsWorld::isBodyTrigger(const std::string& entityName) const {
    auto it = m_bodies.find(entityName);
    if (it != m_bodies.end()) {
        return it->second.isTrigger;
    }
    return false;
}

void PhysicsWorld::registerCustomBodyName(const btCollisionObject* body, const std::string& name) {
    if (body) {
        m_bodyToName[body] = name;
    }
}

void PhysicsWorld::unregisterCustomBodyName(const btCollisionObject* body) {
    if (body) {
        m_bodyToName.erase(body);
    }
}

void PhysicsWorld::checkTriggerEvents(ScriptEngine* scriptEngine) {
    if (!scriptEngine || !m_dispatcher) return;

    std::set<std::pair<std::string, std::string>> currentOverlaps;
    int numManifolds = m_dispatcher->getNumManifolds();

    for (int i = 0; i < numManifolds; ++i) {
        btPersistentManifold* contactManifold = m_dispatcher->getManifoldByIndexInternal(i);
        if (!contactManifold || contactManifold->getNumContacts() == 0) continue;

        const btCollisionObject* obA = contactManifold->getBody0();
        const btCollisionObject* obB = contactManifold->getBody1();

        auto itA = m_bodyToName.find(obA);
        auto itB = m_bodyToName.find(obB);

        if (itA == m_bodyToName.end() || itB == m_bodyToName.end()) continue;

        const std::string& nameA = itA->second;
        const std::string& nameB = itB->second;

        bool aTrigger = isBodyTrigger(nameA);
        bool bTrigger = isBodyTrigger(nameB);

        auto itCompA = m_bodies.find(nameA);
        auto itCompB = m_bodies.find(nameB);
        bool aSpecial = aTrigger || (nameA.find("Barril") != std::string::npos || nameA.find("Portal") != std::string::npos || nameA.find("Moneda") != std::string::npos) || (itCompA != m_bodies.end() && (itCompA->second.triggerTag == "BarrilExplosivo" || itCompA->second.triggerTag == "Portal"));
        bool bSpecial = bTrigger || (nameB.find("Barril") != std::string::npos || nameB.find("Portal") != std::string::npos || nameB.find("Moneda") != std::string::npos) || (itCompB != m_bodies.end() && (itCompB->second.triggerTag == "BarrilExplosivo" || itCompB->second.triggerTag == "Portal"));

        if (aSpecial) {
            currentOverlaps.insert({nameA, nameB});
        }
        if (bSpecial) {
            currentOverlaps.insert({nameB, nameA});
        }
    }

    // Disparar onTriggerEnter para nuevos solapamientos
    for (const auto& pair : currentOverlaps) {
        if (m_activeTriggerOverlaps.find(pair) == m_activeTriggerOverlaps.end()) {
            scriptEngine->dispatchTriggerEnter(pair.first, pair.second);
        }
    }

    // Disparar onTriggerExit para solapamientos finalizados
    for (const auto& pair : m_activeTriggerOverlaps) {
        if (currentOverlaps.find(pair) == currentOverlaps.end()) {
            scriptEngine->dispatchTriggerExit(pair.first, pair.second);
        }
    }

    m_activeTriggerOverlaps = std::move(currentOverlaps);
}

void PhysicsWorld::update(float deltaTime) {
    if (!m_dynamicsWorld || !m_simulationEnabled) return;

    // Sub-stepping fijo a 60 Hz para estabilidad física profesional
    m_dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f / 60.0f);
}

void PhysicsWorld::syncPhysicsToEntities(std::unordered_map<std::string, std::shared_ptr<Entity>>& entities) {
    if (!m_simulationEnabled) return;

    for (auto& [name, comp] : m_bodies) {
        if (!comp.body || comp.mass <= 0.0f) continue; // Los estáticos no se mueven

        auto it = entities.find(name);
        if (it != entities.end() && it->second) {
            // Si la entidad está siendo animada por el Sequencer en reproducción,
            // sincronizar la física HACIA la entidad y no sobrescribir la cinemática
            if (it->second->isControlledBySequencer() && ASIS::Sequencer::get().isPlaying()) {
                syncEntityToPhysics(it->second);
                continue;
            }

            btTransform trans;
            if (comp.body->getMotionState()) {
                comp.body->getMotionState()->getWorldTransform(trans);
            } else {
                trans = comp.body->getWorldTransform();
            }

            // Actualizar Posición
            btVector3 origin = trans.getOrigin();
            it->second->setPosition(origin.x(), origin.y(), origin.z());

            // Actualizar Rotación EXACTA por Cuaternión sin distorsión Euler ni inversión de ejes
            btQuaternion bq = trans.getRotation();
            glm::quat gq(bq.w(), bq.x(), bq.y(), bq.z());
            it->second->setOrientation(gq);
        }
    }
}

void PhysicsWorld::syncEntityToPhysics(std::shared_ptr<Entity> entity) {
    if (!entity) return;

    auto it = m_bodies.find(entity->getName());
    if (it != m_bodies.end() && it->second.body) {
        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(toBt(entity->getPosition()));

        btQuaternion quat;
        if (entity->usesQuaternion()) {
            const glm::quat& q = entity->getOrientation();
            quat = btQuaternion(q.x, q.y, q.z, q.w);
        } else {
            glm::vec3 rot = entity->getRotation();
            quat.setEulerZYX(glm::radians(rot.z), glm::radians(rot.y), glm::radians(rot.x));
        }
        trans.setRotation(quat);

        it->second.body->setWorldTransform(trans);
        if (it->second.motionState) {
            it->second.motionState->setWorldTransform(trans);
        }

        // Anular velocidades para que no salga disparado con impulso residual
        it->second.body->setLinearVelocity(btVector3(0, 0, 0));
        it->second.body->setAngularVelocity(btVector3(0, 0, 0));
        it->second.body->clearForces();
        it->second.body->activate(true);

        m_dynamicsWorld->updateSingleAabb(it->second.body);
    }
}

void PhysicsWorld::applyImpulse(const std::string& entityName, const glm::vec3& impulse) {
    auto it = m_bodies.find(entityName);
    if (it != m_bodies.end() && it->second.body) {
        it->second.body->activate(true);
        it->second.body->applyCentralImpulse(toBt(impulse));
    }
}

void PhysicsWorld::resetVelocity(const std::string& entityName) {
    auto it = m_bodies.find(entityName);
    if (it != m_bodies.end() && it->second.body) {
        it->second.body->setLinearVelocity(btVector3(0, 0, 0));
        it->second.body->setAngularVelocity(btVector3(0, 0, 0));
        it->second.body->clearForces();
    }
}

void PhysicsWorld::clear() {
    for (auto& [name, comp] : m_bodies) {
        if (m_dynamicsWorld && comp.body) {
            m_dynamicsWorld->removeRigidBody(comp.body);
        }
        delete comp.body;
        delete comp.motionState;
        delete comp.shape;
    }
    m_bodies.clear();
    m_bodyToName.clear();
    m_activeTriggerOverlaps.clear();
}

void PhysicsWorld::shutdown() {
    for (auto& [name, comp] : m_bodies) {
        if (m_dynamicsWorld && comp.body) {
            m_dynamicsWorld->removeRigidBody(comp.body);
        }
        delete comp.body;
        delete comp.motionState;
        delete comp.shape;
    }
    m_bodies.clear();
    m_bodyToName.clear();
    m_activeTriggerOverlaps.clear();

    if (m_groundRigidBody) {
        m_dynamicsWorld->removeRigidBody(m_groundRigidBody);
        delete m_groundRigidBody;
        delete m_groundMotionState;
        delete m_groundShape;
        m_groundRigidBody = nullptr;
    }

    delete m_dynamicsWorld;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_collisionConfig;

    m_dynamicsWorld = nullptr;
    m_solver = nullptr;
    m_broadphase = nullptr;
    m_dispatcher = nullptr;
    m_collisionConfig = nullptr;

    std::cout << "[ASIS Physics] Recursos de Bullet Physics liberados.\n";
}

void PhysicsWorld::applyBuoyancy(const std::function<float(float, float)>& getWaterHeight, float deltaTime) {
    if (!m_simulationEnabled || !m_dynamicsWorld || !getWaterHeight) return;

    for (auto& [name, comp] : m_bodies) {
        if (!comp.body || comp.isKinematic || comp.isTrigger || comp.mass <= 0.0001f) continue;

        btTransform trans;
        if (comp.body->getMotionState()) {
            comp.body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = comp.body->getWorldTransform();
        }

        btVector3 origin = trans.getOrigin();
        float waterY = getWaterHeight(origin.x(), origin.z());

        if (origin.y() < waterY) {
            float depth = waterY - origin.y();
            float gravity = std::abs(m_dynamicsWorld->getGravity().y());
            if (gravity < 1.0f) gravity = 9.81f;

            // Principio de Arquímedes: Fuerza de empuje vertical proporcional a masa y profundidad sumergida
            float lift = comp.mass * gravity * std::clamp(1.0f + depth * 2.0f, 1.0f, 4.0f);

            comp.body->activate(true);
            comp.body->applyCentralForce(btVector3(0.0f, lift, 0.0f));

            // Resistencia y amortiguamiento hidrodinámico
            btVector3 vel = comp.body->getLinearVelocity();
            comp.body->setLinearVelocity(vel * (1.0f - std::min(deltaTime * 2.2f, 0.6f)));

            btVector3 angVel = comp.body->getAngularVelocity();
            comp.body->setAngularVelocity(angVel * (1.0f - std::min(deltaTime * 1.8f, 0.5f)));
        }
    }
}

void PhysicsWorld::applyExplosionImpulse(const glm::vec3& center, float radius, float force) {
    if (!m_dynamicsWorld) return;

    for (auto& [name, comp] : m_bodies) {
        if (!comp.body || comp.mass <= 0.0f || comp.isKinematic || comp.isTrigger) continue;

        btTransform trans;
        if (comp.body->getMotionState()) {
            comp.body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = comp.body->getWorldTransform();
        }

        btVector3 origin = trans.getOrigin();
        glm::vec3 pos(origin.x(), origin.y(), origin.z());
        float dist = glm::length(pos - center);

        if (dist > 0.001f && dist < radius) {
            glm::vec3 dir = glm::normalize(pos - center);
            float falloff = 1.0f - (dist / radius);
            glm::vec3 impulse = (dir * 0.75f + glm::vec3(0.0f, 0.65f, 0.0f)) * (force * falloff * std::max(comp.mass, 1.0f));

            comp.body->activate(true);
            comp.body->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
        }
    }
}

} // namespace ASIS
