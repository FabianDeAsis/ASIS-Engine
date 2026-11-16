#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;

// Soporte nativo para OASIS
uniform int u_OASIS_Enabled;
uniform vec2 u_ChunkOffset;
uniform float u_ChunkSize;
uniform float u_MorphFactor;
uniform float u_WorldSize;
uniform float u_BaseOffset;
uniform float u_PatchGridSize;
uniform sampler2D u_Heightmap;

out vec4 FragPos;

void main()
{
    if (u_OASIS_Enabled == 1) {
        vec2 normPos = aPos.xz;
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
        vec2 outUV = clamp((worldXZ + vec2(u_WorldSize * 0.5)) / u_WorldSize, 0.0, 1.0);
        float h = textureLod(u_Heightmap, outUV, 0.0).r;
        FragPos = vec4(worldXZ.x, h + u_BaseOffset, worldXZ.y, 1.0);
        gl_Position = u_LightSpaceMatrix * FragPos;
    } else {
        FragPos = u_Model * vec4(aPos, 1.0);
        gl_Position = u_LightSpaceMatrix * FragPos;
    }
}
