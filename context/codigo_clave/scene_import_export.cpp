// ========================================================================
// ASIS Engine - Importación y Exportación de Modelos 3D y Drag & Drop
// ========================================================================

#include "SceneIO.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>

namespace ASIS {

// Importación Multiformato usando Assimp con Normalización de AABB
std::shared_ptr<Entity> SceneIO::importModel(Scene& scene, const std::string& filePath, glm::vec3 spawnPosition) {
    if (!std::filesystem::exists(filePath)) return nullptr;

    std::filesystem::path p(filePath);
    std::string baseName = p.stem().string();
    if (baseName.empty()) baseName = "Modelo";

    // Nombre único en la jerarquía
    std::string uniqueName = baseName;
    int counter = 1;
    while (scene.getEntity(uniqueName) != nullptr) {
        uniqueName = baseName + "_" + std::to_string(counter++);
    }

    auto model = std::make_shared<Model>(filePath);
    auto entity = std::make_shared<Entity>(uniqueName);
    entity->setModel(model);

    // Calcular cuadro delimitador (AABB) y reposar sobre el suelo Y = 0
    glm::vec3 minAABB(1e9f), maxAABB(-1e9f);
    for (const auto& mesh : model->getMeshes()) {
        for (const auto& v : mesh->getVertices()) {
            minAABB = glm::min(minAABB, v.position);
            maxAABB = glm::max(maxAABB, v.position);
        }
    }

    glm::vec3 size = maxAABB - minAABB;
    float maxDim = std::max({ size.x, size.y, size.z });
    float scaleFactor = 1.0f;
    if (maxDim > 15.0f || maxDim < 0.2f) {
        scaleFactor = 2.5f / (maxDim > 0.001f ? maxDim : 1.0f);
        entity->setUniformScale(scaleFactor);
    }

    float groundOffset = -minAABB.y * scaleFactor;
    if (groundOffset < 0.0f) groundOffset = 0.0f;
    entity->setPosition(spawnPosition.x, spawnPosition.y + groundOffset, spawnPosition.z);

    scene.addEntity(entity);
    return entity;
}

// Exportador Wavefront OBJ + Materiales MTL con Coordenadas de Mundo
bool SceneIO::exportSceneToOBJ(const Scene& scene, const std::string& filePath) {
    std::filesystem::path objPath(filePath);
    std::filesystem::path mtlPath = objPath;
    mtlPath.replace_extension(".mtl");

    std::ofstream objFile(objPath);
    std::ofstream mtlFile(mtlPath);
    if (!objFile.is_open() || !mtlFile.is_open()) return false;

    objFile << "mtllib " << mtlPath.filename().string() << "\n\n";

    unsigned int vertexOffset = 1;
    auto entities = scene.getEntities();
    for (const auto& [name, entity] : entities) {
        if (!entity || !entity->isVisible()) continue;
        if (entity->getName() == "GroundGrid" || entity->getName() == "ShadowPlane") continue;

        glm::mat4 modelMatrix = entity->getModelMatrix();
        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

        std::string safeName = entity->getName();
        std::replace(safeName.begin(), safeName.end(), ' ', '_');
        std::string matName = "mat_" + safeName;

        // Material con color difuso de la entidad
        glm::vec3 col = entity->hasCustomColor() ? entity->getColor() : glm::vec3(0.85f);
        mtlFile << "newmtl " << matName << "\n";
        mtlFile << "Kd " << col.r << " " << col.g << " " << col.b << "\n\n";

        objFile << "o " << safeName << "\nusemtl " << matName << "\ns 1\n";

        auto exportMesh = [&](const Mesh& mesh) {
            if (mesh.isLines()) return;
            const auto& verts = mesh.getVertices();
            const auto& indices = mesh.getIndices();

            for (const auto& v : verts) {
                glm::vec3 worldPos = glm::vec3(modelMatrix * glm::vec4(v.position, 1.0f));
                glm::vec3 worldNorm = glm::normalize(normalMatrix * v.normal);
                objFile << "v " << worldPos.x << " " << worldPos.y << " " << worldPos.z << "\n";
                objFile << "vn " << worldNorm.x << " " << worldNorm.y << " " << worldNorm.z << "\n";
                objFile << "vt " << v.texCoords.x << " " << v.texCoords.y << "\n";
            }

            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                unsigned int i1 = vertexOffset + indices[i];
                unsigned int i2 = vertexOffset + indices[i + 1];
                unsigned int i3 = vertexOffset + indices[i + 2];
                objFile << "f " << i1 << "/" << i1 << "/" << i1 << " "
                                << i2 << "/" << i2 << "/" << i2 << " "
                                << i3 << "/" << i3 << "/" << i3 << "\n";
            }
            vertexOffset += static_cast<unsigned int>(verts.size());
        };

        if (entity->getMesh()) exportMesh(*entity->getMesh());
        if (entity->getModel()) {
            for (const auto& mesh : entity->getModel()->getMeshes()) {
                if (mesh) exportMesh(*mesh);
            }
        }
        objFile << "\n";
    }
    return true;
}

} // namespace ASIS
