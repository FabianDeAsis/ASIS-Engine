#version 330 core

// Color recibido e interpolado desde el Vertex Shader
in vec3 ourColor;

// Salida final del color del fragmento/píxel (RGBA)
out vec4 FragColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
