// Snippet Clave: Inicialización de Ventana GLFW y Carga de OpenGL Moderno con GLAD
// Archivo de referencia: src/core/Window.cpp

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

GLFWwindow* setupWindowAndOpenGL(int width, int height, const char* title) {
    if (!glfwInit()) {
        throw std::runtime_error("Fallo glfwInit");
    }

    // Configurar OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Fallo glfwCreateWindow");
    }

    glfwMakeContextCurrent(window);

    // Cargar punteros de funciones con GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Fallo gladLoadGLLoader");
    }

    glViewport(0, 0, width, height);
    glfwSwapInterval(1); // V-Sync

    return window;
}
