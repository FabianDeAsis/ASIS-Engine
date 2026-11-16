#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D u_DepthTexture;
uniform sampler2D u_NoiseTexture;

const int KERNEL_SIZE = 32;
uniform vec3 u_Samples[32];

uniform mat4 u_Projection;
uniform mat4 u_InvProjection;
uniform vec2 u_NoiseScale;
uniform float u_Radius;
uniform float u_Bias;
uniform float u_Power;

vec3 getViewPos(vec2 uv) {
    float depth = texture(u_DepthTexture, uv).r;
    if (depth >= 0.99999) {
        return vec3(0.0, 0.0, -10000.0);
    }
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = u_InvProjection * clip;
    return viewPos.xyz / viewPos.w;
}

vec3 reconstructNormal(vec3 fragPos) {
    vec3 dX = dFdx(fragPos);
    vec3 dY = dFdy(fragPos);
    vec3 normalCross = cross(dX, dY);
    float len2 = dot(normalCross, normalCross);
    if (len2 < 1e-7) {
        return vec3(0.0, 0.0, 1.0);
    }
    vec3 n = normalCross * inversesqrt(len2);
    // Garantizar que la normal apunte hacia la cámara (-fragPos en espacio de vista)
    if (dot(n, -fragPos) < 0.0) {
        n = -n;
    }
    return n;
}

void main() {
    float depth = texture(u_DepthTexture, TexCoords).r;
    if (depth >= 0.99999) {
        FragColor = 1.0;
        return;
    }

    vec3 fragPos = getViewPos(TexCoords);
    if (fragPos.z < -9000.0) {
        FragColor = 1.0;
        return;
    }

    vec3 normal = reconstructNormal(fragPos);
    float fragDist = -fragPos.z;

    // Vector de rotación pseudo-aleatoria por texel para eliminar bandas
    vec2 noise = texture(u_NoiseTexture, TexCoords * u_NoiseScale).xy * 2.0 - 1.0;
    float randomAngle = atan(noise.y, noise.x);

    // Escala focal en Y de la matriz de proyección (f = 1 / tan(fov/2))
    float focalScale = u_Projection[1][1] * 0.5;
    // Limitar el radio de búsqueda en pantalla para evitar falsas oclusiones gigantescas a corta distancia
    float screenRadius = clamp((u_Radius * focalScale) / max(fragDist, 0.15), 0.003, 0.18);

    // Relación de aspecto de píxeles en pantalla (alto / ancho)
    float aspect = u_NoiseScale.y / u_NoiseScale.x;

    const int NUM_SAMPLES = 24;
    const float GOLDEN_ANGLE = 2.39996323; // Ángulo áureo (distribución de Fermat)

    float occlusion = 0.0;
    float totalWeight = 0.0;

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float alpha = (float(i) + 0.5) / float(NUM_SAMPLES);
        float angle = float(i) * GOLDEN_ANGLE + randomAngle;

        // Distribución cuadrática (mayor densidad cerca del punto de contacto)
        float r = alpha * alpha;
        vec2 sampleOffset = vec2(cos(angle) * aspect, sin(angle)) * (r * screenRadius);
        vec2 sampleUV = TexCoords + sampleOffset;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            continue;
        }

        vec3 geomPos = getViewPos(sampleUV);
        if (geomPos.z < -9000.0) continue; // fondo / cielo

        vec3 v = geomPos - fragPos;
        float d = length(v);

        // Omitir si la distancia supera el radio de influencia o es el mismo punto
        if (d < 0.002 || d > u_Radius) continue;

        // ELEVACIÓN SOBRE EL PLANO TANGENTE: dot(normal, v)
        // En cualquier superficie plana o terreno suave, v yace en el plano tangente (dot <= 0),
        // por lo que NUNCA produce auto-oclusión ni terreno negro.
        // Solo contribuye a oclusión si la geometría vecina se eleva por encima del plano tangente.
        float elevation = dot(normal, v);

        // Umbral de sesgo proporcional a la distancia del vecino
        float biasThreshold = max(u_Bias * d, 0.004);

        if (elevation > biasThreshold) {
            // Factor de caída suave cuadrática inversa
            float falloff = max(0.0, 1.0 - (d / u_Radius));
            falloff = falloff * falloff;

            // Oclusión angular proporcional
            float sampleOcc = ((elevation - biasThreshold) / (d + 0.02)) * falloff;
            occlusion += sampleOcc;
        }

        totalWeight += 1.0;
    }

    if (totalWeight > 0.0) {
        occlusion = occlusion / totalWeight;
    }

    // Curva de oclusión limpia y modulable
    float rawAO = clamp(1.0 - (occlusion * 2.8), 0.0, 1.0);
    float finalAO = pow(rawAO, u_Power);

    if (isnan(finalAO) || isinf(finalAO)) {
        finalAO = 1.0;
    }

    FragColor = finalAO;
}
