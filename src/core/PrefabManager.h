#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Scene.h"
#include "Entity.h"

namespace ASIS {

class PhysicsWorld;

struct PrefabEntityData {
    std::string name;
    std::string primitiveType = "Cube"; // "Cube", "Sphere", "Cylinder", "Cone", "Torus", "Plane", "Model"
    std::string modelPath;
    std::string group;
    glm::vec3 relativePosition = glm::vec3(0.0f);
    glm::vec3 relativeRotation = glm::vec3(0.0f);
    glm::vec3 relativeScale = glm::vec3(1.0f);
    glm::vec3 color = glm::vec3(1.0f);
    bool hasCustomColor = false;

    // Material PBR
    float roughness = 0.5f;
    float metallic = 0.0f;
    float specular = 0.5f;
    float normalStrength = 1.0f;
    float ambientOcclusion = 1.0f;
    glm::vec3 emissionColor = glm::vec3(0.0f);
    float emissionIntensity = 1.0f;

    // Física y Trigger (Fase 17)
    bool hasPhysics = false;
    float mass = 1.0f;
    std::string colliderType = "BOX"; // "BOX", "SPHERE"
    bool isTrigger = false;
    std::string triggerTag = "Default";
};

class PrefabManager {
public:
    // Guardar una o más entidades como un archivo .prefab (formato JSON)
    static bool savePrefab(const std::string& filePath, const std::vector<std::shared_ptr<Entity>>& entities, const PhysicsWorld* physicsWorld = nullptr);

    // Guardar entidad individual como prefab en assets/prefabs/<name>.prefab
    static bool saveEntityAsPrefab(const std::shared_ptr<Entity>& entity, const std::string& prefabName, const PhysicsWorld* physicsWorld = nullptr);

    // Instanciar un prefab en la escena en una posición del mundo
    static std::vector<std::shared_ptr<Entity>> instantiatePrefab(
        const std::string& filePathOrName,
        Scene& scene,
        PhysicsWorld* physicsWorld,
        const glm::vec3& spawnPosition = glm::vec3(0.0f),
        const glm::vec3& spawnRotation = glm::vec3(0.0f),
        const glm::vec3& spawnScale = glm::vec3(1.0f)
    );

    // Listar todos los archivos .prefab disponibles en el directorio de assets
    static std::vector<std::string> getAvailablePrefabs(const std::string& directory = "assets/prefabs");

    // Crear prefabs por defecto (Moneda Coleccionable, Caja de Madera, Portal, Barril Explosivo)
    static void createDefaultPrefabs(const std::string& directory = "assets/prefabs");
};

} // namespace ASIS
