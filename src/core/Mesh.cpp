#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "OASISTerrain.h"
#include <iostream>

namespace ASIS {

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<unsigned int>& indices,
           const std::vector<std::shared_ptr<Texture>>& textures)
    : m_VAO(0), m_VBO(0), m_EBO(0),
      m_vertices(vertices), m_indices(indices), m_textures(textures),
      m_isLines(false)
{
    setupMesh();
}

Mesh::~Mesh() {
    if (m_EBO != 0) glDeleteBuffers(1, &m_EBO);
    if (m_VBO != 0) glDeleteBuffers(1, &m_VBO);
    if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO),
      m_vertices(std::move(other.m_vertices)),
      m_indices(std::move(other.m_indices)),
      m_textures(std::move(other.m_textures)),
      m_isLines(other.m_isLines)
{
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_EBO = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (m_EBO != 0) glDeleteBuffers(1, &m_EBO);
        if (m_VBO != 0) glDeleteBuffers(1, &m_VBO);
        if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);

        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_EBO = other.m_EBO;
        m_vertices = std::move(other.m_vertices);
        m_indices = std::move(other.m_indices);
        m_textures = std::move(other.m_textures);
        m_isLines = other.m_isLines;

        other.m_VAO = 0;
        other.m_VBO = 0;
        other.m_EBO = 0;
    }
    return *this;
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

    if (!m_indices.empty()) {
        m_meshlets.buildFromMesh(m_vertices, m_indices, 64);
        glGenBuffers(1, &m_EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);
    }

    // Atributo 0: Posición vec3 (X, Y, Z)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Atributo 1: Normal vec3 (NX, NY, NZ)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Atributo 2: Coordenadas UV vec2 (U, V)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    // Atributo 3: Color vec3 (R, G, B)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    // Atributo 4: Huesos ivec4 (Bone IDs) (Fase 13)
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));

    // Atributo 5: Pesos de Hueso vec4 (Bone Weights) (Fase 13)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

    glBindVertexArray(0);
}

void Mesh::draw(const Shader* shader) const {
    if (shader) {
        if (!m_textures.empty()) {
            for (size_t i = 0; i < m_textures.size(); ++i) {
                m_textures[i]->bind(static_cast<unsigned int>(i));
            }
            shader->setInt("u_DiffuseTexture", 0);
            shader->setInt("u_UseTexture", 1);
        } else {
            Texture::getDefaultWhite()->bind(0);
            shader->setInt("u_DiffuseTexture", 0);
            shader->setInt("u_UseTexture", 0);
        }
    }

    glBindVertexArray(m_VAO);

    if (m_isLines) {
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    } else if (!m_indices.empty()) {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
    }

    glBindVertexArray(0);
}

void Mesh::drawOASIS(const Shader* shader, const glm::mat4& modelMatrix, const Camera* camera,
                     const OASISMeshletConfig& config, OASISMeshletTelemetry& telemetry) const {
    if (!m_meshlets.hasMeshlets() || !config.enabled || !camera) {
        draw(shader);
        return;
    }

    if (shader) {
        if (!m_textures.empty()) {
            for (size_t i = 0; i < m_textures.size(); ++i) {
                m_textures[i]->bind(static_cast<unsigned int>(i));
            }
            shader->setInt("u_DiffuseTexture", 0);
            shader->setInt("u_UseTexture", 1);
        } else {
            Texture::getDefaultWhite()->bind(0);
            shader->setInt("u_DiffuseTexture", 0);
            shader->setInt("u_UseTexture", 0);
        }
    }

    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float aspect = (vp[3] > 0) ? (static_cast<float>(vp[2]) / static_cast<float>(vp[3])) : (16.0f / 9.0f);

    Frustum frustum;
    frustum.update(camera->getProjectionMatrix(aspect) * camera->getViewMatrix());

    std::vector<GLsizei> counts;
    std::vector<void*> offsets;
    std::vector<glm::vec3> colors;
    counts.reserve(m_meshlets.getMeshletCount());
    offsets.reserve(m_meshlets.getMeshletCount());
    colors.reserve(m_meshlets.getMeshletCount());

    m_meshlets.cullAndCollect(modelMatrix, camera->getPosition(), frustum, config, counts, offsets, colors, telemetry);

    if (counts.empty()) {
        return;
    }

    glBindVertexArray(m_VAO);

    if (config.debugColors && shader) {
        shader->setInt("u_OASIS_DebugMeshlets", 1);
        for (size_t i = 0; i < counts.size(); ++i) {
            shader->setVec3("u_OASIS_MeshletColor", colors[i]);
            glDrawElements(GL_TRIANGLES, counts[i], GL_UNSIGNED_INT, offsets[i]);
        }
        shader->setInt("u_OASIS_DebugMeshlets", 0);
    } else {
        if (shader) {
            shader->setInt("u_OASIS_DebugMeshlets", 0);
        }
        glMultiDrawElements(GL_TRIANGLES, counts.data(), GL_UNSIGNED_INT, offsets.data(), static_cast<GLsizei>(counts.size()));
    }

    glBindVertexArray(0);
}

