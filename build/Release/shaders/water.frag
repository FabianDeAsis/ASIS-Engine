#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 ClipSpacePos;
in float WaveCrest;
in vec3 ViewDir;

out vec4 FragColor;

// Uniforms de Cámara y Profundidad
uniform vec3 u_ViewPos;
uniform float u_NearPlane;
uniform float u_FarPlane;
uniform mat4 u_View;
uniform mat4 u_Projection;

// Screen-Space Reflections (SSR - Fase 20)
uniform int u_SSREnabled;
uniform int u_SSRMaxSteps;
uniform float u_SSRStepSize;
uniform float u_SSRThickness;
uniform float u_SSRIntensity;

// Texturas de Escena (Captura de Opacos)
uniform sampler2D u_OpaqueColorTexture;
uniform sampler2D u_OpaqueDepthTexture;
uniform sampler2D u_WaterNormalMap;
uniform sampler2D u_FoamNoiseMap;
uniform sampler2D u_EnvMap;
uniform int u_HasEnvMap;

// Parámetros Ópticos y PBR
uniform vec3 u_ShallowColor;
uniform vec3 u_DeepColor;
uniform float u_Clarity;
uniform vec3 u_FoamColor;
uniform float u_FoamThreshold;
uniform float u_FoamIntensity;
uniform float u_RefractionStrength;
uniform float u_Roughness;
uniform float u_Specular;

// Iluminación
uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform vec3 u_AmbientColor;
uniform float u_Time;

// Neblina Atmosférica Global (Fase 20)
uniform int u_GlobalFogEnabled;
uniform float u_GlobalFogStart;
uniform float u_GlobalFogEnd;
uniform float u_GlobalFogDensity;
uniform vec3 u_GlobalFogColor;

