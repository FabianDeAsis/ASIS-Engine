#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "OASISTerrain.h"

namespace ASIS {

class Shader;
class PhysicsWorld;

enum class TerrainPreset {
    FLAT,
    ROLLING_HILLS,
    MOUNTAINS,
    CANYONS
};

enum class SculptBrushType {
    RAISE,
    LOWER,
    SMOOTH,
    FLATTEN,
    NOISE
};

enum class SculptFalloff {
    SMOOTH,
    SPHERICAL,
    LINEAR,
    FLAT
};

enum class TerrainToolMode {
    SCULPT,
    PAINT_TEXTURE
};

enum class TerrainReliefMode {
    OASIS_TESSELLATION = 0, // Desplazamiento Geométrico de Vértices con OASIS (Recomendado)
    POM = 1,                 // Parallax Occlusion Mapping (Raymarching en Fragment)
    DISABLED = 2             // Desactivado (Plano / Solo Normal Map)
};

enum class TerrainPaintLayer {
    LAYER_0 = 0, // Pasto / Césped
    LAYER_1 = 1, // Tierra / Arena
    LAYER_2 = 2, // Roca / Acantilado
    LAYER_3 = 3, // Nieve / Cumbres
    GRASS = 0,
    DIRT = 1,
    ROCK = 2,
    SNOW = 3
};

/**
 * @brief Capa de Texturizado PBR de Terreno (Arquitectura Estilo Photoshop / Nodos Blender)
 * 
 * Cada capa contiene mapas PBR (Albedo/Color, Normal de Relieve Tangente y Rugosidad/Specular),
 * parámetros de repetición UV (Tiling), factor de rugosidad, intensidad de normal,
 * tinte cromático y visibilidad, permitiendo mezclas continuas en tiempo real vía Splatmap.
 */
struct TerrainLayer {
    std::string name;
    bool enabled = true;
    bool visible = true;
    float opacity = 1.0f;

    // Rutas de archivos en disco (.png, .jpg, .tif, .tiff, .bmp, .tga)
    std::string albedoPath;
    std::string normalPath;
    std::string roughSpecPath;
    std::string heightPath;
    std::string aoPath;

    // Texturas OpenGL asignadas
    GLuint albedoTex = 0;
    GLuint normalTex = 0;
    GLuint roughSpecTex = 0;
    GLuint heightTex = 0;
    GLuint aoTex = 0;

    // Parámetros de superficie PBR escalables a sistema de nodos
    float tiling = 15.0f;
    float roughness = 0.8f;
    float specular = 0.5f;
    float metallic = 0.0f;
    float normalStrength = 1.0f;
    float heightScale = 0.03f; // Profundidad 3D Parallax Occlusion Mapping (POM)
    float aoStrength = 1.0f;   // Intensidad de sombras de oclusión ambiental
    glm::vec3 tint = glm::vec3(1.0f);
    glm::vec3 fallbackColor = glm::vec3(0.5f);
};

struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct TerrainSettings {
    int resolution = 128;      // Resolución de malla (128x128 vértices)
    float worldSize = 120.0f;  // Tamaño en metros (120m x 120m)
    float maxHeight = 26.0f;   // Altura máxima de cumbres
    float noiseFrequency = 0.022f;
    int noiseOctaves = 5;
    float noisePersistence = 0.48f;
    int seed = 1337;
};

struct HeightFogSettings {
    bool enabled = false;                                       // Deshabilitado por defecto (se unificará en Fase 20 con Niebla Global)
    float density = 0.012f;                                     // Densidad global de niebla
    float baseHeight = 0.0f;                                    // Altura base (suelo / valles)
    float heightFalloff = 0.075f;                               // Factor de disipación vertical
    glm::vec3 ambientColor = glm::vec3(0.55f, 0.65f, 0.78f);    // Tono azul grisáceo atmosférico
    glm::vec3 sunColor = glm::vec3(1.0f, 0.88f, 0.65f);        // Resplandor solar dorado cálido
    float sunIntensity = 1.6f;                                  // Intensidad de dispersión solar (inscattering)
};

/**
 * @brief Sistema de Terreno 3D Procedural con Escultura en Tiempo Real y Niebla Atmosférica
 * 
 * Gestiona una malla de terreno de alta resolución basada en Heightmaps dinámicos,
 * generación procedural de biomas (montañas alpinas, colinas suaves, cañones),
 * pinceles de escultura interactiva 3D con retroalimentación visual,
 * colisión física 3D en tiempo real con Bullet Physics (btHeightfieldTerrainShape),
 * y sombreado PBR multicapa con niebla volumétrica de altura.
 */
class Terrain {
public:
    Terrain();
    ~Terrain();

