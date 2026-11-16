#pragma once

#include <string>
#include <functional>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace ASIS {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void pollEvents() const;
    void swapBuffers() const;
    bool shouldClose() const;
    void close();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    GLFWwindow* getNativeWindow() const { return m_window; }

    // Registradores de callbacks para interactividad fluida
    void setCursorPosCallback(std::function<void(double xpos, double ypos)> callback) {
        m_cursorPosCallback = callback;
    }
    void setMouseButtonCallback(std::function<void(int button, int action, int mods)> callback) {
        m_mouseButtonCallback = callback;
    }
    void setScrollCallback(std::function<void(double xoffset, double yoffset)> callback) {
        m_scrollCallback = callback;
    }
    void setDropCallback(std::function<void(const std::vector<std::string>& paths)> callback) {
        m_dropCallback = callback;
    }

private:
    int m_width;
    int m_height;
    std::string m_title;
    GLFWwindow* m_window;

    std::function<void(double, double)> m_cursorPosCallback;
    std::function<void(int, int, int)>  m_mouseButtonCallback;
    std::function<void(double, double)> m_scrollCallback;
    std::function<void(const std::vector<std::string>&)> m_dropCallback;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfwDropCallback(GLFWwindow* window, int count, const char** paths);
};

} // namespace ASIS
