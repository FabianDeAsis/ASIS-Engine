#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_Scene;

void main() {
    vec3 color = texture(u_Scene, TexCoords).rgb;
    // Luminancia perceptual estándar (Rec. 709 / sRGB)
    float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));
    lum = clamp(lum, 0.0001, 10000.0);
    // Canal R: logaritmo natural para media geométrica (evita distorsión por puntos de luz extremos)
    // Canal G: luminancia directa
    FragColor = vec4(log(lum), lum, 0.0, 1.0);
}
