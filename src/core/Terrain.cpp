#include "Terrain.h"
#include "Shader.h"
#include "PhysicsWorld.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

static bool loadTextureWithWIC(const std::wstring& filename, int& width, int& height, std::vector<unsigned char>& outRGBA) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(filename.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return false;

    UINT w = 0, h = 0;
    frame->GetSize(&w, &h);
    if (w == 0 || h == 0) return false;

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return false;

    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false;

    width = static_cast<int>(w);
    height = static_cast<int>(h);
    outRGBA.resize(width * height * 4);
    UINT stride = width * 4;
    hr = converter->CopyPixels(NULL, stride, static_cast<UINT>(outRGBA.size()), outRGBA.data());
    return SUCCEEDED(hr);
}
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static int s_currentAnisotropyLevel = 16;

static void applyAnisotropyToTexture(GLuint texID, int level) {
    if (texID == 0) return;
    GLfloat maxSupported = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxSupported);
    float target = std::clamp(static_cast<float>(level), 1.0f, maxSupported);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, target);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static GLuint createOpenGLTexture2D(const unsigned char* data, int width, int height, int channels) {
    if (!data || width <= 0 || height <= 0) return 0;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLenum internalFormat = GL_RGBA;
    GLenum format = GL_RGBA;
    if (channels == 1) {
        internalFormat = GL_RED;
        format = GL_RED;
    } else if (channels == 3) {
        internalFormat = GL_RGB;
        format = GL_RGB;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Filtrado Anisotrópico (AF) para máxima nitidez a ángulos rasantes
    GLfloat maxSupported = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxSupported);
    float targetAniso = std::clamp(static_cast<float>(s_currentAnisotropyLevel), 1.0f, maxSupported);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, targetAniso);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static GLuint loadUniversalTexture(const std::string& path) {
    if (path.empty()) return 0;

    std::filesystem::path p(path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Si es formato TIFF (.tif / .tiff), decodificar nativamente con WIC
    if (ext == ".tif" || ext == ".tiff") {
#ifdef _WIN32
        int w = 0, h = 0;
        std::vector<unsigned char> rgba;
        std::wstring wpath = p.wstring();
        if (loadTextureWithWIC(wpath, w, h, rgba)) {
            int stride = w * 4;
            std::vector<unsigned char> row(stride);
            for (int y = 0; y < h / 2; ++y) {
                unsigned char* pTop = rgba.data() + y * stride;
                unsigned char* pBottom = rgba.data() + (h - 1 - y) * stride;
                memcpy(row.data(), pTop, stride);
                memcpy(pTop, pBottom, stride);
                memcpy(pBottom, row.data(), stride);
            }
            std::cout << "[Terreno PBR] Textura TIFF cargada con WIC: " << path << " (" << w << "x" << h << ")\n";
            return createOpenGLTexture2D(rgba.data(), w, h, 4);
        }
#endif
    }

    // Para PNG, JPG, BMP, TGA, etc. usar stb_image
    int w = 0, h = 0, c = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
    if (data) {
        GLuint tex = createOpenGLTexture2D(data, w, h, c);
        stbi_image_free(data);
        std::cout << "[Terreno PBR] Textura cargada con stb_image: " << path << " (" << w << "x" << h << ", ch: " << c << ")\n";
        return tex;
    }

#ifdef _WIN32
    // Fallback con WIC por si stb_image falló o tiene formato extendido
    int w2 = 0, h2 = 0;
    std::vector<unsigned char> rgba;
    std::wstring wpath = p.wstring();
    if (loadTextureWithWIC(wpath, w2, h2, rgba)) {
        int stride = w2 * 4;
        std::vector<unsigned char> row(stride);
        for (int y = 0; y < h2 / 2; ++y) {
            unsigned char* pTop = rgba.data() + y * stride;
            unsigned char* pBottom = rgba.data() + (h2 - 1 - y) * stride;
            memcpy(row.data(), pTop, stride);
            memcpy(pTop, pBottom, stride);
            memcpy(pBottom, row.data(), stride);
        }
        std::cout << "[Terreno PBR] Textura cargada via WIC fallback: " << path << "\n";
        return createOpenGLTexture2D(rgba.data(), w2, h2, 4);
    }
#endif

    std::cerr << "[Terreno PBR ERROR] No se pudo cargar la textura: " << path << "\n";
    return 0;
}

namespace ASIS {

// Funciones auxiliares de ruido hash rápido para generación procedural determinista
static inline float hash12(float x, float y, int seed) {
    float n = std::sin(x * 12.9898f + y * 78.233f + (float)seed * 0.1337f) * 43758.5453f;
    return n - std::floor(n);
}

Terrain::Terrain()
    : m_resolution(128),
      m_worldSize(120.0f),
      m_minHeight(0.0f),
      m_maxHeight(0.0f),
      m_baseOffset(-8.0f),
      m_chunksX(1),
      m_chunksZ(1),
      m_hasCustomSplatmap(false),
      m_visible(true),
      m_isDirty(true),
      m_vao(0),
      m_vbo(0),
      m_ebo(0),
      m_splatmapTex(0),
      m_physicsWorld(nullptr),
      m_terrainShape(nullptr),
      m_terrainBody(nullptr),
      m_motionState(nullptr),
      m_physicsEnabled(true),
      m_reflectionIntensity(1.0f),
      m_anisotropyLevel(16),
      m_reliefMode(TerrainReliefMode::OASIS_TESSELLATION),
      m_tessellationDisplacementScale(1.0f)
{
}

void Terrain::setAnisotropyLevel(int level) {
    if (m_anisotropyLevel == level) return;
    m_anisotropyLevel = level;
    s_currentAnisotropyLevel = level;
    for (auto& l : m_layers) {
        applyAnisotropyToTexture(l.albedoTex, level);
        applyAnisotropyToTexture(l.normalTex, level);
        applyAnisotropyToTexture(l.roughSpecTex, level);
        applyAnisotropyToTexture(l.heightTex, level);
        applyAnisotropyToTexture(l.aoTex, level);
    }
}

Terrain::~Terrain() {
    shutdown();
}

bool Terrain::init(int resolution, float worldSize) {
    m_resolution = (resolution >= 16) ? resolution : 128;
    m_worldSize = (worldSize > 10.0f) ? worldSize : 120.0f;
    m_settings.resolution = m_resolution;
    m_settings.worldSize = m_worldSize;

    m_heights.resize(m_resolution * m_resolution, 0.0f);

    // Inicializar las 4 capas de textura PBR por defecto (Estilo Photoshop / Nodos Blender)
    m_layers.clear();
    m_layers.resize(4);

    m_layers[0].name = "Capa 1: Pasto / Cesped";
    m_layers[0].tiling = 16.0f;
    m_layers[0].roughness = 0.86f;
    m_layers[0].normalStrength = 1.0f;
    m_layers[0].fallbackColor = glm::vec3(0.18f, 0.40f, 0.12f);

    m_layers[1].name = "Capa 2: Tierra / Arena";
    m_layers[1].tiling = 14.0f;
    m_layers[1].roughness = 0.92f;
    m_layers[1].normalStrength = 1.0f;
    m_layers[1].fallbackColor = glm::vec3(0.38f, 0.28f, 0.17f);

    m_layers[2].name = "Capa 3: Roca / Acantilado";
    m_layers[2].tiling = 10.0f;
    m_layers[2].roughness = 0.64f;
    m_layers[2].normalStrength = 1.2f;
    m_layers[2].fallbackColor = glm::vec3(0.32f, 0.32f, 0.34f);

    m_layers[3].name = "Capa 4: Nieve / Cumbres";
    m_layers[3].tiling = 12.0f;
    m_layers[3].roughness = 0.45f;
    m_layers[3].normalStrength = 0.8f;
    m_layers[3].fallbackColor = glm::vec3(0.93f, 0.95f, 0.98f);

    buildGridMesh();

    // Inicializar subsistema OASIS (Geometría Virtual y Streaming Continuo CDLOD)
    m_oasis = std::make_unique<OASISTerrain>();
    m_oasis->init(33);

    // Generar un terreno inicial de colinas suaves para que la escena nazca viva y orgánica
    generatePreset(TerrainPreset::ROLLING_HILLS);

    std::cout << "[ASIS Terrain] Terreno 3D inicializado con exito (" << m_resolution 
              << "x" << m_resolution << " vertices, " << m_worldSize << "m x " << m_worldSize << "m).\n";
    return true;
}

void Terrain::shutdown() {
    if (m_oasis) {
        m_oasis->shutdown();
        m_oasis.reset();
    }
    if (m_physicsWorld && m_terrainBody) {
        m_physicsWorld->removeRigidBodyDirect(m_terrainBody);
        delete m_terrainBody;
        m_terrainBody = nullptr;
    }
    if (m_motionState) {
        delete m_motionState;
        m_motionState = nullptr;
    }
    if (m_terrainShape) {
        delete m_terrainShape;
        m_terrainShape = nullptr;
    }

    // Liberar texturas OpenGL de todas las capas PBR
    for (auto& l : m_layers) {
        if (l.albedoTex != 0) {
            glDeleteTextures(1, &l.albedoTex);
            l.albedoTex = 0;
        }
        if (l.normalTex != 0) {
            glDeleteTextures(1, &l.normalTex);
            l.normalTex = 0;
        }
        if (l.roughSpecTex != 0) {
            glDeleteTextures(1, &l.roughSpecTex);
            l.roughSpecTex = 0;
        }
        if (l.heightTex != 0) {
            glDeleteTextures(1, &l.heightTex);
            l.heightTex = 0;
        }
        if (l.aoTex != 0) {
            glDeleteTextures(1, &l.aoTex);
            l.aoTex = 0;
        }
    }
    m_layers.clear();

    if (m_splatmapTex != 0) {
        glDeleteTextures(1, &m_splatmapTex);
        m_splatmapTex = 0;
    }
    m_splatmapData.clear();
    m_hasCustomSplatmap = false;

    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    m_heights.clear();
    m_vertices.clear();
    m_indices.clear();
}

bool Terrain::rebuild(int resolution, float worldSize) {
    PhysicsWorld* savedPhysics = m_physicsWorld;
    shutdown();
    init(resolution, worldSize);
    if (savedPhysics) {
        initPhysics(savedPhysics);
    }
    return true;
}

void Terrain::resetToDefaults() {
    // 1. Liberar texturas OpenGL previas de capas
    for (auto& l : m_layers) {
        if (l.albedoTex != 0) glDeleteTextures(1, &l.albedoTex);
        if (l.normalTex != 0) glDeleteTextures(1, &l.normalTex);
        if (l.roughSpecTex != 0) glDeleteTextures(1, &l.roughSpecTex);
        if (l.heightTex != 0) glDeleteTextures(1, &l.heightTex);
        if (l.aoTex != 0) glDeleteTextures(1, &l.aoTex);
    }

    // 2. Restaurar capas PBR predeterminadas
    m_layers.clear();
    m_layers.resize(4);

    m_layers[0].name = "Capa 1: Pasto / Cesped";
    m_layers[0].tiling = 20.0f;
    m_layers[0].roughness = 0.85f;
    m_layers[0].specular = 0.35f;
    m_layers[0].fallbackColor = glm::vec3(0.28f, 0.55f, 0.22f);

    m_layers[1].name = "Capa 2: Tierra / Arena";
    m_layers[1].tiling = 18.0f;
    m_layers[1].roughness = 0.90f;
    m_layers[1].specular = 0.20f;
    m_layers[1].fallbackColor = glm::vec3(0.50f, 0.38f, 0.24f);

    m_layers[2].name = "Capa 3: Roca / Acantilado";
    m_layers[2].tiling = 12.0f;
    m_layers[2].roughness = 0.75f;
    m_layers[2].specular = 0.40f;
    m_layers[2].fallbackColor = glm::vec3(0.42f, 0.42f, 0.44f);

    m_layers[3].name = "Capa 4: Nieve / Cumbre";
    m_layers[3].tiling = 14.0f;
    m_layers[3].roughness = 0.60f;
    m_layers[3].specular = 0.55f;
    m_layers[3].fallbackColor = glm::vec3(0.92f, 0.94f, 0.98f);

    m_baseOffset = -8.0f;
    m_reflectionIntensity = 1.0f;
    m_anisotropyLevel = 16;
    m_reliefMode = TerrainReliefMode::OASIS_TESSELLATION;
    m_tessellationDisplacementScale = 1.0f;
    m_visible = true;
    m_hasCustomSplatmap = false;

    // 3. Regenerar mapa alpino procedural
    generatePreset(TerrainPreset::MOUNTAINS);
}

void Terrain::setBaseOffset(float offset) {
    m_baseOffset = offset;
    updateNormalsAndVBO();
    syncPhysics();
}

void Terrain::buildGridMesh() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }

    // La malla VBO directa de respaldo se acota a 256x256 (65K vértices / 2MB VRAM)
    // para evitar saturar el bus PCIe y congelar la CPU. La resolución completa (4096+)
    // la gestiona el sistema de geometría virtual OASIS mediante CDLOD y textura R32F en GPU.
    int meshRes = (std::min)(m_resolution, 256);
    m_vertices.resize(meshRes * meshRes);
    m_indices.clear();
    m_indices.reserve((meshRes - 1) * (meshRes - 1) * 6);

    float halfSize = m_worldSize * 0.5f;
    float dx = m_worldSize / static_cast<float>(meshRes - 1);
    float dz = dx;

    // Crear vértices en cuadrícula regular centrada en (0, 0, 0)
    for (int z = 0; z < meshRes; ++z) {
        for (int x = 0; x < meshRes; ++x) {
            int idx = z * meshRes + x;
            float worldX = -halfSize + static_cast<float>(x) * dx;
            float worldZ = -halfSize + static_cast<float>(z) * dz;
            float normX = static_cast<float>(x) / static_cast<float>(meshRes - 1);
            float normZ = static_cast<float>(z) / static_cast<float>(meshRes - 1);
            int hX = static_cast<int>(normX * (m_resolution - 1));
            int hZ = static_cast<int>(normZ * (m_resolution - 1));
            float height = getHeight(hX, hZ) + m_baseOffset;

            m_vertices[idx].position = glm::vec3(worldX, height, worldZ);
            m_vertices[idx].normal = glm::vec3(0.0f, 1.0f, 0.0f);
            m_vertices[idx].uv = glm::vec2(normX, normZ);
        }
    }

    // Generar índices para dos triángulos por celda cuadriculada
    for (int z = 0; z < meshRes - 1; ++z) {
        for (int x = 0; x < meshRes - 1; ++x) {
            uint32_t i0 = z * meshRes + x;
            uint32_t i1 = (z + 1) * meshRes + x;
            uint32_t i2 = (z + 1) * meshRes + (x + 1);
            uint32_t i3 = z * meshRes + (x + 1);

            // Triángulo 1
            m_indices.push_back(i0);
            m_indices.push_back(i1);
            m_indices.push_back(i2);

            // Triángulo 2
            m_indices.push_back(i0);
            m_indices.push_back(i2);
            m_indices.push_back(i3);
        }
    }

    // Crear y configurar buffers OpenGL
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(TerrainVertex), m_vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t), m_indices.data(), GL_STATIC_DRAW);

    // Posición (Location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, position));

    // Normal (Location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, normal));

    // Coordenadas UV (Location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, uv));

    glBindVertexArray(0);

    updateNormalsAndVBO();
}

