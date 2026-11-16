#include "CharacterController.h"
#include "PhysicsWorld.h"
#include "Camera.h"
#include "Shader.h"
#include "AudioEngine.h"
#include "Terrain.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ASIS {

static inline btVector3 toBt(const glm::vec3& v) {
    return btVector3(v.x, v.y, v.z);
}

static inline glm::vec3 toGlm(const btVector3& v) {
    return glm::vec3(v.x(), v.y(), v.z());
}

static FootstepSurface detectPlayerSurface(const glm::vec3& playerPos, const Terrain* terrain) {
    if (terrain && terrain->isVisible()) {
        float terrainH = terrain->getHeightAtWorld(playerPos.x, playerPos.z);
        // Si el jugador está cerca del suelo del terreno, muestrear bioma
        if (std::abs(playerPos.y - terrainH) < 1.6f) {
            glm::vec3 norm = terrain->getNormalAtWorld(playerPos.x, playerPos.z);
            if (norm.y < 0.72f) return FootstepSurface::ROCK;
            float relativeH = terrainH - terrain->getBaseOffset();
            if (relativeH > 14.0f) return FootstepSurface::SNOW;
            if (relativeH > 5.5f) return FootstepSurface::DIRT;
            return FootstepSurface::GRASS;
        }
    }
    return FootstepSurface::CONCRETE;
}

CharacterController::CharacterController()
    : m_isSpawned(false),
      m_cameraMode(CharacterCameraMode::FIRST_PERSON),
      m_position(0.0f, 2.0f, 0.0f),
      m_velocity(0.0f),
      m_yaw(-90.0f),
      m_pitch(0.0f),
      m_mass(75.0f),
      m_capsuleRadius(0.38f),
      m_capsuleHeight(1.80f),
      m_eyeHeight(1.65f),
      m_standingEyeHeight(1.65f),
      m_crouchEyeHeight(0.95f),
      m_currentEyeHeight(1.65f),
      m_shoulderHeight(1.40f),
      m_thirdPersonDistance(3.5f),
      m_shoulderOffset(0.40f),
      m_walkSpeed(5.5f),
      m_sprintSpeed(10.5f),
      m_crouchSpeed(2.6f),
      m_jumpForce(8.2f),
      m_mouseSensitivity(0.12f),
      m_isGrounded(false),
      m_wasGrounded(false),
      m_isSprinting(false),
      m_isCrouching(false),
      m_headBobTimer(0.0f),
      m_enableHeadBob(true),
      m_stepTimer(0.0f),
      m_capsuleShape(nullptr),
      m_rigidBody(nullptr),
      m_motionState(nullptr)
{
}

CharacterController::~CharacterController() {
    shutdown();
}

bool CharacterController::init() {
    // Crear piezas geométricas del Avatar 3D (para modo 3ra persona)
    m_torsoMesh = Mesh::createCylinder(0.32f, 0.85f, 24);
    m_headMesh  = Mesh::createSphere(0.22f, 20, 20);
    m_visorMesh = Mesh::createSphere(0.15f, 16, 16);
    m_packMesh  = Mesh::createCube();

    std::cout << "[ASIS Character] Controlador de Personaje 3D inicializado con éxito.\n";
    return true;
}

void CharacterController::shutdown() {
    m_torsoMesh.reset();
    m_headMesh.reset();
    m_visorMesh.reset();
    m_packMesh.reset();
}