void main() {
    // 1. Coordenadas de Pantalla Normalizadas [0, 1]
    vec2 screenUV = (ClipSpacePos.xy / ClipSpacePos.w) * 0.5 + 0.5;

    // 2. Reconstrucción de Profundidad Lineal para Detección de Orillas y Absorción
    float depthSample = texture(u_OpaqueDepthTexture, screenUV).r;
    float z_ndc = depthSample * 2.0 - 1.0;
    float sceneLinearDepth = (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - z_ndc * (u_FarPlane - u_NearPlane));

    float surface_ndc = (gl_FragCoord.z) * 2.0 - 1.0;
    float surfaceLinearDepth = (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - surface_ndc * (u_FarPlane - u_NearPlane));

    float waterDepth = max(0.0, sceneLinearDepth - surfaceLinearDepth);

    // 3. Normales de Micro-superficie (Viento y ondulaciones en flujo bidireccional)
    vec2 flowUV1 = FragPos.xz * 0.08 + vec2(u_Time * 0.035, u_Time * 0.02);
    vec2 flowUV2 = FragPos.xz * 0.15 - vec2(u_Time * 0.025, u_Time * 0.04);
    vec3 n1 = texture(u_WaterNormalMap, flowUV1).rgb * 2.0 - 1.0;
    vec3 n2 = texture(u_WaterNormalMap, flowUV2).rgb * 2.0 - 1.0;
    vec3 microNormal = normalize(n1 + n2);

    // Combinar la normal de Gerstner con el micro-relieve
    vec3 N = normalize(Normal + microNormal * 0.35);
    bool isUnderwater = (u_ViewPos.y < FragPos.y);
    if (isUnderwater) {
        N = -N; // Invertir la normal cuando el observador está sumergido bajo el agua
    }

    // 4. Refracción Subacuática (Distorsión óptica del fondo marino y rocas sumergidas)
    vec2 refractOffset = N.xz * u_RefractionStrength * clamp(waterDepth * 0.4, 0.0, 1.0);
    vec2 refractUV = clamp(screenUV + refractOffset, vec2(0.002), vec2(0.998));

    // Descartar refracciones si el objeto muestreado está en frente del agua
    float checkDepth = texture(u_OpaqueDepthTexture, refractUV).r;
    float checkLinear = (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - (checkDepth * 2.0 - 1.0) * (u_FarPlane - u_NearPlane));
    if (checkLinear < surfaceLinearDepth) {
        refractUV = screenUV;
    }
    vec3 groundColor = texture(u_OpaqueColorTexture, refractUV).rgb;

    // 5. Absorción de Luz (Ley de Beer-Lambert)
    // El agua cristalina deja ver el fondo en poca profundidad y adquiere tono azul marino en lo profundo
    float extinction = exp(-waterDepth * u_Clarity);
    vec3 waterTint = mix(u_DeepColor, u_ShallowColor, extinction);
    vec3 waterBody = mix(waterTint, groundColor * waterTint * 2.2, extinction * 0.7);

    // 6. Espuma de Contacto con la Orilla (Shoreline Foam Orgánica sin Rejilla)
    vec2 foamUV1 = FragPos.xz * 0.12 + vec2(sin(u_Time * 0.4), cos(u_Time * 0.3)) * 0.04;
    vec2 foamUV2 = FragPos.xz * 0.27 - vec2(cos(u_Time * 0.35), sin(u_Time * 0.45)) * 0.03;
    float foamNoise = (texture(u_FoamNoiseMap, foamUV1).r + texture(u_FoamNoiseMap, foamUV2).r) * 0.5;
    float shoreFactor = clamp(1.0 - (waterDepth / max(u_FoamThreshold, 0.01)), 0.0, 1.0);
    float shorelineFoam = smoothstep(0.18, 0.65, shoreFactor * (foamNoise * 1.5 + 0.1)) * u_FoamIntensity;

    // 7. Espuma en Crestas de Olas (Wave Crest Foam)
    float crestFoam = smoothstep(2.9, 3.8, WaveCrest) * texture(u_FoamNoiseMap, FragPos.xz * 0.35 + u_Time * 0.08).r;
    float totalFoam = clamp(shorelineFoam + crestFoam, 0.0, 1.0);

    // 8. Reflejo de Fresnel y Luz Ambiental del Cielo (IBL)
    vec3 V = normalize(u_ViewPos - FragPos);
    float NdotV = max(dot(N, V), 0.0);
    float F0 = isUnderwater ? 0.35 : 0.02; // Coeficiente de reflectancia base del agua (Schlick)
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    vec3 R = reflect(-V, N);
    vec3 skyReflect = vec3(0.25, 0.48, 0.75); // Fallback atmosférico
    if (u_HasEnvMap == 1) {
        vec2 envUV = vec2(atan(R.z, R.x) * 0.1591549 + 0.5, asin(clamp(R.y, -1.0, 1.0)) * 0.3183098 + 0.5);
        skyReflect = texture(u_EnvMap, envUV).rgb;
    }

    // 8.5. Screen-Space Reflections (SSR - Fase 20)
    vec3 reflectionColor = skyReflect;
    if (u_SSREnabled == 1) {
        vec3 rayOrigin = FragPos;
        vec3 rayDir = normalize(R);

        float stepSize = max(u_SSRStepSize, 0.05);
        int maxSteps = clamp(u_SSRMaxSteps, 16, 96);
        float thickness = max(u_SSRThickness, 0.02);

        vec3 curPos = rayOrigin + rayDir * 0.15;
        float hitFactor = 0.0;
        vec2 hitUV = vec2(0.0);

        for (int step = 0; step < maxSteps; ++step) {
            curPos += rayDir * stepSize;

            vec4 clipPos = u_Projection * u_View * vec4(curPos, 1.0);
            if (clipPos.w <= 0.001) break;
            vec3 ndc = clipPos.xyz / clipPos.w;
            vec2 sampleUV = ndc.xy * 0.5 + 0.5;

            if (sampleUV.x < 0.01 || sampleUV.x > 0.99 || sampleUV.y < 0.01 || sampleUV.y > 0.99) break;

            float sceneDepthSample = texture(u_OpaqueDepthTexture, sampleUV).r;
            float z_sample_ndc = sceneDepthSample * 2.0 - 1.0;
            float sampledLinearDepth = (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - z_sample_ndc * (u_FarPlane - u_NearPlane));

            float rayLinearDepth = (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - ndc.z * (u_FarPlane - u_NearPlane));

            float depthDiff = rayLinearDepth - sampledLinearDepth;
            if (depthDiff > 0.0 && depthDiff < thickness * (1.0 + rayLinearDepth * 0.08)) {
                hitUV = sampleUV;
                vec2 edgeDist = min(sampleUV, vec2(1.0) - sampleUV);
                float edgeFade = clamp(min(edgeDist.x, edgeDist.y) * 10.0, 0.0, 1.0);
                hitFactor = edgeFade * u_SSRIntensity;
                break;
            }
        }

        if (hitFactor > 0.001) {
            vec3 ssrSample = texture(u_OpaqueColorTexture, hitUV).rgb;
            reflectionColor = mix(skyReflect, ssrSample, hitFactor);
        }
    }

    // 9. Brillo Especular Solar Directo (Sun Glints PBR GGX)
    vec3 L = normalize(-u_SunDir);
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float rough = max(u_Roughness, 0.02);
    float specPower = 2.0 / (rough * rough) - 2.0;
    float sunSpec = pow(NdotH, clamp(specPower, 10.0, 4096.0)) * u_Specular * 4.0;
    vec3 directSunGlint = u_SunColor * sunSpec;

    // 10. Sub-surface Scattering y Resplandor Translúcido a Contraluz
    float sunSubsurface = max(dot(V, -L), 0.0);
    vec3 subsurfaceGlow = u_ShallowColor * pow(sunSubsurface, 4.0) * 0.45 * (1.0 - extinction);

    // 11. Composición Final
    vec3 surfaceColor = mix(waterBody, reflectionColor, fresnel);
    surfaceColor += directSunGlint + subsurfaceGlow;
    surfaceColor = mix(surfaceColor, u_FoamColor, totalFoam);

    // Neblina Atmosférica Global (Fase 20)
    if (u_GlobalFogEnabled == 1) {
        float dist = length(u_ViewPos - FragPos);
        float fogFactor = clamp((dist - u_GlobalFogStart) / max(u_GlobalFogEnd - u_GlobalFogStart, 0.1), 0.0, 1.0);
        fogFactor = pow(fogFactor, 1.5) * u_GlobalFogDensity;
        vec3 L_sun = normalize(-u_SunDir);
        float sunScatter = pow(max(dot(-V, L_sun), 0.0), 8.0) * 0.40;
        vec3 fogColor = u_GlobalFogColor + u_SunColor * sunScatter;
        surfaceColor = mix(surfaceColor, fogColor, clamp(fogFactor, 0.0, 0.95));
    }

    // Alpha suave para desvanecer la orilla limpiamente
    float alpha = clamp(waterDepth * 4.0, 0.35, 1.0);
    FragColor = vec4(surfaceColor, alpha);
}
