#pragma once

#include <vector>
#include <string>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "Material.h"
#include "Animation.h"
#include "OASISMeshlet.h"

namespace ASIS {

class Model {
public:
    // Carga un modelo 3D desde disco (.obj, .fbx, .dae, .gltf, etc.)
    Model(const std::string& path);
    ~Model() = default;

    // Renderiza todas las submallas que componen el modelo
    void draw(const Shader& shader) const;
    void drawOASIS(const Shader& shader, const glm::mat4& modelMatrix, const Camera* camera,
                   const OASISMeshletConfig& config, OASISMeshletTelemetry& telemetry) const;

    // Asignar textura a las submallas si el modelo no incluye texturas en su material
    void addTexture(std::shared_ptr<Texture> texture);

    // Sistema de Animación Esquelética (Fase 13)
    bool hasAnimation() const { return m_animator && m_animator->hasActiveAnimation(); }
    Animator* getAnimator() { return m_animator.get(); }
    const Animator* getAnimator() const { return m_animator.get(); }
    void updateAnimation(float dt);

    const std::string& getPath() const { return m_path; }
    size_t getMeshCount() const { return m_meshes.size(); }
    const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return m_meshes; }
    std::shared_ptr<Material> getMaterial() const { return m_material; }

private:
    std::string m_path;
    std::string m_directory;
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::vector<std::shared_ptr<Texture>> m_texturesLoaded;
    std::shared_ptr<Material> m_material;

    // Huesos y Animaciones
    std::unique_ptr<Animator> m_animator;
    std::map<std::string, BoneInfo> m_boneInfoMap;
    int m_boneCounter = 0;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
};

} // namespace ASIS
