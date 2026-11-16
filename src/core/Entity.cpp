#include "Entity.h"

namespace ASIS {

Entity::Entity(const std::string& name)
    : m_name(name),
      m_group(""),
      m_position(0.0f, 0.0f, 0.0f),
      m_rotation(0.0f, 0.0f, 0.0f),
      m_scale(1.0f, 1.0f, 1.0f),
      m_color(1.0f, 1.0f, 1.0f),
      m_hasCustomColor(false),
      m_visible(true),
      m_material(std::make_shared<Material>()),
      m_primitiveType("Cube"),
      m_modelPath("")
{
}

glm::mat4 Entity::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);

    if (m_useQuaternion) {
        model *= glm::mat4_cast(m_orientation);
    } else {
        // Rotación en orden Y -> X -> Z (Yaw, Pitch, Roll)
        model = glm::rotate(model, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    model = glm::scale(model, m_scale);
    return model;
}

void Entity::draw(const Shader& shader, const Camera* camera,
                  const OASISMeshletConfig* oasisConfig,
                  OASISMeshletTelemetry* oasisTelemetry) const {
    if (!m_visible) return;

    shader.use();
    glm::mat4 modelMat = getModelMatrix();
    shader.setMat4("u_Model", modelMat);
    shader.setVec3("u_EntityColor", m_color);

    if (m_material) {
        if (!m_material->hasAlbedoMap() && m_texture) {
            m_material->setAlbedoMap(m_texture);
        }
        m_material->bind(shader);
    }

    if (oasisConfig && oasisConfig->enabled && camera && oasisTelemetry) {
        if (m_model) {
            m_model->drawOASIS(shader, modelMat, camera, *oasisConfig, *oasisTelemetry);
        } else if (m_mesh) {
            m_mesh->drawOASIS(&shader, modelMat, camera, *oasisConfig, *oasisTelemetry);
        }
    } else {
        if (m_model) {
            m_model->draw(shader);
        } else if (m_mesh) {
            m_mesh->draw(&shader);
        }
    }
}

} // namespace ASIS
