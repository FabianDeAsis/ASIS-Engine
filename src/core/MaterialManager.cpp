#include "MaterialManager.h"
#include "Entity.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <iomanip>

namespace ASIS {

MaterialManager& MaterialManager::get() {
    static MaterialManager instance;
    return instance;
}

void MaterialManager::init() {
    m_materials.clear();
    m_materialMap.clear();

    // 1. Registrar Presets Físicos PBR de Alta Fidelidad
    registerPreset("Oro Pulido", glm::vec3(1.0f, 0.766f, 0.336f), 1.0f, 0.12f, 1.0f);
    registerPreset("Cobre", glm::vec3(0.955f, 0.637f, 0.538f), 1.0f, 0.18f, 1.0f);
    registerPreset("Acero Inoxidable", glm::vec3(0.56f, 0.57f, 0.58f), 0.95f, 0.28f, 0.9f);
    registerPreset("Cromo / Espejo", glm::vec3(0.95f, 0.95f, 0.95f), 1.0f, 0.04f, 1.0f);
    registerPreset("Vidrio / Cristal", glm::vec3(0.92f, 0.95f, 0.98f), 0.05f, 0.04f, 1.0f);
    registerPreset("Plastico Brillante", glm::vec3(0.85f, 0.15f, 0.15f), 0.0f, 0.08f, 0.5f);
    registerPreset("Plastico Mate", glm::vec3(0.20f, 0.20f, 0.22f), 0.0f, 0.75f, 0.2f);
    registerPreset("Neon Cian Emisivo", glm::vec3(0.05f, 0.85f, 1.0f), 0.0f, 0.2f, 1.0f, glm::vec3(0.05f, 0.85f, 1.0f), 5.0f);
    registerPreset("Neon Carmesi", glm::vec3(1.0f, 0.08f, 0.18f), 0.0f, 0.2f, 1.0f, glm::vec3(1.0f, 0.08f, 0.18f), 6.0f);
    registerPreset("Obsidiana Negra", glm::vec3(0.08f, 0.08f, 0.09f), 0.1f, 0.06f, 0.9f);
    registerPreset("Marmol Blanco", glm::vec3(0.92f, 0.92f, 0.90f), 0.0f, 0.25f, 0.4f);
    registerPreset("Hormigon / Concreto", glm::vec3(0.55f, 0.55f, 0.54f), 0.0f, 0.90f, 0.1f);
    registerPreset("Hierro Oxidado", glm::vec3(0.50f, 0.25f, 0.15f), 0.4f, 0.82f, 0.2f);
    registerPreset("Madera / Corteza", glm::vec3(0.42f, 0.24f, 0.12f), 0.0f, 0.60f, 0.25f);

    // 2. Escanear directorio assets/materials y cargar materiales guardados
    scanMaterialsDirectory("assets/materials");

    std::cout << "[MaterialManager] Inicializado con " << m_materials.size() << " materiales PBR disponibles.\n";
}

void MaterialManager::shutdown() {
    m_materials.clear();
    m_materialMap.clear();
}

void MaterialManager::registerPreset(const std::string& name, const glm::vec3& albedo, float metallic,
                                     float roughness, float specular, const glm::vec3& emission,
                                     float emissionStrength) {
    auto asset = std::make_shared<MaterialAsset>(name);
    asset->material->albedo = albedo;
    asset->material->metallic = metallic;
    asset->material->roughness = roughness;
    asset->material->specular = specular;
    asset->material->emission = emission;
    asset->material->emissionStrength = emissionStrength;
    asset->material->ao = 1.0f;
    asset->material->normalStrength = 1.0f;

    m_materials.push_back(asset);
    m_materialMap[name] = asset;
}

std::shared_ptr<MaterialAsset> MaterialManager::createMaterial(const std::string& name) {
    std::string uniqueName = name;
    int counter = 1;
    while (m_materialMap.find(uniqueName) != m_materialMap.end()) {
        uniqueName = name + "_" + std::to_string(counter++);
    }

    auto asset = std::make_shared<MaterialAsset>(uniqueName);
    m_materials.push_back(asset);
    m_materialMap[uniqueName] = asset;
    return asset;
}

std::shared_ptr<MaterialAsset> MaterialManager::duplicateMaterial(const std::string& sourceName, const std::string& newName) {
    auto src = getMaterial(sourceName);
    if (!src) return nullptr;

    std::string targetName = newName.empty() ? (src->name + "_Copia") : newName;
    auto clone = createMaterial(targetName);

    // Copiar propiedades
    clone->material->albedo = src->material->albedo;
    clone->material->metallic = src->material->metallic;
    clone->material->roughness = src->material->roughness;
    clone->material->specular = src->material->specular;
    clone->material->emission = src->material->emission;
    clone->material->emissionStrength = src->material->emissionStrength;
    clone->material->normalStrength = src->material->normalStrength;
    clone->material->ao = src->material->ao;

    // Copiar rutas y referencias de textura
    clone->albedoPath = src->albedoPath;
    clone->normalPath = src->normalPath;
    clone->metallicPath = src->metallicPath;
    clone->roughnessPath = src->roughnessPath;
    clone->specularPath = src->specularPath;
    clone->emissionPath = src->emissionPath;
    clone->aoPath = src->aoPath;

    clone->material->albedoMap = src->material->albedoMap;
    clone->material->normalMap = src->material->normalMap;
    clone->material->metallicMap = src->material->metallicMap;
    clone->material->roughnessMap = src->material->roughnessMap;
    clone->material->specularMap = src->material->specularMap;
    clone->material->emissionMap = src->material->emissionMap;
    clone->material->aoMap = src->material->aoMap;

    return clone;
}

bool MaterialManager::removeMaterial(const std::string& name) {
    auto it = m_materialMap.find(name);
    if (it == m_materialMap.end()) return false;

    for (auto vecIt = m_materials.begin(); vecIt != m_materials.end(); ++vecIt) {
        if ((*vecIt)->name == name) {
            m_materials.erase(vecIt);
            break;
        }
    }
    m_materialMap.erase(it);
    return true;
}

std::shared_ptr<MaterialAsset> MaterialManager::getMaterial(const std::string& name) {
    auto it = m_materialMap.find(name);
    if (it != m_materialMap.end()) {
        return it->second;
    }
    return nullptr;
}

bool MaterialManager::applyToEntity(Entity& entity, const std::string& materialName) {
    auto mat = getMaterial(materialName);
    if (!mat) return false;
    return applyToEntity(entity, mat);
}

bool MaterialManager::applyToEntity(Entity& entity, std::shared_ptr<MaterialAsset> asset) {
    if (!asset || !asset->material) return false;

    // Crear una instancia copiada o compartida para la entidad
    auto entMat = std::make_shared<Material>(*asset->material);
    entity.setMaterial(entMat);
    entity.setColor(asset->material->albedo.r, asset->material->albedo.g, asset->material->albedo.b);
    return true;
}

std::shared_ptr<MaterialAsset> MaterialManager::extractFromEntity(const Entity& entity, const std::string& assetName) {
    auto entMat = entity.getMaterial();
    if (!entMat) return nullptr;

    auto asset = createMaterial(assetName.empty() ? (entity.getName() + "_Material") : assetName);
    *asset->material = *entMat;
    return asset;
}

bool MaterialManager::saveMaterialToFile(const MaterialAsset& asset, const std::string& filePath) {
    std::filesystem::path dir = std::filesystem::path(filePath).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[MaterialManager ERROR] No se pudo escribir archivo de material: " << filePath << "\n";
        return false;
    }

