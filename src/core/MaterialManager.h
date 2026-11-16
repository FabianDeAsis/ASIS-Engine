#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Material.h"

namespace ASIS {

class Entity;

/**
 * @brief Descriptor de un Activo de Material PBR (.asismat)
 */
struct MaterialAsset {
    std::string name;
    std::string filePath;
    std::shared_ptr<Material> material;

    // Rutas relativas de mapas de texturas para persistencia
    std::string albedoPath;
    std::string normalPath;
    std::string metallicPath;
    std::string roughnessPath;
    std::string specularPath;
    std::string emissionPath;
    std::string aoPath;

    MaterialAsset(const std::string& n = "Nuevo Material")
        : name(n), material(std::make_shared<Material>()) {}
};

/**
 * @brief Gestor Central de Materiales PBR y Biblioteca de Presets Físicos (Fase 21)
 * 
 * Permite crear, guardar (.asismat), cargar, clonar y asignar materiales PBR
 * a entidades individuales o múltiples en tiempo real.
 */
class MaterialManager {
public:
    static MaterialManager& get();

    void init();
    void shutdown();

    // Creación y Gestión
    std::shared_ptr<MaterialAsset> createMaterial(const std::string& name);
    std::shared_ptr<MaterialAsset> duplicateMaterial(const std::string& sourceName, const std::string& newName = "");
    bool removeMaterial(const std::string& name);
    std::shared_ptr<MaterialAsset> getMaterial(const std::string& name);
    const std::vector<std::shared_ptr<MaterialAsset>>& getMaterials() const { return m_materials; }

    // Asignación
    bool applyToEntity(Entity& entity, const std::string& materialName);
    bool applyToEntity(Entity& entity, std::shared_ptr<MaterialAsset> asset);
    std::shared_ptr<MaterialAsset> extractFromEntity(const Entity& entity, const std::string& assetName);

    // Persistencia (.asismat en formato JSON legible)
    bool saveMaterialToFile(const MaterialAsset& asset, const std::string& filePath);
    std::shared_ptr<MaterialAsset> loadMaterialFromFile(const std::string& filePath);
    void scanMaterialsDirectory(const std::string& directory = "assets/materials");

    // Asignación rápida de texturas con resolución de ruta
    void setTextureSlot(MaterialAsset& asset, int slot, const std::string& texturePath);

private:
    MaterialManager() = default;
    ~MaterialManager() = default;

    void registerPreset(const std::string& name, const glm::vec3& albedo, float metallic, float roughness,
                        float specular = 0.5f, const glm::vec3& emission = glm::vec3(0.0f),
                        float emissionStrength = 0.0f);

    std::vector<std::shared_ptr<MaterialAsset>> m_materials;
    std::unordered_map<std::string, std::shared_ptr<MaterialAsset>> m_materialMap;
};

} // namespace ASIS
