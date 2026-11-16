#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
out vec4 FragColor;

in vec4 FragPosLightSpace;
in float WorldElevation;
in float SlopeCos;
in vec3 OASIS_DebugColor;
uniform int u_OASIS_DebugLOD;

const float PI = 3.14159265359;

// ========================================================================
// 1. Uniforms de Iluminación y Cámara
// ========================================================================
uniform vec3 u_ViewPos;
uniform mat4 u_View;
uniform vec3 u_LightDir;        // Hacia donde apunta la luz solar
uniform vec3 u_LightColor;      // Color e intensidad del sol
uniform vec3 u_AmbientColor;    // Color ambiental base

// Sombras 4K/8K PCF y PCSS
uniform sampler2D u_ShadowMap;
uniform int u_ShadowsEnabled;
uniform float u_ShadowBias;
uniform int u_ShadowKernel;     // 1=3x3, 2=5x5, 3=7x7
uniform float u_ShadowSoftness;

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

// Luces Puntuales
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

// Sombras Omnidireccionales de Luces Puntuales (Point Light Shadows)
uniform samplerCube u_PointShadowMap;
uniform float u_PointShadowFarPlane;
uniform int u_PointShadowsEnabled;
uniform float u_PointShadowLightSize;
uniform float u_PointShadowBias;

vec3 g_TerrainPointShadowSamples[20] = vec3[](
   vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), 
   vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
   vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
   vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);

// IBL y Skybox
uniform int u_UseIBL;
uniform float u_IBLIntensity;
uniform float u_SkyboxRotation;
uniform int u_FlipX;
uniform int u_FlipY;
uniform sampler2D u_EnvMap;

// Pintura de Texturas / Splatmap Personalizado (Fase 16)
uniform sampler2D u_Splatmap;
uniform int u_UseCustomSplatmap;

// Control de Reflejos IBL del Terreno
uniform float u_TerrainReflectionIntensity;

// Capas de Textura PBR Multicapa (Arquitectura Modular de Nodos)
struct TerrainLayerData {
    int hasAlbedo;
    int hasNormal;
    int hasRoughSpec;
    int hasHeight;
    int hasAO;
    float tiling;
    float roughness;
    float specular;
    float metallic;
    float normalStrength;
    float heightScale;
    float aoStrength;
    float opacity;
    vec3 tint;
    vec3 fallbackColor;
};
uniform TerrainLayerData u_Layers[4];
uniform sampler2D u_LayerAlbedo[4];
uniform sampler2D u_LayerNormal[4];
uniform sampler2D u_LayerRoughSpec[4];
uniform sampler2D u_LayerHeight[4];
uniform sampler2D u_LayerAO[4];

// Modo de Relieve 3D de Superficie: 0 = Teselacion Geometrica OASIS, 1 = POM, 2 = Desactivado
uniform int u_ReliefMode;

// Mapeo Tonal y Gestión de Color
uniform int u_ColorProfile;   // 0 = ACES Filmic, 1 = AgX, 2 = Reinhard, 3 = Linear
uniform float u_Exposure;
uniform float u_Gamma;
uniform float u_Saturation;
uniform float u_Contrast;
uniform float u_Wetness;                 // Humedad de lluvia atmosférica (Fase 20)

// Neblina Atmosférica Global y Volumétrica (Fase 20)
uniform int u_GlobalFogEnabled;
uniform float u_GlobalFogStart;
uniform float u_GlobalFogEnd;
uniform float u_GlobalFogDensity;
uniform vec3 u_GlobalFogColor;

// ========================================================================
// 3. Funciones Matemáticas PBR y Auxiliares
// ========================================================================
const vec2 invAtan = vec2(0.1591549, 0.3183098);

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

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

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
    return num / max(denom, 0.0000001);
}

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

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Generador de Ruido Procedural de Superficie para evitar repeticiones
float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Disco de Vogel para muestreo estocástico uniforme de sombras PCSS
vec2 vogelDiskSampleTerrain(int sampleIndex, int sampleCount, float phi) {
    float GoldenAngle = 2.399963;
    float r = sqrt(float(sampleIndex) + 0.5) / sqrt(float(sampleCount));
    float theta = float(sampleIndex) * GoldenAngle + phi;
    return vec2(r * cos(theta), r * sin(theta));
}

