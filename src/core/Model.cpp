#include "Model.h"
#include <iostream>
#include <filesystem>

namespace ASIS {

Model::Model(const std::string& path)
    : m_path(path)
{
    loadModel(path);
}

void Model::draw(const Shader& shader) const {
    shader.use();
    if (m_animator && m_animator->hasActiveAnimation()) {
        shader.setInt("u_IsAnimated", 1);
        const auto& transforms = m_animator->getFinalBoneMatrices();
        int count = std::min((int)transforms.size(), 100);
        for (int i = 0; i < count; ++i) {
            shader.setMat4("u_BoneTransforms[" + std::to_string(i) + "]", transforms[i]);
        }
    } else {
        shader.setInt("u_IsAnimated", 0);
    }

    for (const auto& mesh : m_meshes) {
        mesh->draw(&shader);
    }
}

void Model::drawOASIS(const Shader& shader, const glm::mat4& modelMatrix, const Camera* camera,
                     const OASISMeshletConfig& config, OASISMeshletTelemetry& telemetry) const {
    if (m_animator && m_animator->hasActiveAnimation()) {
        shader.setInt("u_IsAnimated", 1);
        const auto& transforms = m_animator->getFinalBoneMatrices();
        for (size_t i = 0; i < transforms.size() && i < 100; ++i) {
            shader.setMat4("u_BoneTransforms[" + std::to_string(i) + "]", transforms[i]);
        }
    } else {
        shader.setInt("u_IsAnimated", 0);
    }

    for (const auto& mesh : m_meshes) {
        if (mesh) {
            mesh->drawOASIS(&shader, modelMatrix, camera, config, telemetry);
        }
    }
}

void Model::updateAnimation(float dt) {
    if (m_animator) {
        m_animator->updateAnimation(dt);
    }
}

void Model::addTexture(std::shared_ptr<Texture> texture) {
    if (!texture) return;
    for (auto& mesh : m_meshes) {
        mesh->addTexture(texture);
    }
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;

    // Buscar el archivo en rutas comunes
    std::vector<std::string> candidates = {
        path,
        "assets/" + path,
        "../" + path,
        "../../" + path,
        "../../assets/" + path
    };

    std::string resolvedPath = path;
    const aiScene* scene = nullptr;

    for (const auto& candidate : candidates) {
        scene = importer.ReadFile(candidate,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices);

        if (scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode) {
            resolvedPath = candidate;
            break;
        }
    }

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "[ASIS Engine ERROR Assimp] Fallo al cargar el modelo: " << importer.GetErrorString() << std::endl;
        throw std::runtime_error("Error Assimp: " + std::string(importer.GetErrorString()));
    }

    // Extraer el directorio base del modelo para buscar sus texturas asociadas
    std::filesystem::path p(resolvedPath);
    m_directory = p.parent_path().string();

    std::cout << "[ASIS Engine] Cargando modelo 3D: " << resolvedPath << " (Meshes en escena: " << scene->mNumMeshes << ")\n";

    // Inicializar material PBR
    m_material = std::make_shared<Material>();

    // Extraer propiedades PBR del primer material disponible
    if (scene->mNumMaterials > 0) {
        aiMaterial* mat = scene->mMaterials[0];
        
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            m_material->albedo = glm::vec3(color.r, color.g, color.b);
        }

        float shininess = 0.0f;
        if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS && shininess > 0.0f) {
            m_material->roughness = glm::clamp(1.0f - (shininess / 128.0f), 0.04f, 1.0f);
        }

        // Texturas Albedo / Difusa
        auto albedoMaps = loadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse");
        if (albedoMaps.empty()) {
            albedoMaps = loadMaterialTextures(mat, aiTextureType_BASE_COLOR, "texture_diffuse");
        }
        if (!albedoMaps.empty()) {
            m_material->setAlbedoMap(albedoMaps[0]);
        }

        // Texturas de Normales / Relieve
        auto normalMaps = loadMaterialTextures(mat, aiTextureType_NORMALS, "texture_normal");
        if (normalMaps.empty()) {
            normalMaps = loadMaterialTextures(mat, aiTextureType_HEIGHT, "texture_normal");
        }
        if (!normalMaps.empty()) {
            m_material->setNormalMap(normalMaps[0]);
        }

        // Texturas de Rugosidad y Metalicidad
        auto roughMaps = loadMaterialTextures(mat, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness");
        if (!roughMaps.empty()) {
            m_material->setRoughnessMap(roughMaps[0]);
        }

        auto metalMaps = loadMaterialTextures(mat, aiTextureType_METALNESS, "texture_metallic");
        if (!metalMaps.empty()) {
            m_material->setMetallicMap(metalMaps[0]);
        }

        // Textura Especular
        auto specMaps = loadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular");
        if (!specMaps.empty()) {
            m_material->setSpecularMap(specMaps[0]);
        }

        // Textura de Emisión
        auto emissiveMaps = loadMaterialTextures(mat, aiTextureType_EMISSIVE, "texture_emission");
        if (!emissiveMaps.empty()) {
            m_material->setEmissionMap(emissiveMaps[0]);
        }

        // Textura de Oclusión Ambiental
        auto aoMaps = loadMaterialTextures(mat, aiTextureType_AMBIENT_OCCLUSION, "texture_ao");
        if (aoMaps.empty()) {
            aoMaps = loadMaterialTextures(mat, aiTextureType_LIGHTMAP, "texture_ao");
        }
        if (!aoMaps.empty()) {
            m_material->setAOMap(aoMaps[0]);
        }
    }

    processNode(scene->mRootNode, scene);

    // 5. Cargar animaciones esqueléticas si existen en el archivo (Fase 13)
    if (scene->mNumAnimations > 0) {
        std::cout << "[ASIS Engine] Animaciones esqueleticas detectadas: " << scene->mNumAnimations << " clips.\n";
        auto anim = std::make_shared<SkeletalAnimation>(resolvedPath, scene);
        for (const auto& pair : m_boneInfoMap) {
            anim->getBoneInfoMapMutable()[pair.first] = pair.second;
        }
        m_animator = std::make_unique<Animator>();
        m_animator->playAnimation(anim);
    }

    std::cout << "[ASIS Engine] Modelo cargado con exito: " << m_meshes.size() << " mallas procesadas.\n";
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // Procesar todas las mallas asociadas a este nodo
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }

    // Recorrer recursivamente todos los nodos hijos
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene);
    }
}

