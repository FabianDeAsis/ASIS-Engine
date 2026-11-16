#include "PrefabManager.h"
#include "PhysicsWorld.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace ASIS {

bool PrefabManager::savePrefab(const std::string& filePath, const std::vector<std::shared_ptr<Entity>>& entities, const PhysicsWorld* physicsWorld) {
    if (entities.empty()) return false;

    // Crear directorio padre si no existe
    std::filesystem::path p(filePath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ASIS PrefabManager ERROR] No se pudo crear el archivo prefab en: " << filePath << std::endl;
        return false;
    }

    glm::vec3 rootPos = entities[0]->getPosition();

    file << "{\n";
    file << "  \"asis_prefab_version\": \"1.0\",\n";
    file << "  \"name\": \"" << p.stem().string() << "\",\n";
    file << "  \"entities\": [\n";

    for (size_t i = 0; i < entities.size(); ++i) {
        const auto& ent = entities[i];
        glm::vec3 relPos = (i == 0) ? glm::vec3(0.0f) : (ent->getPosition() - rootPos);
        glm::vec3 rot = ent->getRotation();
        glm::vec3 sc = ent->getScale();
        glm::vec3 col = ent->getColor();

        file << "    {\n";
        file << "      \"name\": \"" << ent->getName() << "\",\n";
        file << "      \"type\": \"" << (ent->getPrimitiveType().empty() ? "Cube" : ent->getPrimitiveType()) << "\",\n";
        file << "      \"modelPath\": \"" << ent->getModelPath() << "\",\n";
        file << "      \"group\": \"" << ent->getGroup() << "\",\n";
        file << "      \"position\": [" << relPos.x << ", " << relPos.y << ", " << relPos.z << "],\n";
        file << "      \"rotation\": [" << rot.x << ", " << rot.y << ", " << rot.z << "],\n";
        file << "      \"scale\": [" << sc.x << ", " << sc.y << ", " << sc.z << "],\n";
        file << "      \"color\": [" << col.r << ", " << col.g << ", " << col.b << "],\n";
        file << "      \"hasCustomColor\": " << (ent->hasCustomColor() ? "true" : "false") << ",\n";

        // Material PBR
        auto mat = ent->getMaterial();
        if (mat) {
            file << "      \"material\": {\n";
            file << "        \"roughness\": " << mat->roughness << ",\n";
            file << "        \"metallic\": " << mat->metallic << ",\n";
            file << "        \"specular\": " << mat->specular << ",\n";
            file << "        \"normalStrength\": " << mat->normalStrength << ",\n";
            file << "        \"ambientOcclusion\": " << mat->ao << ",\n";
            glm::vec3 emCol = mat->emission;
            file << "        \"emissionColor\": [" << emCol.r << ", " << emCol.g << ", " << emCol.b << "],\n";
            file << "        \"emissionIntensity\": " << mat->emissionStrength << "\n";
            file << "      },\n";
        } else {
            file << "      \"material\": {\n";
            file << "        \"roughness\": 0.5,\n";
            file << "        \"metallic\": 0.0,\n";
            file << "        \"specular\": 0.5,\n";
            file << "        \"normalStrength\": 1.0,\n";
            file << "        \"ambientOcclusion\": 1.0,\n";
            file << "        \"emissionColor\": [0.0, 0.0, 0.0],\n";
            file << "        \"emissionIntensity\": 1.0\n";
            file << "      },\n";
        }

        // Física y Trigger
        bool hasPhys = ent->hasPhysics();
        float mass = ent->getMass();
        std::string colliderType = ent->getColliderType().empty() ? "BOX" : ent->getColliderType();
        if (physicsWorld && physicsWorld->hasRigidBody(ent->getName())) {
            hasPhys = true;
            auto* comp = const_cast<PhysicsWorld*>(physicsWorld)->getRigidBodyComponent(ent->getName());
            if (comp) {
                mass = comp->mass;
                if (comp->colliderType == ColliderType::SPHERE) colliderType = "SPHERE";
                else if (comp->colliderType == ColliderType::PLANE) colliderType = "PLANE";
                else colliderType = "BOX";
            }
        }

        file << "      \"physics\": {\n";
        file << "        \"hasPhysics\": " << (hasPhys ? "true" : "false") << ",\n";
        file << "        \"mass\": " << mass << ",\n";
        file << "        \"colliderType\": \"" << colliderType << "\",\n";
        file << "        \"isTrigger\": " << (ent->isTrigger() ? "true" : "false") << ",\n";
        file << "        \"triggerTag\": \"" << ent->getTriggerTag() << "\"\n";
        file << "      }\n";

        file << "    }" << (i + 1 < entities.size() ? "," : "") << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
    std::cout << "[ASIS PrefabManager] Prefab guardado exitosamente: " << filePath << std::endl;
    return true;
}

