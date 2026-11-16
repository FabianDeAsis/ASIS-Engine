#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 VertexColor;
in vec4 FragPosLightSpace;

out vec4 FragColor;

const float PI = 3.14159265359;

// ========================================================================
// 1. Estructura de Material PBR (Cook-Torrance Microfacet GGX)
// ========================================================================
struct PBRMaterial {
    vec3 albedo;
    float metallic;
    float roughness;
    float specular;
    vec3 emission;
    float emissionStrength;
    float normalStrength;
    float ao;

    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D specularMap;
    sampler2D emissionMap;
    sampler2D aoMap;

    int useAlbedoMap;
    int useNormalMap;
    int useMetallicMap;
    int useRoughnessMap;
    int useSpecularMap;
    int useEmissionMap;
    int useAOMap;
};

uniform PBRMaterial u_Material;
uniform vec3 u_ViewPos;
uniform mat4 u_View;
uniform vec3 u_EntityColor;

// ========================================================================
// 2. Parámetros de Iluminación Direccional (Sol) y Sombras
// ========================================================================
uniform vec3 u_LightDir;        // Dirección de la luz solar incidente
uniform vec3 u_LightColor;      // Color e intensidad del sol directo
uniform vec3 u_AmbientColor;    // Luz ambiental

uniform sampler2D u_ShadowMap;  // Textura de profundidad (Depth Map hasta 8K)
uniform int u_ShadowsEnabled;   // 1 = activadas, 0 = desactivadas
uniform float u_ShadowBias;     // Tolerancia contra Shadow Acne
uniform int u_ShadowKernel;     // 1 = 3x3 (9 muestras), 2 = 5x5 (25 muestras), 3 = 7x7 (49 muestras)
uniform float u_ShadowSoftness; // Factor de suavizado / dispersión de penumbra

// Sombras Suaves Físicas PCSS (Percentage-Closer Soft Shadows)
uniform int u_ShadowFilterMode;       // 0 = Tradicional PCF, 1 = PCSS
uniform float u_PCSS_LightSize;       // Escala física de la fuente de luz
uniform int u_PCSS_BlockerSamples;    // Muestras para búsqueda de bloqueador (def: 16)
uniform int u_PCSS_FilterSamples;     // Muestras para filtro de penumbra (def: 32)

// Sombras en Cascada (Cascaded Shadow Maps - CSM)
uniform sampler2DArray u_CascadeShadowMaps;
uniform mat4 u_CascadeLightSpaceMatrices[3];
uniform float u_CascadeSplits[3];
uniform int u_CSMEnabled;
uniform int u_CSMDebugColors;

// ========================================================================
// 3. Luces Puntuales (Point Lights)
// ========================================================================
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    int enabled;
    int castShadows;
};

#define MAX_POINT_LIGHTS 8
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];
uniform int u_NumPointLights;

// Sombras Omnidireccionales de Luces Puntuales (Cubemap Shadow Mapping)
uniform samplerCube u_PointShadowMap;
uniform float u_PointShadowFarPlane;
uniform int u_PointShadowsEnabled;
uniform float u_PointShadowLightSize;
uniform float u_PointShadowBias;

vec3 g_PointShadowSamples[20] = vec3[](
   vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), 
   vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
   vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
   vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);

// ========================================================================
// 4. Gestión de Color y Mapeo Tonal (Color Management & Tone Mapping - Fase 11)
// ========================================================================
uniform int u_ColorProfile;   // 0 = ACES Filmic, 1 = AgX, 2 = Reinhard, 3 = Linear
uniform float u_Exposure;     // Multiplicador de exposición (def: 1.0)
uniform float u_Gamma;        // Factor de corrección gamma (def: 2.2)
uniform float u_Saturation;   // Saturación de color (def: 1.0)
uniform float u_Contrast;     // Contraste tonal (def: 1.0)

// ========================================================================
// 5. Iluminación Basada en Imágenes (IBL) y Skybox HDRI (Fase 12)
// ========================================================================
uniform int u_UseIBL;             // 1 = activado, 0 = usar solo ambiente constante
uniform float u_IBLIntensity;     // Intensidad de IBL y reflejos de entorno
uniform float u_SkyboxRotation;   // Rotación en radianes del entorno
uniform int u_FlipX;              // Inversión horizontal de entorno HDRI
uniform int u_FlipY;              // Inversión vertical de entorno HDRI
uniform sampler2D u_EnvMap;
uniform int u_OASIS_DebugMeshlets;
uniform vec3 u_OASIS_MeshletColor;       // Textura equirrectangular HDR
uniform float u_Wetness;                 // Humedad de lluvia atmosférica (Fase 20)

