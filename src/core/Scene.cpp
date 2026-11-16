#include "Scene.h"
#include <iostream>
#include <algorithm>

namespace ASIS {

Scene::Scene() {
    // Instanciar mallas de primitivas compartidas para máximo rendimiento
    m_cubeMesh = Mesh::createCube();
    m_sphereMesh = Mesh::createSphere();
    m_cylinderMesh = Mesh::createCylinder();
    m_coneMesh = Mesh::createCone();
    m_torusMesh = Mesh::createTorus();
    m_planeMesh = Mesh::createPlane(10.0f);
}

std::shared_ptr<Entity> Scene::createCube(const std::string& name, float x, float y, float z, float size) {
    auto entity = std::make_shared<Entity>(name);
    entity->setPosition(x, y, z);
    entity->setScale(size, size, size);
    entity->setPrimitiveType("Cube");
    entity->setMesh(m_cubeMesh);

    if (m_defaultTexture) {
        entity->setTexture(m_defaultTexture);
    }

    m_entities[name] = entity;
    if (std::find(m_entityOrder.begin(), m_entityOrder.end(), name) == m_entityOrder.end()) {
        m_entityOrder.push_back(name);
    }

    std::cout << "[ASIS Scene] Cubo '" << name << "' creado en (" << x << ", " << y << ", " << z << ")\n";
    return entity;
}

std::shared_ptr<Entity> Scene::createSphere(const std::string& name, float x, float y, float z, float radius) {
    auto entity = std::make_shared<Entity>(name);
    entity->setPosition(x, y, z);
    entity->setScale(radius * 2.0f, radius * 2.0f, radius * 2.0f);
    entity->setPrimitiveType("Sphere");
    entity->setMesh(m_sphereMesh);

    if (m_defaultTexture) {
        entity->setTexture(m_defaultTexture);
    }

    m_entities[name] = entity;
    if (std::find(m_entityOrder.begin(), m_entityOrder.end(), name) == m_entityOrder.end()) {
        m_entityOrder.push_back(name);
    }

    std::cout << "[ASIS Scene] Esfera '" << name << "' creada en (" << x << ", " << y << ", " << z << ")\n";
    return entity;
}

std::shared_ptr<Entity> Scene::createCylinder(const std::string& name, float x, float y, float z, float radius, float height) {
    auto entity = std::make_shared<Entity>(name);
    entity->setPosition(x, y, z);
    entity->setScale(radius * 2.0f, height, radius * 2.0f);
    entity->setPrimitiveType("Cylinder");
    entity->setMesh(m_cylinderMesh);

    if (m_defaultTexture) {
        entity->setTexture(m_defaultTexture);
    }

    m_entities[name] = entity;
    if (std::find(m_entityOrder.begin(), m_entityOrder.end(), name) == m_entityOrder.end()) {
        m_entityOrder.push_back(name);
    }

    std::cout << "[ASIS Scene] Cilindro '" << name << "' creado en (" << x << ", " << y << ", " << z << ")\n";
    return entity;
}

std::shared_ptr<Entity> Scene::createCone(const std::string& name, float x, float y, float z, float radius, float height) {
    auto entity = std::make_shared<Entity>(name);
    entity->setPosition(x, y, z);
    entity->setScale(radius * 2.0f, height, radius * 2.0f);
    entity->setPrimitiveType("Cone");
    entity->setMesh(m_coneMesh);

    if (m_defaultTexture) {
        entity->setTexture(m_defaultTexture);
    }

    m_entities[name] = entity;
    if (std::find(m_entityOrder.begin(), m_entityOrder.end(), name) == m_entityOrder.end()) {
        m_entityOrder.push_back(name);
    }

    std::cout << "[ASIS Scene] Cono '" << name << "' creado en (" << x << ", " << y << ", " << z << ")\n";
    return entity;
}

std::shared_ptr<Entity> Scene::createTorus(const std::string& name, float x, float y, float z, float mainRadius, float tubeRadius) {
    auto entity = std::make_shared<Entity>(name);
    entity->setPosition(x, y, z);
    entity->setScale(mainRadius, mainRadius, mainRadius);
    entity->setPrimitiveType("Torus");
    entity->setMesh(m_torusMesh);

    if (m_defaultTexture) {
        entity->setTexture(m_defaultTexture);
    }

    m_entities[name] = entity;
    if (std::find(m_entityOrder.begin(), m_entityOrder.end(), name) == m_entityOrder.end()) {
        m_entityOrder.push_back(name);
    }

    std::cout << "[ASIS Scene] Toroide '" << name << "' creado en (" << x << ", " << y << ", " << z << ")\n";
    return entity;
}

std::shared_ptr<Entity> Scene::createPlane(const std::string& name, float x, float y, float z, float size) {
    auto entity = std::make_shared<Entity>(name);
    entity->setPosition(x, y, z);
    entity->setScale(size * 0.1f, 1.0f, size * 0.1f);
    entity->setPrimitiveType("Plane");
    entity->setMesh(m_planeMesh);

    if (m_defaultTexture) {
        entity->setTexture(m_defaultTexture);
    }

    m_entities[name] = entity;
    if (std::find(m_entityOrder.begin(), m_entityOrder.end(), name) == m_entityOrder.end()) {
        m_entityOrder.push_back(name);
    }

    std::cout << "[ASIS Scene] Plano '" << name << "' creado en (" << x << ", " << y << ", " << z << ")\n";
    return entity;
}

std::shared_ptr<Entity> Scene::createModel(const std::string& name, const std::string& modelPath, float x, float y, float z) {
    auto entity = std::make_shared<Entity>(name);
    entity->setPosition(x, y, z);
    entity->setPrimitiveType("Model");
    entity->setModelPath(modelPath);

    // Reutilizar modelo de la caché si ya fue cargado
    if (m_modelCache.find(modelPath) == m_modelCache.end()) {
        try {
            m_modelCache[modelPath] = std::make_shared<Model>(modelPath);
            if (m_defaultTexture) {
                m_modelCache[modelPath]->addTexture(m_defaultTexture);
            }
        } catch (const std::exception& e) {
            std::cerr << "[ASIS Scene ERROR] Falló al cargar modelo para entidad '" << name << "': " << e.what() << "\n";
            return nullptr;
        }
    }

    entity->setModel(m_modelCache[modelPath]);
    if (m_defaultTexture) {
        entity->setTexture(m_defaultTexture);
    }

    m_entities[name] = entity;
    if (std::find(m_entityOrder.begin(), m_entityOrder.end(), name) == m_entityOrder.end()) {
        m_entityOrder.push_back(name);
    }

    std::cout << "[ASIS Scene] Modelo 3D '" << name << "' cargado desde " << modelPath << "\n";
    return entity;
}

std::shared_ptr<Entity> Scene::getEntity(const std::string& name) {
    auto it = m_entities.find(name);
    if (it != m_entities.end()) {
        return it->second;
    }
    return nullptr;
}

bool Scene::removeEntity(const std::string& name) {
    auto it = m_entities.find(name);
    if (it != m_entities.end()) {
        m_entities.erase(it);
        m_entityOrder.erase(std::remove(m_entityOrder.begin(), m_entityOrder.end(), name), m_entityOrder.end());
        std::cout << "[ASIS Scene] Entidad '" << name << "' eliminada de la escena.\n";
        return true;
    }
    return false;
}

bool Scene::renameEntity(const std::string& oldName, const std::string& newName) {
    if (oldName == newName || newName.empty()) return false;
    if (m_entities.find(newName) != m_entities.end()) return false;

    auto it = m_entities.find(oldName);
    if (it == m_entities.end()) return false;

    auto ent = it->second;
    m_entities.erase(it);
    ent->setName(newName);
    m_entities[newName] = ent;

    for (auto& n : m_entityOrder) {
        if (n == oldName) {
            n = newName;
            break;
        }
    }
    return true;
}

bool Scene::renameGroup(const std::string& oldGroup, const std::string& newGroup) {
    if (oldGroup == newGroup || newGroup.empty()) return false;
    auto ents = getEntitiesInGroup(oldGroup);
    if (ents.empty()) return false;

    for (auto& ent : ents) {
        if (ent) ent->setGroup(newGroup);
    }
    return true;
}

void Scene::clear() {
    m_entities.clear();
    m_entityOrder.clear();
    std::cout << "[ASIS Scene] Escena limpiada por completo.\n";
}

void Scene::draw(const Shader& shader, const Camera* camera,
                 const OASISMeshletConfig* oasisConfig,
                 OASISMeshletTelemetry* oasisTelemetry) const {
    if (oasisTelemetry) {
        oasisTelemetry->reset();
    }
    for (const auto& name : m_entityOrder) {
        auto it = m_entities.find(name);
        if (it != m_entities.end() && it->second) {
            it->second->draw(shader, camera, oasisConfig, oasisTelemetry);
        }
    }
}

std::vector<std::string> Scene::getGroupNames() const {
    std::vector<std::string> groups;
    for (const auto& name : m_entityOrder) {
        auto it = m_entities.find(name);
        if (it != m_entities.end() && it->second) {
            const std::string& grp = it->second->getGroup();
            if (!grp.empty() && std::find(groups.begin(), groups.end(), grp) == groups.end()) {
                groups.push_back(grp);
            }
        }
    }
    return groups;
}

std::vector<std::shared_ptr<Entity>> Scene::getEntitiesInGroup(const std::string& groupName) const {
    std::vector<std::shared_ptr<Entity>> result;
    for (const auto& name : m_entityOrder) {
        auto it = m_entities.find(name);
        if (it != m_entities.end() && it->second) {
            if (it->second->getGroup() == groupName) {
                result.push_back(it->second);
            }
        }
    }
    return result;
}

void Scene::setEntityGroup(const std::string& entityName, const std::string& groupName) {
    auto ent = getEntity(entityName);
    if (ent) {
        ent->setGroup(groupName);
    }
}

void Scene::setGroupVisibility(const std::string& groupName, bool visible) {
    for (auto ent : getEntitiesInGroup(groupName)) {
        if (ent) ent->setVisible(visible);
    }
}

void Scene::deleteGroup(const std::string& groupName, bool deleteEntities) {
    auto ents = getEntitiesInGroup(groupName);
    for (auto ent : ents) {
        if (ent) {
            if (deleteEntities) {
                removeEntity(ent->getName());
            } else {
                ent->setGroup("");
            }
        }
    }
}

void Scene::translateGroup(const std::string& groupName, const glm::vec3& delta) {
    for (auto ent : getEntitiesInGroup(groupName)) {
        if (ent) ent->move(delta.x, delta.y, delta.z);
    }
}

void Scene::scaleGroup(const std::string& groupName, const glm::vec3& scaleFactor) {
    auto ents = getEntitiesInGroup(groupName);
    if (ents.empty()) return;

    // Calcular centro del grupo
    glm::vec3 center(0.0f);
    for (auto ent : ents) {
        center += ent->getPosition();
    }
    center /= static_cast<float>(ents.size());

    // Escalar posiciones relativas al centro y dimensiones de cada entidad
    for (auto ent : ents) {
        glm::vec3 relPos = ent->getPosition() - center;
        ent->setPosition(center.x + relPos.x * scaleFactor.x,
                         center.y + relPos.y * scaleFactor.y,
                         center.z + relPos.z * scaleFactor.z);
        glm::vec3 curScale = ent->getScale();
        ent->setScale(curScale.x * scaleFactor.x,
                      curScale.y * scaleFactor.y,
                      curScale.z * scaleFactor.z);
    }
}

} // namespace ASIS
