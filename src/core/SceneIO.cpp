#include "SceneIO.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ASIS {

std::string SceneIO::getFileExtension(const std::string& filePath) {
    std::filesystem::path p(filePath);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool SceneIO::isSupported3DFormat(const std::string& filePath) {
    std::string ext = getFileExtension(filePath);
    return (ext == ".obj" || ext == ".gltf" || ext == ".glb" ||
            ext == ".fbx" || ext == ".stl"  || ext == ".ply" ||
            ext == ".dae" || ext == ".3ds"  || ext == ".blend");
}

std::string SceneIO::openFileDialog(const char* filter) {
    char filename[MAX_PATH] = { 0 };
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        std::string path(filename);
        std::replace(path.begin(), path.end(), '\\', '/');
        return path;
    }
    return "";
}

std::string SceneIO::saveFileDialog(const char* filter, const char* defaultExt) {
    char filename[MAX_PATH] = { 0 };
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) {
        std::string path(filename);
        std::replace(path.begin(), path.end(), '\\', '/');
        return path;
    }
    return "";
}

std::shared_ptr<Entity> SceneIO::importModel(Scene& scene, const std::string& filePath, glm::vec3 spawnPosition) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[SceneIO ERROR] El archivo no existe: " << filePath << std::endl;
        return nullptr;
    }

    try {
        std::filesystem::path p(filePath);
        std::string baseName = p.stem().string();
        if (baseName.empty()) baseName = "ModeloImportado";

        // Asegurar nombre único en la jerarquía de la escena
        std::string uniqueName = baseName;
        int counter = 1;
        while (scene.getEntity(uniqueName) != nullptr) {
            uniqueName = baseName + "_" + std::to_string(counter++);
        }

        std::cout << "[SceneIO] Importando modelo 3D desde: " << filePath << " como '" << uniqueName << "'...\n";

        auto model = std::make_shared<Model>(filePath);
        auto entity = std::make_shared<Entity>(uniqueName);
        entity->setModel(model);

        // Calcular cuadro delimitador (AABB) de todas las submallas del modelo
        glm::vec3 minAABB(1e9f);
        glm::vec3 maxAABB(-1e9f);
        size_t totalVertices = 0;

        for (const auto& mesh : model->getMeshes()) {
            for (const auto& v : mesh->getVertices()) {
                minAABB = glm::min(minAABB, v.position);
                maxAABB = glm::max(maxAABB, v.position);
                totalVertices++;
            }
        }

        glm::vec3 size = maxAABB - minAABB;
        float maxDim = std::max({ size.x, size.y, size.z });

        // Normalización inteligente: si el modelo es gigantesco o microscópico, ajustarlo a ~2.5 unidades
        float scaleFactor = 1.0f;
        if (maxDim > 15.0f || maxDim < 0.2f) {
            scaleFactor = 2.5f / (maxDim > 0.001f ? maxDim : 1.0f);
            entity->setUniformScale(scaleFactor);
        }

        // Reposar la base del modelo sobre el suelo Y = 0.0f
        float groundOffset = -minAABB.y * scaleFactor;
        if (groundOffset < 0.0f) groundOffset = 0.0f;

        entity->setPosition(spawnPosition.x, spawnPosition.y + groundOffset, spawnPosition.z);

        if (model->getMaterial()) {
            entity->setMaterial(model->getMaterial());
        }

        scene.addEntity(entity);

        std::cout << "[SceneIO] ✓ Modelo '" << uniqueName << "' importado con exito: "
                  << model->getMeshCount() << " submallas, " << totalVertices << " vertices.\n";

        return entity;

    } catch (const std::exception& e) {
        std::cerr << "[SceneIO ERROR] Excepción al importar modelo: " << e.what() << std::endl;
        return nullptr;
    }
}

bool SceneIO::exportSceneToOBJ(const Scene& scene, const std::string& filePath) {
    if (filePath.empty()) return false;

    std::filesystem::path objPath(filePath);
    std::filesystem::path mtlPath = objPath;
    mtlPath.replace_extension(".mtl");

    std::ofstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "[SceneIO ERROR] No se pudo crear el archivo OBJ: " << filePath << std::endl;
        return false;
    }

    std::ofstream mtlFile(mtlPath);
    if (!mtlFile.is_open()) {
        std::cerr << "[SceneIO ERROR] No se pudo crear el archivo MTL: " << mtlPath.string() << std::endl;
        return false;
    }

    // Cabecera OBJ
    objFile << "# ========================================================\n";
    objFile << "# ASIS Engine v0.9 - Wavefront OBJ Scene Exporter\n";
    objFile << "# Fecha: Exportado en tiempo real\n";
    objFile << "# ========================================================\n\n";
    objFile << "mtllib " << mtlPath.filename().string() << "\n\n";

    // Cabecera MTL
    mtlFile << "# ========================================================\n";
    mtlFile << "# ASIS Engine v0.9 - Wavefront MTL Material Library\n";
    mtlFile << "# ========================================================\n\n";

    unsigned int vertexOffset = 1;
    size_t exportedEntities = 0;
    size_t totalVerticesExported = 0;

    auto entities = scene.getEntities();
    for (const auto& [name, entity] : entities) {
        if (!entity || !entity->isVisible()) continue;

        // Si es una cuadrícula o suelo infinito de debug, saltar
        if (entity->getName() == "GroundGrid" || entity->getName() == "ShadowPlane") continue;

        glm::mat4 modelMatrix = entity->getModelMatrix();
        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

        std::string safeName = entity->getName();
        std::replace(safeName.begin(), safeName.end(), ' ', '_');

        std::string matName = "mat_" + safeName;

        // Escribir material en .mtl
        glm::vec3 col = entity->hasCustomColor() ? entity->getColor() : glm::vec3(0.85f, 0.85f, 0.88f);
        mtlFile << "newmtl " << matName << "\n";
        mtlFile << "Ka 0.2000 0.2000 0.2000\n";
        mtlFile << "Kd " << std::fixed << std::setprecision(4) << col.r << " " << col.g << " " << col.b << "\n";
        mtlFile << "Ks 0.5000 0.5000 0.5000\n";
        mtlFile << "Ns 32.0000\n";
        mtlFile << "d 1.0000\n";
        mtlFile << "illum 2\n\n";

        // Escribir objeto en .obj
        objFile << "o " << safeName << "\n";
        objFile << "usemtl " << matName << "\n";
        objFile << "s 1\n";

        auto exportMeshGeometry = [&](const Mesh& mesh) {
            if (mesh.isLines()) return;

            const auto& verts = mesh.getVertices();
            const auto& indices = mesh.getIndices();

            // Vértices, Normales y UVs
            for (const auto& v : verts) {
                glm::vec3 worldPos = glm::vec3(modelMatrix * glm::vec4(v.position, 1.0f));
                glm::vec3 worldNorm = glm::normalize(normalMatrix * v.normal);

                objFile << "v " << std::fixed << std::setprecision(5) << worldPos.x << " " << worldPos.y << " " << worldPos.z << "\n";
                objFile << "vn " << std::fixed << std::setprecision(4) << worldNorm.x << " " << worldNorm.y << " " << worldNorm.z << "\n";
                objFile << "vt " << std::fixed << std::setprecision(4) << v.texCoords.x << " " << v.texCoords.y << "\n";
            }

            // Caras (triángulos)
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                unsigned int i1 = vertexOffset + indices[i];
                unsigned int i2 = vertexOffset + indices[i + 1];
                unsigned int i3 = vertexOffset + indices[i + 2];
                objFile << "f " << i1 << "/" << i1 << "/" << i1 << " "
                                << i2 << "/" << i2 << "/" << i2 << " "
                                << i3 << "/" << i3 << "/" << i3 << "\n";
            }

            vertexOffset += static_cast<unsigned int>(verts.size());
            totalVerticesExported += verts.size();
        };

        if (entity->getMesh()) {
            exportMeshGeometry(*entity->getMesh());
        }

        if (entity->getModel()) {
            for (const auto& mesh : entity->getModel()->getMeshes()) {
                if (mesh) exportMeshGeometry(*mesh);
            }
        }

        objFile << "\n";
        exportedEntities++;
    }

    objFile.close();
    mtlFile.close();

    std::cout << "[SceneIO] ✓ Escena exportada con exito a: " << filePath << " ("
              << exportedEntities << " entidades, " << totalVerticesExported << " vertices)\n";

    return true;
}

bool SceneIO::exportEntityToOBJ(const Entity& entity, const std::string& filePath) {
    Scene tempScene;
    auto copyEntity = std::make_shared<Entity>(entity.getName());
    copyEntity->setPosition(entity.getPosition().x, entity.getPosition().y, entity.getPosition().z);
    copyEntity->setRotation(entity.getRotation().x, entity.getRotation().y, entity.getRotation().z);
    copyEntity->setScale(entity.getScale().x, entity.getScale().y, entity.getScale().z);
    if (entity.hasCustomColor()) {
        copyEntity->setColor(entity.getColor().r, entity.getColor().g, entity.getColor().b);
    }
    copyEntity->setMesh(entity.getMesh());
    copyEntity->setModel(entity.getModel());

    tempScene.addEntity(copyEntity);
    return exportSceneToOBJ(tempScene, filePath);
}

// Codificador Base64 para volcar buffers en glTF 2.0 auto-contenido
static std::string base64Encode(const unsigned char* data, size_t length) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((length + 2) / 3) * 4);
    for (size_t i = 0; i < length; i += 3) {
        unsigned int val = (data[i] << 16);
        if (i + 1 < length) val |= (data[i + 1] << 8);
        if (i + 2 < length) val |= data[i + 2];

        out.push_back(chars[(val >> 18) & 0x3F]);
        out.push_back(chars[(val >> 12) & 0x3F]);
        out.push_back((i + 1 < length) ? chars[(val >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < length) ? chars[val & 0x3F] : '=');
    }
    return out;
}

