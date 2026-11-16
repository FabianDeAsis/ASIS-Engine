#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Texture.h"
#include "OASISMeshlet.h"

namespace ASIS {

struct Vertex {
    glm::vec3 position;  // Coordenadas espaciales (X, Y, Z)
    glm::vec3 normal;    // Vector normal para iluminación (X, Y, Z)
    glm::vec2 texCoords; // Coordenadas de mapeo de textura UV (U, V)
    glm::vec3 color;     // Color de vértice RGB
    int boneIDs[4];      // IDs de hasta 4 huesos influyentes (Fase 13)
    float weights[4];    // Pesos normalizados de cada hueso (Fase 13)

    Vertex(glm::vec3 pos = glm::vec3(0.0f),
           glm::vec3 norm = glm::vec3(0.0f, 1.0f, 0.0f),
           glm::vec2 uv = glm::vec2(0.0f),
           glm::vec3 col = glm::vec3(1.0f))
        : position(pos), normal(norm), texCoords(uv), color(col)
    {
        for (int i = 0; i < 4; ++i) {
            boneIDs[i] = -1;
            weights[i] = 0.0f;
        }
    }

    void addBoneData(int boneID, float weight) {
        for (int i = 0; i < 4; ++i) {
            if (boneIDs[i] < 0) {
                boneIDs[i] = boneID;
                weights[i] = weight;
                return;
            }
        }
        int minIdx = 0;
        for (int i = 1; i < 4; ++i) {
            if (weights[i] < weights[minIdx]) minIdx = i;
        }
        if (weight > weights[minIdx]) {
            boneIDs[minIdx] = boneID;
            weights[minIdx] = weight;
        }
    }
};

class Shader;

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<unsigned int>& indices = {},
         const std::vector<std::shared_ptr<Texture>>& textures = {});
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void draw(const Shader* shader = nullptr) const;
    void drawOASIS(const Shader* shader, const glm::mat4& modelMatrix, const Camera* camera,
                   const OASISMeshletConfig& config, OASISMeshletTelemetry& telemetry) const;

    const OASISMeshletContainer& getMeshletContainer() const { return m_meshlets; }

    void addTexture(std::shared_ptr<Texture> texture) {
        if (texture) m_textures.push_back(texture);
    }

    bool hasTextures() const { return !m_textures.empty(); }

    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    const std::vector<unsigned int>& getIndices() const { return m_indices; }
    bool isLines() const { return m_isLines; }

    static std::unique_ptr<Mesh> createTriangle();
    static std::unique_ptr<Mesh> createCube(std::shared_ptr<Texture> texture = nullptr);
    static std::unique_ptr<Mesh> createGrid(float size = 40.0f, int divisions = 40);
    static std::unique_ptr<Mesh> createPlane(float size = 80.0f);
    static std::unique_ptr<Mesh> createSphere(float radius = 0.5f, unsigned int rings = 32, unsigned int sectors = 32);
    static std::unique_ptr<Mesh> createCylinder(float radius = 0.5f, float height = 1.0f, unsigned int sectors = 32);
    static std::unique_ptr<Mesh> createCone(float radius = 0.5f, float height = 1.0f, unsigned int sectors = 32);
    static std::unique_ptr<Mesh> createTorus(float mainRadius = 0.6f, float tubeRadius = 0.2f, unsigned int mainSegments = 32, unsigned int tubeSegments = 16);

private:
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<std::shared_ptr<Texture>> m_textures;
    bool m_isLines;
    OASISMeshletContainer m_meshlets;

    void setupMesh();
};

} // namespace ASIS