void CharacterController::spawn(PhysicsWorld& physicsWorld, const glm::vec3& startPosition, float startYaw) {
    if (m_isSpawned) {
        despawn(physicsWorld);
    }

    m_position = startPosition;
    m_yaw = startYaw;
    m_pitch = 0.0f;
    m_velocity = glm::vec3(0.0f);

    // En Bullet, la altura de btCapsuleShape es la distancia entre los centros de las esferas
    float cylinderHeight = m_capsuleHeight - 2.0f * m_capsuleRadius;
    if (cylinderHeight < 0.1f) cylinderHeight = 0.1f;

    m_capsuleShape = new btCapsuleShape(m_capsuleRadius, cylinderHeight);

    btTransform startTransform;
    startTransform.setIdentity();
    // Posición del centro de masa de la cápsula
    startTransform.setOrigin(btVector3(startPosition.x, startPosition.y + m_capsuleHeight * 0.5f, startPosition.z));

    m_motionState = new btDefaultMotionState(startTransform);

    btVector3 localInertia(0, 0, 0);
    m_capsuleShape->calculateLocalInertia(m_mass, localInertia);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(m_mass, m_motionState, m_capsuleShape, localInertia);
    rbInfo.m_friction = 0.85f;
    rbInfo.m_restitution = 0.0f; // Sin rebote al aterrizar

    m_rigidBody = new btRigidBody(rbInfo);

    // Congelar rotaciones en X, Y, Z para que el personaje se mantenga estrictamente erguido
    m_rigidBody->setAngularFactor(btVector3(0.0f, 0.0f, 0.0f));

    // Evitar que Bullet desactive el cuerpo cuando el jugador se queda quieto
    m_rigidBody->setActivationState(DISABLE_DEACTIVATION);

    physicsWorld.addRigidBodyDirect(m_rigidBody);
    physicsWorld.registerCustomBodyName(m_rigidBody, "Player");
    m_isSpawned = true;

    std::cout << "[ASIS Character] Personaje aparecido en (" 
              << m_position.x << ", " << m_position.y << ", " << m_position.z << ")\n";
}

void CharacterController::despawn(PhysicsWorld& physicsWorld) {
    if (!m_isSpawned) return;

    if (m_rigidBody) {
        physicsWorld.unregisterCustomBodyName(m_rigidBody);
        physicsWorld.removeRigidBodyDirect(m_rigidBody);
        delete m_rigidBody;
        m_rigidBody = nullptr;
    }
    if (m_motionState) {
        delete m_motionState;
        m_motionState = nullptr;
    }
    if (m_capsuleShape) {
        delete m_capsuleShape;
        m_capsuleShape = nullptr;
    }

    m_isSpawned = false;
    std::cout << "[ASIS Character] Personaje retirado del mundo físico.\n";
}

