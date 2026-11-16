#include "Texture.h"
#include <iostream>
#include <vector>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace ASIS {

static int s_globalAnisotropy = 16;

void Texture::setGlobalAnisotropy(int level) {
    s_globalAnisotropy = level;
}

int Texture::getGlobalAnisotropy() {
    return s_globalAnisotropy;
}

std::shared_ptr<Texture> Texture::getDefaultWhite() {
    static std::shared_ptr<Texture> defaultWhite = std::make_shared<Texture>(255, 255, 255, 255);
    return defaultWhite;
}

std::shared_ptr<Texture> Texture::getDefaultNormal() {
    static std::shared_ptr<Texture> defaultNormal = std::make_shared<Texture>(128, 128, 255, 255);
    return defaultNormal;
}

Texture::Texture(const std::string& path, const std::string& type)
    : m_textureID(0), m_type(type), m_path(path), m_width(0), m_height(0), m_nrChannels(0)
{
    loadFromFile(path);
}

Texture::Texture(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    : m_textureID(0), m_type("texture_diffuse"), m_path("internal:solid_color"), m_width(1), m_height(1), m_nrChannels(4)
{
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    unsigned char data[4] = { r, g, b, a };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_textureID(other.m_textureID), m_type(std::move(other.m_type)),
      m_path(std::move(other.m_path)), m_width(other.m_width),
      m_height(other.m_height), m_nrChannels(other.m_nrChannels)
{
    other.m_textureID = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_textureID != 0) glDeleteTextures(1, &m_textureID);

        m_textureID = other.m_textureID;
        m_type = std::move(other.m_type);
        m_path = std::move(other.m_path);
        m_width = other.m_width;
        m_height = other.m_height;
        m_nrChannels = other.m_nrChannels;

        other.m_textureID = 0;
    }
    return *this;
}

void Texture::loadFromFile(const std::string& path) {
    glGenTextures(1, &m_textureID);

    // Invertir imágenes en el eje Y porque OpenGL espera que el origen (0,0) esté abajo a la izquierda
    stbi_set_flip_vertically_on_load(true);

    // Rutas candidatas
    std::vector<std::string> candidates = {
        path,
        "assets/" + path,
        "../" + path,
        "../assets/" + path,
        "../../" + path,
        "../../assets/" + path,
        "motor_grafico/assets/" + path,
        "motor_grafico/build/Release/assets/" + path
    };

    unsigned char* data = nullptr;
    std::string resolvedPath = path;

    for (const auto& candidate : candidates) {
        data = stbi_load(candidate.c_str(), &m_width, &m_height, &m_nrChannels, 0);
        if (data) {
            resolvedPath = candidate;
            break;
        }
    }

    if (data) {
        GLenum format = GL_RGB;
        if (m_nrChannels == 1) format = GL_RED;
        else if (m_nrChannels == 3) format = GL_RGB;
        else if (m_nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, m_textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Parámetros de repetición y filtrado trilineal para evitar pixelado en la distancia
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Filtrado Anisotrópico (AF) para nitidez a ángulos oblicuos/rasantes
        GLfloat maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::clamp(static_cast<float>(s_globalAnisotropy), 1.0f, maxAniso));

        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);

        std::cout << "[ASIS Engine] Textura cargada: " << resolvedPath << " (" << m_width << "x" << m_height << ", canales: " << m_nrChannels << ")\n";
    } else {
        std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo cargar la textura: " << path << ". Usando textura blanca por defecto.\n";
        stbi_image_free(data);

        // Fallback: textura blanca 1x1
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        unsigned char defaultPixel[4] = { 240, 240, 240, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultPixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace ASIS