float interleavedGradientNoiseTerrain(vec2 screenPos) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

// Búsqueda de bloqueadores para PCSS (Percentage-Closer Soft Shadows)
float findBlockerTerrain(sampler2D shadowMap, vec2 uv, float zReceiver, float searchRadius, float bias) {
    int blockers = 0;
    float avgBlockerDepth = 0.0;
    float randomAngle = interleavedGradientNoiseTerrain(gl_FragCoord.xy) * 6.2831853;

    int samples = clamp(u_PCSS_BlockerSamples, 8, 32);
    for (int i = 0; i < samples; ++i) {
        vec2 offset = vogelDiskSampleTerrain(i, samples, randomAngle) * searchRadius;
        float shadowDepth = texture(shadowMap, uv + offset).r;
        if (shadowDepth < zReceiver - bias) {
            avgBlockerDepth += shadowDepth;
            blockers++;
        }
    }

    if (blockers == 0) return -1.0;
    return avgBlockerDepth / float(blockers);
}

float pcssShadowTerrain(sampler2D shadowMap, vec2 uv, float zReceiver, vec2 texelSize, float bias) {
    float lightSize = max(u_PCSS_LightSize, 0.05);

    // 1. Búsqueda de bloqueador
    float searchRadius = lightSize * (zReceiver - 0.0001) / max(zReceiver, 0.0001) * texelSize.x * 24.0;
    searchRadius = clamp(searchRadius, texelSize.x * 2.0, texelSize.x * 60.0);

    float avgBlockerDepth = findBlockerTerrain(shadowMap, uv, zReceiver, searchRadius, bias);
    if (avgBlockerDepth < 0.0) return 0.0;

    // 2. Estimación analítica del radio de penumbra
    float penumbra = (zReceiver - avgBlockerDepth) / max(avgBlockerDepth, 0.0001);
    float filterRadius = clamp(penumbra * lightSize * texelSize.x * 40.0, texelSize.x * 0.5, texelSize.x * 80.0);

    // 3. Filtrado Percentage-Closer
    float shadow = 0.0;
    int filterSamples = clamp(u_PCSS_FilterSamples, 16, 64);
    float randomAngle = interleavedGradientNoiseTerrain(gl_FragCoord.xy + vec2(1.23, 4.56)) * 6.2831853;

    for (int i = 0; i < filterSamples; ++i) {
        vec2 offset = vogelDiskSampleTerrain(i, filterSamples, randomAngle) * filterRadius;
        float pcfDepth = texture(shadowMap, uv + offset).r;
        shadow += (zReceiver - bias > pcfDepth) ? 1.0 : 0.0;
    }
    return shadow / float(filterSamples);
}

// Búsqueda de bloqueador en arreglo de sombras CSM (sampler2DArray) para Terreno
float findBlockerArrayTerrain(sampler2DArray shadowArray, vec2 uv, float cascade, float zReceiver, float searchRadius, float bias) {
    int blockers = 0;
    float avgBlockerDepth = 0.0;
    float randomAngle = interleavedGradientNoiseTerrain(gl_FragCoord.xy) * 6.2831853;

    int samples = clamp(u_PCSS_BlockerSamples, 8, 32);
    for (int i = 0; i < samples; ++i) {
        vec2 offset = vogelDiskSampleTerrain(i, samples, randomAngle) * searchRadius;
        float shadowDepth = texture(shadowArray, vec3(uv + offset, cascade)).r;
        if (shadowDepth < zReceiver - bias) {
            avgBlockerDepth += shadowDepth;
            blockers++;
        }
    }

    if (blockers == 0) return -1.0;
    return avgBlockerDepth / float(blockers);
}