    // Inicializar sistema de terreno
    bool init(int resolution = 128, float worldSize = 120.0f);
    void shutdown();

    // Reconstruir terreno con nueva resolución o dimensiones (Fase 16)
    bool rebuild(int resolution, float worldSize);

    // Renderizar la malla de terreno (con soporte OASIS CDLOD si se pasa camara)
    void draw(Shader& shader, const Camera* camera = nullptr, float aspectRatio = 0.0f);

    void setViewportAspectRatio(float aspect) { if (aspect > 0.05f) m_viewportAspectRatio = aspect; }
    float getViewportAspectRatio() const { return m_viewportAspectRatio; }

    // Sistema OASIS: Geometria Virtual y LOD Continuo (Fase 18)
    OASISTerrain* getOASIS() { return m_oasis.get(); }
    const OASISTerrain* getOASIS() const { return m_oasis.get(); }

    // Ajuste de altura base global (Fase 16)
    void setBaseOffset(float offset);
    float getBaseOffset() const { return m_baseOffset; }

    // Segmentación por trozos / Chunks (Fase 16)
    int getChunksX() const { return m_chunksX; }
    int getChunksZ() const { return m_chunksZ; }
    void setChunks(int cx, int cz) { m_chunksX = (cx > 0) ? cx : 1; m_chunksZ = (cz > 0) ? cz : 1; }

    // Sistema de Pintura de Texturas con Pinceles y Capas Estilo Photoshop (Fase 16 / 17)
    TerrainToolMode activeToolMode = TerrainToolMode::SCULPT;
    TerrainPaintLayer activePaintLayer = TerrainPaintLayer::GRASS;
    float paintRadius = 6.0f;
    float paintStrength = 1.0f;

    void paintTexture(const glm::vec3& worldPos, TerrainPaintLayer layer, float radius, float strength, float dt);
    void resetTexturePaint();
    bool hasCustomSplatmap() const { return m_hasCustomSplatmap; }
    GLuint getSplatmapTexture() const { return m_splatmapTex; }

    // Gestión de Capas PBR de Textura
    std::vector<TerrainLayer>& getLayers() { return m_layers; }
    const std::vector<TerrainLayer>& getLayers() const { return m_layers; }
    int getLayerCount() const { return static_cast<int>(m_layers.size()); }
    TerrainLayer& getLayer(int idx) { return m_layers[idx]; }
    const TerrainLayer& getLayer(int idx) const { return m_layers[idx]; }

    int addLayer(const std::string& name = "Nueva Capa PBR");
    bool removeLayer(int index);
    bool moveLayerUp(int index);
    bool moveLayerDown(int index);

    bool loadLayerAlbedo(int layerIndex, const std::string& path);
    bool loadLayerNormal(int layerIndex, const std::string& path);
    bool loadLayerRoughSpec(int layerIndex, const std::string& path);
    bool loadLayerHeight(int layerIndex, const std::string& path);
    bool loadLayerAO(int layerIndex, const std::string& path);
    void clearLayerTexture(int layerIndex, int slot); // 0=Albedo, 1=Normal, 2=RoughSpec, 3=Height, 4=AO
    void fillSplatmapWithLayer(int layerIndex);

    // Control de Reflejos IBL del Terreno
    float getReflectionIntensity() const { return m_reflectionIntensity; }
    void setReflectionIntensity(float val) { m_reflectionIntensity = (val >= 0.0f) ? val : 0.0f; }

    // Modo de Relieve 3D de Superficie (Teselación OASIS vs POM)
    TerrainReliefMode getReliefMode() const { return m_reliefMode; }
    void setReliefMode(TerrainReliefMode mode) { m_reliefMode = mode; }
    float getTessellationDisplacementScale() const { return m_tessellationDisplacementScale; }
    void setTessellationDisplacementScale(float scale) { m_tessellationDisplacementScale = (scale >= 0.0f) ? scale : 0.0f; }

