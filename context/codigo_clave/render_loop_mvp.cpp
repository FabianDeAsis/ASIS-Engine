// Snippet Clave: Cálculo de Matrices MVP y Rotación en Loop de Renderizado
// Archivo de referencia: src/core/main.cpp

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void updateMVP(float rotationAngle, float aspectRatio, GLuint programID) {
    // Proyección en Perspectiva
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    // Cámara (Vista)
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Transformación del objeto (Rotación)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(rotationAngle * 0.5f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Enviar a uniforms
    GLint locProj = glGetUniformLocation(programID, "u_Projection");
    GLint locView = glGetUniformLocation(programID, "u_View");
    GLint locModel = glGetUniformLocation(programID, "u_Model");

    glUniformMatrix4fv(locProj, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);
}