void Terrain::updateMinMaxHeight() {
    if (m_heights.empty()) {
        m_minHeight = 0.0f;
        m_maxHeight = 0.0f;
        return;
    }
    m_minHeight = m_heights[0];
    m_maxHeight = m_heights[0];
    for (float h : m_heights) {
        if (h < m_minHeight) m_minHeight = h;
        if (h > m_maxHeight) m_maxHeight = h;
    }
    // Asegurar un rango no-nulo para Bullet Physics
    if (std::abs(m_maxHeight - m_minHeight) < 0.01f) {
        m_maxHeight += 0.02f;
    }
}

void Terrain::updateNormalsAndVBO() {
    int meshRes = (std::min)(m_resolution, 256);
    float dx = m_worldSize / static_cast<float>(meshRes - 1);
    float dz = dx;

    // Recalcular normales analíticas mediante diferencias centrales para la malla de respaldo
    for (int z = 0; z < meshRes; ++z) {
        for (int x = 0; x < meshRes; ++x) {
            int idx = z * meshRes + x;
            float normX = static_cast<float>(x) / static_cast<float>(meshRes - 1);
            float normZ = static_cast<float>(z) / static_cast<float>(meshRes - 1);
            int hX = static_cast<int>(normX * (m_resolution - 1));
            int hZ = static_cast<int>(normZ * (m_resolution - 1));

            float hL = getHeight((hX > 0) ? hX - 1 : 0, hZ);
            float hR = getHeight((hX < m_resolution - 1) ? hX + 1 : m_resolution - 1, hZ);
            float hD = getHeight(hX, (hZ > 0) ? hZ - 1 : 0);
            float hU = getHeight(hX, (hZ < m_resolution - 1) ? hZ + 1 : m_resolution - 1);

            glm::vec3 norm((hL - hR) / (2.0f * dx), 1.0f, (hD - hU) / (2.0f * dz));
            m_vertices[idx].position.y = getHeight(hX, hZ) + m_baseOffset;
            m_vertices[idx].normal = glm::normalize(norm);
        }
    }

    updateMinMaxHeight();

    // Actualizar VBO en memoria de GPU
    if (m_vbo != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(TerrainVertex), m_vertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Actualizar Heightmap GPU en OASIS (R32F de ultra-resolución 4096+)
    if (m_oasis) {
        m_oasis->updateHeightmap(m_heights, m_resolution);
    }

    m_isDirty = false;
}

void Terrain::restoreHeightsAndSplatmap(const std::vector<float>& heights, const std::vector<uint8_t>& splatmap) {
    if (heights.size() == m_heights.size()) {
        m_heights = heights;
    }
    if (!splatmap.empty() && splatmap.size() == m_splatmapData.size()) {
        m_splatmapData = splatmap;
        m_hasCustomSplatmap = true;
        if (m_splatmapTex != 0) {
            glBindTexture(GL_TEXTURE_2D, m_splatmapTex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_resolution, m_resolution, GL_RGBA, GL_UNSIGNED_BYTE, m_splatmapData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    updateNormalsAndVBO();
    syncPhysics();
}

float Terrain::noise2D(float x, float y) const {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);

    // Suavizado Hermite cúbico
    float sx = fx * fx * (3.0f - 2.0f * fx);
    float sy = fy * fy * (3.0f - 2.0f * fy);

    float n00 = hash12(static_cast<float>(ix), static_cast<float>(iy), m_settings.seed);
    float n10 = hash12(static_cast<float>(ix + 1), static_cast<float>(iy), m_settings.seed);
    float n01 = hash12(static_cast<float>(ix), static_cast<float>(iy + 1), m_settings.seed);
    float n11 = hash12(static_cast<float>(ix + 1), static_cast<float>(iy + 1), m_settings.seed);

    float ix0 = n00 + sx * (n10 - n00);
    float ix1 = n01 + sx * (n11 - n01);
    return ix0 + sy * (ix1 - ix0);
}

float Terrain::fbm(float x, float y, int octaves, float persistence, float frequency) const {
    float total = 0.0f;
    float amplitude = 1.0f;
    float maxAmp = 0.0f;
    float freq = frequency;

    for (int i = 0; i < octaves; ++i) {
        total += noise2D(x * freq, y * freq) * amplitude;
        maxAmp += amplitude;
        amplitude *= persistence;
        freq *= 2.0f;
    }
    return total / maxAmp;
}

float Terrain::ridgedNoise(float x, float y, int octaves, float frequency) const {
    float total = 0.0f;
    float amplitude = 1.0f;
    float maxAmp = 0.0f;
    float freq = frequency;

    for (int i = 0; i < octaves; ++i) {
        float n = noise2D(x * freq, y * freq);
        // Invertir valor absoluto para crear aristas de montaña afiladas
        float ridge = 1.0f - std::abs(n * 2.0f - 1.0f);
        ridge = ridge * ridge;
        total += ridge * amplitude;
        maxAmp += amplitude;
        amplitude *= 0.5f;
        freq *= 2.0f;
    }
    return total / maxAmp;
}

void Terrain::generatePreset(TerrainPreset preset) {
    switch (preset) {
        case TerrainPreset::FLAT:
            resetFlat();
            break;
        case TerrainPreset::ROLLING_HILLS: {
            m_settings.maxHeight = 9.5f;
            m_settings.noiseFrequency = 0.024f;
            m_settings.noiseOctaves = 4;
            m_settings.noisePersistence = 0.45f;
            generateProcedural(m_settings);
            break;
        }
        case TerrainPreset::MOUNTAINS: {
            m_settings.maxHeight = 28.0f;
            m_settings.noiseFrequency = 0.020f;
            m_settings.noiseOctaves = 6;
            m_settings.noisePersistence = 0.52f;

            float halfSize = m_worldSize * 0.5f;
            float dx = m_worldSize / static_cast<float>(m_resolution - 1);

            for (int z = 0; z < m_resolution; ++z) {
                for (int x = 0; x < m_resolution; ++x) {
                    float wx = -halfSize + static_cast<float>(x) * dx;
                    float wz = -halfSize + static_cast<float>(z) * dx;

                    // Máscara de borde radial para que el terreno descienda suavemente hacia los extremos
                    float distFromCenter = std::sqrt(wx * wx + wz * wz);
                    float edgeFade = 1.0f - glm::smoothstep(m_worldSize * 0.35f, m_worldSize * 0.49f, distFromCenter);

                    float r = ridgedNoise(wx, wz, m_settings.noiseOctaves, m_settings.noiseFrequency);
                    float f = fbm(wx + 50.0f, wz + 50.0f, 4, 0.45f, 0.045f);

                    float h = (r * 0.75f + f * 0.25f) * m_settings.maxHeight * edgeFade;
                    m_heights[z * m_resolution + x] = h;
                }
            }
            updateNormalsAndVBO();
            syncPhysics();
            break;
        }
        case TerrainPreset::CANYONS: {
            m_settings.maxHeight = 18.0f;
            m_settings.noiseFrequency = 0.028f;
            m_settings.noiseOctaves = 5;

            float halfSize = m_worldSize * 0.5f;
            float dx = m_worldSize / static_cast<float>(m_resolution - 1);

            for (int z = 0; z < m_resolution; ++z) {
                for (int x = 0; x < m_resolution; ++x) {
                    float wx = -halfSize + static_cast<float>(x) * dx;
                    float wz = -halfSize + static_cast<float>(z) * dx;

                    float n = fbm(wx, wz, m_settings.noiseOctaves, 0.5f, m_settings.noiseFrequency);
                    // Efecto de mesetas escalonadas (Terracing)
                    float terraced = std::floor(n * 4.0f) / 4.0f + std::pow(std::fmod(n * 4.0f, 1.0f), 3.0f) * 0.25f;
                    float h = terraced * m_settings.maxHeight;
                    m_heights[z * m_resolution + x] = h;
                }
            }
            updateNormalsAndVBO();
            syncPhysics();
            break;
        }
    }
}

void Terrain::generateProcedural(const TerrainSettings& settings) {
    m_settings = settings;
    float halfSize = m_worldSize * 0.5f;
    float dx = m_worldSize / static_cast<float>(m_resolution - 1);

    for (int z = 0; z < m_resolution; ++z) {
        for (int x = 0; x < m_resolution; ++x) {
            float wx = -halfSize + static_cast<float>(x) * dx;
            float wz = -halfSize + static_cast<float>(z) * dx;

            float n = fbm(wx, wz, m_settings.noiseOctaves, m_settings.noisePersistence, m_settings.noiseFrequency);
            m_heights[z * m_resolution + x] = n * m_settings.maxHeight;
        }
    }

    updateNormalsAndVBO();
    syncPhysics();
}

void Terrain::resetFlat() {
    std::fill(m_heights.begin(), m_heights.end(), 0.0f);
    updateNormalsAndVBO();
    syncPhysics();
}

void Terrain::sculpt(const glm::vec3& worldPos, float radius, float strength, 
                     SculptBrushType type, SculptFalloff falloff, float targetHeight, float dt) {
    if (radius <= 0.01f || strength <= 0.0f) return;

    float halfSize = m_worldSize * 0.5f;
    float dx = m_worldSize / static_cast<float>(m_resolution - 1);
    float invDx = 1.0f / dx;

    // Calcular caja envolvente en índices de la cuadrícula
    int minX = std::clamp(static_cast<int>(std::floor((worldPos.x - radius + halfSize) * invDx)), 0, m_resolution - 1);
    int maxX = std::clamp(static_cast<int>(std::ceil((worldPos.x + radius + halfSize) * invDx)), 0, m_resolution - 1);
    int minZ = std::clamp(static_cast<int>(std::floor((worldPos.z - radius + halfSize) * invDx)), 0, m_resolution - 1);
    int maxZ = std::clamp(static_cast<int>(std::ceil((worldPos.z + radius + halfSize) * invDx)), 0, m_resolution - 1);

    float radiusSq = radius * radius;
    float rate = strength * dt * 10.0f;

    for (int z = minZ; z <= maxZ; ++z) {
        float wz = -halfSize + static_cast<float>(z) * dx;
        float dzDist = wz - worldPos.z;

        for (int x = minX; x <= maxX; ++x) {
            float wx = -halfSize + static_cast<float>(x) * dx;
            float dxDist = wx - worldPos.x;
            float distSq = dxDist * dxDist + dzDist * dzDist;

            if (distSq > radiusSq) continue;

            float dist = std::sqrt(distSq);
            float r = dist / radius; // [0.0, 1.0]

            // Calcular factor de atenuación según el perfil seleccionado
            float factor = 1.0f;
            switch (falloff) {
                case SculptFalloff::SMOOTH:
                    factor = 0.5f * (1.0f + std::cos(3.14159265f * r));
                    break;
                case SculptFalloff::SPHERICAL:
                    factor = std::sqrt(std::max(0.0f, 1.0f - r * r));
                    break;
                case SculptFalloff::LINEAR:
                    factor = 1.0f - r;
                    break;
                case SculptFalloff::FLAT:
                    factor = 1.0f;
                    break;
            }

            int idx = z * m_resolution + x;
            float currentH = m_heights[idx];

            switch (type) {
                case SculptBrushType::RAISE:
                    m_heights[idx] += factor * rate;
                    break;
                case SculptBrushType::LOWER:
                    m_heights[idx] -= factor * rate;
                    break;
                case SculptBrushType::SMOOTH: {
                    // Promediar alturas adyacentes para suavizado de Laplace
                    float sum = 0.0f;
                    int count = 0;
                    for (int sz = -1; sz <= 1; ++sz) {
                        for (int sx = -1; sx <= 1; ++sx) {
                            int nx = std::clamp(x + sx, 0, m_resolution - 1);
                            int nz = std::clamp(z + sz, 0, m_resolution - 1);
                            sum += m_heights[nz * m_resolution + nx];
                            count++;
                        }
                    }
                    float avg = (count > 0) ? (sum / static_cast<float>(count)) : currentH;
                    float blend = std::clamp(factor * strength * dt * 8.0f, 0.0f, 1.0f);
                    m_heights[idx] = glm::mix(currentH, avg, blend);
                    break;
                }
                case SculptBrushType::FLATTEN: {
                    float blend = std::clamp(factor * strength * dt * 8.0f, 0.0f, 1.0f);
                    m_heights[idx] = glm::mix(currentH, targetHeight, blend);
                    break;
                }
                case SculptBrushType::NOISE: {
                    float n = (noise2D(wx * 0.4f, wz * 0.4f) - 0.5f) * 2.0f;
                    m_heights[idx] += n * factor * rate * 0.8f;
                    break;
                }
            }
        }
    }

    updateMinMaxHeight();

    // Actualización parcial ultrarrápida de la textura GPU (R32F) vía OASIS si está activo
    if (m_oasis && m_oasis->isEnabled()) {
        int subW = maxX - minX + 1;
        int subH = maxZ - minZ + 1;
        m_oasis->updateHeightmapSubRegion(m_heights, m_resolution, minX, minZ, subW, subH);
        m_isDirty = true;
    } else {
        updateNormalsAndVBO();
    }
}

bool Terrain::raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, 
                      glm::vec3& outHitPos, glm::vec3& outHitNormal, float maxDist) const {
    float halfSize = m_worldSize * 0.5f;
    float stepSize = 0.28f;
    float currentDist = 0.0f;

    glm::vec3 lastRayPos = rayOrigin;
    bool hadEnteredBounds = false;

    while (currentDist < maxDist) {
        glm::vec3 rayPos = rayOrigin + rayDir * currentDist;

        // Comprobar si el rayo se encuentra dentro de los límites X/Z del terreno
        if (rayPos.x >= -halfSize && rayPos.x <= halfSize &&
            rayPos.z >= -halfSize && rayPos.z <= halfSize) {
            
            hadEnteredBounds = true;
            float terrainH = getHeightAtWorld(rayPos.x, rayPos.z);

            if (rayPos.y <= terrainH) {
                // Intersección encontrada: refinamiento por búsqueda binaria de 10 iteraciones
                float t0 = currentDist - stepSize;
                float t1 = currentDist;
                for (int i = 0; i < 10; ++i) {
                    float tMid = (t0 + t1) * 0.5f;
                    glm::vec3 midPos = rayOrigin + rayDir * tMid;
                    float midH = getHeightAtWorld(midPos.x, midPos.z);
                    if (midPos.y <= midH) {
                        t1 = tMid;
                    } else {
                        t0 = tMid;
                    }
                }

                float finalT = (t0 + t1) * 0.5f;
                outHitPos = rayOrigin + rayDir * finalT;
                outHitNormal = getNormalAtWorld(outHitPos.x, outHitPos.z);
                return true;
            }
        } else if (hadEnteredBounds && (rayPos.y < m_minHeight - 2.0f)) {
            // El rayo cruzó y ya cayó por debajo del terreno
            break;
        }

        currentDist += stepSize;
    }

    return false;
}

float Terrain::getHeight(int x, int z) const {
    if (x < 0 || x >= m_resolution || z < 0 || z >= m_resolution) return 0.0f;
    return m_heights[z * m_resolution + x];
}

void Terrain::setHeight(int x, int z, float h) {
    if (x < 0 || x >= m_resolution || z < 0 || z >= m_resolution) return;
    m_heights[z * m_resolution + x] = h;
    m_isDirty = true;
}

float Terrain::getHeightAtWorld(float worldX, float worldZ) const {
    float halfSize = m_worldSize * 0.5f;
    float dx = m_worldSize / static_cast<float>(m_resolution - 1);
    float gx = (worldX + halfSize) / dx;
    float gz = (worldZ + halfSize) / dx;

    int x0 = std::clamp(static_cast<int>(std::floor(gx)), 0, m_resolution - 1);
    int z0 = std::clamp(static_cast<int>(std::floor(gz)), 0, m_resolution - 1);
    int x1 = std::clamp(x0 + 1, 0, m_resolution - 1);
    int z1 = std::clamp(z0 + 1, 0, m_resolution - 1);

    float fx = gx - static_cast<float>(x0);
    float fz = gz - static_cast<float>(z0);

    float h00 = m_heights[z0 * m_resolution + x0];
    float h10 = m_heights[z0 * m_resolution + x1];
    float h01 = m_heights[z1 * m_resolution + x0];
    float h11 = m_heights[z1 * m_resolution + x1];

    // Interpolación bilineal
    float hBottom = h00 + fx * (h10 - h00);
    float hTop = h01 + fx * (h11 - h01);
    return hBottom + fz * (hTop - hBottom) + m_baseOffset;
}

glm::vec3 Terrain::getNormalAtWorld(float worldX, float worldZ) const {
    float delta = m_worldSize / static_cast<float>(m_resolution - 1);
    float hL = getHeightAtWorld(worldX - delta, worldZ);
    float hR = getHeightAtWorld(worldX + delta, worldZ);
    float hD = getHeightAtWorld(worldX, worldZ - delta);
    float hU = getHeightAtWorld(worldX, worldZ + delta);

    glm::vec3 n((hL - hR) / (2.0f * delta), 1.0f, (hD - hU) / (2.0f * delta));
    return glm::normalize(n);
}

void Terrain::initPhysics(PhysicsWorld* physicsWorld) {
    m_physicsWorld = physicsWorld;
    syncPhysics();
}

void Terrain::syncPhysics() {
    if (!m_physicsWorld || !m_physicsEnabled) return;

    // Eliminar cuerpo y forma anterior si existen
    if (m_terrainBody) {
        m_physicsWorld->removeRigidBodyDirect(m_terrainBody);
        delete m_terrainBody;
        m_terrainBody = nullptr;
    }
    if (m_motionState) {
        delete m_motionState;
        m_motionState = nullptr;
    }
    if (m_terrainShape) {
        delete m_terrainShape;
        m_terrainShape = nullptr;
    }

    updateMinMaxHeight();

    // Crear forma de terreno de Bullet Physics basada en el array continuo m_heights
    m_terrainShape = new btHeightfieldTerrainShape(
        m_resolution, m_resolution,
        m_heights.data(),
        1.0f, // Escala de altura directa
        m_minHeight, m_maxHeight,
        1,    // upAxis = 1 (eje Y vertical)
        PHY_FLOAT,
        false // flipQuadEdges
    );

    float spacing = m_worldSize / static_cast<float>(m_resolution - 1);
    m_terrainShape->setLocalScaling(btVector3(spacing, 1.0f, spacing));

    // Bullet centra internamente el AABB del heightfield en (minHeight + maxHeight) * 0.5
    btTransform transform;
    transform.setIdentity();
    float centerY = (m_minHeight + m_maxHeight) * 0.5f + m_baseOffset;
    transform.setOrigin(btVector3(0.0f, centerY, 0.0f));

    m_motionState = new btDefaultMotionState(transform);

    // Masa = 0 para cuerpo estático
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, m_motionState, m_terrainShape, btVector3(0, 0, 0));
    rbInfo.m_friction = 0.85f;
    rbInfo.m_restitution = 0.20f;

    m_terrainBody = new btRigidBody(rbInfo);
    m_terrainBody->setUserPointer((void*)this);

    m_physicsWorld->addRigidBodyDirect(m_terrainBody);
}

void Terrain::setPhysicsEnabled(bool enabled) {
    m_physicsEnabled = enabled;
    if (m_physicsWorld && m_terrainBody) {
        if (m_physicsEnabled) {
            m_physicsWorld->addRigidBodyDirect(m_terrainBody);
        } else {
            m_physicsWorld->removeRigidBodyDirect(m_terrainBody);
        }
    }
}

void Terrain::initSplatmap() {
    if (m_splatmapTex == 0) {
        glGenTextures(1, &m_splatmapTex);
    }
    m_splatmapData.resize(m_resolution * m_resolution * 4);

    float gridSpacing = m_worldSize / static_cast<float>(m_resolution - 1);

    // Inicializar el splatmap según el bioma procedural actual
    for (int z = 0; z < m_resolution; ++z) {
        int zD = (z > 0) ? z - 1 : 0;
        int zU = (z < m_resolution - 1) ? z + 1 : m_resolution - 1;
        float dz = static_cast<float>(zU - zD) * gridSpacing;
        if (dz <= 0.0001f) dz = gridSpacing;

        for (int x = 0; x < m_resolution; ++x) {
            int idx = z * m_resolution + x;
            int pixelIdx = idx * 4;

            float elevation = m_heights[idx]; // Altura local sin restar m_baseOffset

            // Cálculo de normal analítica y pendiente (slopeY) mediante diferencias centrales
            int xL = (x > 0) ? x - 1 : 0;
            int xR = (x < m_resolution - 1) ? x + 1 : m_resolution - 1;
            float dx = static_cast<float>(xR - xL) * gridSpacing;
            if (dx <= 0.0001f) dx = gridSpacing;

            float hL = m_heights[z * m_resolution + xL];
            float hR = m_heights[z * m_resolution + xR];
            float hD = m_heights[zD * m_resolution + x];
            float hU = m_heights[zU * m_resolution + x];

            float dHdX = (hR - hL) / dx;
            float dHdZ = (hU - hD) / dz;
            float slopeY = 1.0f / std::sqrt(dHdX * dHdX + dHdZ * dHdZ + 1.0f);

            uint8_t r = 0, g = 0, b = 0, a = 0;
            if (slopeY < 0.72f) {
                // Roca (Canal Azul)
                b = 255;
            } else if (elevation > 14.0f) {
                // Nieve Alpina (Canal Alfa)
                a = 255;
            } else if (elevation > 5.5f) {
                // Tierra / Sendero (Canal Verde)
                g = 255;
            } else {
                // Césped / Valle (Canal Rojo)
                r = 255;
            }

            m_splatmapData[pixelIdx + 0] = r;
            m_splatmapData[pixelIdx + 1] = g;
            m_splatmapData[pixelIdx + 2] = b;
            m_splatmapData[pixelIdx + 3] = a;
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_splatmapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_resolution, m_resolution, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_splatmapData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_hasCustomSplatmap = true;
}

void Terrain::resetTexturePaint() {
    if (m_splatmapTex != 0) {
        glDeleteTextures(1, &m_splatmapTex);
        m_splatmapTex = 0;
    }
    m_splatmapData.clear();
    m_hasCustomSplatmap = false;
}

void Terrain::paintTexture(const glm::vec3& worldPos, TerrainPaintLayer layer, float radius, float strength, float dt) {
    if (radius <= 0.01f || strength <= 0.0f) return;
    if (!m_hasCustomSplatmap || m_splatmapTex == 0) {
        initSplatmap();
    }

    float halfSize = m_worldSize * 0.5f;
    float dx = m_worldSize / static_cast<float>(m_resolution - 1);
    float invDx = 1.0f / dx;

    int minX = std::clamp(static_cast<int>(std::floor((worldPos.x - radius + halfSize) * invDx)), 0, m_resolution - 1);
    int maxX = std::clamp(static_cast<int>(std::ceil((worldPos.x + radius + halfSize) * invDx)), 0, m_resolution - 1);
    int minZ = std::clamp(static_cast<int>(std::floor((worldPos.z - radius + halfSize) * invDx)), 0, m_resolution - 1);
    int maxZ = std::clamp(static_cast<int>(std::ceil((worldPos.z + radius + halfSize) * invDx)), 0, m_resolution - 1);

    float radiusSq = radius * radius;
    int targetChan = static_cast<int>(layer);

    for (int z = minZ; z <= maxZ; ++z) {
        float wz = -halfSize + static_cast<float>(z) * dx;
        float dzDist = wz - worldPos.z;

        for (int x = minX; x <= maxX; ++x) {
            float wx = -halfSize + static_cast<float>(x) * dx;
            float dxDist = wx - worldPos.x;
            float distSq = dxDist * dxDist + dzDist * dzDist;

            if (distSq > radiusSq) continue;

            float dist = std::sqrt(distSq);
            float rNorm = dist / radius; // [0, 1]
            float factor = 0.5f * (1.0f + std::cos(3.14159265f * rNorm));
            float deltaAmt = std::clamp(factor * strength * dt * 5.0f, 0.0f, 1.0f);

            int pixelIdx = (z * m_resolution + x) * 4;

            float channels[4] = {
                static_cast<float>(m_splatmapData[pixelIdx + 0]) / 255.0f,
                static_cast<float>(m_splatmapData[pixelIdx + 1]) / 255.0f,
                static_cast<float>(m_splatmapData[pixelIdx + 2]) / 255.0f,
                static_cast<float>(m_splatmapData[pixelIdx + 3]) / 255.0f
            };

            channels[targetChan] = std::clamp(channels[targetChan] + deltaAmt, 0.0f, 1.0f);

            // Re-normalizar canales para que la suma sea exactamente 1.0
            float sum = channels[0] + channels[1] + channels[2] + channels[3];
            if (sum > 0.001f) {
                channels[0] /= sum;
                channels[1] /= sum;
                channels[2] /= sum;
                channels[3] /= sum;
            }

            m_splatmapData[pixelIdx + 0] = static_cast<uint8_t>(std::round(channels[0] * 255.0f));
            m_splatmapData[pixelIdx + 1] = static_cast<uint8_t>(std::round(channels[1] * 255.0f));
            m_splatmapData[pixelIdx + 2] = static_cast<uint8_t>(std::round(channels[2] * 255.0f));
            m_splatmapData[pixelIdx + 3] = static_cast<uint8_t>(std::round(channels[3] * 255.0f));
        }
    }

    // Subir textura actualizada a GPU
    glBindTexture(GL_TEXTURE_2D, m_splatmapTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_resolution, m_resolution, GL_RGBA, GL_UNSIGNED_BYTE, m_splatmapData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Terrain::loadLayerAlbedo(int layerIndex, const std::string& path) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return false;
    GLuint tex = loadUniversalTexture(path);
    if (tex != 0) {
        if (m_layers[layerIndex].albedoTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].albedoTex);
        }
        m_layers[layerIndex].albedoTex = tex;
        m_layers[layerIndex].albedoPath = path;
        return true;
    }
    return false;
}

bool Terrain::loadLayerNormal(int layerIndex, const std::string& path) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return false;
    GLuint tex = loadUniversalTexture(path);
    if (tex != 0) {
        if (m_layers[layerIndex].normalTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].normalTex);
        }
        m_layers[layerIndex].normalTex = tex;
        m_layers[layerIndex].normalPath = path;
        return true;
    }
    return false;
}