float pcssShadowArrayTerrain(sampler2DArray shadowArray, vec2 uv, float cascade, float zReceiver, vec2 texelSize, float bias) {
    float lightSize = max(u_PCSS_LightSize, 0.05);

    float searchRadius = lightSize * (zReceiver - 0.0001) / max(zReceiver, 0.0001) * texelSize.x * 24.0;
    searchRadius = clamp(searchRadius, texelSize.x * 2.0, texelSize.x * 60.0);

    float avgBlockerDepth = findBlockerArrayTerrain(shadowArray, uv, cascade, zReceiver, searchRadius, bias);
    if (avgBlockerDepth < 0.0) return 0.0;

    float penumbra = (zReceiver - avgBlockerDepth) / max(avgBlockerDepth, 0.0001);
    float filterRadius = clamp(penumbra * lightSize * texelSize.x * 40.0, texelSize.x * 0.5, texelSize.x * 80.0);

    float shadow = 0.0;
    int filterSamples = clamp(u_PCSS_FilterSamples, 16, 64);
    float randomAngle = interleavedGradientNoiseTerrain(gl_FragCoord.xy + vec2(1.23, 4.56)) * 6.2831853;

    for (int i = 0; i < filterSamples; ++i) {
        vec2 offset = vogelDiskSampleTerrain(i, filterSamples, randomAngle) * filterRadius;
        float pcfDepth = texture(shadowArray, vec3(uv + offset, cascade)).r;
        shadow += (zReceiver - bias > pcfDepth) ? 1.0 : 0.0;
    }
    return shadow / float(filterSamples);
}

// Filtrado de Sombras Avanzado con CSM (3 Cascadas) y Modos PCF / PCSS
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (u_ShadowsEnabled == 0) return 0.0;

    float cosTheta = clamp(dot(normal, -lightDir), 0.0, 1.0);
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
            return 0.0; // Fuera del alcance máximo de sombras
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
            return pcssShadowArrayTerrain(u_CascadeShadowMaps, projCoords.xy, float(cascadeIndex), currentDepth, texelSize, bias);
        }

        float shadow = 0.0;
        int kernelRadius = (u_ShadowKernel >= 3) ? 3 : ((u_ShadowKernel == 2) ? 2 : 1);
        float totalSamples = float((2 * kernelRadius + 1) * (2 * kernelRadius + 1));

        for (int x = -kernelRadius; x <= kernelRadius; ++x) {
            for (int y = -kernelRadius; y <= kernelRadius; ++y) {
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

    // Si está activo PCSS
    if (u_ShadowFilterMode == 1) {
        return pcssShadowTerrain(u_ShadowMap, projCoords.xy, currentDepth, texelSize, bias);
    }

    // Modo Tradicional PCF
    float currentDepthBiased = currentDepth - bias;
    float shadow = 0.0;
    int kernelRadius = u_ShadowKernel;
    float totalSamples = 0.0;

    for (int x = -kernelRadius; x <= kernelRadius; ++x) {
        for (int y = -kernelRadius; y <= kernelRadius; ++y) {
            vec2 offset = vec2(x, y) * texelSize * u_ShadowSoftness;
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + offset).r;
            shadow += (currentDepthBiased > pcfDepth) ? 1.0 : 0.0;
            totalSamples += 1.0;
        }
    }
    return shadow / totalSamples;
}

// Cálculo de Sombras Omnidireccionales con PCSS Físico (Contact Hardening en Cubemap) para Terreno
float calculateTerrainPointShadow(vec3 fragPos, vec3 lightPos) {
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

        // 1. Búsqueda de bloqueadores en el cono angular de luz
        float searchRadius = clamp((lightSize / currentDepth) * 0.85, 0.008, 0.25);
        int blockerSamples = 16;
        int blockers = 0;
        float avgBlockerDist = 0.0;
        float blockerAngle = interleavedGradientNoiseTerrain(gl_FragCoord.xy) * 6.2831853;

        for (int i = 0; i < blockerSamples; ++i) {
            vec2 disk = vogelDiskSampleTerrain(i, blockerSamples, blockerAngle) * searchRadius;
            vec3 sampleDir = normalize(dir + T1 * disk.x + T2 * disk.y);
            float shadowDist = texture(u_PointShadowMap, sampleDir).r * u_PointShadowFarPlane;
            if (shadowDist < currentDepth - bias) {
                avgBlockerDist += shadowDist;
                blockers++;
            }
        }

        if (blockers == 0) return 0.0;

        avgBlockerDist /= float(blockers);

        // 2. Estimación analítica del radio de penumbra (contacto endurecido)
        float penumbra = (currentDepth - avgBlockerDist) / max(avgBlockerDist, 0.01);
        float filterRadius = clamp(penumbra * (lightSize / currentDepth) * 1.5, 0.003, 0.20);

        // 3. Filtrado suave estocástico por disco de Vogel
        float shadow = 0.0;
        int filterSamples = 24;
        float filterAngle = interleavedGradientNoiseTerrain(gl_FragCoord.xy + vec2(4.56, 1.23)) * 6.2831853;

        for (int i = 0; i < filterSamples; ++i) {
            vec2 disk = vogelDiskSampleTerrain(i, filterSamples, filterAngle) * filterRadius;
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
        float closestDepth = texture(u_PointShadowMap, fragToLight + g_TerrainPointShadowSamples[s] * diskRadius).r;
        closestDepth *= u_PointShadowFarPlane;
        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }
    return shadow / 20.0;
}

