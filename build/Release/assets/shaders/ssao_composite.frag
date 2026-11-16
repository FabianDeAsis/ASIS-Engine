#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_SceneTexture;
uniform sampler2D u_SSAOTexture;
uniform float u_Intensity;

void main() {
    vec3 scene = texture(u_SceneTexture, TexCoords).rgb;
    float ao = texture(u_SSAOTexture, TexCoords).r;
    if (isnan(ao) || isinf(ao)) {
        ao = 1.0;
    }
    
    // Atenuación suave y natural de oclusión ambiental:
    // En superficies abiertas y planas: ao = 1.0 -> factor = 1.0 (cero oscurecimiento)
    // En esquinas, grietas y hendiduras de contacto: sombra controlada por u_Intensity
    float shadow = (1.0 - ao) * u_Intensity;
    float factor = clamp(1.0 - shadow, 0.08, 1.0);
    FragColor = vec4(scene * factor, 1.0);
}
