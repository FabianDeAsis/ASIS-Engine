#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out float WorldElevation;
out float SlopeCos;
out vec3 OASIS_DebugColor;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightSpaceMatrix;

// Uniforms de OASIS (Occlusion and Adaptive Streaming of Infinite Surfaces)
uniform int u_OASIS_Enabled;
uniform vec2 u_ChunkOffset;
uniform float u_ChunkSize;
uniform int u_ChunkLOD;
uniform float u_MorphFactor;
uniform float u_WorldSize;
uniform float u_BaseOffset;
uniform float u_PatchGridSize;
uniform sampler2D u_Heightmap;

// Modo de Relieve 3D y Teselación OASIS
uniform int u_ReliefMode; // 0 = Teselacion Geometrica OASIS, 1 = POM, 2 = Desactivado
uniform float u_TessellationDisplacementScale;

// Splatmap y Capas de Material PBR
uniform sampler2D u_Splatmap;
uniform int u_UseCustomSplatmap;

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
uniform sampler2D u_LayerHeight[4];

void main() {
    vec4 worldPos;
    vec3 outNormal;
    vec2 outUV;

    if (u_OASIS_Enabled == 1) {
        // Coordenada normalizada en el chunk [0, 1]
        vec2 normPos = aPos.xz;

        // Geomorphing suave de CDLOD para evitar T-Junctions y popping adaptado a la resolución del patch
        if (u_MorphFactor > 0.0) {
            float gridN = (u_PatchGridSize > 1.0) ? u_PatchGridSize : 32.0;
            vec2 gridPos = normPos * gridN;
            vec2 fracPos = fract(gridPos * 0.5);
            if (fracPos.x > 0.25 || fracPos.y > 0.25) {
                vec2 snapPos = floor(gridPos * 0.5 + 0.001) * 2.0 / gridN;
                normPos = mix(normPos, snapPos, u_MorphFactor);
            }
        }

        vec2 worldXZ = u_ChunkOffset + normPos * u_ChunkSize;
        outUV = clamp((worldXZ + vec2(u_WorldSize * 0.5)) / u_WorldSize, 0.0, 1.0);

        // Desplazamiento geométrico macro de vértices con altura muestreada en GPU
        float h = textureLod(u_Heightmap, outUV, 0.0).r;
        worldPos = vec4(worldXZ.x, h + u_BaseOffset, worldXZ.y, 1.0);

        // Cálculo de normales analíticas precisas mediante diferencias centrales en el heightmap
        ivec2 texSize = textureSize(u_Heightmap, 0);
        float texelSize = 1.0 / max(float(texSize.x), 1.0);
        float hL = textureLod(u_Heightmap, outUV - vec2(texelSize, 0.0), 0.0).r;
        float hR = textureLod(u_Heightmap, outUV + vec2(texelSize, 0.0), 0.0).r;
        float hD = textureLod(u_Heightmap, outUV - vec2(0.0, texelSize), 0.0).r;
        float hU = textureLod(u_Heightmap, outUV + vec2(0.0, texelSize), 0.0).r;

        float worldStep = (u_WorldSize * texelSize);
        outNormal = normalize(vec3((hL - hR) / (2.0 * worldStep), 1.0, (hD - hU) / (2.0 * worldStep)));
    } else {
        worldPos = u_Model * vec4(aPos, 1.0);
        mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
        outNormal = normalize(normalMatrix * aNormal);
        outUV = aTexCoords;
    }

    // ========================================================================
    // Teselación Geométrica OASIS (Micro-desplazamiento Físico de Capas PBR)
    // ========================================================================
    if (u_ReliefMode == 0) {
        vec4 w;
        if (u_UseCustomSplatmap == 1) {
            w = textureLod(u_Splatmap, outUV, 0.0);
            float totalW = w.r + w.g + w.b + w.a;
            if (totalW > 0.001) {
                w /= totalW;
            } else {
                w = vec4(1.0, 0.0, 0.0, 0.0);
            }
        } else {
            // Reglas de distribución procedural en vértice
            float cliffFactor = smoothstep(0.78, 0.58, outNormal.y);
            float snowFactor = smoothstep(13.0, 22.0, worldPos.y);
            float dirtFactor = smoothstep(1.5, 9.0, worldPos.y);

            float r_rock = cliffFactor;
            float r_snow = (1.0 - r_rock) * snowFactor;
            float r_dirt = (1.0 - r_rock) * (1.0 - snowFactor) * dirtFactor;
            float r_grass = (1.0 - r_rock) * (1.0 - snowFactor) * (1.0 - dirtFactor);

            w = vec4(r_grass, r_dirt, r_rock, r_snow);
            float totalW = w.r + w.g + w.b + w.a;
            if (totalW > 0.001) w /= totalW;
        }

        float totalDisplacement = 0.0;
        for (int i = 0; i < 4; ++i) {
            float weight = (i == 0) ? w.r : ((i == 1) ? w.g : ((i == 2) ? w.b : w.a));
            if (weight > 0.001 && u_Layers[i].hasHeight == 1) {
                vec2 layerUV = outUV * u_Layers[i].tiling;
                float hVal = textureLod(u_LayerHeight[i], layerUV, 0.0).r;
                // Escala de altura de la capa centrada en 0.5
                float layerDisp = (hVal - 0.5) * u_Layers[i].heightScale * 4.0;
                totalDisplacement += layerDisp * weight * u_Layers[i].opacity;
            }
        }

        // Desvanecimiento suave continuo según la distancia euclidiana exacta a la cámara
        // 100% de teselación física en primer plano (hasta 35m), disipando suavemente hasta 75m
        vec4 viewPos = u_View * worldPos;
        float distToCam = length(viewPos.xyz);
        float distFade = smoothstep(75.0, 35.0, distToCam);

        float finalDisp = totalDisplacement * u_TessellationDisplacementScale * distFade;
        worldPos.xyz += outNormal * finalDisp;
    }

    FragPos = worldPos.xyz;
    WorldElevation = worldPos.y;
    Normal = outNormal;
    SlopeCos = clamp(outNormal.y, 0.0, 1.0);
    TexCoords = outUV;
    FragPosLightSpace = u_LightSpaceMatrix * vec4(FragPos, 1.0);

    // Paleta de visualización de niveles de LOD en OASIS
    if (u_ChunkLOD == 0)      OASIS_DebugColor = vec3(0.12, 0.95, 0.35); // LOD 0
    else if (u_ChunkLOD == 1) OASIS_DebugColor = vec3(0.95, 0.88, 0.15); // LOD 1
    else if (u_ChunkLOD == 2) OASIS_DebugColor = vec3(1.0, 0.52, 0.12);  // LOD 2
    else if (u_ChunkLOD == 3) OASIS_DebugColor = vec3(0.92, 0.15, 0.22); // LOD 3
    else if (u_ChunkLOD == 4) OASIS_DebugColor = vec3(0.68, 0.22, 0.95); // LOD 4
    else                      OASIS_DebugColor = vec3(0.35, 0.85, 0.95); // LOD 5+ (Centimétrico)

    gl_Position = u_Projection * u_View * worldPos;
}