// ========================================================================
// 4. Splatmapping PBR Multicapa de Terreno con Normal Mapping Cotangente y Parallax POM
// ========================================================================

// Perturbación de Normal con derivadas precalculadas de la superficie geométrica (Schüler Cotangent Frame)
vec3 perturbNormalGrad(vec3 N, vec2 ddxUV, vec2 ddyUV, vec3 mapNormal, float normalScale) {
    vec3 map = mapNormal * 2.0 - 1.0;
    map.xy *= normalScale;
    map = normalize(map);

    vec3 q0 = dFdx(FragPos);
    vec3 q1 = dFdy(FragPos);
    vec2 st0 = ddxUV;
    vec2 st1 = ddyUV;

    vec3 normal = normalize(N);
    vec3 q1perp = cross(q1, normal);
    vec3 q0perp = cross(normal, q0);

    vec3 T = q1perp * st0.x + q0perp * st1.x;
    vec3 B = q1perp * st0.y + q0perp * st1.y;

    float det = max(dot(T, T), dot(B, B));
    if (det <= 1e-12) return normal;
    float scale = inversesqrt(det);

    return normalize(T * (map.x * scale) + B * (map.y * scale) + normal * map.z);
}

// Parallax Occlusion Mapping (POM) para relieve 3D profundo con Heightmaps
// Utiliza textureGrad con derivadas continuas para prevenir el colapso de LOD en distancias cortas (Close-Up)
vec2 parallaxMapping(int idx, vec2 texcoord, vec2 ddx, vec2 ddy, vec3 V, vec3 N, float heightScale) {
    if (u_Layers[idx].hasHeight == 0 || heightScale <= 0.0005) {
        return texcoord;
    }

    vec3 q0 = dFdx(FragPos);
    vec3 q1 = dFdy(FragPos);
    vec2 st0 = ddx;
    vec2 st1 = ddy;

    vec3 normal = normalize(N);
    vec3 q1perp = cross(q1, normal);
    vec3 q0perp = cross(normal, q0);

    vec3 T = q1perp * st0.x + q0perp * st1.x;
    vec3 B = q1perp * st0.y + q0perp * st1.y;

    float det = max(dot(T, T), dot(B, B));
    if (det <= 1e-12) return texcoord;
    float scale = inversesqrt(det);

    mat3 tbn = mat3(normalize(T * scale), normalize(B * scale), normal);
    vec3 V_ts = transpose(tbn) * V;
    float vLen = length(V_ts);
    if (vLen < 1e-5) return texcoord;
    V_ts /= vLen;

    // Si la cámara mira desde atrás de la superficie o rasante límite, evitar desplazamientos aberrantes
    if (V_ts.z <= 0.001) return texcoord;

    // Número adaptativo de pasos de raymarching según el ángulo de incidencia
    float minLayers = 10.0;
    float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, clamp(V_ts.z, 0.0, 1.0));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    // Desplazamiento de paralaje con proyección en perspectiva: P = (V_ts.xy / V_ts.z) * heightScale
    vec2 P = (V_ts.xy / max(V_ts.z, 0.25)) * (heightScale * 0.04);
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texcoord;
    float currentDepthMapValue = 1.0 - textureGrad(u_LayerHeight[idx], currentTexCoords, ddx, ddy).r;

    for (int i = 0; i < 32; ++i) {
        if (currentLayerDepth >= currentDepthMapValue) break;
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - textureGrad(u_LayerHeight[idx], currentTexCoords, ddx, ddy).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0 - textureGrad(u_LayerHeight[idx], prevTexCoords, ddx, ddy).r) - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth + 1e-6);
    return mix(currentTexCoords, prevTexCoords, clamp(weight, 0.0, 1.0));
}

