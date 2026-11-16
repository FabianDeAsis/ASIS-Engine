#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "Texture.h"
#include "Shader.h"

namespace ASIS {

class Material {
public:
    Material();
    ~Material() = default;

    // 1. Propiedades Físicas PBR (Physically Based Rendering)
    glm::vec3 albedo;         // Color base / Reflectancia difusa RGB (0.0 - 1.0)
    float metallic;           // Metalicidad (0.0 = Dieléctrico/Plástico/Madera, 1.0 = Metal Puro)
    float roughness;          // Rugosidad microfacetal (0.04 = Espejo pulido, 1.0 = Mate difuso)
    float specular;           // Factor de reflexión especular (0.0 - 1.0)
    glm::vec3 emission;       // Color de emisión / resplandor propio RGB
    float emissionStrength;   // Intensidad de emisión (0.0 - 10.0x)
    float normalStrength;     // Intensidad del relieve de normales (0.0 - 3.0x)
    float ao;                 // Factor de Oclusión Ambiental (0.0 - 1.0)

    // 2. Mapas de Textura PBR
    std::shared_ptr<Texture> albedoMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> metallicMap;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> specularMap;
    std::shared_ptr<Texture> emissionMap;
    std::shared_ptr<Texture> aoMap;

    void setAlbedoMap(std::shared_ptr<Texture> tex) { albedoMap = tex; }
    void setNormalMap(std::shared_ptr<Texture> tex) { normalMap = tex; }
    void setMetallicMap(std::shared_ptr<Texture> tex) { metallicMap = tex; }
    void setRoughnessMap(std::shared_ptr<Texture> tex) { roughnessMap = tex; }
    void setSpecularMap(std::shared_ptr<Texture> tex) { specularMap = tex; }
    void setEmissionMap(std::shared_ptr<Texture> tex) { emissionMap = tex; }
    void setAOMap(std::shared_ptr<Texture> tex) { aoMap = tex; }

    bool hasAlbedoMap() const { return albedoMap != nullptr; }
    bool hasNormalMap() const { return normalMap != nullptr; }
    bool hasMetallicMap() const { return metallicMap != nullptr; }
    bool hasRoughnessMap() const { return roughnessMap != nullptr; }
    bool hasSpecularMap() const { return specularMap != nullptr; }
    bool hasEmissionMap() const { return emissionMap != nullptr; }
    bool hasAOMap() const { return aoMap != nullptr; }

    // Enlazar parámetros y texturas al Shader
    void bind(const Shader& shader) const;
};

} // namespace ASIS
