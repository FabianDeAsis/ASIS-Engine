#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_Scene;
uniform sampler2D u_Depth;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_InvProjection;
uniform vec3 u_LightDir; // En World Space (dirección hacia donde viaja la luz)

uniform float u_Distance;
uniform int u_Steps;
uniform float u_Intensity;
uniform float u_Near;
uniform float u_Far;

float linearDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * u_Near * u_Far) / (u_Far + u_Near - z * (u_Far - u_Near));
}

vec3 getViewPos(vec2 uv, float d) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, d * 2.0 - 1.0, 1.0);
    vec4 viewPos4 = u_InvProjection * clipPos;
    return viewPos4.xyz / viewPos4.w;
}

void main() {
    vec4 sceneColor = texture(u_Scene, TexCoords);
    float depth = texture(u_Depth, TexCoords).r;
    if (depth >= 1.0) {
        FragColor = sceneColor;
        return;
    }

    // Reconstruir posición en View Space
    vec3 viewPos = getViewPos(TexCoords, depth);
    float camDist = -viewPos.z;

    // 1. Desvanecimiento suave por distancia a la cámara
    // Las sombras de contacto son micro-sombras de alta frecuencia pensadas para objetos cercanos (< 20m)
    // Más allá de 20m el buffer de profundidad pierde precisión submilimétrica y genera ruido
    if (camDist > 25.0) {
        FragColor = sceneColor;
        return;
    }
    float distFade = clamp(1.0 - (camDist - 12.0) / 13.0, 0.0, 1.0);

    // Vector hacia el sol/luz en View Space (-u_LightDir apunta hacia la fuente de luz)
    vec3 lightDirView = normalize((u_View * vec4(-u_LightDir, 0.0)).xyz);

    // 2. Reconstrucción de normal de superficie para descartar caras en sombra propia
    ivec2 texSize = textureSize(u_Depth, 0);
    vec2 texel = 1.0 / vec2(max(texSize.x, 1), max(texSize.y, 1));
    float dR = texture(u_Depth, TexCoords + vec2(texel.x, 0.0)).r;
    float dU = texture(u_Depth, TexCoords + vec2(0.0, texel.y)).r;
    vec3 pR = getViewPos(TexCoords + vec2(texel.x, 0.0), dR);
    vec3 pU = getViewPos(TexCoords + vec2(0.0, texel.y), dU);
    vec3 N_view = normalize(cross(pR - viewPos, pU - viewPos));

    float NdotL = dot(N_view, lightDirView);
    if (NdotL <= 0.01) {
        // Cara opuesta al sol: no necesita micro-sombras adicionales
        FragColor = sceneColor;
        return;
    }
    float angleFade = smoothstep(0.01, 0.20, NdotL);

    // 3. Configuración de Raymarching con Dithering Per-Pixel
    float maxDist = clamp(u_Distance, 0.05, 1.5);
    int steps = clamp(u_Steps, 6, 48);
    float stepSize = maxDist / float(steps);

    // Dither pseudo-aleatorio para dispersar las muestras y evitar cortes duros
    float dither = fract(sin(dot(TexCoords * vec2(texSize), vec2(12.9898, 78.233))) * 43758.5453);

    // Offset inicial a lo largo de la normal y del rayo para evitar auto-ensombrecido (Self-Shadow Acne)
    float startOffset = stepSize * (1.2 + dither * 0.8);
    vec3 rayPos = viewPos + N_view * 0.015 + lightDirView * startOffset;

    float shadow = 0.0;
    float thickness = max(0.045, 0.007 * camDist);

    for (int i = 0; i < steps; ++i) {
        rayPos += lightDirView * stepSize;

        vec4 proj = u_Projection * vec4(rayPos, 1.0);
        if (proj.w <= 0.001) break;
        vec3 ndc = proj.xyz / proj.w;
        vec2 sampleUV = ndc.xy * 0.5 + 0.5;

        if (sampleUV.x < 0.005 || sampleUV.x > 0.995 || sampleUV.y < 0.005 || sampleUV.y > 0.995) {
            break;
        }

        float sampleDepth = texture(u_Depth, sampleUV).r;
        if (sampleDepth >= 1.0) continue;

        float sceneLinZ = linearDepth(sampleDepth);
        float rayLinZ = -rayPos.z;

        float diff = rayLinZ - sceneLinZ;
        if (diff > 0.005 && diff < thickness) {
            float rayFalloff = 1.0 - (float(i) / float(steps));
            vec2 edge = smoothstep(0.0, 0.06, sampleUV) * (1.0 - smoothstep(0.94, 1.0, sampleUV));
            float vignette = edge.x * edge.y;

            shadow = max(shadow, rayFalloff * vignette);
            if (shadow > 0.90) break;
        }
    }

    float finalShadow = clamp(shadow * u_Intensity * distFade * angleFade, 0.0, 0.95);
    FragColor = vec4(sceneColor.rgb * (1.0 - finalShadow), sceneColor.a);
}