void sampleLayer(int idx, vec2 uv, vec3 V, vec3 baseNormal, out vec3 outAlbedo, out vec3 outNormal, out float outRoughness, out float outMetallic, out float outAO) {
    TerrainLayerData L = u_Layers[idx];
    vec2 tiledUV = uv * L.tiling;
    vec2 ddx = dFdx(tiledUV);
    vec2 ddy = dFdy(tiledUV);

    // 0. Parallax Occlusion Mapping (POM) únicamente si está activado el modo POM (1)
    vec2 finalUV = tiledUV;
    if (u_ReliefMode == 1 && L.hasHeight == 1) {
        finalUV = parallaxMapping(idx, tiledUV, ddx, ddy, V, baseNormal, L.heightScale);
    }

    // 1. Albedo / Color (Muestreo con textureGrad para garantizar nitidez y evitar caídas de MIP level en primer plano)
    if (L.hasAlbedo == 1) {
        vec3 texCol = textureGrad(u_LayerAlbedo[idx], finalUV, ddx, ddy).rgb;
        texCol = pow(texCol, vec3(2.2)); // Convertir sRGB a lineal
        outAlbedo = texCol * L.tint;
    } else {
        outAlbedo = L.fallbackColor * L.tint;
    }

    // 2. Normal Map (Relieve de micro-superficie sobre el frame cotangente geométrico)
    if (L.hasNormal == 1) {
        vec3 nMap = textureGrad(u_LayerNormal[idx], finalUV, ddx, ddy).rgb;
        outNormal = perturbNormalGrad(baseNormal, ddx, ddy, nMap, L.normalStrength);
    } else {
        outNormal = baseNormal;
    }

    // 3. Rugosidad y Especular
    if (L.hasRoughSpec == 1) {
        vec4 rMap = textureGrad(u_LayerRoughSpec[idx], finalUV, ddx, ddy);
        outRoughness = clamp(rMap.r * L.roughness, 0.04, 1.0);
    } else {
        outRoughness = L.roughness;
    }
    outMetallic = L.metallic;

    // 4. Ambient Occlusion (AO)
    if (L.hasAO == 1) {
        float aoVal = textureGrad(u_LayerAO[idx], finalUV, ddx, ddy).r;
        outAO = mix(1.0, aoVal, L.aoStrength);
    } else {
        outAO = 1.0;
    }
}

