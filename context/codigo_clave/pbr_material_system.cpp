/**
 * @file pbr_material_system.cpp
 * @brief Implementación clave del pipeline de Materiales PBR Cook-Torrance GGX,
 * Normal Mapping por derivadas de pantalla (dFdx/dFdy), Sombras Ultra-HD 4K y Multiluz.
 */

#include "Material.h"
#include "Shader.h"
#include "Light.h"
#include "ShadowMap.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace ASIS {

// 1. Enlazado de Parámetros PBR y Texturas a la GPU (Unidades 0..6)
void Material::bind(const Shader& shader) const {
    // Parámetros Físicos PBR
    shader.setVec3("u_Material.albedo", albedo);
    shader.setFloat("u_Material.metallic", metallic);
    shader.setFloat("u_Material.roughness", roughness);
    shader.setFloat("u_Material.specular", specular);
    shader.setVec3("u_Material.emission", emission);
    shader.setFloat("u_Material.emissionStrength", emissionStrength);
    shader.setFloat("u_Material.normalStrength", normalStrength);
    shader.setFloat("u_Material.ao", ao);

    // Mapas de Textura PBR (0 = Albedo, 1 = Normal, 2 = Metallic, 3 = Roughness, 4 = Specular, 5 = Emission, 6 = AO)
    if (albedoMap) {
        albedoMap->bind(0);
        shader.setInt("u_Material.albedoMap", 0);
        shader.setInt("u_Material.useAlbedoMap", 1);
    } else {
        Texture::getDefaultWhite()->bind(0);
        shader.setInt("u_Material.albedoMap", 0);
        shader.setInt("u_Material.useAlbedoMap", 0);
    }

    if (normalMap) {
        normalMap->bind(1);
        shader.setInt("u_Material.normalMap", 1);
        shader.setInt("u_Material.useNormalMap", 1);
    } else {
        Texture::getDefaultNormal()->bind(1);
        shader.setInt("u_Material.normalMap", 1);
        shader.setInt("u_Material.useNormalMap", 0);
    }

    if (metallicMap) {
        metallicMap->bind(2);
        shader.setInt("u_Material.metallicMap", 2);
        shader.setInt("u_Material.useMetallicMap", 1);
    } else {
        shader.setInt("u_Material.useMetallicMap", 0);
    }

    if (roughnessMap) {
        roughnessMap->bind(3);
        shader.setInt("u_Material.roughnessMap", 3);
        shader.setInt("u_Material.useRoughnessMap", 1);
    } else {
        shader.setInt("u_Material.useRoughnessMap", 0);
    }

    if (specularMap) {
        specularMap->bind(4);
        shader.setInt("u_Material.specularMap", 4);
        shader.setInt("u_Material.useSpecularMap", 1);
    } else {
        shader.setInt("u_Material.useSpecularMap", 0);
    }

    if (emissionMap) {
        emissionMap->bind(5);
        shader.setInt("u_Material.emissionMap", 5);
        shader.setInt("u_Material.useEmissionMap", 1);
    } else {
        shader.setInt("u_Material.useEmissionMap", 0);
    }

    if (aoMap) {
        aoMap->bind(6);
        shader.setInt("u_Material.aoMap", 6);
        shader.setInt("u_Material.useAOMap", 1);
    } else {
        shader.setInt("u_Material.useAOMap", 0);
    }
}

// 2. Redimensionamiento dinámico de FBO de Sombras (1K, 2K, 4K)
bool ShadowMap::resize(unsigned int width, unsigned int height) {
    if (m_width == width && m_height == height) return true;
    m_width = width;
    m_height = height;

    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

} // namespace ASIS
