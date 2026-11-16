#version 330 core
out vec4 FragColor;

in vec3 WorldPos;

uniform sampler2D u_EnvMap;
uniform int u_ShowSkybox;
uniform int u_HasCustomHDRI;
uniform float u_SkyboxExposure;
uniform float u_SkyboxRotation;
uniform float u_SkyboxBlur;
uniform int u_FlipX;
uniform int u_FlipY;

// Uniforms de Bóveda Celeste Dinámica y Ciclo Día/Noche (Fase 20)
uniform vec3 u_SunDir;
uniform vec3 u_MoonDir;
uniform vec3 u_SunColor;
uniform vec3 u_MoonColor;
uniform vec3 u_ZenithColor;
uniform vec3 u_HorizonColor;
uniform float u_HorizonHeight;
uniform float u_StarsIntensity;

// Neblina Atmosférica Global (Fase 20)
uniform int u_GlobalFogEnabled;
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

void main() {
    vec3 dir = normalize(WorldPos);

    if (u_ShowSkybox == 1) {
        vec3 color;
        if (u_HasCustomHDRI == 1) {
            // Modo HDRI personalizado externo (.hdr / .exr / .png 360)
            vec3 rotDir = rotateY(dir, u_SkyboxRotation);
            vec2 uv = dirToUV(rotDir);
            color = textureLod(u_EnvMap, uv, u_SkyboxBlur).rgb;
        } else {
            // Modo Bóveda Celeste Procedural Dinámica 24h (Fase 20)
            float normY = dir.y - u_HorizonHeight;

            if (normY >= 0.0) {
                // Degradado atmosférico de horizonte a cenit
                float t = pow(clamp(normY * 2.5, 0.0, 1.0), 0.65);
                color = mix(u_HorizonColor, u_ZenithColor, t);

                // Disco solar y corona atmosférica de dispersión
                vec3 L_sun = normalize(-u_SunDir);
                float sunDot = max(0.0, dot(dir, L_sun));
                if (sunDot > 0.0) {
                    float sunDisk = smoothstep(0.9991, 0.9998, sunDot) * 14.0;
                    float sunHalo = pow(sunDot, 12.0) * 1.6 + pow(sunDot, 128.0) * 2.8;
                    color += u_SunColor * (sunDisk + sunHalo);
                }

                // Disco lunar y resplandor de noche
                vec3 L_moon = normalize(-u_MoonDir);
                float moonDot = max(0.0, dot(dir, L_moon));
                if (moonDot > 0.0) {
                    float moonDisk = smoothstep(0.9982, 0.9994, moonDot) * 3.8;
                    float moonGlow = pow(moonDot, 24.0) * 0.45;
                    color += u_MoonColor * (moonDisk + moonGlow);
                }

                // Estrellas titilantes procedimentales en la noche
                if (u_StarsIntensity > 0.01 && normY > 0.035) {
                    vec3 p = dir * 180.0;
                    vec3 ip = floor(p);
                    float starHash = fract(sin(dot(ip, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
                    if (starHash > 0.983) {
                        float starBrightness = pow((starHash - 0.983) / 0.017, 5.0) * u_StarsIntensity * 3.5;
                        color += vec3(starBrightness);
                    }
                }
            } else {
                // Efecto horizonte atmosférico bajo el horizonte (fundido suave al color del horizonte)
                float groundT = clamp((-normY) * 3.0, 0.0, 1.0);
                color = mix(u_HorizonColor, u_HorizonColor * 0.20, groundT);
            }
        }

        if (isnan(color.r) || isinf(color.r) || color.r < 0.0) color.r = 0.0;
        if (isnan(color.g) || isinf(color.g) || color.g < 0.0) color.g = 0.0;
        if (isnan(color.b) || isinf(color.b) || color.b < 0.0) color.b = 0.0;
        color *= max(u_SkyboxExposure, 0.001);

        // Neblina Atmosférica Global (Funde el horizonte tanto en Procedural como en cualquier HDRI externo)
        if (u_GlobalFogEnabled == 1) {
            float fogFactor = clamp((1.0 - dir.y) * 1.5, 0.0, 1.0);
            fogFactor = pow(fogFactor, 1.6) * u_GlobalFogDensity;
            color = mix(color, u_GlobalFogColor, clamp(fogFactor, 0.0, 0.92));
        }

        FragColor = vec4(color, 1.0);
    } else {
        // Fondo de estudio degradado neutro y elegante estilo Blender / Unreal
        float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 topColor = vec3(0.12, 0.13, 0.16);
        vec3 bottomColor = vec3(0.04, 0.04, 0.05);
        vec3 studioColor = mix(bottomColor, topColor, t);
        FragColor = vec4(studioColor, 1.0);
    }
}
