#version 330 core
out vec4 FragColor;

in vec2 v_TexCoords;
in vec4 v_Color;

void main() {
    // Generar partícula suave circular y radiante con desvanecimiento gaussiano
    float dist = length(v_TexCoords - vec2(0.5));
    if (dist > 0.5) discard;

    // Resplandor central suave
    float alpha = smoothstep(0.5, 0.05, dist) * v_Color.a;
    // Core brillante que acentúa el efecto con Bloom
    vec3 glow = v_Color.rgb * (1.0 + smoothstep(0.25, 0.0, dist) * 2.0);

    FragColor = vec4(glow, alpha);
}
