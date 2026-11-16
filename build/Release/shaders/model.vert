#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aColor;
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 VertexColor;
out vec4 FragPosLightSpace;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightSpaceMatrix;

// Skinning de Huesos para Animaciones Esqueléticas (Fase 13)
const int MAX_BONES = 100;
uniform mat4 u_BoneTransforms[MAX_BONES];
uniform int u_IsAnimated;

void main()
{
    vec3 localPos = aPos;
    vec3 localNormal = aNormal;

    if (u_IsAnimated == 1) {
        vec4 totalPos = vec4(0.0);
        vec3 totalNorm = vec3(0.0);
        float totalWeight = 0.0;

        for (int i = 0; i < 4; ++i) {
            if (aBoneIDs[i] >= 0 && aBoneIDs[i] < MAX_BONES && aWeights[i] > 0.0) {
                mat4 boneMat = u_BoneTransforms[aBoneIDs[i]];
                totalPos += boneMat * vec4(aPos, 1.0) * aWeights[i];
                totalNorm += mat3(boneMat) * aNormal * aWeights[i];
                totalWeight += aWeights[i];
            }
        }

        if (totalWeight > 0.001) {
            localPos = totalPos.xyz / totalWeight;
            localNormal = normalize(totalNorm);
        }
    }

    // Coordenadas mundiales del fragmento
    FragPos = vec3(u_Model * vec4(localPos, 1.0));

    // Matriz normal para transformar las normales sin distorsión por escalas no uniformes
    Normal = mat3(transpose(inverse(u_Model))) * localNormal;

    TexCoords = aTexCoords;
    VertexColor = aColor;

    // Posición del fragmento en el espacio de la luz solar para calcular sombras
    FragPosLightSpace = u_LightSpaceMatrix * vec4(FragPos, 1.0);

    gl_Position = u_Projection * u_View * vec4(FragPos, 1.0);
}
