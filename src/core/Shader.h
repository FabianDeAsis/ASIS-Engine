#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace ASIS {

class Shader {
public:
    // Carga los códigos fuente desde disco, los compila y los enlaza en la GPU
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    // Activar este shader para los siguientes comandos de renderizado
    void use() const;

    // Métodos para enviar variables 'uniform' (matrices, vectores, números) a la GPU
    void setMat4(const std::string& name, const glm::mat4& matrix) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

    GLuint getProgramID() const { return m_programID; }

private:
    GLuint m_programID;

    // Métodos auxiliares para lectura y verificación de errores
    std::string readFile(const std::string& filePath) const;
    void checkCompileErrors(GLuint shader, const std::string& type) const;
};

} // namespace ASIS