    const auto& m = *asset.material;

    file << "{\n";
    file << "  \"asis_material_version\": \"1.0\",\n";
    file << "  \"name\": \"" << asset.name << "\",\n";
    file << "  \"albedo\": [" << m.albedo.r << ", " << m.albedo.g << ", " << m.albedo.b << "],\n";
    file << "  \"metallic\": " << m.metallic << ",\n";
    file << "  \"roughness\": " << m.roughness << ",\n";
    file << "  \"specular\": " << m.specular << ",\n";
    file << "  \"emission\": [" << m.emission.r << ", " << m.emission.g << ", " << m.emission.b << "],\n";
    file << "  \"emissionStrength\": " << m.emissionStrength << ",\n";
    file << "  \"normalStrength\": " << m.normalStrength << ",\n";
    file << "  \"ao\": " << m.ao << ",\n";
    file << "  \"textures\": {\n";
    file << "    \"albedo\": \"" << asset.albedoPath << "\",\n";
    file << "    \"normal\": \"" << asset.normalPath << "\",\n";
    file << "    \"metallic\": \"" << asset.metallicPath << "\",\n";
    file << "    \"roughness\": \"" << asset.roughnessPath << "\",\n";
    file << "    \"specular\": \"" << asset.specularPath << "\",\n";
    file << "    \"emission\": \"" << asset.emissionPath << "\",\n";
    file << "    \"ao\": \"" << asset.aoPath << "\"\n";
    file << "  }\n";
    file << "}\n";

    file.close();
    std::cout << "[MaterialManager] ✓ Material guardado exitosamente: " << filePath << "\n";
    return true;
}

