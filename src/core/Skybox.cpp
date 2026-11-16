#include "Skybox.h"
#include "DayNightCycle.h"
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#define TINYEXR_USE_MINIZ 0
#include <zlib.h>
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

namespace ASIS {

Skybox::Skybox()
    : m_cubeVAO(0), m_cubeVBO(0), m_envTexture(0),
      m_initialized(false), m_hasCustomHDRI(false), m_hdriPath("")
{
}

Skybox::~Skybox() {
    destroy();
}

void Skybox::setupCube() {
    // Vértices de un cubo unitario centrado en el origen
    static const float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);
    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

bool Skybox::init() {
    if (m_initialized) return true;

    setupCube();

    // Crear shader de Skybox
    try {
        m_skyboxShader = std::make_unique<Shader>("shaders/skybox.vert", "shaders/skybox.frag");
    } catch (const std::exception& e) {
        std::cerr << "[Skybox] Error al compilar shaders de skybox: " << e.what() << std::endl;
        return false;
    }

    // Generar cielo procedural de arranque si no hay mapa externo
    generateProceduralSky();

    m_initialized = true;
    std::cout << "[ASIS Engine] Sistema de Skybox HDRI e IBL inicializado.\n";
    return true;
}

void Skybox::generateProceduralSky() {
    m_hasCustomHDRI = false;
    m_hdriPath = "";
    const int width = 512;
    const int height = 256;
    std::vector<float> skyData(width * height * 3);

    glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f));
    glm::vec3 zenithColor(0.12f, 0.28f, 0.55f);
    glm::vec3 horizonColor(0.70f, 0.75f, 0.82f);
    glm::vec3 groundColor(0.15f, 0.16f, 0.18f);
    glm::vec3 sunColor(1.5f, 1.3f, 1.0f);

    for (int y = 0; y < height; ++y) {
        float v = 1.0f - static_cast<float>(y) / static_cast<float>(height);
        float phi = (v - 0.5f) * 3.14159265f; // [-PI/2 .. PI/2]
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(width);
            float theta = (u - 0.5f) * 2.0f * 3.14159265f;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            // Dirección esférica en espacio de mundo
            glm::vec3 dir(cosPhi * sinTheta, sinPhi, cosPhi * cosTheta);

            glm::vec3 col;
            if (dir.y >= 0.0f) {
                // Cielo: degradado suave de horizonte a cenit
                float factor = std::pow(dir.y, 0.65f);
                col = glm::mix(horizonColor, zenithColor, factor);

                // Disco solar y resplandor atmosférico de Mie
                float sunDot = std::max(0.0f, glm::dot(dir, sunDir));
                col += sunColor * std::pow(sunDot, 128.0f) * 4.0f;  // Centro del sol
                col += sunColor * std::pow(sunDot, 16.0f) * 0.8f;   // Halo exterior
            } else {
                // Suelo: degradado hacia la base
                float factor = std::pow(std::abs(dir.y), 0.4f);
                col = glm::mix(horizonColor * 0.5f, groundColor, factor);
            }

            int idx = (y * width + x) * 3;
            skyData[idx + 0] = col.r;
            skyData[idx + 1] = col.g;
            skyData[idx + 2] = col.b;
        }
    }

    if (m_envTexture != 0) {
        glDeleteTextures(1, &m_envTexture);
        m_envTexture = 0;
    }

    glGenTextures(1, &m_envTexture);
    glBindTexture(GL_TEXTURE_2D, m_envTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, skyData.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    m_hasCustomHDRI = false;
    m_hdriPath = "Procedural Studio Sky";
}