void computeTerrainMaterial(vec3 baseNormal, vec3 V, out vec3 albedo, out vec3 blendedNormal, out float roughness, out float metallic, out float ambientOcclusion) {
    float nMacro = noise2D(FragPos.xz * 0.08);
    float nMicro = noise2D(FragPos.xz * 0.45);
    float nDetail = noise2D(FragPos.xz * 1.8);

    vec4 w;

    if (u_UseCustomSplatmap == 1) {
        // Splatmap dinámico pintado con pinceles en tiempo real
        w = texture(u_Splatmap, TexCoords);
        float totalW = w.r + w.g + w.b + w.a;
        if (totalW > 0.001) {
            w /= totalW;
        } else {
            w = vec4(1.0, 0.0, 0.0, 0.0);
        }
    } else {
        // Reglas de distribución procedural por pendiente y altitud
        // Capa 0 (Pasto): Llanura y valles
        // Capa 1 (Tierra): Transición de elevación media
        // Capa 2 (Roca): Acantilados y pendientes pronunciadas
        // Capa 3 (Nieve): Cumbres alpinas

        float cliffFactor = smoothstep(0.78, 0.58 + nMicro * 0.06, SlopeCos);
        float snowFactor = smoothstep(13.0 + nMacro * 3.0, 22.0, WorldElevation);
        float dirtFactor = smoothstep(1.5, 9.0 + nMacro * 4.0, WorldElevation);

        float r_rock = cliffFactor;
        float r_snow = (1.0 - r_rock) * snowFactor;
        float r_dirt = (1.0 - r_rock) * (1.0 - snowFactor) * dirtFactor;
        float r_grass = (1.0 - r_rock) * (1.0 - snowFactor) * (1.0 - dirtFactor);

        w = vec4(r_grass, r_dirt, r_rock, r_snow);
        float totalW = w.r + w.g + w.b + w.a;
        if (totalW > 0.001) w /= totalW;
    }

    vec3 accAlbedo = vec3(0.0);
    vec3 accNormal = vec3(0.0);
    float accRoughness = 0.0;
    float accMetallic = 0.0;
    float accAO = 0.0;

    // Muestrear las 4 capas según sus pesos
    for (int i = 0; i < 4; ++i) {
        float weight = (i == 0) ? w.r : ((i == 1) ? w.g : ((i == 2) ? w.b : w.a));
        if (weight > 0.001) {
            vec3 lAlb, lNorm;
            float lRough, lMet, lAO;
            sampleLayer(i, TexCoords, V, baseNormal, lAlb, lNorm, lRough, lMet, lAO);

            accAlbedo += lAlb * weight;
            accNormal += lNorm * weight;
            accRoughness += lRough * weight;
            accMetallic += lMet * weight;
            accAO += lAO * weight;
        }
    }

    albedo = accAlbedo;
    blendedNormal = (length(accNormal) > 0.001) ? normalize(accNormal) : baseNormal;
    roughness = clamp(accRoughness, 0.04, 1.0);
    metallic = clamp(accMetallic, 0.0, 1.0);
    ambientOcclusion = (accAO > 0.001) ? clamp(accAO, 0.0, 1.0) : 1.0;
}

