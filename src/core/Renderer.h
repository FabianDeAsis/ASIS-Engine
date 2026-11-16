#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Mesh.h"

namespace ASIS {

class Renderer {
public:
    Renderer();
    ~Renderer() = default;

    // Configura el color de fondo para limpiar el buffer
    void setClearColor(float r, float g, float b, float a = 1.0f);

    // Limpia los buffers de color y profundidad
    void clear() const;

    // Ejecuta el renderizado de una malla con un shader activo
    void draw(const Shader& shader, const Mesh& mesh) const;

    // Muestra en consola las capacidades de la GPU detectada
    void printGPUInfo() const;
};

} // namespace ASIS
