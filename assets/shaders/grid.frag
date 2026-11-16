#version 330 core

in vec3 nearPoint;
in vec3 farPoint;

out vec4 FragColor;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_CameraPos;

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = u_Projection * u_View * vec4(pos, 1.0);
    return (clip_space_pos.z / clip_space_pos.w) * 0.5 + 0.5;
}

void main() {
    float dy = farPoint.y - nearPoint.y;
    if (abs(dy) < 0.0001) {
        discard;
    }

    float t = -nearPoint.y / dy;
    if (t <= 0.0) {
        discard;
    }

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    // Escribir profundidad verdadera en el Z-Buffer
    gl_FragDepth = computeDepth(fragPos3D);

    // Atenuación suave hacia el horizonte basada en distancia
    float dist = length(fragPos3D.xz - u_CameraPos.xz);
    float fade = clamp(1.0 - dist / 320.0, 0.0, 1.0);
    fade = fade * fade; // Caída cuadrática suave

    if (fade <= 0.001) {
        discard;
    }

    // Cuadrícula analítica anti-aliasing con derivadas fwidth
    vec2 coord = fragPos3D.xz;
    vec2 deriv = fwidth(coord);

    // Cuadrícula menor (1 metro)
    vec2 g1 = abs(fract(coord - 0.5) - 0.5) / max(deriv, vec2(0.0001));
    float line1 = min(g1.x, g1.y);
    float a1 = 1.0 - min(line1, 1.0);

    // Cuadrícula mayor (10 metros)
    vec2 g10 = abs(fract(coord * 0.1 - 0.5) - 0.5) / max(deriv * 0.1, vec2(0.0001));
    float line10 = min(g10.x, g10.y);
    float a10 = 1.0 - min(line10, 1.0);

    // Ejes cartesianos coloreados (Estilo Blender 4: Rojo = Eje X, Azul = Eje Z)
    bool isZAxis = abs(fragPos3D.x) < max(deriv.x * 2.0, 0.04);
    bool isXAxis = abs(fragPos3D.z) < max(deriv.y * 2.0, 0.04);

    vec4 color = vec4(0.0);
    if (isZAxis) {
        // Eje Z (Azul vibrante Blender)
        color = vec4(0.20, 0.58, 1.0, 0.95);
    } else if (isXAxis) {
        // Eje X (Rojo vibrante Blender)
        color = vec4(1.0, 0.28, 0.28, 0.95);
    } else {
        // Líneas mayores (10m) y menores (1m) nítidas y contrastadas
        vec3 majorCol = vec3(0.55, 0.60, 0.70);
        vec3 minorCol = vec3(0.32, 0.36, 0.44);
        
        float alpha = max(a1 * 0.45, a10 * 0.85);
        vec3 rgb = (a10 > a1) ? majorCol : minorCol;
        color = vec4(rgb, alpha);
    }

    color.a *= fade;
    if (color.a < 0.01) {
        discard;
    }

    FragColor = color;
}
