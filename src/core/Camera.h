#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ASIS {

enum class CameraMode {
    ORBIT, // Modo tipo Blender: orbita alrededor de un punto pivote
    FLY    // Modo tipo Unreal: vuelo libre en primera persona (RMB + WASD)
};

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 3.5f, 6.0f),
           glm::vec3 target = glm::vec3(0.0f, 0.5f, 0.0f),
           float fov = 45.0f);

    // Obtener matrices para los shaders
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio = 16.0f / 9.0f) const;

    // Procesamiento de entradas
    void processKeyboard(GLFWwindow* window, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    void onMouseButton(int button, int action, int mods);

    // Re-enfocar vista (Tecla F)
    void resetToFocus(glm::vec3 target = glm::vec3(0.0f, 0.5f, 0.0f));

    // Getters y Setters
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getTarget() const { return m_target; }
    glm::vec3 getFront() const { return m_front; }
    glm::vec3 getRight() const { return m_right; }
    glm::vec3 getUp() const { return m_up; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    float getOrbitDistance() const { return m_orbitDistance; }
    void setOrbitDistance(float d) { m_orbitDistance = glm::max(0.5f, d); }
    void syncOrbitDistance() {
        float d = glm::length(m_target - m_position);
        if (d > 0.05f) {
            m_orbitDistance = glm::max(0.5f, d);
        }
    }
    void setPosition(const glm::vec3& pos) {
        m_position = pos;
        syncOrbitDistance();
    }
    void setTarget(const glm::vec3& target) {
        m_target = target;
        syncOrbitDistance();
    }
    void setYawPitch(float yaw, float pitch) {
        m_yaw = yaw;
        m_pitch = glm::clamp(pitch, -89.0f, 89.0f);
        updateVectorsFromAngles();
    }
    void updateVectorsFromAngles();
    CameraMode getMode() const { return m_mode; }
    bool isFlying() const { return m_isFlying; }
    bool isOrbiting() const { return m_isOrbiting; }
    bool isPanning() const { return m_isPanning; }

    // Parámetros de Lente y Proyección (Fase 10)
    float getFOV() const { return m_fov; }
    void setFOV(float fov) { m_fov = glm::clamp(fov, 10.0f, 130.0f); }
    float& getFOVRef() { return m_fov; }

    float getNearPlane() const { return m_nearPlane; }
    void setNearPlane(float n) { m_nearPlane = n; }
    float& getNearPlaneRef() { return m_nearPlane; }

    float getFarPlane() const { return m_farPlane; }
    void setFarPlane(float f) { m_farPlane = f; }
    float& getFarPlaneRef() { return m_farPlane; }

    bool isOrthographic() const { return m_isOrthographic; }
    void setOrthographic(bool ortho) { m_isOrthographic = ortho; }
    bool& getOrthographicRef() { return m_isOrthographic; }

private:
    CameraMode m_mode;

    // Vectores espaciales unificados (un solo sistema de referencia)
    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    // Ángulos de orientación (Yaw y Pitch unificados para ambos modos)
    float m_yaw;
    float m_pitch;

    // Parámetros
    float m_orbitDistance;
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_fov;
    float m_nearPlane;
    float m_farPlane;
    bool m_isOrthographic;

    // Estados de botones
    bool m_isFlying;   // Clic derecho (Unreal fly)
    bool m_isOrbiting; // Clic central o Alt+LMB (Blender orbit)
    bool m_isPanning;  // Shift + Clic central (Blender pan)
};

} // namespace ASIS