// Neblina Atmosférica Global y Volumétrica (Fase 20)
uniform int u_GlobalFogEnabled;
uniform float u_GlobalFogStart;
uniform float u_GlobalFogEnd;
uniform float u_GlobalFogDensity;
uniform vec3 u_GlobalFogColor;

const vec2 invAtan = vec2(0.1591549, 0.3183098); // 1/(2*PI), 1/PI

vec2 dirToUV(vec3 v) {
    float len = length(v);
    if (len < 0.00001) return vec2(0.5);
    vec3 dir = v / len;
    float x = dir.x;
    float z = dir.z;
    if (abs(x) < 1e-6 && abs(z) < 1e-6) x = 1e-6; // Prevenir atan(0.0, 0.0) indefinido / NaN en vectores verticales
    vec2 uv = vec2(atan(z, x), asin(clamp(dir.y, -1.0, 1.0)));
    uv *= invAtan;
    uv += 0.5;
    if (u_FlipX == 1) uv.x = 1.0 - uv.x;
    if (u_FlipY == 1) uv.y = 1.0 - uv.y;
    return clamp(uv, vec2(0.0), vec2(1.0));
}

vec3 rotateY(vec3 v, float angleRad) {
    float c = cos(angleRad);
    float s = sin(angleRad);
    return vec3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

// Fresnel-Schlick con rugosidad para IBL
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Curva de Mapeo Tonal Fílmico ACES (Academy Color Encoding System)
vec3 ACESFilm(vec3 x) {
    if (isnan(x.r) || isinf(x.r) || x.r < 0.0) x.r = 0.0;
    if (isnan(x.g) || isinf(x.g) || x.g < 0.0) x.g = 0.0;
    if (isnan(x.b) || isinf(x.b) || x.b < 0.0) x.b = 0.0;
    x = min(x, vec3(65000.0));
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// ========================================================================
// Funciones Matemáticas PBR Cook-Torrance
// ========================================================================

// 1. Distribución Normal de Microfacetas (Trowbridge-Reitz GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0000001);
}

// 2. Función de Enmascaramiento Geométrico (Smith Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 3. Ecuación de Fresnel (Aproximación de Schlick)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Perturbación de Normales mediante Normal Map con Derivadas de Espacio de Pantalla
vec3 getNormalFromMap() {
    if (u_Material.useNormalMap == 0) {
        return normalize(Normal);
    }

    vec3 tangentNormal = texture(u_Material.normalMap, TexCoords).xyz * 2.0 - 1.0;
    tangentNormal.xy *= max(u_Material.normalStrength, 0.0);
    tangentNormal = normalize(tangentNormal);

    // Calcular matriz TBN al vuelo sin requerir tangentes precomputadas
    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T   = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// Disco de Vogel para muestreo estocástico uniforme de sombras PCSS
vec2 vogelDiskSample(int sampleIndex, int sampleCount, float phi) {
    float GoldenAngle = 2.399963;
    float r = sqrt(float(sampleIndex) + 0.5) / sqrt(float(sampleCount));
    float theta = float(sampleIndex) * GoldenAngle + phi;
    return vec2(r * cos(theta), r * sin(theta));
}

float interleavedGradientNoise(vec2 screenPos) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

// Búsqueda de bloqueadores para PCSS (Percentage-Closer Soft Shadows)
float findBlocker(sampler2D shadowMap, vec2 uv, float zReceiver, float searchRadius, float bias) {
    int blockers = 0;
    float avgBlockerDepth = 0.0;
    float randomAngle = interleavedGradientNoise(gl_FragCoord.xy) * 6.2831853;

    int samples = clamp(u_PCSS_BlockerSamples, 8, 32);
    for (int i = 0; i < samples; ++i) {
        vec2 offset = vogelDiskSample(i, samples, randomAngle) * searchRadius;
        float shadowDepth = texture(shadowMap, uv + offset).r;
        if (shadowDepth < zReceiver - bias) {
            avgBlockerDepth += shadowDepth;
            blockers++;
        }
    }

    if (blockers == 0) return -1.0;
    return avgBlockerDepth / float(blockers);
}

float pcssShadow(sampler2D shadowMap, vec2 uv, float zReceiver, vec2 texelSize, float bias) {
    float lightSize = max(u_PCSS_LightSize, 0.05);

    // 1. Radio de búsqueda de bloqueador proporcional a la distancia
    float searchRadius = lightSize * (zReceiver - 0.0001) / max(zReceiver, 0.0001) * texelSize.x * 24.0;
    searchRadius = clamp(searchRadius, texelSize.x * 2.0, texelSize.x * 60.0);

    float avgBlockerDepth = findBlocker(shadowMap, uv, zReceiver, searchRadius, bias);
    if (avgBlockerDepth < 0.0) return 0.0; // Sin bloqueador -> completamente iluminado

    // 2. Estimación física del radio de penumbra (contacto endurecido)
    float penumbra = (zReceiver - avgBlockerDepth) / max(avgBlockerDepth, 0.0001);
    float filterRadius = clamp(penumbra * lightSize * texelSize.x * 40.0, texelSize.x * 0.5, texelSize.x * 80.0);

    // 3. Filtrado suave estocástico por disco de Vogel
    float shadow = 0.0;
    int filterSamples = clamp(u_PCSS_FilterSamples, 16, 64);
    float randomAngle = interleavedGradientNoise(gl_FragCoord.xy + vec2(1.23, 4.56)) * 6.2831853;

    for (int i = 0; i < filterSamples; ++i) {
        vec2 offset = vogelDiskSample(i, filterSamples, randomAngle) * filterRadius;
        float pcfDepth = texture(shadowMap, uv + offset).r;
        shadow += (zReceiver - bias > pcfDepth) ? 1.0 : 0.0;
    }
    return shadow / float(filterSamples);
}

// Búsqueda de bloqueadores para PCSS en arreglo de sombras CSM (sampler2DArray)
float findBlockerArray(sampler2DArray shadowArray, vec2 uv, float cascade, float zReceiver, float searchRadius, float bias) {
    int blockers = 0;
    float avgBlockerDepth = 0.0;
    float randomAngle = interleavedGradientNoise(gl_FragCoord.xy) * 6.2831853;

    int samples = clamp(u_PCSS_BlockerSamples, 8, 32);
    for (int i = 0; i < samples; ++i) {
        vec2 offset = vogelDiskSample(i, samples, randomAngle) * searchRadius;
        float shadowDepth = texture(shadowArray, vec3(uv + offset, cascade)).r;
        if (shadowDepth < zReceiver - bias) {
            avgBlockerDepth += shadowDepth;
            blockers++;
        }
    }

    if (blockers == 0) return -1.0;
    return avgBlockerDepth / float(blockers);
}

float pcssShadowArray(sampler2DArray shadowArray, vec2 uv, float cascade, float zReceiver, vec2 texelSize, float bias) {
    float lightSize = max(u_PCSS_LightSize, 0.05);

    float searchRadius = lightSize * (zReceiver - 0.0001) / max(zReceiver, 0.0001) * texelSize.x * 24.0;
    searchRadius = clamp(searchRadius, texelSize.x * 2.0, texelSize.x * 60.0);

    float avgBlockerDepth = findBlockerArray(shadowArray, uv, cascade, zReceiver, searchRadius, bias);
    if (avgBlockerDepth < 0.0) return 0.0;

    float penumbra = (zReceiver - avgBlockerDepth) / max(avgBlockerDepth, 0.0001);
    float filterRadius = clamp(penumbra * lightSize * texelSize.x * 40.0, texelSize.x * 0.5, texelSize.x * 80.0);

    float shadow = 0.0;
    int filterSamples = clamp(u_PCSS_FilterSamples, 16, 64);
    float randomAngle = interleavedGradientNoise(gl_FragCoord.xy + vec2(1.23, 4.56)) * 6.2831853;

    for (int i = 0; i < filterSamples; ++i) {
        vec2 offset = vogelDiskSample(i, filterSamples, randomAngle) * filterRadius;
        float pcfDepth = texture(shadowArray, vec3(uv + offset, cascade)).r;
        shadow += (zReceiver - bias > pcfDepth) ? 1.0 : 0.0;
    }
    return shadow / float(filterSamples);
}

// Cálculo de Sombras Avanzado con Soporte para CSM (3 Cascadas) y Modos PCF / PCSS
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (u_ShadowsEnabled == 0) return 0.0;

    vec3 toLight = normalize(-lightDir);
    float cosTheta = clamp(dot(normal, toLight), 0.0, 1.0);
    float tanTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta)) / max(cosTheta, 0.001);
    float slopeBias = clamp(tanTheta * 0.0015, 0.0, 0.012);
    float bias = max(u_ShadowBias, 0.0003) + slopeBias;

    // =========================================================
    // MODO CSM (Cascaded Shadow Maps con textura 2D Array)
    // =========================================================
    if (u_CSMEnabled == 1) {
        float viewDepth = abs((u_View * vec4(FragPos, 1.0)).z);
        int cascadeIndex = 2;
        if (viewDepth < u_CascadeSplits[0]) {
            cascadeIndex = 0;
        } else if (viewDepth < u_CascadeSplits[1]) {
            cascadeIndex = 1;
        } else if (viewDepth < u_CascadeSplits[2]) {
            cascadeIndex = 2;
        } else {
            return 0.0; // Más allá de la distancia máxima de cascadas
        }

        vec4 fragPosCascade = u_CascadeLightSpaceMatrices[cascadeIndex] * vec4(FragPos, 1.0);
        vec3 projCoords = fragPosCascade.xyz / fragPosCascade.w;
        projCoords = projCoords * 0.5 + 0.5;

        if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
            return 0.0;
        }

        float currentDepth = projCoords.z;
        vec2 texelSize = (1.0 / vec2(textureSize(u_CascadeShadowMaps, 0).xy)) * max(u_ShadowSoftness, 0.5);

        if (u_ShadowFilterMode == 1) {
            return pcssShadowArray(u_CascadeShadowMaps, projCoords.xy, float(cascadeIndex), currentDepth, texelSize, bias);
        }

        float shadow = 0.0;
        int range = (u_ShadowKernel >= 3) ? 3 : ((u_ShadowKernel == 2) ? 2 : 1);
        float totalSamples = float((2 * range + 1) * (2 * range + 1));

        for (int x = -range; x <= range; ++x) {
            for (int y = -range; y <= range; ++y) {
                float pcfDepth = texture(u_CascadeShadowMaps, vec3(projCoords.xy + vec2(x, y) * texelSize, float(cascadeIndex))).r;
                shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
            }
        }
        return shadow / totalSamples;
    }

    // =========================================================
    // MODO TRADICIONAL MONOLÍTICO (u_CSMEnabled == 0)
    // =========================================================
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    vec2 texelSize = (1.0 / textureSize(u_ShadowMap, 0)) * max(u_ShadowSoftness, 0.5);

    // Si está activo el modo PCSS, aplicar penumbra físicamente variable
    if (u_ShadowFilterMode == 1) {
        return pcssShadow(u_ShadowMap, projCoords.xy, currentDepth, texelSize, bias);
    }

    // Modo Tradicional PCF (3x3, 5x5, 7x7)
    float shadow = 0.0;
    int range = (u_ShadowKernel >= 3) ? 3 : ((u_ShadowKernel == 2) ? 2 : 1);
    float totalSamples = float((2 * range + 1) * (2 * range + 1));

    for (int x = -range; x <= range; ++x) {
        for (int y = -range; y <= range; ++y) {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= totalSamples;

    return shadow;
}

// Cálculo de Sombras Omnidireccionales con PCSS Físico (Contact Hardening en Cubemap)
float calculatePointShadow(vec3 fragPos, vec3 lightPos) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    if (currentDepth >= u_PointShadowFarPlane || currentDepth < 0.001) return 0.0;

    float bias = max(u_PointShadowBias, 0.005);
    vec3 dir = fragToLight / currentDepth;

    // Base ortonormal perpendicular al rayo incidente del cubemap
    vec3 up = abs(dir.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T1 = normalize(cross(up, dir));
    vec3 T2 = cross(dir, T1);

    // =========================================================
    // MODO OMNI-PCSS (Percentage-Closer Soft Shadows)
    // =========================================================
    if (u_ShadowFilterMode == 1) {
        float lightSize = max(u_PointShadowLightSize, 0.05);

        // 1. Radio angular de búsqueda de bloqueadores en el cono de luz
        float searchRadius = clamp((lightSize / currentDepth) * 0.85, 0.008, 0.25);
        int blockerSamples = 16;
        int blockers = 0;
        float avgBlockerDist = 0.0;
        float blockerAngle = interleavedGradientNoise(gl_FragCoord.xy) * 6.2831853;

        for (int i = 0; i < blockerSamples; ++i) {
            vec2 disk = vogelDiskSample(i, blockerSamples, blockerAngle) * searchRadius;
            vec3 sampleDir = normalize(dir + T1 * disk.x + T2 * disk.y);
            float shadowDist = texture(u_PointShadowMap, sampleDir).r * u_PointShadowFarPlane;
            if (shadowDist < currentDepth - bias) {
                avgBlockerDist += shadowDist;
                blockers++;
            }
        }

        if (blockers == 0) return 0.0; // Sin bloqueador -> completamente iluminado

        avgBlockerDist /= float(blockers);

        // 2. Estimación física del radio de penumbra (contacto endurecido)
        float penumbra = (currentDepth - avgBlockerDist) / max(avgBlockerDist, 0.01);
        float filterRadius = clamp(penumbra * (lightSize / currentDepth) * 1.5, 0.003, 0.20);

        // 3. Filtrado suave estocástico por disco de Vogel
        float shadow = 0.0;
        int filterSamples = 24;
        float filterAngle = interleavedGradientNoise(gl_FragCoord.xy + vec2(4.56, 1.23)) * 6.2831853;

        for (int i = 0; i < filterSamples; ++i) {
            vec2 disk = vogelDiskSample(i, filterSamples, filterAngle) * filterRadius;
            vec3 sampleDir = normalize(dir + T1 * disk.x + T2 * disk.y);
            float shadowDist = texture(u_PointShadowMap, sampleDir).r * u_PointShadowFarPlane;
            shadow += (currentDepth - bias > shadowDist) ? 1.0 : 0.0;
        }

        return shadow / float(filterSamples);
    }

    // =========================================================
    // MODO TRADICIONAL PCF EN CUBEMAP
    // =========================================================
    float shadow = 0.0;
    float viewDist = length(u_ViewPos - fragPos);
    float diskRadius = (1.0 + (viewDist / u_PointShadowFarPlane)) / 35.0;

    for (int s = 0; s < 20; ++s) {
        float closestDepth = texture(u_PointShadowMap, fragToLight + g_PointShadowSamples[s] * diskRadius).r;
        closestDepth *= u_PointShadowFarPlane;
        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }
    return shadow / 20.0;
}

void main() {
    // 1. Extraer Propiedades del Material
    vec3 albedo = u_Material.albedo;
    if (length(u_EntityColor) > 0.001) {
        albedo *= u_EntityColor;
    }
    albedo *= VertexColor;

    if (u_Material.useAlbedoMap == 1) {
        vec3 albedoTex = texture(u_Material.albedoMap, TexCoords).rgb;
        // Linealizar espacio sRGB a Linear para cálculo PBR físico (elimina aspecto lechoso/lavado)
        albedoTex = pow(albedoTex, vec3(2.2));
        albedo *= albedoTex;
    }

    float metallic = clamp(u_Material.metallic, 0.0, 1.0);
    if (u_Material.useMetallicMap == 1) {
        metallic *= texture(u_Material.metallicMap, TexCoords).r;
    }

    float roughness = clamp(u_Material.roughness, 0.04, 1.0);
    if (u_Material.useRoughnessMap == 1) {
        roughness *= texture(u_Material.roughnessMap, TexCoords).r;
    }

    float specFactor = clamp(u_Material.specular, 0.0, 1.0);
    if (u_Material.useSpecularMap == 1) {
        specFactor *= texture(u_Material.specularMap, TexCoords).r;
    }

    float ao = clamp(u_Material.ao, 0.0, 1.0);
    if (u_Material.useAOMap == 1) {
        ao *= texture(u_Material.aoMap, TexCoords).r;
    }

    vec3 emission = u_Material.emission * u_Material.emissionStrength;
    if (u_Material.useEmissionMap == 1) {
        emission *= texture(u_Material.emissionMap, TexCoords).rgb;
    }

    // Fase 20: Clima Atmosférico y Humedad de Superficie (Wetness)
    if (u_Wetness > 0.001) {
        albedo = mix(albedo, albedo * 0.55, u_Wetness * (1.0 - metallic * 0.7));
        roughness = mix(roughness, max(roughness * 0.20, 0.03), u_Wetness);
        specFactor = mix(specFactor, max(specFactor, 0.95), u_Wetness);
    }

    // 2. Vectores de Geometría y Visión
    vec3 N = getNormalFromMap();
    vec3 V = normalize(u_ViewPos - FragPos);

    // Reflectancia en incidencia normal F0 (Dieléctricos ~0.04, Metales usan Albedo)
    vec3 F0 = vec3(0.04 * specFactor * 2.0);
    F0 = mix(F0, albedo, metallic);

    // 3. Luminancia Saliente Acumulada
    vec3 Lo = vec3(0.0);

    // Luz Direccional (Sol)
    vec3 L = normalize(-u_LightDir);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);

    if (NdotL > 0.0) {
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specularBRDF = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float shadow = calculateShadow(FragPosLightSpace, N, u_LightDir);
        Lo += (1.0 - shadow) * (kD * albedo / PI + specularBRDF) * u_LightColor * NdotL;
    }

    // Luces Puntuales (Point Lights)
    for (int i = 0; i < u_NumPointLights && i < MAX_POINT_LIGHTS; ++i) {
        if (u_PointLights[i].enabled == 0) continue;

        vec3 pLightDir = u_PointLights[i].position - FragPos;
        float distance = length(pLightDir);
        if (distance > 30.0) continue; // Corte de distancia

        vec3 pL = normalize(pLightDir);
        vec3 pH = normalize(V + pL);
        float pNdotL = max(dot(N, pL), 0.0);

        if (pNdotL > 0.0) {
            float attenuation = 1.0 / (u_PointLights[i].constant +
                                       u_PointLights[i].linear * distance +
                                       u_PointLights[i].quadratic * (distance * distance));
            vec3 radiance = u_PointLights[i].color * u_PointLights[i].intensity * attenuation;

            float NDF = DistributionGGX(N, pH, roughness);
            float G   = GeometrySmith(N, V, pL, roughness);
            vec3 F    = fresnelSchlick(max(dot(pH, V), 0.0), F0);

            vec3 numerator    = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * pNdotL + 0.0001;
            vec3 specularBRDF = numerator / denominator;

            vec3 kS = F;
            vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

            float pShadow = 0.0;
            if (u_PointShadowsEnabled == 1 && u_PointLights[i].castShadows == 1) {
                pShadow = calculatePointShadow(FragPos, u_PointLights[i].position);
            }

            Lo += (1.0 - pShadow) * (kD * albedo / PI + specularBRDF) * radiance * pNdotL;
        }
    }

    // 4. Luz Ambiental modulada por Oclusión Ambiental (AO) y Emisión
    vec3 ambient = u_AmbientColor * albedo * ao;

    // Fase 12: Iluminación Basada en Imágenes (IBL) y Reflejos Especulares de Entorno
    if (u_UseIBL == 1) {
        // 1. Reflejo especular en metales y superficies pulidas
        vec3 R = reflect(-V, N);
        vec3 rotR = rotateY(R, u_SkyboxRotation);
        vec2 uvR = dirToUV(rotR);
        // Nivel de mipmap según rugosidad (roughness 0.0 = espejo perfecto, 1.0 = mate)
        vec3 prefilteredSpecular = textureLod(u_EnvMap, uvR, roughness * 7.0).rgb;
        if (isnan(prefilteredSpecular.r) || isinf(prefilteredSpecular.r)) prefilteredSpecular = vec3(0.0);
        prefilteredSpecular = clamp(prefilteredSpecular, vec3(0.0), vec3(256.0));

        // Factor de Fresnel para IBL
        vec3 F_IBL = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        // 2. Irradiancia difusa ambiental (luz del cielo envolvente)
        vec3 rotN = rotateY(N, u_SkyboxRotation);
        vec2 uvN = dirToUV(rotN);
        vec3 irradiance = textureLod(u_EnvMap, uvN, 6.5).rgb;
        if (isnan(irradiance.r) || isinf(irradiance.r)) irradiance = vec3(0.0);
        irradiance = clamp(irradiance, vec3(0.0), vec3(64.0));

        vec3 kD_IBL = (vec3(1.0) - F_IBL) * (1.0 - metallic);
        vec3 diffuseIBL = irradiance * albedo;
        vec3 specularIBL = prefilteredSpecular * (F_IBL * specFactor + 0.04);

        vec3 iblTotal = (kD_IBL * diffuseIBL + specularIBL) * ao * u_IBLIntensity;
        if (isnan(iblTotal.r) || isinf(iblTotal.r)) iblTotal = vec3(0.0);
        iblTotal = max(iblTotal, vec3(0.0));

        // Preservar siempre un piso de luz ambiental base para que caras en sombra nunca colapsen a negro absoluto
        ambient = (u_AmbientColor * albedo * ao) * 0.25 + iblTotal;
    }

    vec3 color = ambient + Lo + emission;
    if (isnan(color.r) || isinf(color.r) || color.r < 0.0) color.r = 0.0;
    if (isnan(color.g) || isinf(color.g) || color.g < 0.0) color.g = 0.0;
    if (isnan(color.b) || isinf(color.b) || color.b < 0.0) color.b = 0.0;

    // Neblina Atmosférica Global (Fase 20)
    if (u_GlobalFogEnabled == 1) {
        float dist = length(u_ViewPos - FragPos);
        float fogFactor = clamp((dist - u_GlobalFogStart) / max(u_GlobalFogEnd - u_GlobalFogStart, 0.1), 0.0, 1.0);
        fogFactor = pow(fogFactor, 1.5) * u_GlobalFogDensity;
        vec3 L_sun = normalize(-u_LightDir);
        float sunScatter = pow(max(dot(-V, L_sun), 0.0), 8.0) * 0.40;
        vec3 fogColor = u_GlobalFogColor + u_LightColor * sunScatter;
        color = mix(color, fogColor, clamp(fogFactor, 0.0, 0.95));
    }

    // Aplicar Exposición (Exposure)
    color *= max(u_Exposure, 0.001);

    // 5. Perfil de Mapeo Tonal Seleccionable (Fase 11)
    vec3 mapped = color;
    if (u_ColorProfile == 0) {
        // ACES Filmic (Fotorrealista estilo Unreal Engine / Hollywood)
        mapped = ACESFilm(color);
    } else if (u_ColorProfile == 1) {
        // AgX (Estilo Blender 4 con preservación de altas luces)
        vec3 x = max(color, vec3(0.0));
        vec3 logColor = clamp(log2(x + 0.0001) / 10.0 + 0.5, 0.0, 1.0);
        mapped = logColor * logColor * (3.0 - 2.0 * logColor);
    } else if (u_ColorProfile == 2) {
        // Reinhard Extendido
        mapped = color / (color + vec3(1.0));
    } else {
        // Linear / Raw (Sin compresión)
        mapped = clamp(color, 0.0, 1.0);
    }

    // Ajuste de Saturación y Contraste
    float luma = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
    mapped = mix(vec3(luma), mapped, max(u_Saturation, 0.0));
    mapped = mix(vec3(0.5), mapped, max(u_Contrast, 0.0));
    mapped = clamp(mapped, 0.0, 1.0);

    // Corrección Gamma final
    mapped = pow(mapped, vec3(1.0 / max(u_Gamma, 0.1)));

    if (isnan(mapped.r) || isinf(mapped.r) || mapped.r < 0.0) mapped.r = 0.0;
    if (isnan(mapped.g) || isinf(mapped.g) || mapped.g < 0.0) mapped.g = 0.0;
    if (isnan(mapped.b) || isinf(mapped.b) || mapped.b < 0.0) mapped.b = 0.0;
    mapped = clamp(mapped, 0.0, 1.0);

    if (u_OASIS_DebugMeshlets == 1) {
        mapped = mix(mapped, u_OASIS_MeshletColor, 0.75);
    }

    if (u_CSMEnabled == 1 && u_CSMDebugColors == 1) {
        float viewDepth = abs((u_View * vec4(FragPos, 1.0)).z);
        if (viewDepth < u_CascadeSplits[0]) {
            mapped = mix(mapped, vec3(1.0, 0.15, 0.15), 0.35); // Rojo = Cascada Cercana (0)
        } else if (viewDepth < u_CascadeSplits[1]) {
            mapped = mix(mapped, vec3(0.15, 1.0, 0.15), 0.35); // Verde = Cascada Media (1)
        } else if (viewDepth < u_CascadeSplits[2]) {
            mapped = mix(mapped, vec3(0.2, 0.4, 1.0), 0.35);   // Azul = Cascada Lejana (2)
        }
    }

    FragColor = vec4(mapped, 1.0);
}
