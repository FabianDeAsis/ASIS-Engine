#include "Camera.h"
#include <algorithm>
#include <iostream>

namespace ASIS {

Camera::Camera(glm::vec3 position, glm::vec3 target, float fov)
    : m_mode(CameraMode::ORBIT),
      m_position(position),
      m_target(target),
      m_worldUp(0.0f, 1.0f, 0.0f),
      m_movementSpeed(5.0f),
      m_mouseSensitivity(0.12f),
      m_fov(fov),
      m_nearPlane(0.1f),
      m_farPlane(1000.0f),
      m_isOrthographic(false),
      m_isFlying(false),
      m_isOrbiting(false),
      m_isPanning(false)
{
    // Calcular vector dirección inicial desde la cámara hacia el objetivo
    glm::vec3 dir = m_target - m_position;
    m_orbitDistance = glm::length(dir);
    if (m_orbitDistance < 0.1f) m_orbitDistance = 6.0f;

    dir = glm::normalize(dir);

    // Calcular ángulos esféricos para que coincidan con la posición inicial
    m_pitch = glm::degrees(asin(std::clamp(dir.y, -0.999f, 0.999f)));
    m_yaw = glm::degrees(atan2(dir.z, dir.x));

    updateVectorsFromAngles();

    // Posición sincronizada exactamente con el pivote
    m_position = m_target - m_front * m_orbitDistance;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    if (m_isOrthographic) {
        float orthoSize = m_orbitDistance * 0.5f;
        if (orthoSize < 1.0f) orthoSize = 1.0f;
        return glm::ortho(-orthoSize * aspectRatio, orthoSize * aspectRatio, -orthoSize, orthoSize, m_nearPlane, m_farPlane);
    }
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::onMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        m_isFlying = (action == GLFW_PRESS);
        if (m_isFlying) {
            m_mode = CameraMode::FLY;
            // Asegurar que el objetivo se mantenga al frente de la cámara
            m_target = m_position + m_front * m_orbitDistance;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (mods & GLFW_MOD_SHIFT) {
            m_isPanning = (action == GLFW_PRESS);
            m_isOrbiting = false;
        } else {
            m_isOrbiting = (action == GLFW_PRESS);
            m_isPanning = false;
            if (m_isOrbiting) {
                m_mode = CameraMode::ORBIT;
                syncOrbitDistance();
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && (mods & GLFW_MOD_ALT)) {
        m_isOrbiting = (action == GLFW_PRESS);
        if (m_isOrbiting) {
            m_mode = CameraMode::ORBIT;
            syncOrbitDistance();
        }
    }

    if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            m_isFlying = false;
        }
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            m_isOrbiting = false;
            m_isPanning = false;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            m_isOrbiting = false;
        }
    }
}

void Camera::processKeyboard(GLFWwindow* window, float deltaTime) {
    if (m_isFlying) {
        float velocity = m_movementSpeed * deltaTime;

        // Acelerar con Shift
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            velocity *= 2.5f;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_position += m_front * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_position -= m_front * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_position -= m_right * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_position += m_right * velocity;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_position += m_worldUp * velocity;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) m_position -= m_worldUp * velocity;

        // Mantener el objetivo al frente
        m_target = m_position + m_front * m_orbitDistance;
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    // Protección contra saltos súbitos de cursor por captura o desanclaje de GLFW
    if (std::abs(xoffset) > 150.0f || std::abs(yoffset) > 150.0f) {
        return;
    }

    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    if (m_isFlying) {
        // En modo Unreal Flycam: El ratón orienta la cabeza de la cámara
        m_yaw   += xoffset;
        m_pitch += yoffset;

        if (constrainPitch) {
            m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        }

        updateVectorsFromAngles();
        m_target = m_position + m_front * m_orbitDistance;
    }
    else if (m_isOrbiting) {
        // En modo Blender Orbit: La cámara gira alrededor del punto de enfoque
        m_yaw   += xoffset;
        m_pitch += yoffset;

        if (constrainPitch) {
            m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        }

        updateVectorsFromAngles();
        m_position = m_target - m_front * m_orbitDistance;
    }
    else if (m_isPanning) {
        // En modo Blender Pan: Desplaza el objetivo y la posición paralelamente
        float panSpeed = m_orbitDistance * 0.0015f;
        m_target   -= m_right * (xoffset * panSpeed);
        m_target   -= m_up * (yoffset * panSpeed);
        m_position = m_target - m_front * m_orbitDistance;
    }
}

void Camera::processMouseScroll(float yoffset) {
    if (m_isFlying) {
        // Ajustar velocidad de vuelo
        m_movementSpeed += yoffset * 1.5f;
        m_movementSpeed = std::clamp(m_movementSpeed, 1.0f, 50.0f);
    } else {
        // Zoom hacia el objeto (Blender)
        syncOrbitDistance();
        m_orbitDistance -= yoffset * 0.7f;
        m_orbitDistance = std::clamp(m_orbitDistance, 0.5f, 100.0f);
        m_position = m_target - m_front * m_orbitDistance;
    }
}

void Camera::resetToFocus(glm::vec3 target) {
    m_target = target;
    m_orbitDistance = 6.0f;
    m_yaw = -90.0f;
    m_pitch = -22.0f; // Vista ligeramente inclinada hacia abajo
    m_mode = CameraMode::ORBIT;
    updateVectorsFromAngles();
    m_position = m_target - m_front * m_orbitDistance;

    std::cout << "[ASIS Engine] Cámara re-enfocada en (" << target.x << ", " << target.y << ", " << target.z << ")\n";
}

void Camera::updateVectorsFromAngles() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace ASIS
