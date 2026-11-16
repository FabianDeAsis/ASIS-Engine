#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main() {
    WorldPos = aPos;
    vec4 pos = u_Projection * u_View * vec4(aPos, 1.0);
    // z = w garantiza que el valor de profundidad tras la división de perspectiva sea siempre 1.0 (fondo)
    gl_Position = pos.xyww;
}
