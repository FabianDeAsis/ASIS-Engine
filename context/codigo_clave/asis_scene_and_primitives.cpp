// ==============================================================================
// ASIS ENGINE v0.11 - SNIPPET: FORMATO NATIVO .ASIS, UNDO/REDO Y PRIMITIVAS 3D
// ==============================================================================

#include "SceneIO.h"
#include "Mesh.h"
#include "EditorUI.h"
#include <iostream>
#include <fstream>
#include <cmath>

namespace ASIS {

// 1. Guardado de Escena Nativa en Formato .asis (JSON Estructurado)
bool SceneIO::saveSceneASIS(const Scene& scene, const Camera& camera,
                            float sunAzimuth, float sunElevation, const glm::vec3& sunColor, float sunIntensity,
                            const glm::vec3& ambientColor, float ambientIntensity, bool shadowsEnabled,
                            int shadowRes, int shadowKernel, float shadowSoftness,
                            int colorProfile, float exposure, float gamma, float saturation, float contrast,
                            const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"asis_engine_version\": \"0.11\",\n";
    file << "  \"camera\": {\n";
    file << "    \"position\": [" << camera.getPosition().x << ", " << camera.getPosition().y << ", " << camera.getPosition().z << "],\n";
    file << "    \"fov\": " << camera.getFOV() << ",\n";
    file << "    \"isOrtho\": " << (camera.isOrthographic() ? "true" : "false") << "\n";
    file << "  },\n";
    file << "  \"lighting\": {\n";
    file << "    \"sunAzimuth\": " << sunAzimuth << ",\n";
    file << "    \"sunElevation\": " << sunElevation << ",\n";
    file << "    \"shadowsEnabled\": " << (shadowsEnabled ? "true" : "false") << ",\n";
    file << "    \"shadowResolution\": " << shadowRes << "\n";
    file << "  },\n";
    file << "  \"colorManagement\": {\n";
    file << "    \"profile\": " << colorProfile << ",\n";
    file << "    \"exposure\": " << exposure << ",\n";
    file << "    \"gamma\": " << gamma << ",\n";
    file << "    \"saturation\": " << saturation << ",\n";
    file << "    \"contrast\": " << contrast << "\n";
    file << "  },\n";
    file << "  \"entities\": [\n";
    // Serialización de entidades y materiales PBR...
    file << "  ]\n";
    file << "}\n";
    file.close();
    return true;
}

// 2. Generación Analítica de Toroide Procedural (Donut)
std::shared_ptr<Mesh> Mesh::createTorus(float mainRadius, float tubeRadius, int mainSegments, int tubeSegments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= mainSegments; ++i) {
        float u = static_cast<float>(i) / static_cast<float>(mainSegments);
        float phi = u * 2.0f * 3.14159265359f;
        float cosPhi = std::cos(phi), sinPhi = std::sin(phi);

        for (int j = 0; j <= tubeSegments; ++j) {
            float v = static_cast<float>(j) / static_cast<float>(tubeSegments);
            float theta = v * 2.0f * 3.14159265359f;
            float cosTheta = std::cos(theta), sinTheta = std::sin(theta);

            Vertex vert;
            vert.Position = glm::vec3(
                (mainRadius + tubeRadius * cosTheta) * cosPhi,
                tubeRadius * sinTheta,
                (mainRadius + tubeRadius * cosTheta) * sinPhi
            );
            vert.Normal = glm::normalize(glm::vec3(
                cosTheta * cosPhi,
                sinTheta,
                cosTheta * sinPhi
            ));
            vert.TexCoords = glm::vec2(u * 4.0f, v * 2.0f);
            vertices.push_back(vert);
        }
    }

    for (int i = 0; i < mainSegments; ++i) {
        for (int j = 0; j < tubeSegments; ++j) {
            int first = (i * (tubeSegments + 1)) + j;
            int second = first + tubeSegments + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    return std::make_shared<Mesh>(vertices, indices);
}

// 3. Sistema de Deshacer / Rehacer (Undo / Redo) en EditorUI
void EditorUI::recordSnapshot(const Scene& scene) {
    SceneSnapshot snap;
    for (const auto& name : scene.getEntityOrder()) {
        auto ent = scene.getEntity(name);
        if (!ent) continue;
        EntitySnapshot e;
        e.name = ent->getName();
        e.primitiveType = ent->getPrimitiveType();
        e.position = ent->getPosition();
        e.rotation = ent->getRotation();
        e.scale = ent->getScale();
        snap.entities.push_back(e);
    }
    snap.lights = scene.getPointLights();
    m_undoStack.push_back(snap);
    if (m_undoStack.size() > static_cast<size_t>(m_maxUndoSteps)) {
        m_undoStack.erase(m_undoStack.begin());
    }
    m_redoStack.clear();
}

} // namespace ASIS
