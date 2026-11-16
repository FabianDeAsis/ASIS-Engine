#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "Shader.h"
#include "Camera.h"

namespace ASIS {

class DayNightCycle;

class Skybox {
public:
    Skybox();
    ~Skybox();

    // Inicializar geometría de cubo y shaders de Skybox
    bool init();

    // Cargar mapa equirrectangular HDRI (.hdr) o imagen 360° (.png / .jpg)
    bool loadHDRI(const std::string& filePath);

    // Generar mapa HDR procedural de cielo atmosférico con sol y horizonte
    void generateProceduralSky();

    // Renderizar el fondo de cielo en el Viewport (con soporte de inversión Flip X y Flip Y)
    void draw(const Camera& camera, float aspectRatio, bool showSky, float exposure, float rotationDeg, float blurLOD, bool flipX = false, bool flipY = false, const DayNightCycle* dayNight = nullptr);

    // Vincular la textura HDR de entorno para que el shader PBR de modelos calcule IBL
    void bindEnvironmentMap(GLuint textureUnit = 7) const;

    GLuint getEnvironmentTexture() const { return m_envTexture; }
    bool hasCustomHDRI() const { return m_hasCustomHDRI; }
    const std::string& getHDRIPath() const { return m_hdriPath; }

    void destroy();

private:
    GLuint m_cubeVAO;
    GLuint m_cubeVBO;
    GLuint m_envTexture;
    std::unique_ptr<Shader> m_skyboxShader;

    bool m_initialized;
    bool m_hasCustomHDRI;
    std::string m_hdriPath;

    void setupCube();
};

} // namespace ASIS