bool SceneIO::exportSceneToGLTF(const Scene& scene, const std::string& filePath) {
    if (filePath.empty()) return false;

    // Recopilar toda la geometría combinada de la escena
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;

    unsigned int vertexOffset = 0;

    auto entities = scene.getEntities();
    for (const auto& [name, entity] : entities) {
        if (!entity || !entity->isVisible()) continue;
        if (entity->getName() == "GroundGrid" || entity->getName() == "ShadowPlane") continue;

        glm::mat4 modelMatrix = entity->getModelMatrix();
        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

        auto appendMesh = [&](const Mesh& mesh) {
            if (mesh.isLines()) return;

            const auto& verts = mesh.getVertices();
            const auto& idxs = mesh.getIndices();

            for (const auto& v : verts) {
                glm::vec3 worldPos = glm::vec3(modelMatrix * glm::vec4(v.position, 1.0f));
                glm::vec3 worldNorm = glm::normalize(normalMatrix * v.normal);

                positions.push_back(worldPos.x);
                positions.push_back(worldPos.y);
                positions.push_back(worldPos.z);

                normals.push_back(worldNorm.x);
                normals.push_back(worldNorm.y);
                normals.push_back(worldNorm.z);

                texCoords.push_back(v.texCoords.x);
                texCoords.push_back(v.texCoords.y);
            }

            for (unsigned int id : idxs) {
                indices.push_back(vertexOffset + id);
            }

            vertexOffset += static_cast<unsigned int>(verts.size());
        };

        if (entity->getMesh()) appendMesh(*entity->getMesh());
        if (entity->getModel()) {
            for (const auto& mesh : entity->getModel()->getMeshes()) {
                if (mesh) appendMesh(*mesh);
            }
        }
    }

    if (positions.empty()) {
        std::cerr << "[SceneIO ERROR] No hay geometria valida para exportar a glTF.\n";
        return false;
    }

    // Construir buffer binario continuo
    std::vector<unsigned char> binaryBuffer;
    
    // Indices (unsigned int: 4 bytes)
    size_t indicesByteOffset = binaryBuffer.size();
    size_t indicesByteLength = indices.size() * sizeof(unsigned int);
    binaryBuffer.resize(indicesByteOffset + indicesByteLength);
    std::memcpy(binaryBuffer.data() + indicesByteOffset, indices.data(), indicesByteLength);

    // Posiciones (vec3 float: 12 bytes)
    size_t posByteOffset = binaryBuffer.size();
    size_t posByteLength = positions.size() * sizeof(float);
    binaryBuffer.resize(posByteOffset + posByteLength);
    std::memcpy(binaryBuffer.data() + posByteOffset, positions.data(), posByteLength);

    // Normales (vec3 float: 12 bytes)
    size_t normByteOffset = binaryBuffer.size();
    size_t normByteLength = normals.size() * sizeof(float);
    binaryBuffer.resize(normByteOffset + normByteLength);
    std::memcpy(binaryBuffer.data() + normByteOffset, normals.data(), normByteLength);

    // UVs (vec2 float: 8 bytes)
    size_t uvByteOffset = binaryBuffer.size();
    size_t uvByteLength = texCoords.size() * sizeof(float);
    binaryBuffer.resize(uvByteOffset + uvByteLength);
    std::memcpy(binaryBuffer.data() + uvByteOffset, texCoords.data(), uvByteLength);

    std::string b64Buffer = base64Encode(binaryBuffer.data(), binaryBuffer.size());

    // Escribir archivo glTF 2.0 estándar (formato JSON embebido autónomo)
    std::ofstream gltfFile(filePath);
    if (!gltfFile.is_open()) return false;

    gltfFile << "{\n";
    gltfFile << "  \"asset\": { \"version\": \"2.0\", \"generator\": \"ASIS Engine v0.9\" },\n";
    gltfFile << "  \"scene\": 0,\n";
    gltfFile << "  \"scenes\": [{ \"nodes\": [0] }],\n";
    gltfFile << "  \"nodes\": [{ \"mesh\": 0, \"name\": \"SceneRoot\" }],\n";
    gltfFile << "  \"meshes\": [{\n";
    gltfFile << "    \"name\": \"ExportedScene\",\n";
    gltfFile << "    \"primitives\": [{\n";
    gltfFile << "      \"attributes\": {\n";
    gltfFile << "        \"POSITION\": 1,\n";
    gltfFile << "        \"NORMAL\": 2,\n";
    gltfFile << "        \"TEXCOORD_0\": 3\n";
    gltfFile << "      },\n";
    gltfFile << "      \"indices\": 0,\n";
    gltfFile << "      \"mode\": 4\n";
    gltfFile << "    }]\n";
    gltfFile << "  }],\n";
    gltfFile << "  \"accessors\": [\n";
    gltfFile << "    { \"bufferView\": 0, \"componentType\": 5125, \"count\": " << indices.size() << ", \"type\": \"SCALAR\" },\n";
    gltfFile << "    { \"bufferView\": 1, \"componentType\": 5126, \"count\": " << (positions.size() / 3) << ", \"type\": \"VEC3\" },\n";
    gltfFile << "    { \"bufferView\": 2, \"componentType\": 5126, \"count\": " << (normals.size() / 3) << ", \"type\": \"VEC3\" },\n";
    gltfFile << "    { \"bufferView\": 3, \"componentType\": 5126, \"count\": " << (texCoords.size() / 2) << ", \"type\": \"VEC2\" }\n";
    gltfFile << "  ],\n";
    gltfFile << "  \"bufferViews\": [\n";
    gltfFile << "    { \"buffer\": 0, \"byteOffset\": " << indicesByteOffset << ", \"byteLength\": " << indicesByteLength << ", \"target\": 34963 },\n";
    gltfFile << "    { \"buffer\": 0, \"byteOffset\": " << posByteOffset << ", \"byteLength\": " << posByteLength << ", \"target\": 34962 },\n";
    gltfFile << "    { \"buffer\": 0, \"byteOffset\": " << normByteOffset << ", \"byteLength\": " << normByteLength << ", \"target\": 34962 },\n";
    gltfFile << "    { \"buffer\": 0, \"byteOffset\": " << uvByteOffset << ", \"byteLength\": " << uvByteLength << ", \"target\": 34962 }\n";
    gltfFile << "  ],\n";
    gltfFile << "  \"buffers\": [{\n";
    gltfFile << "    \"byteLength\": " << binaryBuffer.size() << ",\n";
    gltfFile << "    \"uri\": \"data:application/octet-stream;base64," << b64Buffer << "\"\n";
    gltfFile << "  }]\n";
    gltfFile << "}\n";

    gltfFile.close();

    std::cout << "[SceneIO] ✓ Escena exportada con exito a glTF 2.0: " << filePath
              << " (" << (positions.size() / 3) << " vertices, " << (indices.size() / 3) << " triangulos)\n";

    return true;
}