std::shared_ptr<MaterialAsset> MaterialManager::loadMaterialFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return nullptr;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    auto extractString = [&](const std::string& key, const std::string& defVal) -> std::string {
        size_t p = content.find("\"" + key + "\"");
        if (p == std::string::npos) return defVal;
        size_t c = content.find(':', p);
        if (c == std::string::npos) return defVal;
        size_t q1 = content.find('\"', c + 1);
        if (q1 == std::string::npos) return defVal;
        size_t q2 = content.find('\"', q1 + 1);
        if (q2 == std::string::npos) return defVal;
        return content.substr(q1 + 1, q2 - q1 - 1);
    };

    auto extractFloat = [&](const std::string& key, float defVal) -> float {
        size_t p = content.find("\"" + key + "\"");
        if (p == std::string::npos) return defVal;
        size_t c = content.find(':', p);
        if (c == std::string::npos) return defVal;
        try {
            return std::stof(content.substr(c + 1));
        } catch (...) {
            return defVal;
        }
    };

    auto extractVec3 = [&](const std::string& key, const glm::vec3& defVal) -> glm::vec3 {
        size_t p = content.find("\"" + key + "\"");
        if (p == std::string::npos) return defVal;
        size_t b1 = content.find('[', p);
        size_t b2 = content.find(']', b1);
        if (b1 == std::string::npos || b2 == std::string::npos) return defVal;
        std::string arr = content.substr(b1 + 1, b2 - b1 - 1);
        std::stringstream ss(arr);
        std::string tok;
        glm::vec3 val = defVal;
        int i = 0;
        while (std::getline(ss, tok, ',') && i < 3) {
            try {
                val[i++] = std::stof(tok);
            } catch (...) {}
        }
        return val;
    };

    std::string name = extractString("name", std::filesystem::path(filePath).stem().string());
    auto asset = createMaterial(name);
    asset->filePath = filePath;

    asset->material->albedo = extractVec3("albedo", glm::vec3(1.0f));
    asset->material->metallic = extractFloat("metallic", 0.0f);
    asset->material->roughness = extractFloat("roughness", 0.5f);
    asset->material->specular = extractFloat("specular", 0.5f);
    asset->material->emission = extractVec3("emission", glm::vec3(0.0f));
    asset->material->emissionStrength = extractFloat("emissionStrength", 0.0f);
    asset->material->normalStrength = extractFloat("normalStrength", 1.0f);
    asset->material->ao = extractFloat("ao", 1.0f);

    // Extraer rutas de texturas
    asset->albedoPath = extractString("albedo", "");
    asset->normalPath = extractString("normal", "");
    asset->metallicPath = extractString("metallic", "");
    asset->roughnessPath = extractString("roughness", "");
    asset->specularPath = extractString("specular", "");
    asset->emissionPath = extractString("emission", "");
    asset->aoPath = extractString("ao", "");

    if (!asset->albedoPath.empty()) setTextureSlot(*asset, 0, asset->albedoPath);
    if (!asset->normalPath.empty()) setTextureSlot(*asset, 1, asset->normalPath);
    if (!asset->metallicPath.empty()) setTextureSlot(*asset, 2, asset->metallicPath);
    if (!asset->roughnessPath.empty()) setTextureSlot(*asset, 3, asset->roughnessPath);
    if (!asset->specularPath.empty()) setTextureSlot(*asset, 4, asset->specularPath);
    if (!asset->emissionPath.empty()) setTextureSlot(*asset, 5, asset->emissionPath);
    if (!asset->aoPath.empty()) setTextureSlot(*asset, 6, asset->aoPath);

    std::cout << "[MaterialManager] ✓ Material cargado: " << name << " (" << filePath << ")\n";
    return asset;
}

void MaterialManager::scanMaterialsDirectory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
        // Guardar automáticamente los presets iniciales en disco para facilitar compartirlos
        for (const auto& asset : m_materials) {
            std::string fname = asset->name;
            for (char& c : fname) {
                if (c == ' ' || c == '/' || c == '(' || c == ')') c = '_';
            }
            std::string path = directory + "/" + fname + ".asismat";
            saveMaterialToFile(*asset, path);
            asset->filePath = path;
        }
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".asismat") {
            loadMaterialFromFile(entry.path().string());
        }
    }
}

void MaterialManager::setTextureSlot(MaterialAsset& asset, int slot, const std::string& texturePath) {
    if (texturePath.empty()) return;
    if (!std::filesystem::exists(texturePath)) return;

    try {
        auto tex = std::make_shared<Texture>(texturePath);
        switch (slot) {
            case 0: asset.material->albedoMap = tex; asset.albedoPath = texturePath; break;
            case 1: asset.material->normalMap = tex; asset.normalPath = texturePath; break;
            case 2: asset.material->metallicMap = tex; asset.metallicPath = texturePath; break;
            case 3: asset.material->roughnessMap = tex; asset.roughnessPath = texturePath; break;
            case 4: asset.material->specularMap = tex; asset.specularPath = texturePath; break;
            case 5: asset.material->emissionMap = tex; asset.emissionPath = texturePath; break;
            case 6: asset.material->aoMap = tex; asset.aoPath = texturePath; break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[MaterialManager ADVERTENCIA] No se pudo cargar textura: " << texturePath << " (" << e.what() << ")\n";
    }
}

} // namespace ASIS
