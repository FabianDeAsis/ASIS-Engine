#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom;
uniform float u_Intensity;

void main() {
    vec3 sceneColor = texture(u_Scene, TexCoords).rgb;
    vec3 bloomColor = texture(u_Bloom, TexCoords).rgb;

    // Composición aditiva de Bloom HDR
    vec3 result = sceneColor + bloomColor * u_Intensity;

    FragColor = vec4(result, 1.0);
}