bool SceneIO::saveProjectASIS(const Scene& scene, const Camera& camera,
                             const ASISSceneLoadData& projectData,
                             const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[SceneIO ERROR] No se pudo crear el archivo de proyecto .asis en: " << filePath << std::endl;
        return false;
    }

    file << "{\n";
    file << "  \"asis_engine_version\": \"0.17\",\n";

    // 1. Cámara
    file << "  \"camera\": {\n";
    glm::vec3 cPos = camera.getPosition();
    glm::vec3 cTarget = camera.getTarget();
    file << "    \"position\": [" << cPos.x << ", " << cPos.y << ", " << cPos.z << "],\n";
    file << "    \"target\": [" << cTarget.x << ", " << cTarget.y << ", " << cTarget.z << "],\n";
    file << "    \"fov\": " << camera.getFOV() << ",\n";
    file << "    \"orthographic\": " << (camera.isOrthographic() ? "true" : "false") << ",\n";
    file << "    \"nearPlane\": " << camera.getNearPlane() << ",\n";
    file << "    \"farPlane\": " << camera.getFarPlane() << "\n";
    file << "  },\n";

    // 2. Iluminación y Sombras
    file << "  \"lighting\": {\n";
    file << "    \"sunAzimuth\": " << projectData.sunAzimuth << ",\n";
    file << "    \"sunElevation\": " << projectData.sunElevation << ",\n";
    file << "    \"sunColor\": [" << projectData.sunColor.r << ", " << projectData.sunColor.g << ", " << projectData.sunColor.b << "],\n";
    file << "    \"sunIntensity\": " << projectData.sunIntensity << ",\n";
    file << "    \"ambientColor\": [" << projectData.ambientColor.r << ", " << projectData.ambientColor.g << ", " << projectData.ambientColor.b << "],\n";
    file << "    \"ambientIntensity\": " << projectData.ambientIntensity << ",\n";
    file << "    \"shadowsEnabled\": " << (projectData.shadowsEnabled ? "true" : "false") << ",\n";
    file << "    \"shadowResolution\": " << projectData.shadowRes << ",\n";
    file << "    \"shadowKernel\": " << projectData.shadowKernel << ",\n";
    file << "    \"shadowSoftness\": " << projectData.shadowSoftness << ",\n";
    file << "    \"shadowDistance\": " << projectData.shadowDistance << ",\n";
    file << "    \"terrainCastsShadows\": " << (projectData.terrainCastsShadows ? "true" : "false") << ",\n";
    file << "    \"shadowFilterMode\": " << projectData.shadowFilterMode << ",\n";
    file << "    \"pcssLightSize\": " << projectData.pcssLightSize << ",\n";
    file << "    \"pcssBlockerSamples\": " << projectData.pcssBlockerSamples << ",\n";
    file << "    \"pcssFilterSamples\": " << projectData.pcssFilterSamples << ",\n";
    file << "    \"csmEnabled\": " << (projectData.csmEnabled ? "true" : "false") << ",\n";
    file << "    \"pointLights\": [\n";
    const auto& pointLights = scene.getPointLights();
    for (size_t i = 0; i < pointLights.size(); ++i) {
        const auto& pl = pointLights[i];
        file << "      {\n";
        file << "        \"name\": \"" << pl.name << "\",\n";
        file << "        \"position\": [" << pl.position.x << ", " << pl.position.y << ", " << pl.position.z << "],\n";
        file << "        \"color\": [" << pl.color.r << ", " << pl.color.g << ", " << pl.color.b << "],\n";
        file << "        \"intensity\": " << pl.intensity << ",\n";
        file << "        \"enabled\": " << (pl.enabled ? "true" : "false") << ",\n";
        file << "        \"castShadows\": " << (pl.castShadows ? "true" : "false") << ",\n";
        file << "        \"lightSize\": " << pl.lightSize << ",\n";
        file << "        \"bias\": " << pl.bias << "\n";
        file << "      }" << (i + 1 < pointLights.size() ? "," : "") << "\n";
    }
    file << "    ]\n";
    file << "  },\n";

    // 3. Gestión de Color y Adaptación Ocular (Fase 11 y 17)
    file << "  \"colorManagement\": {\n";
    file << "    \"profile\": " << projectData.colorProfile << ",\n";
    file << "    \"exposure\": " << projectData.exposure << ",\n";
    file << "    \"gamma\": " << projectData.gamma << ",\n";
    file << "    \"saturation\": " << projectData.saturation << ",\n";
    file << "    \"contrast\": " << projectData.contrast << ",\n";
    file << "    \"autoExposureEnabled\": " << (projectData.autoExposureEnabled ? "true" : "false") << ",\n";
    file << "    \"autoExposureKey\": " << projectData.autoExposureKey << ",\n";
    file << "    \"autoExposureMin\": " << projectData.autoExposureMin << ",\n";
    file << "    \"autoExposureMax\": " << projectData.autoExposureMax << ",\n";
    file << "    \"autoExposureSpeedUp\": " << projectData.autoExposureSpeedUp << ",\n";
    file << "    \"autoExposureSpeedDown\": " << projectData.autoExposureSpeedDown << "\n";
    file << "  },\n";

    // 4. Cielo HDRI y Entorno
    file << "  \"skybox\": {\n";
    file << "    \"useIBL\": " << (projectData.useIBL ? "true" : "false") << ",\n";
    file << "    \"showSkybox\": " << (projectData.showSkybox ? "true" : "false") << ",\n";
    file << "    \"exposure\": " << projectData.skyboxExposure << ",\n";
    file << "    \"rotation\": " << projectData.skyboxRotation << ",\n";
    file << "    \"blur\": " << projectData.skyboxBlur << ",\n";
    file << "    \"iblIntensity\": " << projectData.iblIntensity << ",\n";
    file << "    \"flipX\": " << (projectData.flipSkyboxX ? "true" : "false") << ",\n";
    file << "    \"flipY\": " << (projectData.flipSkyboxY ? "true" : "false") << ",\n";
    file << "    \"hdriPath\": \"" << projectData.hdriPath << "\"\n";
    file << "  },\n";

    // 5. Post-Proceso y Opciones Gráficas
    file << "  \"postProcess\": {\n";
    file << "    \"bloomEnabled\": " << (projectData.bloomEnabled ? "true" : "false") << ",\n";
    file << "    \"bloomThreshold\": " << projectData.bloomThreshold << ",\n";
    file << "    \"bloomIntensity\": " << projectData.bloomIntensity << ",\n";
    file << "    \"fxaaEnabled\": " << (projectData.fxaaEnabled ? "true" : "false") << ",\n";
    file << "    \"msaaLevel\": " << projectData.msaaLevel << ",\n";
    file << "    \"ssaoEnabled\": " << (projectData.ssaoEnabled ? "true" : "false") << ",\n";
    file << "    \"ssaoRadius\": " << projectData.ssaoRadius << ",\n";
    file << "    \"ssaoBias\": " << projectData.ssaoBias << ",\n";
    file << "    \"ssaoPower\": " << projectData.ssaoPower << ",\n";
    file << "    \"ssaoIntensity\": " << projectData.ssaoIntensity << ",\n";
    file << "    \"dofEnabled\": " << (projectData.dofEnabled ? "true" : "false") << ",\n";
    file << "    \"dofFocusDistance\": " << projectData.dofFocusDistance << ",\n";
    file << "    \"dofFocusRange\": " << projectData.dofFocusRange << ",\n";
    file << "    \"dofBokehRadius\": " << projectData.dofBokehRadius << ",\n";
    file << "    \"anisotropyLevel\": " << projectData.anisotropyLevel << ",\n";
    file << "    \"showGroundPlane\": " << (projectData.showGroundPlane ? "true" : "false") << ",\n";
    file << "    \"showGroundGrid\": " << (projectData.showGroundGrid ? "true" : "false") << "\n";
    file << "  },\n";

    // 6. Terreno 3D
    file << "  \"terrain\": {\n";
    file << "    \"visible\": " << (projectData.terrainVisible ? "true" : "false") << ",\n";
    file << "    \"baseOffset\": " << projectData.terrainBaseOffset << ",\n";
    file << "    \"reflectionIntensity\": " << projectData.terrainReflectionIntensity << ",\n";
    file << "    \"reliefMode\": " << projectData.terrainReliefMode << ",\n";
    file << "    \"tessellationScale\": " << projectData.terrainTessellationScale << ",\n";
    file << "    \"resolution\": " << projectData.terrainResolution << ",\n";
    file << "    \"worldSize\": " << projectData.terrainWorldSize << ",\n";
    file << "    \"hasCompanionData\": " << (!projectData.terrainHeights.empty() ? "true" : "false") << ",\n";
    file << "    \"layers\": [\n";
    for (size_t i = 0; i < projectData.terrainLayers.size(); ++i) {
        const auto& lay = projectData.terrainLayers[i];
        file << "      {\n";
        file << "        \"name\": \"" << lay.name << "\",\n";
        file << "        \"enabled\": " << (lay.enabled ? "true" : "false") << ",\n";
        file << "        \"visible\": " << (lay.visible ? "true" : "false") << ",\n";
        file << "        \"opacity\": " << lay.opacity << ",\n";
        file << "        \"tiling\": " << lay.tiling << ",\n";
        file << "        \"roughness\": " << lay.roughness << ",\n";
        file << "        \"specular\": " << lay.specular << ",\n";
        file << "        \"metallic\": " << lay.metallic << ",\n";
        file << "        \"normalStrength\": " << lay.normalStrength << ",\n";
        file << "        \"heightScale\": " << lay.heightScale << ",\n";
        file << "        \"aoStrength\": " << lay.aoStrength << ",\n";
        file << "        \"tint\": [" << lay.tint.r << ", " << lay.tint.g << ", " << lay.tint.b << "],\n";
        file << "        \"albedoPath\": \"" << lay.albedoPath << "\",\n";
        file << "        \"normalPath\": \"" << lay.normalPath << "\",\n";
        file << "        \"roughSpecPath\": \"" << lay.roughSpecPath << "\",\n";
        file << "        \"heightPath\": \"" << lay.heightPath << "\",\n";
        file << "        \"aoPath\": \"" << lay.aoPath << "\"\n";
        file << "      }" << (i + 1 < projectData.terrainLayers.size() ? "," : "") << "\n";
    }
    file << "    ]\n";
    file << "  },\n";

    // 6. Sistema OASIS (Fase 18)
    file << "  \"oasis\": {\n";
    file << "    \"enabled\": " << (projectData.oasisEnabled ? "true" : "false") << ",\n";
    file << "    \"frustumCulling\": " << (projectData.oasisFrustumCulling ? "true" : "false") << ",\n";
    file << "    \"geomorphing\": " << (projectData.oasisGeomorphing ? "true" : "false") << ",\n";
    file << "    \"debugLOD\": " << (projectData.oasisDebugLOD ? "true" : "false") << ",\n";
    file << "    \"lodDistanceFactor\": " << projectData.oasisLodDistanceFactor << ",\n";
    file << "    \"maxLOD\": " << projectData.oasisMaxLOD << ",\n";
    file << "    \"patchResolution\": " << projectData.oasisPatchResolution << ",\n";
    file << "    \"meshletsEnabled\": " << (projectData.meshletsEnabled ? "true" : "false") << ",\n";
    file << "    \"meshletFrustumCulling\": " << (projectData.meshletFrustumCulling ? "true" : "false") << ",\n";
    file << "    \"meshletBackfaceCulling\": " << (projectData.meshletBackfaceCulling ? "true" : "false") << ",\n";
    file << "    \"meshletDebugColors\": " << (projectData.meshletDebugColors ? "true" : "false") << "\n";
    file << "  },\n";

    // 7. Océano y Cuerpos de Agua 3D
    file << "  \"water\": {\n";
    file << "    \"enabled\": " << (projectData.waterEnabled ? "true" : "false") << ",\n";
    file << "    \"level\": " << projectData.waterLevel << ",\n";
    file << "    \"amplitude\": " << projectData.waterWaveAmplitude << ",\n";
    file << "    \"speed\": " << projectData.waterWaveSpeed << ",\n";
    file << "    \"shallowColor\": [" << projectData.waterShallowColor.r << ", " << projectData.waterShallowColor.g << ", " << projectData.waterShallowColor.b << "],\n";
    file << "    \"deepColor\": [" << projectData.waterDeepColor.r << ", " << projectData.waterDeepColor.g << ", " << projectData.waterDeepColor.b << "],\n";
    file << "    \"clarity\": " << projectData.waterClarity << ",\n";
    file << "    \"foamThreshold\": " << projectData.waterFoamThreshold << ",\n";
    file << "    \"foamIntensity\": " << projectData.waterFoamIntensity << ",\n";
    file << "    \"refraction\": " << projectData.waterRefraction << ",\n";
    file << "    \"roughness\": " << projectData.waterRoughness << ",\n";
    file << "    \"horizonDistance\": " << projectData.waterHorizonDistance << ",\n";
    file << "    \"horizonBlend\": " << projectData.waterHorizonBlend << "\n";
    file << "  },\n";

    // 8. Fase 20: Ciclo Día / Noche, Clima, SSR y Contact Shadows
    file << "  \"dayNight\": {\n";
    file << "    \"cycleEnabled\": " << (projectData.dayNightCycleEnabled ? "true" : "false") << ",\n";
    file << "    \"timeOfDay\": " << projectData.dayNightTimeOfDay << ",\n";
    file << "    \"duration\": " << projectData.dayNightDuration << ",\n";
    file << "    \"sunLatitude\": " << projectData.dayNightSunLatitude << ",\n";
    file << "    \"horizonHeight\": " << projectData.dayNightHorizonHeight << ",\n";
    file << "    \"weatherType\": " << projectData.weatherType << ",\n";
    file << "    \"wetness\": " << projectData.weatherWetness << "\n";
    file << "  },\n";
    file << "  \"phase20\": {\n";
    file << "    \"ssrEnabled\": " << (projectData.ssrEnabled ? "true" : "false") << ",\n";
    file << "    \"ssrMaxSteps\": " << projectData.ssrMaxSteps << ",\n";
    file << "    \"ssrStepSize\": " << projectData.ssrStepSize << ",\n";
    file << "    \"ssrThickness\": " << projectData.ssrThickness << ",\n";
    file << "    \"ssrIntensity\": " << projectData.ssrIntensity << ",\n";
    file << "    \"contactShadowsEnabled\": " << (projectData.contactShadowsEnabled ? "true" : "false") << ",\n";
    file << "    \"contactShadowsDistance\": " << projectData.contactShadowsDistance << ",\n";
    file << "    \"contactShadowsRaySteps\": " << projectData.contactShadowsRaySteps << ",\n";
    file << "    \"contactShadowsIntensity\": " << projectData.contactShadowsIntensity << ",\n";
    file << "    \"globalFogEnabled\": " << (projectData.globalFogEnabled ? "true" : "false") << ",\n";
    file << "    \"globalFogStart\": " << projectData.globalFogStart << ",\n";
    file << "    \"globalFogEnd\": " << projectData.globalFogEnd << ",\n";
    file << "    \"globalFogDensity\": " << projectData.globalFogDensity << ",\n";
    file << "    \"globalFogColor\": [" << projectData.globalFogColor.r << ", " << projectData.globalFogColor.g << ", " << projectData.globalFogColor.b << "]\n";
    file << "  },\n";

    // 9. Entidades
    file << "  \"entities\": [\n";
    const auto& order = scene.getEntityOrder();
    const auto& entities = scene.getEntities();
    for (size_t i = 0; i < order.size(); ++i) {
        auto it = entities.find(order[i]);
        if (it == entities.end()) continue;
        const auto& ent = it->second;

        file << "    {\n";
        file << "      \"name\": \"" << ent->getName() << "\",\n";
        file << "      \"group\": \"" << ent->getGroup() << "\",\n";
        file << "      \"type\": \"" << (ent->getPrimitiveType().empty() ? "Cube" : ent->getPrimitiveType()) << "\",\n";
        file << "      \"modelPath\": \"" << ent->getModelPath() << "\",\n";

        glm::vec3 pos = ent->getPosition();
        glm::vec3 rot = ent->getRotation();
        glm::vec3 sc = ent->getScale();
        glm::vec3 col = ent->getColor();

        file << "      \"position\": [" << pos.x << ", " << pos.y << ", " << pos.z << "],\n";
        file << "      \"rotation\": [" << rot.x << ", " << rot.y << ", " << rot.z << "],\n";
        file << "      \"scale\": [" << sc.x << ", " << sc.y << ", " << sc.z << "],\n";
        file << "      \"color\": [" << col.r << ", " << col.g << ", " << col.b << "],\n";
        file << "      \"hasCustomColor\": " << (ent->hasCustomColor() ? "true" : "false") << ",\n";
        file << "      \"visible\": " << (ent->isVisible() ? "true" : "false") << ",\n";
        file << "      \"isTrigger\": " << (ent->isTrigger() ? "true" : "false") << ",\n";
        file << "      \"triggerType\": \"" << ent->getTriggerType() << "\",\n";
        file << "      \"hasPhysics\": " << (ent->hasPhysics() ? "true" : "false") << ",\n";
        file << "      \"mass\": " << ent->getMass() << ",\n";
        file << "      \"colliderType\": \"" << ent->getColliderType() << "\",\n";
        file << "      \"friction\": " << ent->getFriction() << ",\n";
        file << "      \"restitution\": " << ent->getRestitution() << ",\n";
        file << "      \"isKinematic\": " << (ent->isKinematic() ? "true" : "false") << ",\n";

        auto mat = ent->getMaterial();
        file << "      \"material\": {\n";
        if (mat) {
            file << "        \"albedo\": [" << mat->albedo.r << ", " << mat->albedo.g << ", " << mat->albedo.b << "],\n";
            file << "        \"metallic\": " << mat->metallic << ",\n";
            file << "        \"roughness\": " << mat->roughness << ",\n";
            file << "        \"specular\": " << mat->specular << ",\n";
            file << "        \"emission\": [" << mat->emission.r << ", " << mat->emission.g << ", " << mat->emission.b << "],\n";
            file << "        \"emissionStrength\": " << mat->emissionStrength << ",\n";
            file << "        \"normalStrength\": " << mat->normalStrength << ",\n";
            file << "        \"ao\": " << mat->ao << ",\n";
            file << "        \"albedoMap\": \"" << (mat->albedoMap ? mat->albedoMap->getPath() : "") << "\",\n";
            file << "        \"normalMap\": \"" << (mat->normalMap ? mat->normalMap->getPath() : "") << "\",\n";
            file << "        \"roughnessMap\": \"" << (mat->roughnessMap ? mat->roughnessMap->getPath() : "") << "\",\n";
            file << "        \"metallicMap\": \"" << (mat->metallicMap ? mat->metallicMap->getPath() : "") << "\",\n";
            file << "        \"specularMap\": \"" << (mat->specularMap ? mat->specularMap->getPath() : "") << "\",\n";
            file << "        \"emissionMap\": \"" << (mat->emissionMap ? mat->emissionMap->getPath() : "") << "\",\n";
            file << "        \"aoMap\": \"" << (mat->aoMap ? mat->aoMap->getPath() : "") << "\"\n";
        } else {
            file << "        \"albedo\": [1.0, 1.0, 1.0],\n";
            file << "        \"metallic\": 0.0,\n";
            file << "        \"roughness\": 0.5,\n";
            file << "        \"specular\": 0.5,\n";
            file << "        \"emission\": [0.0, 0.0, 0.0],\n";
            file << "        \"emissionStrength\": 1.0,\n";
            file << "        \"normalStrength\": 1.0,\n";
            file << "        \"ao\": 1.0,\n";
            file << "        \"albedoMap\": \"\",\n";
            file << "        \"normalMap\": \"\",\n";
            file << "        \"roughnessMap\": \"\",\n";
            file << "        \"metallicMap\": \"\",\n";
            file << "        \"specularMap\": \"\",\n";
            file << "        \"emissionMap\": \"\",\n";
            file << "        \"aoMap\": \"\"\n";
        }
        file << "      }\n";

        file << "    }" << (i + 1 < order.size() ? "," : "") << "\n";
    }
    file << "  ]\n";
    file << "}\n";

    file.close();

    // Guardar datos binarios de alta velocidad del Terreno (.asis.terrain)
    if (!projectData.terrainHeights.empty()) {
        std::string terrainBinPath = filePath + ".terrain";
        std::ofstream tBin(terrainBinPath, std::ios::binary);
        if (tBin.is_open()) {
            const char magic[4] = {'A', 'T', 'E', 'R'};
            tBin.write(magic, 4);
            uint32_t version = 1;
            tBin.write(reinterpret_cast<const char*>(&version), sizeof(version));
            int32_t res = projectData.terrainResolution;
            float ws = projectData.terrainWorldSize;
            tBin.write(reinterpret_cast<const char*>(&res), sizeof(res));
            tBin.write(reinterpret_cast<const char*>(&ws), sizeof(ws));
            uint32_t hCount = static_cast<uint32_t>(projectData.terrainHeights.size());
            tBin.write(reinterpret_cast<const char*>(&hCount), sizeof(hCount));
            tBin.write(reinterpret_cast<const char*>(projectData.terrainHeights.data()), hCount * sizeof(float));
            uint32_t sCount = static_cast<uint32_t>(projectData.terrainSplatmap.size());
            tBin.write(reinterpret_cast<const char*>(&sCount), sizeof(sCount));
            if (sCount > 0) {
                tBin.write(reinterpret_cast<const char*>(projectData.terrainSplatmap.data()), sCount);
            }
            tBin.close();
            std::cout << "[SceneIO] ✓ Datos binarios de terreno (" << hCount << " vertices, " << sCount << " bytes de splatmap) guardados en: " << terrainBinPath << std::endl;
        }
    }

    std::cout << "[SceneIO] ✓ Proyecto completo guardado exitosamente en: " << filePath << std::endl;
    return true;
}