bool Terrain::loadLayerRoughSpec(int layerIndex, const std::string& path) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return false;
    GLuint tex = loadUniversalTexture(path);
    if (tex != 0) {
        if (m_layers[layerIndex].roughSpecTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].roughSpecTex);
        }
        m_layers[layerIndex].roughSpecTex = tex;
        m_layers[layerIndex].roughSpecPath = path;
        return true;
    }
    return false;
}

int Terrain::addLayer(const std::string& name) {
    if (m_layers.size() >= 8) {
        std::cerr << "[Terrain] Límite máximo de capas alcanzado.\n";
        return -1;
    }
    TerrainLayer newLayer;
    newLayer.name = name.empty() ? ("Capa " + std::to_string(m_layers.size() + 1)) : name;
    newLayer.tiling = 14.0f;
    newLayer.roughness = 0.8f;
    newLayer.normalStrength = 1.0f;
    newLayer.heightScale = 0.03f;
    newLayer.aoStrength = 1.0f;
    newLayer.fallbackColor = glm::vec3(0.45f, 0.45f, 0.45f);

    m_layers.push_back(newLayer);
    return static_cast<int>(m_layers.size() - 1);
}

bool Terrain::removeLayer(int index) {
    if (index < 0 || index >= static_cast<int>(m_layers.size())) return false;
    if (m_layers.size() <= 1) {
        std::cerr << "[Terrain] No se puede eliminar la única capa restante.\n";
        return false;
    }

    auto& l = m_layers[index];
    if (l.albedoTex != 0) glDeleteTextures(1, &l.albedoTex);
    if (l.normalTex != 0) glDeleteTextures(1, &l.normalTex);
    if (l.roughSpecTex != 0) glDeleteTextures(1, &l.roughSpecTex);
    if (l.heightTex != 0) glDeleteTextures(1, &l.heightTex);
    if (l.aoTex != 0) glDeleteTextures(1, &l.aoTex);

    m_layers.erase(m_layers.begin() + index);

    if (static_cast<int>(activePaintLayer) >= static_cast<int>(m_layers.size())) {
        activePaintLayer = static_cast<TerrainPaintLayer>(m_layers.size() - 1);
    }
    return true;
}

