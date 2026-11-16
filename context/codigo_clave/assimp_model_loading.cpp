// Snippet Clave: Carga de Modelos 3D (.obj / .fbx) con Assimp
// Archivo de referencia: src/core/Model.cpp

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <stdexcept>

const aiScene* load3DModel(const std::string& path, Assimp::Importer& importer) {
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |           // Forzar todas las caras a triángulos
        aiProcess_GenSmoothNormals |      // Generar normales suaves si el modelo no las tiene
        aiProcess_FlipUVs |               // Invertir eje V de texturas para OpenGL
        aiProcess_JoinIdenticalVertices); // Optimizar vértices compartidos

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        throw std::runtime_error(importer.GetErrorString());
    }

    return scene;
}
