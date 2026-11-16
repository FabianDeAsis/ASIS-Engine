#include "Window.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

namespace ASIS {

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height), m_title(title), m_window(nullptr)
{
    if (!glfwInit()) {
        throw std::runtime_error("Error critico: No se pudo inicializar GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Error critico: Fallo la creacion de la ventana GLFW");
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Error critico: Fallo la inicializacion de GLAD para OpenGL");
    }

    // Guardar el puntero a esta instancia de Window en GLFW para recuperar en callbacks
    glfwSetWindowUserPointer(m_window, this);

    // Callbacks de GLFW
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_window, glfwCursorPosCallback);
    glfwSetMouseButtonCallback(m_window, glfwMouseButtonCallback);
    glfwSetScrollCallback(m_window, glfwScrollCallback);
    glfwSetDropCallback(m_window, glfwDropCallback);

    glViewport(0, 0, m_width, m_height);
    glfwSwapInterval(1); // V-Sync

#ifdef _WIN32
    // Barra de título oscura de Windows 11 / Windows 10 (Fase 22.5)
    HWND hwnd = glfwGetWin32Window(m_window);
    if (hwnd) {
        BOOL useDarkMode = TRUE;
        if (FAILED(DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDarkMode, sizeof(useDarkMode)))) {
            DwmSetWindowAttribute(hwnd, 19, &useDarkMode, sizeof(useDarkMode));
        }
    }
#endif

    // Generar icono nativo de 64x64 con la formula exacta de asisengine.svg (Fase 22.5)
    {
        const int iconSize = 64;
        std::vector<unsigned char> iconPixels(iconSize * iconSize * 4, 0);

        for (int py = 0; py < iconSize; ++py) {
            float y = (float)py / (float)(iconSize - 1);
            float triLeft = 0.5f * (1.0f - y);
            float triRight = 0.5f + 0.5f * y;

            bool inCut = (y >= 0.674f && y <= 0.708f) ||
                         (y >= 0.752f && y <= 0.786f) ||
                         (y >= 0.830f && y <= 0.864f) ||
                         (y >= 0.908f && y <= 0.942f);
            float cutRight = 0.370f + (y - 0.674f) * 0.46f;

            for (int px = 0; px < iconSize; ++px) {
                float x = (float)px / (float)(iconSize - 1);
                if (x >= triLeft && x <= triRight) {
                    if (inCut && x <= cutRight) {
                        continue; // Corte negativo (transparente)
                    }
                    int idx = (py * iconSize + px) * 4;
                    iconPixels[idx + 0] = 255; // R
                    iconPixels[idx + 1] = 255; // G
                    iconPixels[idx + 2] = 255; // B
                    iconPixels[idx + 3] = 255; // A
                }
            }
        }

        GLFWimage iconImage;
        iconImage.width = iconSize;
        iconImage.height = iconSize;
        iconImage.pixels = iconPixels.data();
        glfwSetWindowIcon(m_window, 1, &iconImage);
    }

    std::cout << "[ASIS Engine] Ventana creada con exito: " << m_title << " (" << m_width << "x" << m_height << ")\n";
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
    std::cout << "[ASIS Engine] Ventana y contexto GLFW destruidos correctamente.\n";
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(m_window);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::close() {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->m_width = width;
        win->m_height = height;
    }
    glViewport(0, 0, width, height);
}

void Window::glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_cursorPosCallback) {
        win->m_cursorPosCallback(xpos, ypos);
    }
}

void Window::glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_mouseButtonCallback) {
        win->m_mouseButtonCallback(button, action, mods);
    }
}

void Window::glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_scrollCallback) {
        win->m_scrollCallback(xoffset, yoffset);
    }
}

void Window::glfwDropCallback(GLFWwindow* window, int count, const char** paths) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_dropCallback && count > 0 && paths) {
        std::vector<std::string> pathList;
        pathList.reserve(count);
        for (int i = 0; i < count; ++i) {
            if (paths[i]) pathList.push_back(paths[i]);
        }
        win->m_dropCallback(pathList);
    }
}

} // namespace ASIS
