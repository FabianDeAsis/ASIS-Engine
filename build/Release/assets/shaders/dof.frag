#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_ColorTexture;
uniform sampler2D u_DepthTexture;

uniform float u_FocusDistance;
uniform float u_FocusRange;
uniform float u_BokehRadius;
uniform float u_Near;
uniform float u_Far;
uniform vec2 u_TexelSize;

// Muestreo concéntrico de apertura circular estilo diafragma DSLR (32 muestras en 3 anillos)
const int NUM_SAMPLES = 32;
const vec2 BOKEH_KERNEL[32] = vec2[](
    // Anillo interior (4 muestras, r = 0.28)
    vec2( 0.198,  0.198), vec2(-0.198,  0.198),
    vec2(-0.198, -0.198), vec2( 0.198, -0.198),
    
    // Anillo medio (10 muestras, r = 0.62)
    vec2( 0.620,  0.000), vec2( 0.502,  0.364),
    vec2( 0.192,  0.589), vec2(-0.192,  0.589),
    vec2(-0.502,  0.364), vec2(-0.620,  0.000),
    vec2(-0.502, -0.364), vec2(-0.192, -0.589),
    vec2( 0.192, -0.589), vec2( 0.502, -0.364),
    
    // Anillo exterior perimetral (18 muestras, r = 0.98) - Produce el borde nítido del diafragma
    vec2( 0.980,  0.000), vec2( 0.921,  0.335),
    vec2( 0.751,  0.630), vec2( 0.490,  0.849),
    vec2( 0.170,  0.965), vec2(-0.170,  0.965),
    vec2(-0.490,  0.849), vec2(-0.751,  0.630),
    vec2(-0.921,  0.335), vec2(-0.980,  0.000),
    vec2(-0.921, -0.335), vec2(-0.751, -0.630),
    vec2(-0.490, -0.849), vec2(-0.170, -0.965),
    vec2( 0.170, -0.965), vec2( 0.490, -0.849),
    vec2( 0.751, -0.630), vec2( 0.921, -0.335)
);

void main() {
    vec4 baseColor = texture(u_ColorTexture, TexCoords);
    float depth = texture(u_DepthTexture, TexCoords).r;

    // Linearizar profundidad
    float z_ndc = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * u_Near * u_Far) / (u_Far + u_Near - z_ndc * (u_Far - u_Near));

    // Si es el fondo lejano infinito, tratar como desenfocado al máximo
    if (depth >= 0.99999) {
        linearDepth = u_Far;
    }

    // Círculo de Confusión (CoC) normalizado [0, 1]
    float coc = clamp(abs(linearDepth - u_FocusDistance) / max(u_FocusRange, 0.001), 0.0, 1.0);

    if (coc <= 0.005 || u_BokehRadius <= 0.01) {
        FragColor = baseColor;
        return;
    }

    // Generador de rotacion pseudoaleatoria por pixel (Interleaved Gradient Noise / Hash)
    // Distribuye el patron circular de 32 muestras uniformemente entre pixeles adyacentes,
    // transformando las copias fantasma en un desenfoque bokeh continuo, cremoso y cinematografico DSLR
    float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    float angle = noise * 6.28318530718;
    float cosA = cos(angle);
    float sinA = sin(angle);
    mat2 rot = mat2(cosA, -sinA, sinA, cosA);

    vec2 blurRadius = coc * u_BokehRadius * u_TexelSize * 2.5;
    vec3 bokehSum = baseColor.rgb;
    float totalWeight = 1.0;

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        vec2 rotatedOffset = rot * BOKEH_KERNEL[i];
        vec2 sampleUV = TexCoords + rotatedOffset * blurRadius;
        vec3 sampleColor = texture(u_ColorTexture, sampleUV).rgb;

        // Ponderación de aberración esférica de lente Reflex (anillo exterior más brillante)
        float ringWeight = (i >= 14) ? 1.55 : ((i >= 4) ? 1.15 : 1.0);

        // Potenciador exponencial de altas luces ópticas (crea los orbes / discos luminosos característicos de cámaras reflex)
        float luma = dot(sampleColor, vec3(0.2126, 0.7152, 0.0722));
        float highlightBoost = 1.0 + pow(max(0.0, luma), 2.5) * 3.5;

        float weight = ringWeight * highlightBoost;
        bokehSum += sampleColor * weight;
        totalWeight += weight;
    }

    vec3 blurred = bokehSum / totalWeight;
    FragColor = vec4(mix(baseColor.rgb, blurred, coc), baseColor.a);
}
