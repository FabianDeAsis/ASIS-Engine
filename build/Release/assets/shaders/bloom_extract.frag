#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_Scene;
uniform float u_Threshold;

void main() {
    vec3 color = texture(u_Scene, TexCoords).rgb;
    // Luminancia perceptual
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float thresh = max(u_Threshold, 0.05);

    // Curva de rodilla suave (soft knee) para extracción de resplandor natural
    if (luma > thresh) {
        float factor = clamp((luma - thresh) / max(1.0 - thresh, 0.1), 0.0, 1.0);
        FragColor = vec4(color * factor, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