void CharacterController::update(PhysicsWorld& physicsWorld, GLFWwindow* window, float deltaTime, const Terrain* terrain) {
    if (!m_isSpawned || !m_rigidBody) return;

    // 1. Sincronizar posición actual del cuerpo rígido de Bullet
    btTransform trans;
    m_rigidBody->getMotionState()->getWorldTransform(trans);
    btVector3 origin = trans.getOrigin();

    // Posición a nivel de los pies del personaje
    m_position = glm::vec3(origin.x(), origin.y() - m_capsuleHeight * 0.5f, origin.z());
    m_velocity = toGlm(m_rigidBody->getLinearVelocity());

    // 2. Detección de suelo (Ground Check) mediante Raycast hacia abajo
    glm::vec3 rayStart = m_position + glm::vec3(0.0f, m_capsuleRadius + 0.10f, 0.0f);
    glm::vec3 rayEnd   = m_position - glm::vec3(0.0f, 0.22f, 0.0f);

    glm::vec3 groundHitPos, groundHitNorm;
    m_isGrounded = physicsWorld.rayTest(rayStart, rayEnd, groundHitPos, groundHitNorm, m_rigidBody);

    // Detección de impacto de aterrizaje al tocar suelo desde el aire
    if (!m_wasGrounded && m_isGrounded) {
        FootstepSurface surf = detectPlayerSurface(m_position, terrain);
        float landVol = std::clamp(std::abs(m_velocity.y) * 0.15f, 0.35f, 1.0f);
        AudioEngine::get().playLand(surf, landVol);
    }
    m_wasGrounded = m_isGrounded;

    // 3. Procesar entradas de movimiento horizontal (WASD)
    float yawRad = glm::radians(m_yaw);
    glm::vec3 forward(std::cos(yawRad), 0.0f, std::sin(yawRad));
    forward = glm::normalize(forward);
    glm::vec3 right(-forward.z, 0.0f, forward.x);

    glm::vec3 moveDir(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += right;

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir);
    }

    // 4. Modificadores de velocidad (Sprint con Shift, Agacharse con C o Ctrl - Fase 16)
    m_isSprinting = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    m_isCrouching = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);

    // Transición suave de altura visual de los ojos al agacharse
    float targetEye = m_isCrouching ? m_crouchEyeHeight : m_standingEyeHeight;
    m_currentEyeHeight = glm::mix(m_currentEyeHeight, targetEye, glm::clamp(deltaTime * 12.0f, 0.0f, 1.0f));

    // Escalar la forma de colisión de la cápsula física para agacharse bajo obstáculos
    if (m_capsuleShape) {
        float targetScaleY = m_isCrouching ? 0.65f : 1.0f;
        m_capsuleShape->setLocalScaling(btVector3(1.0f, targetScaleY, 1.0f));
    }

    float currentTargetSpeed = m_walkSpeed;
    if (m_isCrouching) {
        currentTargetSpeed = m_crouchSpeed;
    } else if (m_isSprinting && glm::dot(moveDir, forward) > 0.3f) {
        currentTargetSpeed = m_sprintSpeed;
    }

    glm::vec3 desiredHorizVel = moveDir * currentTargetSpeed;

    // 5. Salto físico con gravedad reactiva (caída rápida sin flotabilidad estilo globo)
    float currentVy = m_velocity.y;
    static bool spaceWasPressed = false;
    bool spaceIsPressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

    if (spaceIsPressed && !spaceWasPressed && m_isGrounded) {
        currentVy = m_jumpForce;
        AudioEngine::get().playJump();
    }
    spaceWasPressed = spaceIsPressed;

    // Gravedad reactiva profesional: cae más rápido que al subir para un control ágil y preciso
    if (!m_isGrounded) {
        if (currentVy < 0.0f) {
            // Caída acelerada (FPS AAA / Unreal): duplica la atracción al descender
            currentVy -= 18.0f * deltaTime;
        } else if (!spaceIsPressed) {
            // Salto dinámico: cortar impulso si el jugador suelta el espacio antes
            currentVy -= 10.0f * deltaTime;
        }
    }

    // 6. Aceleración y amortiguamiento suave horizontal
    float accelRate = (glm::length(moveDir) > 0.01f) ? (m_isGrounded ? 18.0f : 6.0f) : (m_isGrounded ? 22.0f : 2.0f);
    float lerpT = glm::clamp(accelRate * deltaTime, 0.0f, 1.0f);

    float newVx = glm::mix(m_velocity.x, desiredHorizVel.x, lerpT);
    float newVz = glm::mix(m_velocity.z, desiredHorizVel.z, lerpT);

    m_rigidBody->setLinearVelocity(btVector3(newVx, currentVy, newVz));

    // 7. Cadencia rítmica de pisadas adaptativas según superficie de terreno (Fase 16)
    float horizSpeed = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
    if (m_isGrounded && horizSpeed > 0.3f) {
        float stepInterval = m_isSprinting ? 0.30f : (m_isCrouching ? 0.65f : 0.46f);
        m_stepTimer += deltaTime;
        if (m_stepTimer >= stepInterval) {
            m_stepTimer = 0.0f;
            FootstepSurface surf = detectPlayerSurface(m_position, terrain);
            float footVol = m_isCrouching ? 0.35f : (m_isSprinting ? 0.95f : 0.65f);
            AudioEngine::get().playFootstep(surf, footVol);
        }

        // 8. Bamboleo de cabeza (Head Bobbing) independiente de FPS
        if (m_enableHeadBob) {
            m_headBobTimer += deltaTime * (m_isSprinting ? 15.0f : (m_isCrouching ? 7.0f : 10.5f));
        }
    } else {
        m_stepTimer = 0.0f;
    }
}

void CharacterController::processMouseLook(float xoffset, float yoffset) {
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw   += xoffset;
    m_pitch += yoffset;

    // Limitar inclinación vertical para evitar voltear la cabeza
    m_pitch = glm::clamp(m_pitch, -85.0f, 85.0f);
}

void CharacterController::toggleCameraMode() {
    if (m_cameraMode == CharacterCameraMode::FIRST_PERSON) {
        m_cameraMode = CharacterCameraMode::THIRD_PERSON;
    } else {
        m_cameraMode = CharacterCameraMode::FIRST_PERSON;
    }
}