bool SceneIO::saveSceneASIS(const Scene& scene, const Camera& camera,
                            float sunAzimuth, float sunElevation, const glm::vec3& sunColor, float sunIntensity,
                            const glm::vec3& ambientColor, float ambientIntensity, bool shadowsEnabled,
                            int shadowRes, int shadowKernel, float shadowSoftness,
                            int colorProfile, float exposure, float gamma, float saturation, float contrast,
                            const std::string& filePath,
                            bool autoExposureEnabled, float autoExposureKey, float autoExposureMin,
                            float autoExposureMax, float autoExposureSpeedUp, float autoExposureSpeedDown) {
    ASISSceneLoadData projectData;
    projectData.sunAzimuth = sunAzimuth;
    projectData.sunElevation = sunElevation;
    projectData.sunColor = sunColor;
    projectData.sunIntensity = sunIntensity;
    projectData.ambientColor = ambientColor;
    projectData.ambientIntensity = ambientIntensity;
    projectData.shadowsEnabled = shadowsEnabled;
    projectData.shadowRes = shadowRes;
    projectData.shadowKernel = shadowKernel;
    projectData.shadowSoftness = shadowSoftness;
    projectData.colorProfile = colorProfile;
    projectData.exposure = exposure;
    projectData.gamma = gamma;
    projectData.saturation = saturation;
    projectData.contrast = contrast;
    projectData.autoExposureEnabled = autoExposureEnabled;
    projectData.autoExposureKey = autoExposureKey;
    projectData.autoExposureMin = autoExposureMin;
    projectData.autoExposureMax = autoExposureMax;
    projectData.autoExposureSpeedUp = autoExposureSpeedUp;
    projectData.autoExposureSpeedDown = autoExposureSpeedDown;

    return saveProjectASIS(scene, camera, projectData, filePath);
}

