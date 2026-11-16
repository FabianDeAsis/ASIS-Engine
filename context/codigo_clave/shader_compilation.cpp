// Snippet Clave: Compilación y Enlace de Shaders en OpenGL
// Archivo de referencia: src/core/Shader.cpp

#include <glad/glad.h>
#include <iostream>
#include <stdexcept>

GLuint compileAndLinkShaders(const char* vSource, const char* fSource) {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vSource, nullptr);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fSource, nullptr);
    glCompileShader(fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}