bool Terrain::moveLayerUp(int index) {
    if (index <= 0 || index >= static_cast<int>(m_layers.size())) return false;
    std::swap(m_layers[index], m_layers[index - 1]);
    return true;
}

bool Terrain::moveLayerDown(int index) {
    if (index < 0 || index >= static_cast<int>(m_layers.size()) - 1) return false;
    std::swap(m_layers[index], m_layers[index + 1]);
    return true;
}

bool Terrain::loadLayerHeight(int layerIndex, const std::string& path) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return false;
    GLuint tex = loadUniversalTexture(path);
    if (tex != 0) {
        if (m_layers[layerIndex].heightTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].heightTex);
        }
        m_layers[layerIndex].heightTex = tex;
        m_layers[layerIndex].heightPath = path;
        return true;
    }
    return false;
}

bool Terrain::loadLayerAO(int layerIndex, const std::string& path) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return false;
    GLuint tex = loadUniversalTexture(path);
    if (tex != 0) {
        if (m_layers[layerIndex].aoTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].aoTex);
        }
        m_layers[layerIndex].aoTex = tex;
        m_layers[layerIndex].aoPath = path;
        return true;
    }
    return false;
}

void Terrain::clearLayerTexture(int layerIndex, int slot) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return;
    if (slot == 0) {
        if (m_layers[layerIndex].albedoTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].albedoTex);
            m_layers[layerIndex].albedoTex = 0;
        }
        m_layers[layerIndex].albedoPath.clear();
    } else if (slot == 1) {
        if (m_layers[layerIndex].normalTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].normalTex);
            m_layers[layerIndex].normalTex = 0;
        }
        m_layers[layerIndex].normalPath.clear();
    } else if (slot == 2) {
        if (m_layers[layerIndex].roughSpecTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].roughSpecTex);
            m_layers[layerIndex].roughSpecTex = 0;
        }
        m_layers[layerIndex].roughSpecPath.clear();
    } else if (slot == 3) {
        if (m_layers[layerIndex].heightTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].heightTex);
            m_layers[layerIndex].heightTex = 0;
        }
        m_layers[layerIndex].heightPath.clear();
    } else if (slot == 4) {
        if (m_layers[layerIndex].aoTex != 0) {
            glDeleteTextures(1, &m_layers[layerIndex].aoTex);
            m_layers[layerIndex].aoTex = 0;
        }
        m_layers[layerIndex].aoPath.clear();
    }
}

