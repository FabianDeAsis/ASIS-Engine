#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 ClipSpacePos;
out float WaveCrest;
out vec3 ViewDir;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_ViewPos;

uniform float u_WaterLevel;
uniform float u_Time;
uniform float u_WaveAmplitude;
uniform float u_WaveSpeed;

struct GerstnerWave {
    vec2 direction;
    float amplitude;
    float wavelength;
    float speed;
    float steepness;
};
uniform GerstnerWave u_Waves[4];

// OASIS Océano (Geometría Virtual y Streaming Adaptativo CDLOD)
uniform int u_WaterOASIS_Enabled;
uniform vec2 u_WaterChunkOffset;
uniform float u_WaterChunkSize;

void main() {
    vec3 worldPos;
    vec2 effectiveTexCoords;

    if (u_WaterOASIS_Enabled == 1) {
        // En modo OASIS, aPos está en el rango [0, 1] y se escala al chunk
        worldPos = vec3(u_WaterChunkOffset.x + aPos.x * u_WaterChunkSize, u_WaterLevel, u_WaterChunkOffset.y + aPos.z * u_WaterChunkSize);
        effectiveTexCoords = vec2(worldPos.x, worldPos.z) * 0.05;
    } else {
        worldPos = (u_Model * vec4(aPos, 1.0)).xyz;
        effectiveTexCoords = aTexCoords;
    }

    vec3 P = vec3(worldPos.x, u_WaterLevel, worldPos.z);
    vec3 normal = vec3(0.0, 1.0, 0.0);
    float crestFactor = 0.0;

    const float PI = 3.14159265359;

    // Simulación de 4 trenes de olas armónicas de Gerstner
    for (int i = 0; i < 4; ++i) {
        vec2 d = normalize(u_Waves[i].direction);
        float wLen = max(u_Waves[i].wavelength, 0.2);
        float k = 2.0 * PI / wLen;
        float c = sqrt(9.81 / k) * u_Waves[i].speed * u_WaveSpeed;
        float amp = u_Waves[i].amplitude * u_WaveAmplitude;
        float f = k * dot(d, worldPos.xz) - (k * c) * u_Time;
        float q = u_Waves[i].steepness / max(k * amp * 4.0, 0.001);

        // Desplazamiento horizontal hacia las crestas y vertical
        P.x += q * amp * d.x * cos(f);
        P.z += q * amp * d.y * cos(f);
        P.y += amp * sin(f);

        // Normal analítica exacta por derivadas parciales
        normal.x -= d.x * (k * amp) * cos(f);
        normal.z -= d.y * (k * amp) * cos(f);
        normal.y -= q * (k * amp) * sin(f);

        // Curvatura de cresta para espuma de olas rompientes
        crestFactor += clamp(sin(f) * 0.5 + 0.5, 0.0, 1.0);
    }

    FragPos = P;
    Normal = normalize(normal);
    TexCoords = effectiveTexCoords;
    ViewDir = normalize(u_ViewPos - P);
    WaveCrest = crestFactor;

    ClipSpacePos = u_Projection * u_View * vec4(P, 1.0);
    gl_Position = ClipSpacePos;
}
