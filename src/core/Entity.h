#pragma once

#include <string>
#include <memory>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Mesh.h"
#include "Model.h"
#include "Texture.h"
#include "Material.h"
#include "Shader.h"
#include "OASISMeshlet.h"

namespace ASIS {

class Camera;

class Entity {
public:
    Entity(const std::string& name);
    ~Entity() = default;

    // Sincronización analítica de Cuaternión desde Euler con orden idéntico a getModelMatrix (Y -> X -> Z)
    void syncQuaternionFromEuler() {
        glm::mat4 R(1.0f);
        R = glm::rotate(R, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        R = glm::rotate(R, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        R = glm::rotate(R, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        m_orientation = glm::quat_cast(R);
        m_useQuaternion = true;
    }

    // Métodos de transformación (accesibles desde C++ y Lua)
    void setName(const std::string& name) { m_name = name; }
    void setPosition(float x, float y, float z) { m_position = glm::vec3(x, y, z); }
    void setRotation(float pitch, float yaw, float roll) {
        pitch = std::fmod(pitch, 360.0f); if (pitch < 0.0f) pitch += 360.0f;
        yaw   = std::fmod(yaw,   360.0f); if (yaw   < 0.0f) yaw   += 360.0f;
        roll  = std::fmod(roll,  360.0f); if (roll  < 0.0f) roll  += 360.0f;
        m_rotation = glm::vec3(pitch, yaw, roll);
        syncQuaternionFromEuler();
    }
    void setOrientation(const glm::quat& q) {
        m_orientation = q;
        m_useQuaternion = true;
        m_rotation = glm::degrees(glm::eulerAngles(q));
        m_rotation.x = std::fmod(m_rotation.x, 360.0f); if (m_rotation.x < 0.0f) m_rotation.x += 360.0f;
        m_rotation.y = std::fmod(m_rotation.y, 360.0f); if (m_rotation.y < 0.0f) m_rotation.y += 360.0f;
        m_rotation.z = std::fmod(m_rotation.z, 360.0f); if (m_rotation.z < 0.0f) m_rotation.z += 360.0f;
    }
    const glm::quat& getOrientation() const { return m_orientation; }
    bool usesQuaternion() const { return m_useQuaternion; }
    void setScale(float x, float y, float z) { m_scale = glm::vec3(x, y, z); }
    void setUniformScale(float s) { m_scale = glm::vec3(s, s, s); }
    
    void setColor(float r, float g, float b) {
        m_color = glm::vec3(r, g, b);
        m_hasCustomColor = true;
    }
    void setColorVec3(const glm::vec3& c) {
        m_color = c;
        m_hasCustomColor = true;
    }

    void move(float dx, float dy, float dz) {
        if (m_controlledBySequencer) return;
        m_position += glm::vec3(dx, dy, dz);
    }
    void rotate(float dpitch, float dyaw, float droll) {
        if (m_controlledBySequencer) return;
        m_rotation += glm::vec3(dpitch, dyaw, droll);
        m_rotation.x = std::fmod(m_rotation.x, 360.0f); if (m_rotation.x < 0.0f) m_rotation.x += 360.0f;
        m_rotation.y = std::fmod(m_rotation.y, 360.0f); if (m_rotation.y < 0.0f) m_rotation.y += 360.0f;
        m_rotation.z = std::fmod(m_rotation.z, 360.0f); if (m_rotation.z < 0.0f) m_rotation.z += 360.0f;
        syncQuaternionFromEuler();
    }

    // Control de Prioridad Cinemática (Sequencer vs Lua - Fase 22.5)
    bool isControlledBySequencer() const { return m_controlledBySequencer; }
    void setControlledBySequencer(bool controlled) { m_controlledBySequencer = controlled; }

    // Getters y Setters
    const std::string& getName() const { return m_name; }
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getRotation() const { return m_rotation; }
    glm::vec3 getScale() const { return m_scale; }
    glm::vec3 getColor() const { return m_color; }
    bool hasCustomColor() const { return m_hasCustomColor; }
    void setHasCustomColor(bool v) { m_hasCustomColor = v; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    // Referencias mutables para el Inspector de ImGui
    glm::vec3& getPositionRef() { return m_position; }
    glm::vec3& getRotationRef() { return m_rotation; }
    glm::vec3& getScaleRef() { return m_scale; }
    glm::vec3& getColorRef() { return m_color; }

    // Asignación de componentes visuales
    void setMesh(std::shared_ptr<Mesh> mesh) { m_mesh = mesh; }
    void setModel(std::shared_ptr<Model> model) { m_model = model; }
    void setTexture(std::shared_ptr<Texture> texture) { m_texture = texture; }

    std::shared_ptr<Mesh> getMesh() const { return m_mesh; }
    std::shared_ptr<Model> getModel() const { return m_model; }
    std::shared_ptr<Texture> getTexture() const { return m_texture; }

    void setMaterial(std::shared_ptr<Material> mat) { m_material = mat; }
    std::shared_ptr<Material> getMaterial() const { return m_material; }
    std::shared_ptr<Material>& getMaterialRef() { return m_material; }

    // Matriz de transformación local a mundo
    glm::mat4 getModelMatrix() const;

    // Actualizar lógica y animación de la entidad (Fase 13)
    void update(float dt) {
        if (m_model) {
            m_model->updateAnimation(dt);
        }
    }

    // Renderizar entidad en el shader (con soporte para OASIS Meshlets)
    void draw(const Shader& shader, const Camera* camera = nullptr,
              const OASISMeshletConfig* oasisConfig = nullptr,
              OASISMeshletTelemetry* oasisTelemetry = nullptr) const;

    // Tipo de geometría / primitiva (Fase 11)
    const std::string& getPrimitiveType() const { return m_primitiveType; }
    void setPrimitiveType(const std::string& type) { m_primitiveType = type; }
    const std::string& getModelPath() const { return m_modelPath; }
    void setModelPath(const std::string& path) { m_modelPath = path; }

    // Sistema de Grupos / Carpetas (Fase 16)
    const std::string& getGroup() const { return m_group; }
    void setGroup(const std::string& group) { m_group = group; }

    // Zonas de Trigger / Disparadores de Eventos (Fase 17)
    bool isTrigger() const { return m_isTrigger; }
    void setTrigger(bool trigger) { m_isTrigger = trigger; }
    const std::string& getTriggerTag() const { return m_triggerTag; }
    void setTriggerTag(const std::string& tag) { m_triggerTag = tag; }
    const std::string& getTriggerType() const { return m_triggerTag; }
    void setTriggerType(const std::string& type) { m_triggerTag = type; }

    // Propiedades de Dinámica y Física Bullet (Fase 17 / 19 / 20)
    bool hasPhysics() const { return m_hasPhysics; }
    void setHasPhysics(bool hasPhys) { m_hasPhysics = hasPhys; }
    float getMass() const { return m_mass; }
    void setMass(float m) { m_mass = m; }
    const std::string& getColliderType() const { return m_colliderType; }
    void setColliderType(const std::string& type) { m_colliderType = type; }
    float getFriction() const { return m_friction; }
    void setFriction(float f) { m_friction = f; }
    float getRestitution() const { return m_restitution; }
    void setRestitution(float r) { m_restitution = r; }
    bool isKinematic() const { return m_isKinematic; }
    void setKinematic(bool k) { m_isKinematic = k; }

private:
    std::string m_name;
    std::string m_group; // Nombre de grupo o vacío si no está agrupado
    bool m_isTrigger = false;
    std::string m_triggerTag = "Default";
    bool m_hasPhysics = false;
    float m_mass = 1.0f;
    std::string m_colliderType = "BOX";
    float m_friction = 0.6f;
    float m_restitution = 0.4f;
    bool m_isKinematic = false;
    glm::vec3 m_position;
    glm::vec3 m_rotation; // Grados
    glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    bool m_useQuaternion = false;
    glm::vec3 m_scale;
    glm::vec3 m_color;    // Tinte o color base
    bool m_hasCustomColor;
    bool m_visible;
    bool m_controlledBySequencer = false;
    std::string m_primitiveType; // "Cube", "Sphere", "Cylinder", "Cone", "Torus", "Plane", "Model"
    std::string m_modelPath;

    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Model> m_model;
    std::shared_ptr<Texture> m_texture;
    std::shared_ptr<Material> m_material;
};

} // namespace ASIS