std::unique_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;

    // 1. Procesar vértices (posiciones, normales, coordenadas de textura)
    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;

        // Posición
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        // Normales
        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Coordenadas UV de textura
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        // Color base suave por defecto
        vertex.color = glm::vec3(0.85f, 0.85f, 0.88f);

        vertices.push_back(vertex);
    }

    // 1.1 Extraer datos de influencia de huesos para animacion esqueletica (Fase 13)
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        if (m_boneInfoMap.find(boneName) == m_boneInfoMap.end()) {
            BoneInfo newBoneInfo;
            newBoneInfo.id = m_boneCounter++;
            const aiMatrix4x4& from = mesh->mBones[boneIndex]->mOffsetMatrix;
            glm::mat4 to;
            to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
            to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
            to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
            to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
            newBoneInfo.offset = to;
            m_boneInfoMap[boneName] = newBoneInfo;
            boneID = newBoneInfo.id;
        } else {
            boneID = m_boneInfoMap[boneName].id;
        }

        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            if (vertexId < (int)vertices.size()) {
                vertices[vertexId].addBoneData(boneID, weight);
            }
        }
    }

    // 2. Procesar índices de las caras trianguladas
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // 3. Procesar materiales y texturas
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Texturas difusas (color principal)
        std::vector<std::shared_ptr<Texture>> diffuseMaps =
            loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    return std::make_unique<Mesh>(vertices, indices, textures);
}

std::vector<std::shared_ptr<Texture>> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) {
    std::vector<std::shared_ptr<Texture>> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::string rawPath = std::string(str.C_Str());
        std::replace(rawPath.begin(), rawPath.end(), '\\', '/');

        std::filesystem::path fp(rawPath);
        std::string filenameOnly = fp.filename().string();

        std::vector<std::string> searchPaths = {
            rawPath,
            m_directory.empty() ? rawPath : (m_directory + "/" + rawPath),
            m_directory.empty() ? filenameOnly : (m_directory + "/" + filenameOnly),
            m_directory + "/textures/" + filenameOnly,
            m_directory + "/Textures/" + filenameOnly,
            "assets/textures/" + filenameOnly,
            "textures/" + filenameOnly
        };

        std::string resolvedTexturePath = "";
        for (const auto& sp : searchPaths) {
            if (std::filesystem::exists(sp)) {
                resolvedTexturePath = sp;
                break;
            }
        }

        if (resolvedTexturePath.empty()) {
            std::cout << "[ASIS Model] Advertencia: Textura '" << rawPath << "' no encontrada en disco.\n";
            continue;
        }

        bool alreadyLoaded = false;
        for (const auto& loadedTex : m_texturesLoaded) {
            if (loadedTex->getPath() == resolvedTexturePath) {
                textures.push_back(loadedTex);
                alreadyLoaded = true;
                break;
            }
        }

        if (!alreadyLoaded) {
            try {
                auto texture = std::make_shared<Texture>(resolvedTexturePath, typeName);
                textures.push_back(texture);
                m_texturesLoaded.push_back(texture);
                std::cout << "[ASIS Model] ✓ Textura cargada: " << resolvedTexturePath << " (" << typeName << ")\n";
            } catch (const std::exception& e) {
                std::cerr << "[ASIS Model ERROR] No se pudo cargar textura: " << resolvedTexturePath << " (" << e.what() << ")\n";
            }
        }
    }
    return textures;
}

} // namespace ASIS