bool Skybox::loadHDRI(const std::string& filePath) {
    int width = 0, height = 0, channels = 3;
    stbi_set_flip_vertically_on_load(true);

    float* data = nullptr;
    bool isEXR = (filePath.find(".exr") != std::string::npos || filePath.find(".EXR") != std::string::npos);
    bool isHDR = (filePath.find(".hdr") != std::string::npos || filePath.find(".HDR") != std::string::npos);
    bool isLoadedEXR = false;

    if (isEXR) {
        float* rgba = nullptr;
        const char* err = nullptr;
        int ret = LoadEXR(&rgba, &width, &height, filePath.c_str(), &err);
        if (ret == TINYEXR_SUCCESS && rgba) {
            data = rgba;
            channels = 4;
            isLoadedEXR = true;

            // Invertir verticalmente filas de OpenEXR para coincidir con el estándar de stb_image (V=0 suelo, V=1 cenit)
            int rowSize = width * channels;
            std::vector<float> rowTemp(rowSize);
            for (int y = 0; y < height / 2; ++y) {
                float* rowTop = data + (y * rowSize);
                float* rowBottom = data + ((height - 1 - y) * rowSize);
                std::memcpy(rowTemp.data(), rowTop, rowSize * sizeof(float));
                std::memcpy(rowTop, rowBottom, rowSize * sizeof(float));
                std::memcpy(rowBottom, rowTemp.data(), rowSize * sizeof(float));
            }
            std::cout << "[Skybox] Archivo OpenEXR decodificado e invertido para OpenGL: " << width << "x" << height << "\n";
        } else {
            std::cerr << "[Skybox] Error cargando OpenEXR: " << (err ? err : "desconocido") << "\n";
            if (err) FreeEXRErrorMessage(err);
        }
    } else if (isHDR) {
        data = stbi_loadf(filePath.c_str(), &width, &height, &channels, 0);
    }

    if (!data) {
        // Intentar como imagen estándar LDR (.jpg / .png) y convertir a espacio de color lineal HDR
        unsigned char* udata = stbi_load(filePath.c_str(), &width, &height, &channels, 3);
        if (!udata) {
            std::cerr << "[Skybox] No se pudo abrir la imagen de cielo: " << filePath << std::endl;
            return false;
        }

        std::vector<float> converted(width * height * 3);
        for (int i = 0; i < width * height * 3; ++i) {
            float srgb = udata[i] / 255.0f;
            converted[i] = std::pow(srgb, 2.2f); // Linearizar sRGB a HDR
        }
        stbi_image_free(udata);

        if (m_envTexture != 0) glDeleteTextures(1, &m_envTexture);
        glGenTextures(1, &m_envTexture);
        glBindTexture(GL_TEXTURE_2D, m_envTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, converted.data());
    } else {
        // Sanitizar rigurosamente valores flotantes: eliminar NaNs, Infs y valores negativos
        // Limitar a un máximo de 64000.0f para prevenir desbordamiento de 16-bit half float (+Infinity en GPU)
        int totalElements = width * height * channels;
        int sanitizedCount = 0;
        for (int i = 0; i < totalElements; ++i) {
            float& val = data[i];
            if (std::isnan(val) || std::isinf(val) || val < 0.0f) {
                val = 0.0f;
                sanitizedCount++;
            } else if (val > 64000.0f) {
                val = 64000.0f;
                sanitizedCount++;
            }
        }
        if (sanitizedCount > 0) {
            std::cout << "[Skybox] Sanitizados " << sanitizedCount << " píxeles no finitos / extremos en mapa HDRI.\n";
        }

        GLenum internalFmt = (channels == 4 ? GL_RGBA16F : GL_RGB16F);
        GLenum dataFmt = (channels == 4 ? GL_RGBA : GL_RGB);

        // Protección contra desbordamiento de VRAM para mapas 8K/16K (ej: 16384x8192)
        // Reducir suavemente a un máximo de 4096x2048 para garantizar 60+ FPS y evitar fallos de asignación en GPU
        if (width > 4096 || height > 2048) {
            int targetW = 4096;
            int targetH = (height * targetW) / width;
            if (targetH < 512) targetH = 2048;

            std::cout << "[Skybox] Optimizando mapa HDRI ultra-HD de " << width << "x" << height 
                      << " a " << targetW << "x" << targetH << " para maximo rendimiento GPU...\n";

            std::vector<float> downsampled(targetW * targetH * channels);
            float scaleX = float(width) / float(targetW);
            float scaleY = float(height) / float(targetH);

            for (int y = 0; y < targetH; ++y) {
                int y0 = (std::min)(int(y * scaleY), height - 1);
                for (int x = 0; x < targetW; ++x) {
                    int x0 = (std::min)(int(x * scaleX), width - 1);
                    for (int c = 0; c < channels; ++c) {
                        downsampled[(y * targetW + x) * channels + c] = data[(y0 * width + x0) * channels + c];
                    }
                }
            }

            if (m_envTexture != 0) glDeleteTextures(1, &m_envTexture);
            glGenTextures(1, &m_envTexture);
            glBindTexture(GL_TEXTURE_2D, m_envTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, targetW, targetH, 0, dataFmt, GL_FLOAT, downsampled.data());
            width = targetW;
            height = targetH;
        } else {
            if (m_envTexture != 0) glDeleteTextures(1, &m_envTexture);
            glGenTextures(1, &m_envTexture);
            glBindTexture(GL_TEXTURE_2D, m_envTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, width, height, 0, dataFmt, GL_FLOAT, data);
        }

        if (isLoadedEXR) {
            free(data);
        } else {
            stbi_image_free(data);
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    m_hasCustomHDRI = true;
    m_hdriPath = filePath;
    std::cout << "[Skybox] ✓ HDRI activo: " << filePath << " (" << width << "x" << height << ")\n";
    return true;
}

void Skybox::draw(const Camera& camera, float aspectRatio, bool showSky, float exposure, float rotationDeg, float blurLOD, bool flipX, bool flipY, const DayNightCycle* dayNight) {
    if (!m_initialized || !m_skyboxShader) return;

    // Dibujar al fondo con depth <= 1.0
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    m_skyboxShader->use();
    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix())); // Eliminar traslación para centrar en cámara
    glm::mat4 proj = camera.getProjectionMatrix(aspectRatio);

    m_skyboxShader->setMat4("u_View", view);
    m_skyboxShader->setMat4("u_Projection", proj);
    m_skyboxShader->setInt("u_ShowSkybox", showSky ? 1 : 0);
    m_skyboxShader->setInt("u_HasCustomHDRI", m_hasCustomHDRI ? 1 : 0);
    m_skyboxShader->setFloat("u_SkyboxExposure", exposure);
    m_skyboxShader->setFloat("u_SkyboxRotation", glm::radians(rotationDeg));
    m_skyboxShader->setFloat("u_SkyboxBlur", blurLOD);
    m_skyboxShader->setInt("u_FlipX", flipX ? 1 : 0);
    m_skyboxShader->setInt("u_FlipY", flipY ? 1 : 0);

    // Uniforms de Bóveda Celeste Dinámica 24h y Efecto Horizonte Global (Fase 20)
    if (dayNight) {
        m_skyboxShader->setInt("u_GlobalFogEnabled", dayNight->isFogEnabled() ? 1 : 0);
        m_skyboxShader->setFloat("u_GlobalFogDensity", dayNight->getFogDensity());
        m_skyboxShader->setVec3("u_GlobalFogColor", dayNight->getFogColor());

        m_skyboxShader->setVec3("u_SunDir", dayNight->getSunDirection());
        m_skyboxShader->setVec3("u_MoonDir", dayNight->getMoonDirection());
        m_skyboxShader->setVec3("u_SunColor", dayNight->getSunColor());
        m_skyboxShader->setVec3("u_MoonColor", glm::vec3(0.45f, 0.60f, 0.88f));
        m_skyboxShader->setVec3("u_ZenithColor", dayNight->getZenithColor());
        m_skyboxShader->setVec3("u_HorizonColor", dayNight->getHorizonColor());
        m_skyboxShader->setFloat("u_HorizonHeight", dayNight->getHorizonHeight());
        m_skyboxShader->setFloat("u_StarsIntensity", dayNight->getStarsIntensity());
    } else {
        m_skyboxShader->setInt("u_GlobalFogEnabled", 0);
        m_skyboxShader->setFloat("u_GlobalFogDensity", 0.0f);
        m_skyboxShader->setVec3("u_GlobalFogColor", glm::vec3(0.7f));

        m_skyboxShader->setVec3("u_SunDir", glm::vec3(0.5f, -0.7f, 0.5f));
        m_skyboxShader->setVec3("u_MoonDir", glm::vec3(-0.5f, 0.7f, -0.5f));
        m_skyboxShader->setVec3("u_SunColor", glm::vec3(1.0f, 0.95f, 0.88f));
        m_skyboxShader->setVec3("u_MoonColor", glm::vec3(0.45f, 0.60f, 0.88f));
        m_skyboxShader->setVec3("u_ZenithColor", glm::vec3(0.16f, 0.38f, 0.78f));
        m_skyboxShader->setVec3("u_HorizonColor", glm::vec3(0.68f, 0.78f, 0.88f));
        m_skyboxShader->setFloat("u_HorizonHeight", 0.0f);
        m_skyboxShader->setFloat("u_StarsIntensity", 0.0f);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_envTexture);
    m_skyboxShader->setInt("u_EnvMap", 0);

    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void Skybox::bindEnvironmentMap(GLuint textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_envTexture);
}

void Skybox::destroy() {
    if (m_cubeVAO != 0) {
        glDeleteVertexArrays(1, &m_cubeVAO);
        m_cubeVAO = 0;
    }
    if (m_cubeVBO != 0) {
        glDeleteBuffers(1, &m_cubeVBO);
        m_cubeVBO = 0;
    }
    if (m_envTexture != 0) {
        glDeleteTextures(1, &m_envTexture);
        m_envTexture = 0;
    }
    m_initialized = false;
}

} // namespace ASIS
