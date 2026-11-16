#version 330 core

// Atributos de entrada por cada vértice (location 0: posición 3D, location 1: color RGB)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

// Salida hacia el Fragment Shader (se interpola suavemente entre vértices)
out vec3 ourColor;

// Matrices de transformación (Modelo, Vista, Proyección)
uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    // Calculamos la posición final en el espacio de recorte de la pantalla
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);
    ourColor = aColor;
}