std::unique_ptr<Mesh> Mesh::createTriangle() {
    std::vector<Vertex> vertices = {
        Vertex({  0.0f,  0.6f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 1.0f }, { 1.0f, 0.1f, 0.1f }),
        Vertex({ -0.6f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.1f, 1.0f, 0.2f }),
        Vertex({  0.6f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.1f, 0.3f, 1.0f })
    };
    return std::make_unique<Mesh>(vertices);
}

std::unique_ptr<Mesh> Mesh::createCube(std::shared_ptr<Texture> texture) {
    // 36 vértices con posiciones, normales hacia afuera y UVs para las 6 caras
    std::vector<Vertex> vertices = {
        // Cara Trasera (Normal: 0, 0, -1)
        Vertex({-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}),
        Vertex({ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {0.8f, 0.8f, 0.8f}),
        Vertex({ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}),
        Vertex({ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {0.8f, 0.8f, 0.8f}),
        Vertex({-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}),
        Vertex({-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {0.8f, 0.8f, 0.8f}),

        // Cara Frontal (Normal: 0, 0, 1)
        Vertex({-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}),
        Vertex({ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {0.9f, 0.9f, 0.9f}),
        Vertex({ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {0.9f, 0.9f, 0.9f}),
        Vertex({ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {0.9f, 0.9f, 0.9f}),
        Vertex({-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {0.9f, 0.9f, 0.9f}),
        Vertex({-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}),

        // Cara Izquierda (Normal: -1, 0, 0)
        Vertex({-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.7f, 0.7f, 0.7f}),

        // Cara Derecha (Normal: 1, 0, 0)
        Vertex({ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.7f, 0.7f, 0.7f}),
        Vertex({ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f}),

        // Cara Inferior (Normal: 0, -1, 0)
        Vertex({-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {0.5f, 0.5f, 0.5f}),
        Vertex({ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {0.5f, 0.5f, 0.5f}),
        Vertex({ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}),
        Vertex({ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}),
        Vertex({-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}),
        Vertex({-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {0.5f, 0.5f, 0.5f}),

        // Cara Superior (Normal: 0, 1, 0)
        Vertex({-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}),
        Vertex({ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}),
        Vertex({ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}),
        Vertex({ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}),
        Vertex({-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}),
        Vertex({-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f})
    };

    std::vector<std::shared_ptr<Texture>> texs;
    if (texture) texs.push_back(texture);

    return std::make_unique<Mesh>(vertices, std::vector<unsigned int>{}, texs);
}

std::unique_ptr<Mesh> Mesh::createGrid(float size, int divisions) {
    std::vector<Vertex> vertices;
    float halfSize = size / 2.0f;
    float step = size / divisions;

    glm::vec3 gridColor(0.28f, 0.32f, 0.38f); // Gris azulado para la cuadrícula
    glm::vec3 xAxisColor(0.85f, 0.20f, 0.20f); // Rojo para el eje X (como en Blender/Unreal)
    glm::vec3 zAxisColor(0.20f, 0.45f, 0.90f); // Azul para el eje Z

    for (int i = 0; i <= divisions; ++i) {
        float coord = -halfSize + i * step;

        // Líneas paralelas al eje Z (variando X)
        glm::vec3 cZ = (std::abs(coord) < 0.001f) ? zAxisColor : gridColor;
        vertices.push_back(Vertex({coord, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, cZ));
        vertices.push_back(Vertex({coord, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, cZ));

        // Líneas paralelas al eje X (variando Z)
        glm::vec3 cX = (std::abs(coord) < 0.001f) ? xAxisColor : gridColor;
        vertices.push_back(Vertex({-halfSize, 0.0f, coord}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, cX));
        vertices.push_back(Vertex({ halfSize, 0.0f, coord}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, cX));
    }

    auto grid = std::make_unique<Mesh>(vertices);
    grid->m_isLines = true;
    return grid;
}

std::unique_ptr<Mesh> Mesh::createPlane(float size) {
    float h = size / 2.0f;
    std::vector<Vertex> vertices = {
        Vertex({-h, 0.0f, -h}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.11f, 0.13f, 0.17f}),
        Vertex({-h, 0.0f,  h}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.11f, 0.13f, 0.17f}),
        Vertex({ h, 0.0f,  h}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.11f, 0.13f, 0.17f}),
        Vertex({-h, 0.0f, -h}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.11f, 0.13f, 0.17f}),
        Vertex({ h, 0.0f,  h}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.11f, 0.13f, 0.17f}),
        Vertex({ h, 0.0f, -h}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.11f, 0.13f, 0.17f})
    };
    return std::make_unique<Mesh>(vertices);
}

std::unique_ptr<Mesh> Mesh::createSphere(float radius, unsigned int rings, unsigned int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265358979323846f;
    float const R = 1.0f / static_cast<float>(rings - 1);
    float const S = 1.0f / static_cast<float>(sectors - 1);

    for (unsigned int r = 0; r < rings; ++r) {
        for (unsigned int s = 0; s < sectors; ++s) {
            float y = sinf(-PI / 2.0f + PI * r * R);
            float x = cosf(2.0f * PI * s * S) * sinf(PI * r * R);
            float z = sinf(2.0f * PI * s * S) * sinf(PI * r * R);

            glm::vec3 pos(x * radius, y * radius, z * radius);
            glm::vec3 norm = glm::normalize(pos);
            glm::vec2 uv(s * S, r * R);

            vertices.push_back(Vertex(pos, norm, uv, glm::vec3(0.9f, 0.9f, 0.9f)));
        }
    }

    for (unsigned int r = 0; r < rings - 1; ++r) {
        for (unsigned int s = 0; s < sectors - 1; ++s) {
            unsigned int cur = r * sectors + s;
            unsigned int next = cur + sectors;

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(next + 1);

            indices.push_back(cur);
            indices.push_back(next + 1);
            indices.push_back(cur + 1);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createCylinder(float radius, float height, unsigned int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265358979323846f;
    float halfH = height * 0.5f;
    float step = 2.0f * PI / sectors;

    // 1. Paredes laterales
    for (unsigned int i = 0; i <= sectors; ++i) {
        float angle = i * step;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;
        glm::vec3 norm = glm::normalize(glm::vec3(x, 0.0f, z));
        float u = static_cast<float>(i) / sectors;

        vertices.push_back(Vertex(glm::vec3(x,  halfH, z), norm, glm::vec2(u, 1.0f), glm::vec3(0.9f)));
        vertices.push_back(Vertex(glm::vec3(x, -halfH, z), norm, glm::vec2(u, 0.0f), glm::vec3(0.9f)));
    }

    for (unsigned int i = 0; i < sectors; ++i) {
        unsigned int top1 = i * 2;
        unsigned int bot1 = top1 + 1;
        unsigned int top2 = (i + 1) * 2;
        unsigned int bot2 = top2 + 1;

        indices.push_back(top1);
        indices.push_back(bot1);
        indices.push_back(top2);

        indices.push_back(top2);
        indices.push_back(bot1);
        indices.push_back(bot2);
    }

    // 2. Tapa Superior (Y = +halfH)
    unsigned int topCenterIdx = static_cast<unsigned int>(vertices.size());
    vertices.push_back(Vertex(glm::vec3(0.0f, halfH, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f), glm::vec3(0.9f)));
    unsigned int topStartIdx = static_cast<unsigned int>(vertices.size());
    for (unsigned int i = 0; i <= sectors; ++i) {
        float angle = i * step;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;
        vertices.push_back(Vertex(glm::vec3(x, halfH, z), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(x / (2.0f * radius) + 0.5f, z / (2.0f * radius) + 0.5f), glm::vec3(0.9f)));
    }
    for (unsigned int i = 0; i < sectors; ++i) {
        indices.push_back(topCenterIdx);
        indices.push_back(topStartIdx + i);
        indices.push_back(topStartIdx + i + 1);
    }

    // 3. Tapa Inferior (Y = -halfH)
    unsigned int botCenterIdx = static_cast<unsigned int>(vertices.size());
    vertices.push_back(Vertex(glm::vec3(0.0f, -halfH, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f), glm::vec3(0.9f)));
    unsigned int botStartIdx = static_cast<unsigned int>(vertices.size());
    for (unsigned int i = 0; i <= sectors; ++i) {
        float angle = i * step;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;
        vertices.push_back(Vertex(glm::vec3(x, -halfH, z), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(x / (2.0f * radius) + 0.5f, z / (2.0f * radius) + 0.5f), glm::vec3(0.9f)));
    }
    for (unsigned int i = 0; i < sectors; ++i) {
        indices.push_back(botCenterIdx);
        indices.push_back(botStartIdx + i + 1);
        indices.push_back(botStartIdx + i);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createCone(float radius, float height, unsigned int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265358979323846f;
    float halfH = height * 0.5f;
    float step = 2.0f * PI / sectors;

    // 1. Caras laterales inclinadas
    for (unsigned int i = 0; i < sectors; ++i) {
        float a1 = i * step;
        float a2 = (i + 1) * step;

        glm::vec3 pApex(0.0f, halfH, 0.0f);
        glm::vec3 p1(cosf(a1) * radius, -halfH, sinf(a1) * radius);
        glm::vec3 p2(cosf(a2) * radius, -halfH, sinf(a2) * radius);

        glm::vec3 edge1 = p1 - pApex;
        glm::vec3 edge2 = p2 - pApex;
        glm::vec3 norm = glm::normalize(glm::cross(edge1, edge2));

        unsigned int baseIdx = static_cast<unsigned int>(vertices.size());
        vertices.push_back(Vertex(pApex, norm, glm::vec2(0.5f, 1.0f), glm::vec3(0.9f)));
        vertices.push_back(Vertex(p1, norm, glm::vec2(static_cast<float>(i) / sectors, 0.0f), glm::vec3(0.9f)));
        vertices.push_back(Vertex(p2, norm, glm::vec2(static_cast<float>(i + 1) / sectors, 0.0f), glm::vec3(0.9f)));

        indices.push_back(baseIdx);
        indices.push_back(baseIdx + 1);
        indices.push_back(baseIdx + 2);
    }

    // 2. Base circular (Y = -halfH)
    unsigned int centerIdx = static_cast<unsigned int>(vertices.size());
    vertices.push_back(Vertex(glm::vec3(0.0f, -halfH, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f), glm::vec3(0.9f)));
    unsigned int startIdx = static_cast<unsigned int>(vertices.size());
    for (unsigned int i = 0; i <= sectors; ++i) {
        float a = i * step;
        float x = cosf(a) * radius;
        float z = sinf(a) * radius;
        vertices.push_back(Vertex(glm::vec3(x, -halfH, z), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(x / (2.0f * radius) + 0.5f, z / (2.0f * radius) + 0.5f), glm::vec3(0.9f)));
    }
    for (unsigned int i = 0; i < sectors; ++i) {
        indices.push_back(centerIdx);
        indices.push_back(startIdx + i + 1);
        indices.push_back(startIdx + i);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createTorus(float mainRadius, float tubeRadius, unsigned int mainSegments, unsigned int tubeSegments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265358979323846f;
    for (unsigned int i = 0; i <= mainSegments; ++i) {
        float u = static_cast<float>(i) / mainSegments * 2.0f * PI;
        float cosU = cosf(u);
        float sinU = sinf(u);

        for (unsigned int j = 0; j <= tubeSegments; ++j) {
            float v = static_cast<float>(j) / tubeSegments * 2.0f * PI;
            float cosV = cosf(v);
            float sinV = sinf(v);

            float x = (mainRadius + tubeRadius * cosV) * cosU;
            float y = tubeRadius * sinV;
            float z = (mainRadius + tubeRadius * cosV) * sinU;

            glm::vec3 norm = glm::normalize(glm::vec3(cosV * cosU, sinV, cosV * sinU));
            glm::vec2 uv(static_cast<float>(i) / mainSegments, static_cast<float>(j) / tubeSegments);

            vertices.push_back(Vertex(glm::vec3(x, y, z), norm, uv, glm::vec3(0.9f)));
        }
    }

    for (unsigned int i = 0; i < mainSegments; ++i) {
        for (unsigned int j = 0; j < tubeSegments; ++j) {
            unsigned int cur = i * (tubeSegments + 1) + j;
            unsigned int next = (i + 1) * (tubeSegments + 1) + j;

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(next + 1);

            indices.push_back(cur);
            indices.push_back(next + 1);
            indices.push_back(cur + 1);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

} // namespace ASIS
