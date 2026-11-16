#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <btBulletDynamicsCommon.h>
#include <memory>
#include "Mesh.h"

namespace ASIS {

class Camera;
class PhysicsWorld;
class Shader;
class Terrain;
enum class FootstepSurface;

enum class CharacterCameraMode {
    FIRST_PERSON,
    THIRD_PERSON
};

/**
 * @brief Controlador de Personaje 3D (First Person & Third Person Character Controller)
 * 
 * Gestiona el movimiento cinemático/dinámico con Bullet Physics (btCapsuleShape),
 * navegación fluida por terreno y obstáculos (WASD, sprint, agacharse, salto),
 * cámara en primera persona (FPS) y tercera persona (TPS) con brazo elástico (spring arm),
 * y avatar 3D estilizado con visor holográfico emisivo.
 */
class CharacterController {
public:
    CharacterController();
    ~CharacterController();

    // Inicializar geometrías y mallas del avatar
    bool init();
    void shutdown();

    // Aparecer (Spawn) y Desaparecer (Despawn) en el mundo de físicas
    void spawn(PhysicsWorld& physicsWorld, const glm::vec3& startPosition, float startYaw = -90.0f);
    void despawn(PhysicsWorld& physicsWorld);
    bool isSpawned() const { return m_isSpawned; }

    // Procesar entradas de teclado, físicas y locomoción sonora adaptativa (Fase 16)
    void update(PhysicsWorld& physicsWorld, GLFWwindow* window, float deltaTime, const Terrain* terrain = nullptr);

    bool isHeadBobEnabled() const { return m_enableHeadBob; }
    void setHeadBobEnabled(bool enable) { m_enableHeadBob = enable; }
    float getCurrentEyeHeight() const { return m_currentEyeHeight; }

    // Procesar rotación de vista con el ratón
    void processMouseLook(float xoffset, float yoffset);

    // Conmutar entre primera y tercera persona
    void toggleCameraMode();
    void setCameraMode(CharacterCameraMode mode) { m_cameraMode = mode; }
    CharacterCameraMode getCameraMode() const { return m_cameraMode; }

    // Sincronizar la cámara activa del juego (posicionamiento y orientación)
    void updateCamera(Camera& camera, const PhysicsWorld& physicsWorld);

    // Dibujar avatar 3D en tercera persona
    void drawAvatar(Shader& shader);

    // Getters y Setters
    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& pos);

    glm::vec3 getVelocity() const;
    void setVelocity(const glm::vec3& vel);

    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    void setYaw(float yaw) { m_yaw = yaw; }
    void setPitch(float pitch) { m_pitch = glm::clamp(pitch, -85.0f, 85.0f); }

    bool isGrounded() const { return m_isGrounded; }
    bool isSprinting() const { return m_isSprinting; }
    bool isCrouching() const { return m_isCrouching; }

    float getWalkSpeed() const { return m_walkSpeed; }
    void setWalkSpeed(float speed) { m_walkSpeed = speed; }

    float getSprintSpeed() const { return m_sprintSpeed; }
    void setSprintSpeed(float speed) { m_sprintSpeed = speed; }

    float getCrouchSpeed() const { return m_crouchSpeed; }
    void setCrouchSpeed(float speed) { m_crouchSpeed = speed; }

    float getJumpForce() const { return m_jumpForce; }
    void setJumpForce(float force) { m_jumpForce = force; }

    float getMouseSensitivity() const { return m_mouseSensitivity; }
    void setMouseSensitivity(float sens) { m_mouseSensitivity = sens; }

    float getThirdPersonDistance() const { return m_thirdPersonDistance; }
    void setThirdPersonDistance(float dist) { m_thirdPersonDistance = glm::clamp(dist, 1.0f, 15.0f); }

    float getShoulderOffset() const { return m_shoulderOffset; }
    void setShoulderOffset(float offset) { m_shoulderOffset = offset; }

    float getMass() const { return m_mass; }
    float getCapsuleRadius() const { return m_capsuleRadius; }
    float getCapsuleHeight() const { return m_capsuleHeight; }

private:
    bool m_isSpawned;
    CharacterCameraMode m_cameraMode;

    glm::vec3 m_position;
    glm::vec3 m_velocity;
    float m_yaw;
    float m_pitch;

    // Configuración física de cápsula
    float m_mass;
    float m_capsuleRadius;
    float m_capsuleHeight;
    float m_eyeHeight;
    float m_standingEyeHeight;
    float m_crouchEyeHeight;
    float m_currentEyeHeight;
    float m_shoulderHeight;
    float m_thirdPersonDistance;
    float m_shoulderOffset;

    // Parámetros de velocidad
    float m_walkSpeed;
    float m_sprintSpeed;
    float m_crouchSpeed;
    float m_jumpForce;
    float m_mouseSensitivity;

    // Estados
    bool m_isGrounded;
    bool m_wasGrounded;
    bool m_isSprinting;
    bool m_isCrouching;

    // Audio y Bamboleo de Cabeza (Head Bobbing)
    float m_headBobTimer;
    bool m_enableHeadBob;
    float m_stepTimer;

    // Objetos de Bullet Physics
    btCapsuleShape* m_capsuleShape;
    btRigidBody* m_rigidBody;
    btDefaultMotionState* m_motionState;

    // Mallas 3D del Avatar (para 3ra persona)
    std::unique_ptr<Mesh> m_torsoMesh;
    std::unique_ptr<Mesh> m_headMesh;
    std::unique_ptr<Mesh> m_visorMesh;
    std::unique_ptr<Mesh> m_packMesh;
};

} // namespace ASIS
