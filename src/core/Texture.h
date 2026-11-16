#pragma once

#include <string>
#include <memory>
#include <glad/glad.h>

namespace ASIS {

class Texture {
public:
    // Carga una textura desde un archivo en disco (.png, .jpg, etc.)
    Texture(const std::string& path, const std::string& type = "texture_diffuse");

    // Constructor para texturas procedurales de 1x1 píxel (por ejemplo textura blanca sólida por defecto)
    Texture(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Enlazar a una unidad de textura de OpenGL (0, 1, 2...)
    void bind(unsigned int slot = 0) const;
    void unbind() const;

    GLuint getID() const { return m_textureID; }
    const std::string& getType() const { return m_type; }
    const std::string& getPath() const { return m_path; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    // Textura blanca estática por defecto (para modelos que no traen textura)
    static std::shared_ptr<Texture> getDefaultWhite();
    // Textura de normal plana estática por defecto (128, 128, 255)
    static std::shared_ptr<Texture> getDefaultNormal();

    // Control Global de Filtrado Anisotrópico (AF)
    static void setGlobalAnisotropy(int level);
    static int getGlobalAnisotropy();

private:
    GLuint m_textureID;
    std::string m_type;
    std::string m_path;
    int m_width;
    int m_height;
    int m_nrChannels;

    void loadFromFile(const std::string& path);
};

} // namespace ASIS