bool PrefabManager::saveEntityAsPrefab(const std::shared_ptr<Entity>& entity, const std::string& prefabName, const PhysicsWorld* physicsWorld) {
    if (!entity) return false;
    std::string safeName = prefabName.empty() ? entity->getName() : prefabName;
    std::string path = "assets/prefabs/" + safeName + ".prefab";
    return savePrefab(path, { entity }, physicsWorld);
}

std::vector<std::shared_ptr<Entity>> PrefabManager::instantiatePrefab(
    const std::string& filePathOrName,
    Scene& scene,
    PhysicsWorld* physicsWorld,
    const glm::vec3& spawnPosition,
    const glm::vec3& spawnRotation,
    const glm::vec3& spawnScale
) {
    std::vector<std::shared_ptr<Entity>> result;

    std::string actualPath = filePathOrName;
    if (!std::filesystem::exists(actualPath)) {
        actualPath = "assets/prefabs/" + filePathOrName;
        if (!std::filesystem::exists(actualPath) && actualPath.rfind(".prefab") == std::string::npos) {
            actualPath += ".prefab";
        }
    }

    std::ifstream file(actualPath);
    if (!file.is_open()) {
        std::cerr << "[ASIS PrefabManager ERROR] No se encontro el archivo prefab en: " << filePathOrName << std::endl;
        return result;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (content.find("asis_prefab_version") == std::string::npos) {
        std::cerr << "[ASIS PrefabManager ERROR] Formato invalido en prefab: " << actualPath << std::endl;
        return result;
    }

    std::filesystem::path p(actualPath);
    std::string prefabStem = p.stem().string();

    // Helper lambdas para parsear campos JSON simples
    auto extractStr = [](const std::string& block, const std::string& key, const std::string& defaultVal) -> std::string {
        size_t pos = block.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = block.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        pos = block.find('\"', pos);
        if (pos == std::string::npos) return defaultVal;
        size_t end = block.find('\"', pos + 1);
        if (end == std::string::npos) return defaultVal;
        return block.substr(pos + 1, end - pos - 1);
    };

    auto extractFloat = [](const std::string& block, const std::string& key, float defaultVal) -> float {
        size_t pos = block.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = block.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        pos = block.find_first_of("-0123456789.", pos);
        if (pos == std::string::npos) return defaultVal;
        size_t end = block.find_first_not_of("-0123456789.eE", pos);
        try {
            return std::stof(block.substr(pos, end - pos));
        } catch (...) {
            return defaultVal;
        }
    };

    auto extractBool = [](const std::string& block, const std::string& key, bool defaultVal) -> bool {
        size_t pos = block.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = block.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        size_t t = block.find("true", pos);
        size_t f = block.find("false", pos);
        if (t != std::string::npos && (f == std::string::npos || t < f)) return true;
        if (f != std::string::npos) return false;
        return defaultVal;
    };

    auto extractVec3 = [](const std::string& block, const std::string& key, glm::vec3 defaultVal) -> glm::vec3 {
        size_t pos = block.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = block.find('[', pos);
        if (pos == std::string::npos) return defaultVal;
        size_t end = block.find(']', pos);
        if (end == std::string::npos) return defaultVal;
        std::string arr = block.substr(pos + 1, end - pos - 1);
        std::stringstream ss(arr);
        std::string item;
        float vals[3] = { defaultVal.x, defaultVal.y, defaultVal.z };
        int idx = 0;
        while (std::getline(ss, item, ',') && idx < 3) {
            try {
                vals[idx++] = std::stof(item);
            } catch (...) {}
        }
        return glm::vec3(vals[0], vals[1], vals[2]);
    };

    // Parsear bloques de entidades
    size_t entArrayStart = content.find("\"entities\":");
    if (entArrayStart == std::string::npos) return result;
    entArrayStart = content.find('[', entArrayStart);
    if (entArrayStart == std::string::npos) return result;

    size_t cur = entArrayStart + 1;
    int instanceCounter = 1;

    while (cur < content.size()) {
        size_t objStart = content.find('{', cur);
        if (objStart == std::string::npos) break;

        // Encontrar llave de cierre balanceada para el objeto
        int braceDepth = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < content.size() && braceDepth > 0) {
            if (content[objEnd] == '{') braceDepth++;
            else if (content[objEnd] == '}') braceDepth--;
            objEnd++;
        }

        std::string block = content.substr(objStart, objEnd - objStart);
        cur = objEnd;

        std::string rawName = extractStr(block, "name", prefabStem);
        std::string type = extractStr(block, "type", "Cube");
        std::string modelPath = extractStr(block, "modelPath", "");
        std::string group = extractStr(block, "group", "");

        glm::vec3 relPos = extractVec3(block, "position", glm::vec3(0.0f));
        glm::vec3 relRot = extractVec3(block, "rotation", glm::vec3(0.0f));
        glm::vec3 relScale = extractVec3(block, "scale", glm::vec3(1.0f));
        glm::vec3 color = extractVec3(block, "color", glm::vec3(1.0f));
        bool hasCustomColor = extractBool(block, "hasCustomColor", false);

        // Generar nombre único
        std::string finalName = rawName + "_" + std::to_string(instanceCounter++);
        int safetyLoop = 1;
        while (scene.getEntity(finalName) != nullptr) {
            finalName = rawName + "_" + std::to_string(instanceCounter++) + "_" + std::to_string(safetyLoop++);
        }

        // Posición, rotación y escala en mundo
        glm::vec3 worldPos = spawnPosition + relPos * spawnScale;
        glm::vec3 worldRot = spawnRotation + relRot;
        glm::vec3 worldScale = spawnScale * relScale;

        std::shared_ptr<Entity> entity = nullptr;
        if (type == "Model" && !modelPath.empty()) {
            entity = scene.createModel(finalName, modelPath, worldPos.x, worldPos.y, worldPos.z);
        } else if (type == "Sphere") {
            entity = scene.createSphere(finalName, worldPos.x, worldPos.y, worldPos.z, 0.5f);
        } else if (type == "Cylinder") {
            entity = scene.createCylinder(finalName, worldPos.x, worldPos.y, worldPos.z, 0.5f, 1.0f);
        } else if (type == "Cone") {
            entity = scene.createCone(finalName, worldPos.x, worldPos.y, worldPos.z, 0.5f, 1.0f);
        } else if (type == "Torus") {
            entity = scene.createTorus(finalName, worldPos.x, worldPos.y, worldPos.z, 0.6f, 0.2f);
        } else if (type == "Plane") {
            entity = scene.createPlane(finalName, worldPos.x, worldPos.y, worldPos.z, 1.0f);
        } else {
            entity = scene.createCube(finalName, worldPos.x, worldPos.y, worldPos.z, 1.0f);
        }

        if (entity) {
            entity->setRotation(worldRot.x, worldRot.y, worldRot.z);
            entity->setScale(worldScale.x, worldScale.y, worldScale.z);
            if (hasCustomColor) {
                entity->setColor(color.r, color.g, color.b);
            }
            if (!group.empty()) {
                entity->setGroup(group);
            }

            // Material PBR
            auto mat = entity->getMaterial();
            if (!mat) {
                mat = std::make_shared<Material>();
                entity->setMaterial(mat);
            }
            if (block.find("\"material\":") != std::string::npos) {
                size_t matStart = block.find("\"material\":");
                size_t matEnd = block.find('}', matStart);
                std::string matBlock = block.substr(matStart, matEnd - matStart + 1);

                mat->roughness = extractFloat(matBlock, "roughness", 0.5f);
                mat->metallic = extractFloat(matBlock, "metallic", 0.0f);
                mat->specular = extractFloat(matBlock, "specular", 0.5f);
                mat->normalStrength = extractFloat(matBlock, "normalStrength", 1.0f);
                mat->ao = extractFloat(matBlock, "ambientOcclusion", 1.0f);
                mat->emission = extractVec3(matBlock, "emissionColor", glm::vec3(0.0f));
                mat->emissionStrength = extractFloat(matBlock, "emissionIntensity", 1.0f);
            }

            // Física y Trigger
            if (block.find("\"physics\":") != std::string::npos) {
                size_t physStart = block.find("\"physics\":");
                size_t physEnd = block.find('}', physStart);
                std::string physBlock = block.substr(physStart, physEnd - physStart + 1);

                bool hasPhys = extractBool(physBlock, "hasPhysics", false);
                float mass = extractFloat(physBlock, "mass", 1.0f);
                std::string colTypeStr = extractStr(physBlock, "colliderType", "BOX");
                bool isTrig = extractBool(physBlock, "isTrigger", false);
                std::string trigTag = extractStr(physBlock, "triggerTag", "Default");

                entity->setTrigger(isTrig);
                entity->setTriggerTag(trigTag);
                entity->setHasPhysics(hasPhys);
                entity->setMass(isTrig ? 0.0f : mass);
                entity->setColliderType(colTypeStr);

                if (physicsWorld && (hasPhys || isTrig)) {
                    ColliderType colType = ColliderType::BOX;
                    if (colTypeStr == "SPHERE" || type == "Sphere") colType = ColliderType::SPHERE;
                    else if (colTypeStr == "PLANE" || type == "Plane") colType = ColliderType::PLANE;

                    physicsWorld->addRigidBody(entity, isTrig ? 0.0f : mass, colType);
                    if (isTrig) {
                        physicsWorld->setBodyTrigger(entity->getName(), true);
                    }
                }
            }

            result.push_back(entity);
        }
    }

    std::cout << "[ASIS PrefabManager] Instanciado prefab '" << prefabStem << "' (" << result.size() << " entidades)\n";
    return result;
}

