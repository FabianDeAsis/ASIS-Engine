#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Entity.h"
#include "Shader.h"
#include "Light.h"
#include "Texture.h"
#include "Mesh.h"
#include "Model.h"

namespace ASIS {

class Camera;
struct OASISMeshletConfig;
struct OASISMeshletTelemetry;

class Scene {
public:
    Scene();
    ~Scene() = default;

    // Fábricas de entidades accesibles desde C++ y Lua
    std::shared_ptr<Entity> createCube(const std::string& name, float x = 0.0f, float y = 0.0f, float z = 0.0f, float size = 1.0f);
    std::shared_ptr<Entity> createSphere(const std::string& name, float x = 0.0f, float y = 0.0f, float z = 0.0f, float radius = 0.5f);
    std::shared_ptr<Entity> createCylinder(const std::string& name, float x = 0.0f, float y = 0.0f, float z = 0.0f, float radius = 0.5f, float height = 1.0f);
    std::shared_ptr<Entity> createCone(const std::string& name, float x = 0.0f, float y = 0.0f, float z = 0.0f, float radius = 0.5f, float height = 1.0f);
    std::shared_ptr<Entity> createTorus(const std::string& name, float x = 0.0f, float y = 0.0f, float z = 0.0f, float mainRadius = 0.6f, float tubeRadius = 0.2f);
    std::shared_ptr<Entity> createPlane(const std::string& name, float x = 0.0f, float y = 0.0f, float z = 0.0f, float size = 10.0f);
    std::shared_ptr<Entity> createModel(const std::string& name, const std::string& modelPath, float x = 0.0f, float y = 0.0f, float z = 0.0f);

    // Consulta y gestión
    std::shared_ptr<Entity> getEntity(const std::string& name);
    bool removeEntity(const std::string& name);
    bool renameEntity(const std::string& oldName, const std::string& newName);
    bool renameGroup(const std::string& oldGroup, const std::string& newGroup);
    void clear();

    void addEntity(std::shared_ptr<Entity> entity) {
        if (!entity) return;
        m_entities[entity->getName()] = entity;
        if (std::find(m_entityOrder.begin(), m_entityOrder.end(), entity->getName()) == m_entityOrder.end()) {
            m_entityOrder.push_back(entity->getName());
        }
    }

    // Renderizado de todas las entidades activas de la escena (con soporte para OASIS Meshlets)
    void draw(const Shader& shader, const Camera* camera = nullptr,
              const OASISMeshletConfig* oasisConfig = nullptr,
              OASISMeshletTelemetry* oasisTelemetry = nullptr) const;

    // Actualizar animaciones y lógica de todas las entidades (Fase 13)
    void update(float dt) {
        for (auto& pair : m_entities) {
            if (pair.second) {
                pair.second->update(dt);
            }
        }
    }

    size_t getEntityCount() const { return m_entities.size(); }
    const std::vector<std::string>& getEntityOrder() const { return m_entityOrder; }
    const std::unordered_map<std::string, std::shared_ptr<Entity>>& getEntities() const { return m_entities; }
    std::unordered_map<std::string, std::shared_ptr<Entity>>& getEntitiesMutable() { return m_entities; }
    void setDefaultTexture(std::shared_ptr<Texture> tex) { m_defaultTexture = tex; }
    std::shared_ptr<Texture> getDefaultTexture() const { return m_defaultTexture; }

    // Gestión de Grupos / Jerarquía de Escena (Fase 16)
    std::vector<std::string> getGroupNames() const;
    std::vector<std::shared_ptr<Entity>> getEntitiesInGroup(const std::string& groupName) const;
    void setEntityGroup(const std::string& entityName, const std::string& groupName);
    void setGroupVisibility(const std::string& groupName, bool visible);
    void deleteGroup(const std::string& groupName, bool deleteEntities);
    void translateGroup(const std::string& groupName, const glm::vec3& delta);
    void scaleGroup(const std::string& groupName, const glm::vec3& scaleFactor);

    // Sistema de Luces Puntuales (Fase 10)
    std::vector<PointLight>& getPointLights() { return m_pointLights; }
    const std::vector<PointLight>& getPointLights() const { return m_pointLights; }
    void addPointLight(const PointLight& light) { m_pointLights.push_back(light); }
    void removePointLight(size_t index) {
        if (index < m_pointLights.size()) m_pointLights.erase(m_pointLights.begin() + index);
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Entity>> m_entities;
    std::vector<std::string> m_entityOrder;
    std::vector<PointLight> m_pointLights;

    // Caché de recursos compartidos para máximo rendimiento
    std::shared_ptr<Mesh> m_cubeMesh;
    std::shared_ptr<Mesh> m_sphereMesh;
    std::shared_ptr<Mesh> m_cylinderMesh;
    std::shared_ptr<Mesh> m_coneMesh;
    std::shared_ptr<Mesh> m_torusMesh;
    std::shared_ptr<Mesh> m_planeMesh;
    std::shared_ptr<Texture> m_defaultTexture;
    std::unordered_map<std::string, std::shared_ptr<Model>> m_modelCache;
};

} // namespace ASIS