    // Generación procedural
    void generatePreset(TerrainPreset preset);
    void generateProcedural(const TerrainSettings& settings);
    void resetFlat();
    void resetToDefaults();

    // Escultura interactiva en el Viewport 3D
    void sculpt(const glm::vec3& worldPos, float radius, float strength, 
                SculptBrushType type, SculptFalloff falloff, float targetHeight, float dt);

    // Raycasting en CPU para selección e intersección del cursor del pincel
    bool raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, 
                 glm::vec3& outHitPos, glm::vec3& outHitNormal, float maxDist = 250.0f) const;

    // Integración con Bullet Physics 3
    void initPhysics(PhysicsWorld* physicsWorld);
    void syncPhysics();
    void setPhysicsEnabled(bool enabled);
    bool isPhysicsEnabled() const { return m_physicsEnabled; }

    // Consultas de altura y normales en coordenadas de mundo
    float getHeightAtWorld(float worldX, float worldZ) const;
    glm::vec3 getNormalAtWorld(float worldX, float worldZ) const;

    // Acceso al grid y datos para Deshacer/Rehacer
    float getHeight(int x, int z) const;
    void setHeight(int x, int z, float h);
    const std::vector<float>& getHeights() const { return m_heights; }
    const std::vector<uint8_t>& getSplatmapData() const { return m_splatmapData; }
    void restoreHeightsAndSplatmap(const std::vector<float>& heights, const std::vector<uint8_t>& splatmap);

    // Getters y Setters de configuración
    int getResolution() const { return m_resolution; }
    float getWorldSize() const { return m_worldSize; }
    float getMinHeight() const { return m_minHeight; }
    float getMaxHeight() const { return m_maxHeight; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    bool isDirty() const { return m_isDirty; }
    void setDirty(bool dirty) { m_isDirty = dirty; }
    void updateNormalsAndVBO();
    void updateMinMaxHeight();

    TerrainSettings& getSettings() { return m_settings; }
    const TerrainSettings& getSettings() const { return m_settings; }

    HeightFogSettings& getFogSettings() { return m_fogSettings; }
    const HeightFogSettings& getFogSettings() const { return m_fogSettings; }

    // Filtrado Anisotrópico de Texturas PBR
    void setAnisotropyLevel(int level);
    int getAnisotropyLevel() const { return m_anisotropyLevel; }

    // Parámetros activos del pincel de escultura
    SculptBrushType activeBrushType = SculptBrushType::RAISE;
    SculptFalloff activeBrushFalloff = SculptFalloff::SMOOTH;
    float brushRadius = 7.5f;
    float brushStrength = 1.2f;
    float brushTargetHeight = 0.0f;
    bool isSculpting = false;

private:
    void buildGridMesh();
    void initSplatmap();

    // Algoritmos de Ruido Procedural
    float noise2D(float x, float y) const;
    float fbm(float x, float y, int octaves, float persistence, float frequency) const;
    float ridgedNoise(float x, float y, int octaves, float frequency) const;

    int m_resolution;
    float m_worldSize;
    float m_minHeight;
    float m_maxHeight;
    float m_baseOffset;
    int m_chunksX;
    int m_chunksZ;
    bool m_hasCustomSplatmap;
    bool m_visible;
    bool m_isDirty;

    std::vector<float> m_heights;
    std::vector<TerrainVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<uint8_t> m_splatmapData; // RGBA8

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    GLuint m_splatmapTex;

    // Bullet Physics
    PhysicsWorld* m_physicsWorld;
    btHeightfieldTerrainShape* m_terrainShape;
    btRigidBody* m_terrainBody;
    btDefaultMotionState* m_motionState;
    bool m_physicsEnabled;

    std::vector<TerrainLayer> m_layers;
    float m_reflectionIntensity;
    int m_anisotropyLevel;
    TerrainReliefMode m_reliefMode;
    float m_tessellationDisplacementScale;
    TerrainSettings m_settings;
    HeightFogSettings m_fogSettings;
    std::unique_ptr<OASISTerrain> m_oasis;
    float m_viewportAspectRatio = 16.0f / 9.0f;
};

} // namespace ASIS