void CharacterController::updateCamera(Camera& camera, const PhysicsWorld& physicsWorld) {
    if (!m_isSpawned) return;

    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    glm::vec3 front(
        std::cos(yawRad) * std::cos(pitchRad),
        std::sin(pitchRad),
        std::sin(yawRad) * std::cos(pitchRad)
    );
    front = glm::normalize(front);
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

    if (m_cameraMode == CharacterCameraMode::FIRST_PERSON) {
        // En primera persona, cámara a la altura dinámica de los ojos con bamboleo rítmico (Head Bobbing)
        float horizSpeed = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
        float bobY = 0.0f;
        float bobX = 0.0f;
        if (m_enableHeadBob && m_isGrounded && horizSpeed > 0.3f) {
            bobY = std::sin(m_headBobTimer) * (m_isSprinting ? 0.045f : (m_isCrouching ? 0.012f : 0.024f));
            bobX = std::cos(m_headBobTimer * 0.5f) * (m_isSprinting ? 0.020f : (m_isCrouching ? 0.007f : 0.012f));
        }

        glm::vec3 eyePos = m_position + glm::vec3(0.0f, m_currentEyeHeight, 0.0f) + glm::vec3(0.0f, bobY, 0.0f) + right * bobX;
        camera.setPosition(eyePos);
        camera.setYawPitch(m_yaw, m_pitch);
    } else {
        // En tercera persona, usamos un Spring-Arm para evitar que la cámara penetre muros o montañas
        glm::vec3 pivot = m_position + glm::vec3(0.0f, m_shoulderHeight, 0.0f);
        glm::vec3 armDir = -front;

        glm::vec3 targetCamPos = pivot + armDir * m_thirdPersonDistance + right * m_shoulderOffset;

        // Comprobación de oclusión de cámara mediante raycast
        glm::vec3 hitPoint, hitNormal;
        float actualDist = m_thirdPersonDistance;
        if (physicsWorld.rayTest(pivot, pivot + armDir * m_thirdPersonDistance, hitPoint, hitNormal, m_rigidBody)) {
            actualDist = glm::length(hitPoint - pivot) - 0.28f;
            actualDist = (std::max)(0.45f, actualDist);
        }

        glm::vec3 camPos = pivot + armDir * actualDist + right * (m_shoulderOffset * (actualDist / m_thirdPersonDistance));
        camera.setPosition(camPos);
        camera.setYawPitch(m_yaw, m_pitch);
    }
}