// ========================================================================
// 5. Función Principal
// ========================================================================
void main() {
    vec3 N_base = normalize(Normal);
    vec3 V = normalize(u_ViewPos - FragPos);

    // Obtener propiedades PBR de la superficie mediante el splatmapping multicapa
    vec3 albedo;
    vec3 N;
    float roughness;
    float metallic;
    float ao;
    computeTerrainMaterial(N_base, V, albedo, N, roughness, metallic, ao);

    // Fase 20: Clima Atmosférico y Humedad de Superficie (Wetness)
    if (u_Wetness > 0.001) {
        albedo = mix(albedo, albedo * 0.55, u_Wetness * (1.0 - metallic * 0.7));
        roughness = mix(roughness, max(roughness * 0.20, 0.03), u_Wetness);
    }

    // Coeficiente base de reflectancia en incidencia normal F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // --- Iluminación Directa del Sol ---
    vec3 L_sun = normalize(-u_LightDir);
    vec3 H_sun = normalize(V + L_sun);
    float NdotL_sun = max(dot(N, L_sun), 0.0);

    float NDF_sun = DistributionGGX(N, H_sun, roughness);
    float G_sun   = GeometrySmith(N, V, L_sun, roughness);
    vec3 F_sun    = fresnelSchlick(max(dot(H_sun, V), 0.0), F0);

    vec3 numerator_sun   = NDF_sun * G_sun * F_sun;
    float denom_sun      = 4.0 * max(dot(N, V), 0.0) * NdotL_sun + 0.0001;
    float reflScale      = (u_TerrainReflectionIntensity >= 0.0) ? u_TerrainReflectionIntensity : 1.0;
    vec3 specular_sun    = (numerator_sun / denom_sun) * reflScale;

    vec3 kS_sun = F_sun;
    vec3 kD_sun = vec3(1.0) - kS_sun;
    kD_sun *= (1.0 - metallic);

    float shadowFactor = calculateShadow(FragPosLightSpace, N, u_LightDir);
    vec3 sunLightRadiance = u_LightColor * (1.0 - shadowFactor);
    vec3 Lo = (kD_sun * albedo / PI + specular_sun) * sunLightRadiance * NdotL_sun;

    // --- Luces Puntuales (Point Lights) ---
    for (int i = 0; i < u_NumPointLights && i < MAX_POINT_LIGHTS; ++i) {
        if (u_PointLights[i].enabled == 0) continue;

        vec3 lightVec = u_PointLights[i].position - FragPos;
        float distance = length(lightVec);
        vec3 L = normalize(lightVec);
        vec3 H = normalize(V + L);

        float attenuation = 1.0 / (u_PointLights[i].constant +
                                   u_PointLights[i].linear * distance +
                                   u_PointLights[i].quadratic * (distance * distance));
        vec3 radiance = u_PointLights[i].color * (u_PointLights[i].intensity * attenuation);

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = (numerator / denominator) * reflScale;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);

        float pShadow = 0.0;
        if (u_PointShadowsEnabled == 1 && u_PointLights[i].castShadows == 1) {
            pShadow = calculateTerrainPointShadow(FragPos, u_PointLights[i].position);
        }

        Lo += (1.0 - pShadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // --- Iluminación Ambiental e IBL ---
    vec3 ambient = u_AmbientColor * albedo * ao;

    if (u_UseIBL == 1) {
        vec3 R = reflect(-V, N);
        vec3 rotR = rotateY(R, u_SkyboxRotation);
        vec2 uvR = dirToUV(rotR);
        float lod = roughness * 7.0;
        vec3 prefilteredSpecular = textureLod(u_EnvMap, uvR, lod).rgb * reflScale;
        if (isnan(prefilteredSpecular.r) || isinf(prefilteredSpecular.r)) prefilteredSpecular = vec3(0.0);
        prefilteredSpecular = clamp(prefilteredSpecular, vec3(0.0), vec3(256.0));

        vec3 rotN = rotateY(N, u_SkyboxRotation);
        vec2 uvN = dirToUV(rotN);
        vec3 irradiance = textureLod(u_EnvMap, uvN, 6.5).rgb;
        if (isnan(irradiance.r) || isinf(irradiance.r)) irradiance = vec3(0.0);
        irradiance = clamp(irradiance, vec3(0.0), vec3(64.0));

        vec3 F_IBL = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kD_IBL = (vec3(1.0) - F_IBL) * (1.0 - metallic);
        vec3 diffuseIBL = irradiance * albedo * ao;
        vec3 specularIBL = prefilteredSpecular * (F_IBL * (1.0 - roughness) + 0.04) * reflScale;

        vec3 iblTotal = (kD_IBL * diffuseIBL + specularIBL) * u_IBLIntensity;
        if (isnan(iblTotal.r) || isinf(iblTotal.r)) iblTotal = vec3(0.0);
        iblTotal = max(iblTotal, vec3(0.0));

        // Preservar siempre un piso de luz ambiental base para que caras en sombra nunca colapsen a negro absoluto
        ambient = (u_AmbientColor * albedo * ao) * 0.25 + iblTotal;
    }

    vec3 color = ambient + Lo;
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

    // ========================================================================
    // Mapeo Tonal, Exposición y Gestión de Color
    // ========================================================================
    color *= max(u_Exposure, 0.001);

    vec3 mapped = color;
    if (u_ColorProfile == 0) {
        mapped = ACESFilm(color);
    } else if (u_ColorProfile == 1) {
        vec3 x = max(color, vec3(0.0));
        vec3 logColor = clamp(log2(x + 0.0001) / 10.0 + 0.5, 0.0, 1.0);
        mapped = logColor * logColor * (3.0 - 2.0 * logColor);
    } else if (u_ColorProfile == 2) {
        mapped = color / (color + vec3(1.0));
    } else {
        mapped = clamp(color, 0.0, 1.0);
    }

    float luma = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
    mapped = mix(vec3(luma), mapped, max(u_Saturation, 0.0));
    mapped = mix(vec3(0.5), mapped, max(u_Contrast, 0.0));
    mapped = clamp(mapped, 0.0, 1.0);

    mapped = pow(mapped, vec3(1.0 / max(u_Gamma, 0.1)));

    if (isnan(mapped.r) || isinf(mapped.r) || mapped.r < 0.0) mapped.r = 0.0;
    if (isnan(mapped.g) || isinf(mapped.g) || mapped.g < 0.0) mapped.g = 0.0;
    if (isnan(mapped.b) || isinf(mapped.b) || mapped.b < 0.0) mapped.b = 0.0;
    mapped = clamp(mapped, 0.0, 1.0);

    if (u_OASIS_DebugLOD == 1) {
        mapped = mix(mapped, OASIS_DebugColor, 0.65);
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
