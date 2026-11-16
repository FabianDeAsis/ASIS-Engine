#include "Material.h"

namespace ASIS {

Material::Material()
    : albedo(1.0f, 1.0f, 1.0f),
      metallic(0.0f),
      roughness(0.5f),
      specular(0.5f),
      emission(0.0f, 0.0f, 0.0f),
      emissionStrength(1.0f),
      normalStrength(1.0f),
      ao(1.0f)
{
}

void Material::bind(const Shader& shader) const {
    // 1. Parámetros PBR
    shader.setVec3("u_Material.albedo", albedo);
    shader.setFloat("u_Material.metallic", metallic);
    shader.setFloat("u_Material.roughness", roughness);
    shader.setFloat("u_Material.specular", specular);
    shader.setVec3("u_Material.emission", emission);
    shader.setFloat("u_Material.emissionStrength", emissionStrength);
    shader.setFloat("u_Material.normalStrength", normalStrength);
    shader.setFloat("u_Material.ao", ao);

    // 2. Mapas de Textura en unidades 0..6
    // Slot 0: Albedo
    if (albedoMap) {
        albedoMap->bind(0);
        shader.setInt("u_Material.albedoMap", 0);
        shader.setInt("u_Material.useAlbedoMap", 1);
    } else {
        Texture::getDefaultWhite()->bind(0);
        shader.setInt("u_Material.albedoMap", 0);
        shader.setInt("u_Material.useAlbedoMap", 0);
    }

    // Slot 1: Normal Map
    if (normalMap) {
        normalMap->bind(1);
        shader.setInt("u_Material.normalMap", 1);
        shader.setInt("u_Material.useNormalMap", 1);
    } else {
        Texture::getDefaultNormal()->bind(1);
        shader.setInt("u_Material.normalMap", 1);
        shader.setInt("u_Material.useNormalMap", 0);
    }

    // Slot 2: Metallic Map
    if (metallicMap) {
        metallicMap->bind(2);
        shader.setInt("u_Material.metallicMap", 2);
        shader.setInt("u_Material.useMetallicMap", 1);
    } else {
        shader.setInt("u_Material.useMetallicMap", 0);
    }

    // Slot 3: Roughness Map
    if (roughnessMap) {
        roughnessMap->bind(3);
        shader.setInt("u_Material.roughnessMap", 3);
        shader.setInt("u_Material.useRoughnessMap", 1);
    } else {
        shader.setInt("u_Material.useRoughnessMap", 0);
    }

    // Slot 4: Specular Map
    if (specularMap) {
        specularMap->bind(4);
        shader.setInt("u_Material.specularMap", 4);
        shader.setInt("u_Material.useSpecularMap", 1);
    } else {
        shader.setInt("u_Material.useSpecularMap", 0);
    }

    // Slot 5: Emission Map
    if (emissionMap) {
        emissionMap->bind(5);
        shader.setInt("u_Material.emissionMap", 5);
        shader.setInt("u_Material.useEmissionMap", 1);
    } else {
        shader.setInt("u_Material.useEmissionMap", 0);
    }

    // Slot 6: Ambient Occlusion (AO) Map
    if (aoMap) {
        aoMap->bind(6);
        shader.setInt("u_Material.aoMap", 6);
        shader.setInt("u_Material.useAOMap", 1);
    } else {
        shader.setInt("u_Material.useAOMap", 0);
    }
}

} // namespace ASIS