void CharacterController::drawAvatar(Shader& shader) {
    // El avatar solo se dibuja en tercera persona para que en primera persona no tape la visión
    if (!m_isSpawned || m_cameraMode == CharacterCameraMode::FIRST_PERSON) return;

    shader.use();

    // Rotación del avatar para encarar la dirección horizontal de la cámara
    glm::mat4 baseMat = glm::translate(glm::mat4(1.0f), m_position);
    baseMat = glm::rotate(baseMat, glm::radians(-m_yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Propiedades PBR comunes
    shader.setInt("u_Material.useAlbedoMap", 0);
    shader.setInt("u_Material.useNormalMap", 0);
    shader.setInt("u_Material.useMetallicMap", 0);
    shader.setInt("u_Material.useRoughnessMap", 0);
    shader.setInt("u_Material.useSpecularMap", 0);
    shader.setInt("u_Material.useEmissionMap", 0);
    shader.setInt("u_Material.useAOMap", 0);
    shader.setFloat("u_Material.normalStrength", 1.0f);
    shader.setFloat("u_Material.ao", 1.0f);

    // 1. Torso / Armadura Cyber (Azul pizarra oscuro metálico)
    {
        glm::mat4 torsoMat = glm::translate(baseMat, glm::vec3(0.0f, 0.85f, 0.0f));
        torsoMat = glm::scale(torsoMat, glm::vec3(1.0f, 1.0f, 0.75f));
        shader.setMat4("u_Model", torsoMat);
        shader.setVec3("u_EntityColor", glm::vec3(0.18f, 0.24f, 0.35f));
        shader.setVec3("u_Material.albedo", glm::vec3(0.18f, 0.24f, 0.35f));
        shader.setFloat("u_Material.metallic", 0.40f);
        shader.setFloat("u_Material.roughness", 0.45f);
        shader.setFloat("u_Material.specular", 0.60f);
        shader.setVec3("u_Material.emission", glm::vec3(0.0f));
        shader.setFloat("u_Material.emissionStrength", 0.0f);
        if (m_torsoMesh) m_torsoMesh->draw(&shader);
    }

    // 2. Cabeza / Casco (Blanco perla aeroespacial)
    {
        glm::mat4 headMat = glm::translate(baseMat, glm::vec3(0.0f, 1.55f, 0.0f));
        headMat = glm::scale(headMat, glm::vec3(1.0f, 1.08f, 1.0f));
        shader.setMat4("u_Model", headMat);
        shader.setVec3("u_EntityColor", glm::vec3(0.90f, 0.92f, 0.96f));
        shader.setVec3("u_Material.albedo", glm::vec3(0.90f, 0.92f, 0.96f));
        shader.setFloat("u_Material.metallic", 0.15f);
        shader.setFloat("u_Material.roughness", 0.25f);
        shader.setFloat("u_Material.specular", 0.80f);
        shader.setVec3("u_Material.emission", glm::vec3(0.0f));
        shader.setFloat("u_Material.emissionStrength", 0.0f);
        if (m_headMesh) m_headMesh->draw(&shader);
    }

    // 3. Visor Holográfico Incandescente (Cian Neón con Bloom HDR)
    {
        glm::mat4 visorMat = glm::translate(baseMat, glm::vec3(0.0f, 1.56f, 0.15f));
        visorMat = glm::scale(visorMat, glm::vec3(0.85f, 0.45f, 0.60f));
        shader.setMat4("u_Model", visorMat);
        shader.setVec3("u_EntityColor", glm::vec3(0.05f, 0.85f, 1.0f));
        shader.setVec3("u_Material.albedo", glm::vec3(0.05f, 0.85f, 1.0f));
        shader.setFloat("u_Material.metallic", 0.10f);
        shader.setFloat("u_Material.roughness", 0.15f);
        shader.setFloat("u_Material.specular", 0.90f);
        shader.setVec3("u_Material.emission", glm::vec3(0.05f, 0.85f, 1.0f));
        shader.setFloat("u_Material.emissionStrength", 3.0f);
        if (m_visorMesh) m_visorMesh->draw(&shader);
    }

    // 4. Mochila / Jetpack de Exploración en la espalda
    {
        glm::mat4 packMat = glm::translate(baseMat, glm::vec3(0.0f, 0.92f, -0.28f));
        packMat = glm::scale(packMat, glm::vec3(0.40f, 0.65f, 0.22f));
        shader.setMat4("u_Model", packMat);
        shader.setVec3("u_EntityColor", glm::vec3(0.12f, 0.14f, 0.18f));
        shader.setVec3("u_Material.albedo", glm::vec3(0.12f, 0.14f, 0.18f));
        shader.setFloat("u_Material.metallic", 0.60f);
        shader.setFloat("u_Material.roughness", 0.50f);
        shader.setFloat("u_Material.specular", 0.40f);
        shader.setVec3("u_Material.emission", glm::vec3(0.0f));
        shader.setFloat("u_Material.emissionStrength", 0.0f);
        if (m_packMesh) m_packMesh->draw(&shader);
    }
}

glm::vec3 CharacterController::getPosition() const {
    return m_position;
}

void CharacterController::setPosition(const glm::vec3& pos) {
    m_position = pos;
    if (m_rigidBody && m_rigidBody->getMotionState()) {
        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(pos.x, pos.y + m_capsuleHeight * 0.5f, pos.z));
        m_rigidBody->setWorldTransform(trans);
        m_rigidBody->getMotionState()->setWorldTransform(trans);
        m_rigidBody->clearForces();
        m_rigidBody->setLinearVelocity(btVector3(0, 0, 0));
    }
}

glm::vec3 CharacterController::getVelocity() const {
    return m_velocity;
}

void CharacterController::setVelocity(const glm::vec3& vel) {
    m_velocity = vel;
    if (m_rigidBody) {
        m_rigidBody->setLinearVelocity(toBt(vel));
    }
}

} // namespace ASIS
