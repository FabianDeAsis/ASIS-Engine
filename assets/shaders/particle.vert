#version 330 core
layout (location = 0) in vec2 a_QuadPos;
layout (location = 1) in vec2 a_TexCoords;
layout (location = 2) in vec3 a_InstancePos;
layout (location = 3) in float a_InstanceSize;
layout (location = 4) in vec4 a_InstanceColor;
layout (location = 5) in float a_InstanceRotation;

out vec2 v_TexCoords;
out vec4 v_Color;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;

void main() {
    v_TexCoords = a_TexCoords;
    v_Color = a_InstanceColor;

    // Rotación 2D en el plano del billboard
    float c = cos(a_InstanceRotation);
    float s = sin(a_InstanceRotation);
    vec2 rotatedPos = vec2(
        a_QuadPos.x * c - a_QuadPos.y * s,
        a_QuadPos.x * s + a_QuadPos.y * c
    );

    vec3 worldPos = a_InstancePos 
                  + u_CameraRight * (rotatedPos.x * a_InstanceSize)
                  + u_CameraUp    * (rotatedPos.y * a_InstanceSize);

    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);
}
