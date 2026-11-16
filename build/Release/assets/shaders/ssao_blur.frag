#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D u_SSAOInput;
uniform vec2 u_TexelSize;

void main() {
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * u_TexelSize;
            result += texture(u_SSAOInput, TexCoords + offset).r;
        }
    }
    FragColor = result / 16.0;
}