bool SceneIO::loadSceneASIS(Scene& scene, Camera& camera, ASISSceneLoadData& outData, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[SceneIO ERROR] No se pudo abrir el archivo .asis en: " << filePath << std::endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (content.find("asis_engine_version") == std::string::npos) {
        std::cerr << "[SceneIO ERROR] Formato de archivo .asis invalido: " << filePath << std::endl;
        return false;
    }

    // Helper lambda para extraer string
    auto extractString = [&](const std::string& key, const std::string& defaultVal) -> std::string {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        size_t q1 = content.find('"', pos + key.size() + 2);
        if (q1 == std::string::npos) return defaultVal;
        size_t q2 = content.find('"', q1 + 1);
        if (q2 == std::string::npos) return defaultVal;
        return content.substr(q1 + 1, q2 - q1 - 1);
    };

    // Helper lambda para extraer float
    auto extractFloat = [&](const std::string& key, float defaultVal) -> float {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = content.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        return std::stof(content.substr(pos + 1));
    };

    // Helper lambda para extraer int
    auto extractInt = [&](const std::string& key, int defaultVal) -> int {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = content.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        return std::stoi(content.substr(pos + 1));
    };

    // Helper lambda para extraer bool
    auto extractBool = [&](const std::string& key, bool defaultVal) -> bool {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = content.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        std::string sub = content.substr(pos + 1, 10);
        return (sub.find("true") != std::string::npos);
    };

    // Helper lambda para extraer vec3
    auto extractVec3 = [&](const std::string& key, glm::vec3 defaultVal) -> glm::vec3 {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = content.find('[', pos);
        if (pos == std::string::npos) return defaultVal;
        size_t endPos = content.find(']', pos);
        if (endPos == std::string::npos) return defaultVal;

        std::string arr = content.substr(pos + 1, endPos - pos - 1);
        std::stringstream ss(arr);
        float x = defaultVal.x, y = defaultVal.y, z = defaultVal.z;
        char comma;
        ss >> x >> comma >> y >> comma >> z;
        return glm::vec3(x, y, z);
    };

    // 1. Extraer cámara
    glm::vec3 camPos = extractVec3("position", glm::vec3(0.0f, 3.5f, 7.5f));
    glm::vec3 camTarget = extractVec3("target", glm::vec3(0.0f, 0.0f, 0.0f));
    camera.resetToFocus(camTarget);
    camera.setPosition(camPos);
    camera.syncOrbitDistance();
    camera.setFOV(extractFloat("fov", 45.0f));
    camera.setOrthographic(extractBool("orthographic", false));
    camera.setNearPlane(extractFloat("nearPlane", 0.1f));
    camera.setFarPlane(extractFloat("farPlane", 1000.0f));

    // 2. Extraer iluminación y sombras
    outData.sunAzimuth = extractFloat("sunAzimuth", 45.0f);
    outData.sunElevation = extractFloat("sunElevation", 50.0f);
    outData.sunColor = extractVec3("sunColor", glm::vec3(1.0f, 0.96f, 0.88f));
    outData.sunIntensity = extractFloat("sunIntensity", 1.2f);
    outData.ambientColor = extractVec3("ambientColor", glm::vec3(0.24f, 0.27f, 0.35f));
    outData.ambientIntensity = extractFloat("ambientIntensity", 1.0f);
    outData.shadowsEnabled = extractBool("shadowsEnabled", true);
    outData.shadowRes = extractInt("shadowResolution", 4096);
    outData.shadowKernel = extractInt("shadowKernel", 3);
    outData.shadowSoftness = extractFloat("shadowSoftness", 0.57f);
    outData.shadowDistance = extractFloat("shadowDistance", 85.0f);
    outData.terrainCastsShadows = extractBool("terrainCastsShadows", false);
    outData.shadowFilterMode = extractInt("shadowFilterMode", 1);
    outData.pcssLightSize = extractFloat("pcssLightSize", 0.35f);
    outData.pcssBlockerSamples = extractInt("pcssBlockerSamples", 16);
    outData.pcssFilterSamples = extractInt("pcssFilterSamples", 32);
    outData.csmEnabled = extractBool("csmEnabled", true);

    // 3. Extraer gestión de color y adaptación ocular (Fase 11 y 17)
    outData.colorProfile = extractInt("profile", 0);
    outData.exposure = extractFloat("exposure", 0.52f);
    outData.gamma = extractFloat("gamma", 2.2f);
    outData.saturation = extractFloat("saturation", 1.0f);
    outData.contrast = extractFloat("contrast", 1.0f);
    outData.autoExposureEnabled = extractBool("autoExposureEnabled", true);
    outData.autoExposureKey = extractFloat("autoExposureKey", 0.39f);
    outData.autoExposureMin = extractFloat("autoExposureMin", 0.50f);
    outData.autoExposureMax = extractFloat("autoExposureMax", 3.24f);
    outData.autoExposureSpeedUp = extractFloat("autoExposureSpeedUp", 3.0f);
    outData.autoExposureSpeedDown = extractFloat("autoExposureSpeedDown", 1.0f);

    // 3.5. Extraer Cielo HDRI y Entorno (Fase 12)
    size_t skyboxPos = content.find("\"skybox\":");
    if (skyboxPos != std::string::npos) {
        size_t bOpen = content.find('{', skyboxPos);
        size_t bClose = content.find('}', bOpen);
        if (bOpen != std::string::npos && bClose != std::string::npos) {
            std::string skyBlock = content.substr(bOpen, bClose - bOpen + 1);
            auto getSkyStr = [&](const std::string& k) -> std::string {
                size_t p = skyBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return "";
                size_t q1 = skyBlock.find('"', p + k.size() + 2);
                if (q1 == std::string::npos) return "";
                size_t q2 = skyBlock.find('"', q1 + 1);
                if (q2 == std::string::npos) return "";
                return skyBlock.substr(q1 + 1, q2 - q1 - 1);
            };
            auto getSkyFloat = [&](const std::string& k, float defVal) -> float {
                size_t p = skyBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = skyBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stof(skyBlock.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getSkyBool = [&](const std::string& k, bool defVal) -> bool {
                size_t p = skyBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = skyBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                return (skyBlock.substr(c + 1, 8).find("true") != std::string::npos);
            };

            outData.useIBL = getSkyBool("useIBL", true);
            outData.showSkybox = getSkyBool("showSkybox", true);
            outData.skyboxExposure = getSkyFloat("exposure", 0.64f);
            outData.skyboxRotation = getSkyFloat("rotation", 0.0f);
            outData.skyboxBlur = getSkyFloat("blur", 0.0f);
            outData.iblIntensity = getSkyFloat("iblIntensity", 1.0f);
            outData.flipSkyboxX = getSkyBool("flipX", false);
            outData.flipSkyboxY = getSkyBool("flipY", false);
            outData.hdriPath = getSkyStr("hdriPath");
        }
    }

    // 3.6. Extraer Post-Proceso y Opciones Gráficas
    size_t postPos = content.find("\"postProcess\":");
    if (postPos != std::string::npos) {
        size_t bOpen = content.find('{', postPos);
        size_t bClose = content.find('}', bOpen);
        if (bOpen != std::string::npos && bClose != std::string::npos) {
            std::string postBlock = content.substr(bOpen, bClose - bOpen + 1);
            auto getPostFloat = [&](const std::string& k, float defVal) -> float {
                size_t p = postBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = postBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stof(postBlock.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getPostInt = [&](const std::string& k, int defVal) -> int {
                size_t p = postBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = postBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stoi(postBlock.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getPostBool = [&](const std::string& k, bool defVal) -> bool {
                size_t p = postBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = postBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                return (postBlock.substr(c + 1, 8).find("true") != std::string::npos);
            };

            outData.bloomEnabled = getPostBool("bloomEnabled", true);
            outData.bloomThreshold = getPostFloat("bloomThreshold", 0.65f);
            outData.bloomIntensity = getPostFloat("bloomIntensity", 0.60f);
            outData.fxaaEnabled = getPostBool("fxaaEnabled", true);
            outData.msaaLevel = getPostInt("msaaLevel", 4);
            outData.ssaoEnabled = getPostBool("ssaoEnabled", false);
            outData.ssaoRadius = getPostFloat("ssaoRadius", 0.50f);
            outData.ssaoBias = getPostFloat("ssaoBias", 0.035f);
            outData.ssaoPower = getPostFloat("ssaoPower", 1.8f);
            outData.ssaoIntensity = getPostFloat("ssaoIntensity", 0.85f);
            outData.dofEnabled = getPostBool("dofEnabled", false);
            outData.dofFocusDistance = getPostFloat("dofFocusDistance", 6.0f);
            outData.dofFocusRange = getPostFloat("dofFocusRange", 3.0f);
            outData.dofBokehRadius = getPostFloat("dofBokehRadius", 8.0f);
            outData.anisotropyLevel = getPostInt("anisotropyLevel", 16);
            outData.showGroundPlane = getPostBool("showGroundPlane", true);
            outData.showGroundGrid = getPostBool("showGroundGrid", true);
        }
    }

    // 3.7. Extraer Configuración y Capas PBR del Terreno 3D
    size_t terPos = content.find("\"terrain\":");
    if (terPos != std::string::npos) {
        size_t bOpen = content.find('{', terPos);
        size_t layArrayPos = content.find("\"layers\":", bOpen);
        if (bOpen != std::string::npos) {
            std::string terHead = content.substr(bOpen, (layArrayPos != std::string::npos ? layArrayPos - bOpen : 200));
            auto getTerFloat = [&](const std::string& k, float defVal) -> float {
                size_t p = terHead.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = terHead.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stof(terHead.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getTerBool = [&](const std::string& k, bool defVal) -> bool {
                size_t p = terHead.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = terHead.find(':', p);
                if (c == std::string::npos) return defVal;
                return (terHead.substr(c + 1, 8).find("true") != std::string::npos);
            };
            auto getTerInt = [&](const std::string& k, int defVal) -> int {
                size_t p = terHead.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = terHead.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stoi(terHead.substr(c + 1)); } catch (...) { return defVal; }
            };

            outData.terrainVisible = getTerBool("visible", true);
            outData.terrainBaseOffset = getTerFloat("baseOffset", -8.0f);
            outData.terrainReflectionIntensity = getTerFloat("reflectionIntensity", 1.0f);
            outData.terrainReliefMode = getTerInt("reliefMode", 0);
            outData.terrainTessellationScale = getTerFloat("tessellationScale", 1.0f);
            outData.terrainResolution = getTerInt("resolution", 128);
            outData.terrainWorldSize = getTerFloat("worldSize", 120.0f);

            // Cargar datos binarios de altura y splatmap del archivo complementario si existe
            std::string terrainBinPath = filePath + ".terrain";
            if (std::filesystem::exists(terrainBinPath)) {
                std::ifstream tBin(terrainBinPath, std::ios::binary);
                if (tBin.is_open()) {
                    char magic[4];
                    tBin.read(magic, 4);
                    if (magic[0] == 'A' && magic[1] == 'T' && magic[2] == 'E' && magic[3] == 'R') {
                        uint32_t version = 0;
                        tBin.read(reinterpret_cast<char*>(&version), sizeof(version));
                        int32_t res = 0;
                        float ws = 0.0f;
                        tBin.read(reinterpret_cast<char*>(&res), sizeof(res));
                        tBin.read(reinterpret_cast<char*>(&ws), sizeof(ws));
                        if (res > 0) outData.terrainResolution = res;
                        if (ws > 0.0f) outData.terrainWorldSize = ws;
                        uint32_t hCount = 0;
                        tBin.read(reinterpret_cast<char*>(&hCount), sizeof(hCount));
                        if (hCount > 0 && hCount < 20000000) {
                            outData.terrainHeights.resize(hCount);
                            tBin.read(reinterpret_cast<char*>(outData.terrainHeights.data()), hCount * sizeof(float));
                        }
                        uint32_t sCount = 0;
                        tBin.read(reinterpret_cast<char*>(&sCount), sizeof(sCount));
                        if (sCount > 0 && sCount < 80000000) {
                            outData.terrainSplatmap.resize(sCount);
                            tBin.read(reinterpret_cast<char*>(outData.terrainSplatmap.data()), sCount);
                        }
                        std::cout << "[SceneIO] ✓ Datos de relieve (" << hCount << " vertices) y Splatmap (" << sCount << " bytes) cargados desde: " << terrainBinPath << std::endl;
                    }
                    tBin.close();
                }
            }

            outData.terrainLayers.clear();
            if (layArrayPos != std::string::npos) {
                size_t lbStart = content.find('[', layArrayPos);
                size_t lbEnd = std::string::npos;
                if (lbStart != std::string::npos) {
                    int bDepth = 0;
                    for (size_t k = lbStart; k < content.size(); ++k) {
                        if (content[k] == '[') bDepth++;
                        else if (content[k] == ']') {
                            bDepth--;
                            if (bDepth == 0) {
                                lbEnd = k;
                                break;
                            }
                        }
                    }
                }
                if (lbStart != std::string::npos && lbEnd != std::string::npos) {
                    std::string layArrayStr = content.substr(lbStart + 1, lbEnd - lbStart - 1);
                    size_t lIdx = 0;
                    while (true) {
                        size_t oStart = layArrayStr.find('{', lIdx);
                        if (oStart == std::string::npos) break;
                        size_t oEnd = std::string::npos;
                        int objDepth = 0;
                        for (size_t k = oStart; k < layArrayStr.size(); ++k) {
                            if (layArrayStr[k] == '{') objDepth++;
                            else if (layArrayStr[k] == '}') {
                                objDepth--;
                                if (objDepth == 0) {
                                    oEnd = k;
                                    break;
                                }
                            }
                        }
                        if (oEnd == std::string::npos) break;
                        std::string block = layArrayStr.substr(oStart, oEnd - oStart + 1);
                        lIdx = oEnd + 1;

                        auto getLStr = [&](const std::string& k) -> std::string {
                            size_t p = block.find("\"" + k + "\"");
                            if (p == std::string::npos) return "";
                            size_t q1 = block.find('"', p + k.size() + 2);
                            if (q1 == std::string::npos) return "";
                            size_t q2 = block.find('"', q1 + 1);
                            if (q2 == std::string::npos) return "";
                            return block.substr(q1 + 1, q2 - q1 - 1);
                        };
                        auto getLF = [&](const std::string& k, float defVal) -> float {
                            size_t p = block.find("\"" + k + "\"");
                            if (p == std::string::npos) return defVal;
                            size_t c = block.find(':', p);
                            if (c == std::string::npos) return defVal;
                            try { return std::stof(block.substr(c + 1)); } catch (...) { return defVal; }
                        };
                        auto getLBool = [&](const std::string& k, bool defVal) -> bool {
                            size_t p = block.find("\"" + k + "\"");
                            if (p == std::string::npos) return defVal;
                            size_t c = block.find(':', p);
                            if (c == std::string::npos) return defVal;
                            return (block.substr(c + 1, 8).find("true") != std::string::npos);
                        };

                        TerrainLayerSaveData lay;
                        lay.name = getLStr("name");
                        lay.enabled = getLBool("enabled", true);
                        lay.visible = getLBool("visible", true);
                        lay.opacity = getLF("opacity", 1.0f);
                        lay.tiling = getLF("tiling", 15.0f);
                        lay.roughness = getLF("roughness", 0.8f);
                        lay.specular = getLF("specular", 0.5f);
                        lay.metallic = getLF("metallic", 0.0f);
                        lay.normalStrength = getLF("normalStrength", 1.0f);
                        lay.heightScale = getLF("heightScale", 0.03f);
                        lay.aoStrength = getLF("aoStrength", 1.0f);
                        lay.albedoPath = getLStr("albedoPath");
                        lay.normalPath = getLStr("normalPath");
                        lay.roughSpecPath = getLStr("roughSpecPath");
                        lay.heightPath = getLStr("heightPath");
                        lay.aoPath = getLStr("aoPath");

                        outData.terrainLayers.push_back(lay);
                    }
                }
            }
        }
    }

    // 3.8. Extraer Configuración de OASIS (Fase 18)
    size_t oasisPos = content.find("\"oasis\":");
    if (oasisPos != std::string::npos) {
        size_t bOpen = content.find('{', oasisPos);
        size_t bClose = content.find('}', bOpen);
        if (bOpen != std::string::npos && bClose != std::string::npos) {
            std::string oasisBlock = content.substr(bOpen, bClose - bOpen + 1);
            auto getOasisFloat = [&](const std::string& k, float defVal) -> float {
                size_t p = oasisBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = oasisBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stof(oasisBlock.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getOasisInt = [&](const std::string& k, int defVal) -> int {
                size_t p = oasisBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = oasisBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stoi(oasisBlock.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getOasisBool = [&](const std::string& k, bool defVal) -> bool {
                size_t p = oasisBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = oasisBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                return (oasisBlock.substr(c + 1, 8).find("true") != std::string::npos);
            };

            outData.oasisEnabled = getOasisBool("enabled", true);
            outData.oasisFrustumCulling = getOasisBool("frustumCulling", true);
            outData.oasisGeomorphing = getOasisBool("geomorphing", true);
            outData.oasisDebugLOD = getOasisBool("debugLOD", false);
            outData.oasisLodDistanceFactor = getOasisFloat("lodDistanceFactor", 1.0f);
            outData.oasisMaxLOD = getOasisInt("maxLOD", 4);
            outData.oasisPatchResolution = getOasisInt("patchResolution", 33);
            outData.meshletsEnabled = getOasisBool("meshletsEnabled", true);
            outData.meshletFrustumCulling = getOasisBool("meshletFrustumCulling", true);
            outData.meshletBackfaceCulling = getOasisBool("meshletBackfaceCulling", true);
            outData.meshletDebugColors = getOasisBool("meshletDebugColors", false);
        }
    }

    // 3.9. Extraer Configuración de Océano y Agua 3D
    size_t waterPos = content.find("\"water\":");
    if (waterPos != std::string::npos) {
        size_t bOpen = content.find('{', waterPos);
        size_t bClose = content.find('}', bOpen);
        if (bOpen != std::string::npos && bClose != std::string::npos) {
            std::string waterBlock = content.substr(bOpen, bClose - bOpen + 1);
            auto getWFloat = [&](const std::string& k, float defVal) -> float {
                size_t p = waterBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = waterBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stof(waterBlock.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getWBool = [&](const std::string& k, bool defVal) -> bool {
                size_t p = waterBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = waterBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                return (waterBlock.substr(c + 1, 8).find("true") != std::string::npos);
            };
            auto getWVec3 = [&](const std::string& k, const glm::vec3& defVal) -> glm::vec3 {
                size_t p = waterBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t bStart = waterBlock.find('[', p);
                size_t bEnd = waterBlock.find(']', bStart);
                if (bStart != std::string::npos && bEnd != std::string::npos) {
                    std::string aStr = waterBlock.substr(bStart + 1, bEnd - bStart - 1);
                    std::stringstream ss(aStr);
                    std::string item;
                    std::vector<float> vals;
                    while (std::getline(ss, item, ',')) {
                        try { vals.push_back(std::stof(item)); } catch (...) {}
                    }
                    if (vals.size() >= 3) return glm::vec3(vals[0], vals[1], vals[2]);
                }
                return defVal;
            };

            outData.waterEnabled = getWBool("enabled", true);
            outData.waterLevel = getWFloat("level", -3.5f);
            outData.waterWaveAmplitude = getWFloat("amplitude", 0.45f);
            outData.waterWaveSpeed = getWFloat("speed", 1.0f);
            outData.waterShallowColor = getWVec3("shallowColor", glm::vec3(0.08f, 0.58f, 0.65f));
            outData.waterDeepColor = getWVec3("deepColor", glm::vec3(0.01f, 0.08f, 0.28f));
            outData.waterClarity = getWFloat("clarity", 0.22f);
            outData.waterFoamThreshold = getWFloat("foamThreshold", 1.2f);
            outData.waterFoamIntensity = getWFloat("foamIntensity", 1.4f);
            outData.waterRefraction = getWFloat("refraction", 0.035f);
            outData.waterRoughness = getWFloat("roughness", 0.06f);
            outData.waterHorizonDistance = getWFloat("horizonDistance", 330.0f);
            outData.waterHorizonBlend = getWFloat("horizonBlend", 0.40f);
        }
    }

    // 3.95. Extraer Configuración de Fase 20 (Ciclo Día / Noche, Clima, SSR, Contact Shadows)
    size_t dnPos = content.find("\"dayNight\":");
    if (dnPos != std::string::npos) {
        size_t bOpen = content.find('{', dnPos);
        size_t bClose = content.find('}', bOpen);
        if (bOpen != std::string::npos && bClose != std::string::npos) {
            std::string dnBlock = content.substr(bOpen, bClose - bOpen + 1);
            auto getDNBool = [&](const std::string& k, bool defVal) -> bool {
                size_t p = dnBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = dnBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                return (dnBlock.substr(c + 1, 8).find("true") != std::string::npos);
            };
            auto getDNFloat = [&](const std::string& k, float defVal) -> float {
                size_t p = dnBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = dnBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stof(dnBlock.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getDNInt = [&](const std::string& k, int defVal) -> int {
                size_t p = dnBlock.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = dnBlock.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stoi(dnBlock.substr(c + 1)); } catch (...) { return defVal; }
            };

            outData.dayNightCycleEnabled = getDNBool("cycleEnabled", true);
            outData.dayNightTimeOfDay = getDNFloat("timeOfDay", 12.0f);
            outData.dayNightDuration = getDNFloat("duration", 180.0f);
            outData.dayNightSunLatitude = getDNFloat("sunLatitude", 35.0f);
            outData.dayNightHorizonHeight = getDNFloat("horizonHeight", 0.0f);
            outData.weatherType = getDNInt("weatherType", 0);
            outData.weatherWetness = getDNFloat("wetness", 0.0f);
        }
    }

    size_t p20Pos = content.find("\"phase20\":");
    if (p20Pos != std::string::npos) {
        size_t bOpen = content.find('{', p20Pos);
        size_t bClose = content.find('}', bOpen);
        if (bOpen != std::string::npos && bClose != std::string::npos) {
            std::string p20Block = content.substr(bOpen, bClose - bOpen + 1);
            auto getP20Bool = [&](const std::string& k, bool defVal) -> bool {
                size_t p = p20Block.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = p20Block.find(':', p);
                if (c == std::string::npos) return defVal;
                return (p20Block.substr(c + 1, 8).find("true") != std::string::npos);
            };
            auto getP20Float = [&](const std::string& k, float defVal) -> float {
                size_t p = p20Block.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = p20Block.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stof(p20Block.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getP20Int = [&](const std::string& k, int defVal) -> int {
                size_t p = p20Block.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t c = p20Block.find(':', p);
                if (c == std::string::npos) return defVal;
                try { return std::stoi(p20Block.substr(c + 1)); } catch (...) { return defVal; }
            };
            auto getP20Vec3 = [&](const std::string& k, glm::vec3 defVal) -> glm::vec3 {
                size_t p = p20Block.find("\"" + k + "\"");
                if (p == std::string::npos) return defVal;
                size_t b1 = p20Block.find('[', p);
                size_t b2 = p20Block.find(']', b1);
                if (b1 == std::string::npos || b2 == std::string::npos) return defVal;
                std::stringstream ss(p20Block.substr(b1 + 1, b2 - b1 - 1));
                std::string item;
                glm::vec3 val(0.0f);
                int idx = 0;
                while (std::getline(ss, item, ',') && idx < 3) {
                    try { val[idx++] = std::stof(item); } catch (...) {}
                }
                return val;
            };

            outData.ssrEnabled = getP20Bool("ssrEnabled", false);
            outData.ssrMaxSteps = getP20Int("ssrMaxSteps", 64);
            outData.ssrStepSize = getP20Float("ssrStepSize", 0.15f);
            outData.ssrThickness = getP20Float("ssrThickness", 0.05f);
            outData.ssrIntensity = getP20Float("ssrIntensity", 0.85f);
            outData.contactShadowsEnabled = getP20Bool("contactShadowsEnabled", false);
            outData.contactShadowsDistance = getP20Float("contactShadowsDistance", 0.25f);
            outData.contactShadowsRaySteps = getP20Int("contactShadowsRaySteps", 16);
            outData.contactShadowsIntensity = getP20Float("contactShadowsIntensity", 0.75f);

            outData.globalFogEnabled = getP20Bool("globalFogEnabled", true);
            outData.globalFogStart = getP20Float("globalFogStart", 20.0f);
            outData.globalFogEnd = getP20Float("globalFogEnd", 450.0f);
            outData.globalFogDensity = getP20Float("globalFogDensity", 0.45f);
            outData.globalFogColor = getP20Vec3("globalFogColor", glm::vec3(0.70f, 0.78f, 0.88f));
        }
    }

    // 4. Limpiar escena actual para poblarla con la del archivo
    scene.clear();

    // 5. Parsear entidades
    size_t entPos = content.find("\"entities\":");
    if (entPos != std::string::npos) {
        size_t openBracket = content.find('[', entPos);
        size_t closeBracket = std::string::npos;
        if (openBracket != std::string::npos) {
            int bDepth = 0;
            for (size_t k = openBracket; k < content.size(); ++k) {
                if (content[k] == '[') bDepth++;
                else if (content[k] == ']') {
                    bDepth--;
                    if (bDepth == 0) {
                        closeBracket = k;
                        break;
                    }
                }
            }
        }
        if (openBracket != std::string::npos && closeBracket != std::string::npos) {
            std::string entArrayStr = content.substr(openBracket + 1, closeBracket - openBracket - 1);
            size_t idx = 0;
            while (true) {
                size_t objStart = entArrayStr.find('{', idx);
                if (objStart == std::string::npos) break;
                // Buscar cierre correspondiente
                int depth = 0;
                size_t objEnd = objStart;
                for (size_t k = objStart; k < entArrayStr.size(); ++k) {
                    if (entArrayStr[k] == '{') depth++;
                    else if (entArrayStr[k] == '}') {
                        depth--;
                        if (depth == 0) {
                            objEnd = k;
                            break;
                        }
                    }
                }

                std::string entBlock = entArrayStr.substr(objStart, objEnd - objStart + 1);
                idx = objEnd + 1;

                auto getBlockStr = [&](const std::string& k) -> std::string {
                    size_t p = entBlock.find("\"" + k + "\"");
                    if (p == std::string::npos) return "";
                    size_t q1 = entBlock.find('"', p + k.size() + 2);
                    if (q1 == std::string::npos) return "";
                    size_t q2 = entBlock.find('"', q1 + 1);
                    if (q2 == std::string::npos) return "";
                    return entBlock.substr(q1 + 1, q2 - q1 - 1);
                };

                auto getBlockVec3 = [&](const std::string& k, glm::vec3 defVal) -> glm::vec3 {
                    size_t p = entBlock.find("\"" + k + "\"");
                    if (p == std::string::npos) return defVal;
                    size_t b1 = entBlock.find('[', p);
                    size_t b2 = entBlock.find(']', b1);
                    if (b1 == std::string::npos || b2 == std::string::npos) return defVal;
                    std::stringstream ss(entBlock.substr(b1 + 1, b2 - b1 - 1));
                    float x = defVal.x, y = defVal.y, z = defVal.z;
                    char comma;
                    ss >> x >> comma >> y >> comma >> z;
                    return glm::vec3(x, y, z);
                };

                auto getBlockFloat = [&](const std::string& k, float defVal) -> float {
                    size_t p = entBlock.find("\"" + k + "\"");
                    if (p == std::string::npos) return defVal;
                    size_t c = entBlock.find(':', p);
                    if (c == std::string::npos) return defVal;
                    try {
                        return std::stof(entBlock.substr(c + 1));
                    } catch (...) {
                        return defVal;
                    }
                };

                std::string eName = getBlockStr("name");
                std::string eType = getBlockStr("type");
                std::string mPath = getBlockStr("modelPath");
                if (eName.empty()) eName = "Objeto_" + std::to_string(scene.getEntityCount() + 1);

                std::shared_ptr<Entity> entity = nullptr;
                if (eType == "Sphere") {
                    entity = scene.createSphere(eName);
                } else if (eType == "Cylinder") {
                    entity = scene.createCylinder(eName);
                } else if (eType == "Cone") {
                    entity = scene.createCone(eName);
                } else if (eType == "Torus") {
                    entity = scene.createTorus(eName);
                } else if (eType == "Plane") {
                    entity = scene.createPlane(eName);
                } else if (eType == "Model" && !mPath.empty() && std::filesystem::exists(mPath)) {
                    entity = scene.createModel(eName, mPath);
                } else {
                    entity = scene.createCube(eName);
                }

                if (entity) {
                    std::string grp = getBlockStr("group");
                    entity->setGroup(grp);

                    glm::vec3 pos = getBlockVec3("position", glm::vec3(0.0f));
                    glm::vec3 rot = getBlockVec3("rotation", glm::vec3(0.0f));
                    glm::vec3 sc = getBlockVec3("scale", glm::vec3(1.0f));
                    glm::vec3 col = getBlockVec3("color", glm::vec3(1.0f));

                    entity->setPosition(pos.x, pos.y, pos.z);
                    entity->setRotation(rot.x, rot.y, rot.z);
                    entity->setScale(sc.x, sc.y, sc.z);
                    entity->setColor(col.r, col.g, col.b);

                    auto getBlockBool = [&](const std::string& k, bool defVal) -> bool {
                        size_t p = entBlock.find("\"" + k + "\"");
                        if (p == std::string::npos) return defVal;
                        size_t c = entBlock.find(':', p);
                        if (c == std::string::npos) return defVal;
                        return (entBlock.substr(c + 1, 8).find("true") != std::string::npos);
                    };

                    bool isTrig = getBlockBool("isTrigger", false);
                    std::string trigType = getBlockStr("triggerType");
                    entity->setTrigger(isTrig);
                    if (!trigType.empty()) entity->setTriggerType(trigType);

                    // Restaurar propiedades de físicas dinámicas Bullet
                    entity->setHasPhysics(getBlockBool("hasPhysics", isTrig));
                    entity->setMass(getBlockFloat("mass", isTrig ? 0.0f : 1.0f));
                    std::string cTypeStr = getBlockStr("colliderType");
                    if (!cTypeStr.empty()) entity->setColliderType(cTypeStr);
                    entity->setFriction(getBlockFloat("friction", 0.6f));
                    entity->setRestitution(getBlockFloat("restitution", 0.4f));
                    entity->setKinematic(getBlockBool("isKinematic", false));

                    auto mat = entity->getMaterial();
                    if (mat) {
                        mat->albedo = getBlockVec3("albedo", glm::vec3(1.0f));
                        mat->metallic = getBlockFloat("metallic", 0.0f);
                        mat->roughness = getBlockFloat("roughness", 0.5f);
                        mat->specular = getBlockFloat("specular", 0.5f);
                        mat->emission = getBlockVec3("emission", glm::vec3(0.0f));
                        mat->emissionStrength = getBlockFloat("emissionStrength", 1.0f);
                        mat->normalStrength = getBlockFloat("normalStrength", 1.0f);
                        mat->ao = getBlockFloat("ao", 1.0f);

                        // Restaurar mapas de texturas PBR del material
                        std::string alb = getBlockStr("albedoMap");
                        std::string nrm = getBlockStr("normalMap");
                        std::string rgh = getBlockStr("roughnessMap");
                        std::string met = getBlockStr("metallicMap");
                        std::string spc = getBlockStr("specularMap");
                        std::string ems = getBlockStr("emissionMap");
                        std::string aoMapPath = getBlockStr("aoMap");

                        if (!alb.empty() && std::filesystem::exists(alb)) {
                            try { mat->setAlbedoMap(std::make_shared<Texture>(alb)); } catch (...) {}
                        }
                        if (!nrm.empty() && std::filesystem::exists(nrm)) {
                            try { mat->setNormalMap(std::make_shared<Texture>(nrm)); } catch (...) {}
                        }
                        if (!rgh.empty() && std::filesystem::exists(rgh)) {
                            try { mat->setRoughnessMap(std::make_shared<Texture>(rgh)); } catch (...) {}
                        }
                        if (!met.empty() && std::filesystem::exists(met)) {
                            try { mat->setMetallicMap(std::make_shared<Texture>(met)); } catch (...) {}
                        }
                        if (!spc.empty() && std::filesystem::exists(spc)) {
                            try { mat->setSpecularMap(std::make_shared<Texture>(spc)); } catch (...) {}
                        }
                        if (!ems.empty() && std::filesystem::exists(ems)) {
                            try { mat->setEmissionMap(std::make_shared<Texture>(ems)); } catch (...) {}
                        }
                        if (!aoMapPath.empty() && std::filesystem::exists(aoMapPath)) {
                            try { mat->setAOMap(std::make_shared<Texture>(aoMapPath)); } catch (...) {}
                        }
                    }
                }
            }
        }
    }

    // 6. Restaurar luces puntuales
    scene.getPointLights().clear();
    size_t plPos = content.find("\"pointLights\":");
    if (plPos != std::string::npos) {
        size_t bStart = content.find('[', plPos);
        size_t bEnd = std::string::npos;
        if (bStart != std::string::npos) {
            int bDepth = 0;
            for (size_t k = bStart; k < content.size(); ++k) {
                if (content[k] == '[') bDepth++;
                else if (content[k] == ']') {
                    bDepth--;
                    if (bDepth == 0) {
                        bEnd = k;
                        break;
                    }
                }
            }
        }
        if (bStart != std::string::npos && bEnd != std::string::npos) {
            std::string plArrayStr = content.substr(bStart + 1, bEnd - bStart - 1);
            size_t idx = 0;
            while (true) {
                size_t oStart = plArrayStr.find('{', idx);
                if (oStart == std::string::npos) break;
                size_t oEnd = plArrayStr.find('}', oStart);
                if (oEnd == std::string::npos) break;

                std::string block = plArrayStr.substr(oStart, oEnd - oStart + 1);
                idx = oEnd + 1;

                auto getStr = [&](const std::string& k) -> std::string {
                    size_t p = block.find("\"" + k + "\"");
                    if (p == std::string::npos) return "";
                    size_t q1 = block.find('"', p + k.size() + 2);
                    size_t q2 = block.find('"', q1 + 1);
                    return block.substr(q1 + 1, q2 - q1 - 1);
                };
                auto getVec3 = [&](const std::string& k, glm::vec3 defVal) -> glm::vec3 {
                    size_t p = block.find("\"" + k + "\"");
                    if (p == std::string::npos) return defVal;
                    size_t b1 = block.find('[', p);
                    size_t b2 = block.find(']', b1);
                    std::stringstream ss(block.substr(b1 + 1, b2 - b1 - 1));
                    float x = defVal.x, y = defVal.y, z = defVal.z;
                    char comma;
                    ss >> x >> comma >> y >> comma >> z;
                    return glm::vec3(x, y, z);
                };
                auto getF = [&](const std::string& k, float defVal) -> float {
                    size_t p = block.find("\"" + k + "\"");
                    if (p == std::string::npos) return defVal;
                    size_t c = block.find(':', p);
                    return std::stof(block.substr(c + 1));
                };

                PointLight pl(getStr("name"), getVec3("position", glm::vec3(0, 3, 0)),
                              getVec3("color", glm::vec3(1.0f)), getF("intensity", 2.0f));
                if (block.find("\"enabled\": false") != std::string::npos) pl.enabled = false;
                if (block.find("\"castShadows\": true") != std::string::npos) pl.castShadows = true;
                pl.lightSize = getF("lightSize", 0.25f);
                pl.bias = getF("bias", 0.025f);
                scene.addPointLight(pl);
            }
        }
    }

    std::cout << "[SceneIO] ✓ Escena cargada con exito desde archivo nativo: " << filePath
              << " (" << scene.getEntityCount() << " entidades, " << scene.getPointLights().size() << " luces)\n";

    return true;
}

} // namespace ASIS
