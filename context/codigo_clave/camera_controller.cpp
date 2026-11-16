// Snippet Clave: Controlador de Cámara Híbrido Corregido (Blender Orbit / Unreal Flycam)
// Archivo de referencia: src/core/Camera.cpp

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>

// Vectores unificados desde ángulos Yaw y Pitch
glm::vec3 computeFrontVector(float yaw, float pitch) {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(front);
}

// Modo Unreal Flycam (RMB + WASD): El frente orienta la cabeza y la posición se traslada
void updateFlyPosition(glm::vec3& position, glm::vec3& target, const glm::vec3& front, const glm::vec3& right, const glm::vec3& worldUp, float speed, float distance, GLFWwindow* window, float deltaTime) {
    float velocity = speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) velocity *= 2.5f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * velocity;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) position += worldUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) position -= worldUp * velocity;

    // Mantener sincronizado el punto focal
    target = position + front * distance;
}

// Modo Blender Orbit: La posición es exactamente target - front * distance (sin discrepancias de signos)
glm::vec3 computeOrbitPosition(const glm::vec3& target, const glm::vec3& front, float distance) {
    return target - front * distance;
}