void Terrain::fillSplatmapWithLayer(int layerIndex) {
    if (layerIndex < 0 || layerIndex > 3) return;
    initSplatmap();

    for (int i = 0; i < m_resolution * m_resolution; ++i) {
        m_splatmapData[i * 4 + 0] = (layerIndex == 0) ? 255 : 0;
        m_splatmapData[i * 4 + 1] = (layerIndex == 1) ? 255 : 0;
        m_splatmapData[i * 4 + 2] = (layerIndex == 2) ? 255 : 0;
        m_splatmapData[i * 4 + 3] = (layerIndex == 3) ? 255 : 0;
    }

    m_hasCustomSplatmap = true;
    if (m_splatmapTex != 0) {
        glBindTexture(GL_TEXTURE_2D, m_splatmapTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_resolution, m_resolution, GL_RGBA, GL_UNSIGNED_BYTE, m_splatmapData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Terrain::draw(Shader& shader, const Camera* camera, float aspectRatio) {
    if (!m_visible || m_vao == 0) return;

    if (aspectRatio > 0.05f) {
        m_viewportAspectRatio = aspectRatio;
    }

    if (m_isDirty && (!m_oasis || !m_oasis->isEnabled())) {
        updateNormalsAndVBO();
    }

    shader.use();
    glm::mat4 modelMat = glm::mat4(1.0f);
    shader.setMat4("u_Model", modelMat);
    shader.setFloat("u_TerrainReflectionIntensity", m_reflectionIntensity);
    shader.setInt("u_ReliefMode", static_cast<int>(m_reliefMode));
    shader.setFloat("u_TessellationDisplacementScale", m_tessellationDisplacementScale);
    shader.setFloat("u_WorldSize", m_worldSize);
    shader.setFloat("u_BaseOffset", m_baseOffset);

    // 1. Splatmap de pintura en tiempo real
    if (m_hasCustomSplatmap && m_splatmapTex != 0) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, m_splatmapTex);
        shader.setInt("u_Splatmap", 4);
        shader.setInt("u_UseCustomSplatmap", 1);
    } else {
        shader.setInt("u_UseCustomSplatmap", 0);
    }

    // 2. Capas PBR de Textura con Normal Mapping, Parallax POM y Oclusión
    // Asignación de unidades de textura (compatibles con OpenGL 3.3):
    // Unit 1: ShadowMap, Unit 4: Splatmap, Unit 7: EnvMap IBL
    const int albedoUnits[4]    = { 2, 8, 14, 19 };
    const int normalUnits[4]    = { 3, 9, 15, 20 };
    const int roughSpecUnits[4] = { 12, 13, 16, 21 };
    const int heightUnits[4]    = { 5, 10, 17, 22 };
    const int aoUnits[4]        = { 6, 11, 18, 23 };

    for (int i = 0; i < 4 && i < static_cast<int>(m_layers.size()); ++i) {
        const auto& l = m_layers[i];
        std::string prefix = "u_Layers[" + std::to_string(i) + "].";

        // Albedo / Color
        glActiveTexture(GL_TEXTURE0 + albedoUnits[i]);
        shader.setInt("u_LayerAlbedo[" + std::to_string(i) + "]", albedoUnits[i]);
        if (l.albedoTex != 0 && l.enabled && l.visible) {
            glBindTexture(GL_TEXTURE_2D, l.albedoTex);
            shader.setInt(prefix + "hasAlbedo", 1);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setInt(prefix + "hasAlbedo", 0);
        }

        // Normal Map (Relieve Tangente Cotangente)
        glActiveTexture(GL_TEXTURE0 + normalUnits[i]);
        shader.setInt("u_LayerNormal[" + std::to_string(i) + "]", normalUnits[i]);
        if (l.normalTex != 0 && l.enabled && l.visible) {
            glBindTexture(GL_TEXTURE_2D, l.normalTex);
            shader.setInt(prefix + "hasNormal", 1);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setInt(prefix + "hasNormal", 0);
        }

        // Rugosidad / Especular PBR
        glActiveTexture(GL_TEXTURE0 + roughSpecUnits[i]);
        shader.setInt("u_LayerRoughSpec[" + std::to_string(i) + "]", roughSpecUnits[i]);
        if (l.roughSpecTex != 0 && l.enabled && l.visible) {
            glBindTexture(GL_TEXTURE_2D, l.roughSpecTex);
            shader.setInt(prefix + "hasRoughSpec", 1);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setInt(prefix + "hasRoughSpec", 0);
        }

        // Mapa de Altura / Desplazamiento (Teselación OASIS / Parallax POM)
        glActiveTexture(GL_TEXTURE0 + heightUnits[i]);
        shader.setInt("u_LayerHeight[" + std::to_string(i) + "]", heightUnits[i]);
        if (l.heightTex != 0 && l.enabled && l.visible) {
            glBindTexture(GL_TEXTURE_2D, l.heightTex);
            shader.setInt(prefix + "hasHeight", 1);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setInt(prefix + "hasHeight", 0);
        }

        // Oclusión Ambiental (AO)
        glActiveTexture(GL_TEXTURE0 + aoUnits[i]);
        shader.setInt("u_LayerAO[" + std::to_string(i) + "]", aoUnits[i]);
        if (l.aoTex != 0 && l.enabled && l.visible) {
            glBindTexture(GL_TEXTURE_2D, l.aoTex);
            shader.setInt(prefix + "hasAO", 1);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setInt(prefix + "hasAO", 0);
        }

        shader.setFloat(prefix + "tiling", l.tiling);
        shader.setFloat(prefix + "roughness", l.roughness);
        shader.setFloat(prefix + "specular", l.specular);
        shader.setFloat(prefix + "metallic", l.metallic);
        shader.setFloat(prefix + "normalStrength", l.normalStrength);
        shader.setFloat(prefix + "heightScale", l.heightScale);
        shader.setFloat(prefix + "aoStrength", l.aoStrength);
        shader.setFloat(prefix + "opacity", l.opacity);
        shader.setVec3(prefix + "tint", l.tint);
        shader.setVec3(prefix + "fallbackColor", l.fallbackColor);
    }

    bool useOASIS = (m_oasis && m_oasis->isEnabled());
    if (useOASIS) {
        if (camera != nullptr) {
            m_oasis->update(*camera, m_worldSize, m_baseOffset, m_minHeight, m_maxHeight, m_viewportAspectRatio);
        }
        m_oasis->render(shader, m_worldSize, m_baseOffset);
    } else {
        shader.setInt("u_OASIS_Enabled", 0);
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glActiveTexture(GL_TEXTURE0);
}

} // namespace ASIS
