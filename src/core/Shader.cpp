#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace ASIS {

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
    : m_programID(0)
{
    // 1. Cargar el código fuente desde los archivos en disco
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compilar el Vertex Shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // 3. Compilar el Fragment Shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // 4. Crear el Programa de Shaders y enlazar ambos módulos
    m_programID = glCreateProgram();
    glAttachShader(m_programID, vertex);
    glAttachShader(m_programID, fragment);
    glLinkProgram(m_programID);
    checkCompileErrors(m_programID, "PROGRAM");

    // 5. Los shaders individuales ya están enlazados en el binario del programa; liberamos la memoria intermedia
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    std::cout << "[ASIS Engine] Shaders compilados y enlazados exitosamente (ID: " << m_programID << ")" << std::endl;
}

Shader::~Shader() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
    }
}

void Shader::use() const {
    glUseProgram(m_programID);
}

void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const {
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location != -1) {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
}

void Shader::setVec2(const std::string& name, float x, float y) const {
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void Shader::setFloat(const std::string& name, float value) const {
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::setInt(const std::string& name, int value) const {
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location != -1) {
        glUniform1i(location, value);
    }
}

std::string Shader::readFile(const std::string& filePath) const {
    // Lista de rutas candidatas para garantizar que encuentre el shader sin importar desde dónde se ejecute el motor
    std::vector<std::string> candidates = {
        "assets/" + filePath,
        "../assets/" + filePath,
        "../../assets/" + filePath,
        "motor_grafico/assets/" + filePath,
        "motor_grafico/build/Release/assets/" + filePath,
        filePath,
        "../" + filePath,
        "../../" + filePath
    };

    for (const auto& path : candidates) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream stream;
            stream << file.rdbuf();
            file.close();
            return stream.str();
        }
    }

    std::cerr << "[ASIS Engine ERROR] No se pudo encontrar ni leer el archivo de shader: " << filePath << std::endl;
    throw std::runtime_error("Fallo al abrir el archivo de shader: " + filePath);
}

void Shader::checkCompileErrors(GLuint shader, const std::string& type) const {
    GLint success;
    GLchar infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "[ASIS Engine ERROR] Error de compilacion en shader de tipo " << type << ":\n"
                      << infoLog << "\n-----------------------------------------\n" << std::endl;
            throw std::runtime_error("Error compilando shader " + type);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "[ASIS Engine ERROR] Error de enlace del programa de shaders:\n"
                      << infoLog << "\n-----------------------------------------\n" << std::endl;
            throw std::runtime_error("Error enlazando Shader Program");
        }
    }
}

} // namespace ASIS
