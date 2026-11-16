#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_Scene;
uniform sampler2D u_Depth;
uniform float u_Time;
uniform float u_WaterLevel;
uniform vec3 u_CamPos;
uniform mat4 u_InvProjection;
uniform mat4 u_InvView;
uniform float u_Near;
uniform float u_Far;

float linearDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * u_Near * u_Far) / (u_Far + u_Near - z * (u_Far - u_Near));
}

void main() {
    // 1. Distorsión óptica de refracción subacuática (wobble fluido)
    vec2 wobble = vec2(
        sin(TexCoords.y * 24.0 + u_Time * 2.2) * 0.0025,
        cos(TexCoords.x * 20.0 + u_Time * 1.8) * 0.0025
    );
    vec4 sceneColor = texture(u_Scene, TexCoords + wobble);
    float depth = texture(u_Depth, TexCoords).r;

    float linDepth = linearDepth(depth);

    // 2. Reconstruir posición en el mundo
    vec4 clipPos = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = u_InvProjection * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = u_InvView * viewPos;

    // 3. Absorción óptica de Beer-Lambert bajo el agua (filtra rojos y amarillos, retiene turquesa/marino)
    vec3 deepWaterColor = vec3(0.012, 0.14, 0.26);
    float extinction = 1.0 - exp(-linDepth * 0.040);
    vec3 color = mix(sceneColor.rgb, deepWaterColor, clamp(extinction, 0.0, 0.94));

    // Tinte verdoso/azulado general de agua marina
    color *= vec3(0.75, 0.95, 1.05);

    // 4. Cáusticas de luz solar animadas en superficies sumergidas
    if (depth < 0.999 && worldPos.y < u_WaterLevel) {
        vec2 cUV = worldPos.xz * 0.35;
        float c1 = sin(cUV.x * 2.5 + u_Time * 1.6) * cos(cUV.y * 2.5 + u_Time * 1.3);
        float c2 = cos(cUV.x * 4.2 - u_Time * 1.9) * sin(cUV.y * 4.2 - u_Time * 1.5);
        float caustics = max(0.0, c1 + c2) * 0.40 * exp(-linDepth * 0.06);
        color += vec3(0.20, 0.50, 0.65) * caustics;
    }

    FragColor = vec4(color, sceneColor.a);
}
