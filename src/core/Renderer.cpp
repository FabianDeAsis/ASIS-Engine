#include "Renderer.h"
#include <iostream>

namespace ASIS {

Renderer::Renderer() {
    // Activar prueba de profundidad (Z-Buffer) para que los objetos tridimensionales se ocluyan correctamente
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Color de fondo por defecto: Azul oscuro pizarra moderno (#141923)
    setClearColor(0.08f, 0.10f, 0.14f, 1.0f);
}

void Renderer::setClearColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
}

void Renderer::clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw(const Shader& shader, const Mesh& mesh) const {
    shader.use();
    mesh.draw();
}

void Renderer::printGPUInfo() const {
    const GLubyte* vendor   = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    const GLubyte* glsl     = glGetString(GL_SHADING_LANGUAGE_VERSION);

    std::cout << "==========================================\n";
    std::cout << " [ASIS Engine] Informacion del Dispositivo Gráfico:\n";
    std::cout << " Fabricante:      " << (vendor   ? (const char*)vendor   : "Desconocido") << "\n";
    std::cout << " Tarjeta Grafica: " << (renderer ? (const char*)renderer : "Desconocido") << "\n";
    std::cout << " Version OpenGL:  " << (version  ? (const char*)version  : "Desconocido") << "\n";
    std::cout << " Version GLSL:    " << (glsl     ? (const char*)glsl     : "Desconocido") << "\n";
    std::cout << "==========================================\n";
}

} // namespace ASIS