std::vector<std::string> PrefabManager::getAvailablePrefabs(const std::string& directory) {
    std::vector<std::string> prefabs;
    if (!std::filesystem::exists(directory)) {
        createDefaultPrefabs(directory);
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".prefab") {
                prefabs.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ASIS PrefabManager ERROR] Fallo al listar prefabs: " << e.what() << std::endl;
    }

    std::sort(prefabs.begin(), prefabs.end());
    return prefabs;
}

void PrefabManager::createDefaultPrefabs(const std::string& directory) {
    std::filesystem::create_directories(directory);

    // 1. Moneda Dorada Coleccionable (Trigger)
    std::string coinPath = directory + "/Moneda_Dorada.prefab";
    if (!std::filesystem::exists(coinPath)) {
        std::ofstream f(coinPath);
        if (f.is_open()) {
            f << "{\n";
            f << "  \"asis_prefab_version\": \"1.0\",\n";
            f << "  \"name\": \"Moneda_Dorada\",\n";
            f << "  \"entities\": [\n";
            f << "    {\n";
            f << "      \"name\": \"Moneda\",\n";
            f << "      \"type\": \"Cylinder\",\n";
            f << "      \"position\": [0.0, 0.5, 0.0],\n";
            f << "      \"rotation\": [90.0, 0.0, 0.0],\n";
            f << "      \"scale\": [0.55, 0.08, 0.55],\n";
            f << "      \"color\": [1.0, 0.82, 0.05],\n";
            f << "      \"hasCustomColor\": true,\n";
            f << "      \"material\": {\n";
            f << "        \"roughness\": 0.15,\n";
            f << "        \"metallic\": 0.95,\n";
            f << "        \"specular\": 0.9,\n";
            f << "        \"emissionColor\": [0.25, 0.18, 0.0],\n";
            f << "        \"emissionIntensity\": 1.2\n";
            f << "      },\n";
            f << "      \"physics\": {\n";
            f << "        \"hasPhysics\": true,\n";
            f << "        \"mass\": 0.0,\n";
            f << "        \"colliderType\": \"SPHERE\",\n";
            f << "        \"isTrigger\": true,\n";
            f << "        \"triggerTag\": \"MonedaColeccionable\"\n";
            f << "      }\n";
            f << "    }\n";
            f << "  ]\n";
            f << "}\n";
            f.close();
        }
    }

    // 2. Caja de Madera Física (Rigid Body con masa 5 kg)
    std::string cratePath = directory + "/Caja_Madera_Fisica.prefab";
    if (!std::filesystem::exists(cratePath)) {
        std::ofstream f(cratePath);
        if (f.is_open()) {
            f << "{\n";
            f << "  \"asis_prefab_version\": \"1.0\",\n";
            f << "  \"name\": \"Caja_Madera_Fisica\",\n";
            f << "  \"entities\": [\n";
            f << "    {\n";
            f << "      \"name\": \"CajaMadera\",\n";
            f << "      \"type\": \"Cube\",\n";
            f << "      \"position\": [0.0, 0.5, 0.0],\n";
            f << "      \"rotation\": [0.0, 0.0, 0.0],\n";
            f << "      \"scale\": [1.0, 1.0, 1.0],\n";
            f << "      \"color\": [0.65, 0.42, 0.22],\n";
            f << "      \"hasCustomColor\": true,\n";
            f << "      \"material\": {\n";
            f << "        \"roughness\": 0.85,\n";
            f << "        \"metallic\": 0.0,\n";
            f << "        \"specular\": 0.3,\n";
            f << "        \"emissionColor\": [0.0, 0.0, 0.0],\n";
            f << "        \"emissionIntensity\": 0.0\n";
            f << "      },\n";
            f << "      \"physics\": {\n";
            f << "        \"hasPhysics\": true,\n";
            f << "        \"mass\": 5.0,\n";
            f << "        \"colliderType\": \"BOX\",\n";
            f << "        \"isTrigger\": false,\n";
            f << "        \"triggerTag\": \"Obstaculo\"\n";
            f << "      }\n";
            f << "    }\n";
            f << "  ]\n";
            f << "}\n";
            f.close();
        }
    }

    // 3. Portal de Teletransporte Neón (Trigger)
    std::string portalPath = directory + "/Portal_Teletransporte.prefab";
    if (!std::filesystem::exists(portalPath)) {
        std::ofstream f(portalPath);
        if (f.is_open()) {
            f << "{\n";
            f << "  \"asis_prefab_version\": \"1.0\",\n";
            f << "  \"name\": \"Portal_Teletransporte\",\n";
            f << "  \"entities\": [\n";
            f << "    {\n";
            f << "      \"name\": \"PortalRing\",\n";
            f << "      \"type\": \"Torus\",\n";
            f << "      \"position\": [0.0, 1.2, 0.0],\n";
            f << "      \"rotation\": [90.0, 0.0, 0.0],\n";
            f << "      \"scale\": [1.2, 1.2, 0.3],\n";
            f << "      \"color\": [0.1, 0.85, 1.0],\n";
            f << "      \"hasCustomColor\": true,\n";
            f << "      \"material\": {\n";
            f << "        \"roughness\": 0.1,\n";
            f << "        \"metallic\": 0.4,\n";
            f << "        \"specular\": 0.9,\n";
            f << "        \"emissionColor\": [0.0, 0.8, 1.0],\n";
            f << "        \"emissionIntensity\": 2.8\n";
            f << "      },\n";
            f << "      \"physics\": {\n";
            f << "        \"hasPhysics\": true,\n";
            f << "        \"mass\": 0.0,\n";
            f << "        \"colliderType\": \"BOX\",\n";
            f << "        \"isTrigger\": true,\n";
            f << "        \"triggerTag\": \"Portal\"\n";
            f << "      }\n";
            f << "    }\n";
            f << "  ]\n";
            f << "}\n";
            f.close();
        }
    }

    // 4. Barril Explosivo (Física dinámica 8 kg)
    std::string barrelPath = directory + "/Barril_Explosivo.prefab";
    if (!std::filesystem::exists(barrelPath)) {
        std::ofstream f(barrelPath);
        if (f.is_open()) {
            f << "{\n";
            f << "  \"asis_prefab_version\": \"1.0\",\n";
            f << "  \"name\": \"Barril_Explosivo\",\n";
            f << "  \"entities\": [\n";
            f << "    {\n";
            f << "      \"name\": \"Barril\",\n";
            f << "      \"type\": \"Cylinder\",\n";
            f << "      \"position\": [0.0, 0.6, 0.0],\n";
            f << "      \"rotation\": [0.0, 0.0, 0.0],\n";
            f << "      \"scale\": [0.65, 1.1, 0.65],\n";
            f << "      \"color\": [0.92, 0.18, 0.12],\n";
            f << "      \"hasCustomColor\": true,\n";
            f << "      \"material\": {\n";
            f << "        \"roughness\": 0.35,\n";
            f << "        \"metallic\": 0.55,\n";
            f << "        \"specular\": 0.7,\n";
            f << "        \"emissionColor\": [0.4, 0.05, 0.0],\n";
            f << "        \"emissionIntensity\": 1.5\n";
            f << "      },\n";
            f << "      \"physics\": {\n";
            f << "        \"hasPhysics\": true,\n";
            f << "        \"mass\": 8.0,\n";
            f << "        \"colliderType\": \"BOX\",\n";
            f << "        \"isTrigger\": false,\n";
            f << "        \"triggerTag\": \"BarrilExplosivo\"\n";
            f << "      }\n";
            f << "    }\n";
            f << "  ]\n";
            f << "}\n";
            f.close();
        }
    }
}

} // namespace ASIS
