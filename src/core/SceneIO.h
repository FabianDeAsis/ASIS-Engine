#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Scene.h"
#include "Entity.h"
#include "Camera.h"

namespace ASIS {

class SceneIO {
public:
    // Diálogos nativos del Explorador de Archivos de Windows
    static std::string openFileDialog(const char* filter = "Modelos 3D (*.obj;*.gltf;*.glb;*.fbx;*.stl;*.ply;*.dae)\0*.obj;*.gltf;*.glb;*.fbx;*.stl;*.ply;*.dae\0Todos los archivos (*.*)\0*.*\0");
    static std::string saveFileDialog(const char* filter = "Wavefront OBJ (*.obj)\0*.obj\0glTF 2.0 (*.gltf)\0*.gltf\0", const char* defaultExt = "obj");

    // Importación multiformato (.obj, .gltf, .glb, .fbx, .stl, .ply, .dae, .3ds)
    static std::shared_ptr<Entity> importModel(Scene& scene, const std::string& filePath, glm::vec3 spawnPosition = glm::vec3(0.0f, 0.0f, 0.0f));

    // Estructura de Capa PBR del Terreno para Guardado/Carga
    struct TerrainLayerSaveData {
        std::string name;
        bool enabled = true;
        bool visible = true;
        float opacity = 1.0f;
        float tiling = 15.0f;
        float roughness = 0.8f;
        float specular = 0.5f;
        float metallic = 0.0f;
        float normalStrength = 1.0f;
        float heightScale = 0.03f;
        float aoStrength = 1.0f;
        glm::vec3 tint = glm::vec3(1.0f);
        std::string albedoPath;
        std::string normalPath;
        std::string roughSpecPath;
        std::string heightPath;
        std::string aoPath;
    };

    // Formato Nativo de Escena y Proyecto Completo ASIS Engine (.asis)
    struct ASISSceneLoadData {
        // 1. Iluminación Solar y Sombras
        float sunAzimuth = 45.0f;
        float sunElevation = 50.0f;
        glm::vec3 sunColor = glm::vec3(1.0f, 0.96f, 0.88f);
        float sunIntensity = 1.2f;
        glm::vec3 ambientColor = glm::vec3(0.24f, 0.27f, 0.35f);
        float ambientIntensity = 1.0f;
        bool shadowsEnabled = true;
        int shadowRes = 4096;
        int shadowKernel = 3;
        float shadowSoftness = 0.57f;
        float shadowDistance = 85.0f;
        bool terrainCastsShadows = false;
        int shadowFilterMode = 1; // 0 = PCF, 1 = PCSS (Predeterminado)
        float pcssLightSize = 0.35f;
        int pcssBlockerSamples = 16;
        int pcssFilterSamples = 32;
        bool csmEnabled = true;

        // 2. Gestión de Color y Adaptación Ocular
        int colorProfile = 0;
        float exposure = 0.52f;
        float gamma = 2.2f;
        float saturation = 1.0f;
        float contrast = 1.0f;
        bool autoExposureEnabled = true;
        float autoExposureKey = 0.39f;
        float autoExposureMin = 0.50f;
        float autoExposureMax = 3.24f;
        float autoExposureSpeedUp = 3.0f;
        float autoExposureSpeedDown = 1.0f;

        // 3. Cielo HDRI y Entorno
        bool useIBL = true;
        bool showSkybox = true;
        float skyboxExposure = 0.64f;
        float skyboxRotation = 0.0f;
        float skyboxBlur = 0.0f;
        float iblIntensity = 1.0f;
        bool flipSkyboxX = false;
        bool flipSkyboxY = false;
        std::string hdriPath = "";

        // 4. Post-Proceso y Gráficos
        bool bloomEnabled = true;
        float bloomThreshold = 0.65f;
        float bloomIntensity = 0.60f;
        bool fxaaEnabled = true;
        int msaaLevel = 4;
        bool ssaoEnabled = false;
        float ssaoRadius = 0.50f;
        float ssaoBias = 0.035f;
        float ssaoPower = 1.8f;
        float ssaoIntensity = 0.85f;
        bool dofEnabled = false;
        float dofFocusDistance = 6.0f;
        float dofFocusRange = 3.0f;
        float dofBokehRadius = 8.0f;
        int anisotropyLevel = 16;
        bool showGroundPlane = true;
        bool showGroundGrid = true;

        // 5. Terreno 3D
        bool terrainVisible = true;
        float terrainBaseOffset = -8.0f;
        float terrainReflectionIntensity = 1.0f;
        int terrainReliefMode = 0; // 0 = Teselacion OASIS, 1 = POM, 2 = Desactivado
        float terrainTessellationScale = 1.0f;
        int terrainResolution = 128;
        float terrainWorldSize = 120.0f;
        std::vector<float> terrainHeights;
        std::vector<uint8_t> terrainSplatmap;
        std::vector<TerrainLayerSaveData> terrainLayers;

        // 6. Sistema OASIS (Fase 18)
        bool oasisEnabled = true;
        bool oasisFrustumCulling = true;
        bool oasisGeomorphing = true;
        bool oasisDebugLOD = false;
        float oasisLodDistanceFactor = 1.0f;
        int oasisMaxLOD = 6;
        int oasisPatchResolution = 33;
        bool meshletsEnabled = true;
        bool meshletFrustumCulling = true;
        bool meshletBackfaceCulling = true;
        bool meshletDebugColors = false;

        // 7. Océano y Cuerpos de Agua 3D
        bool waterEnabled = true;
        float waterLevel = -3.5f;
        float waterWaveAmplitude = 0.45f;
        float waterWaveSpeed = 1.0f;
        glm::vec3 waterShallowColor = glm::vec3(0.08f, 0.58f, 0.65f);
        glm::vec3 waterDeepColor = glm::vec3(0.01f, 0.08f, 0.28f);
        float waterClarity = 0.22f;
        float waterFoamThreshold = 1.2f;
        float waterFoamIntensity = 1.4f;
        float waterRefraction = 0.035f;
        float waterRoughness = 0.06f;
        float waterHorizonDistance = 330.0f;
        float waterHorizonBlend = 0.40f;

        // 8. Fase 20: Ciclo Día / Noche, Clima, SSR y Contact Shadows
        bool dayNightCycleEnabled = true;
        float dayNightTimeOfDay = 12.0f;
        float dayNightDuration = 180.0f;
        float dayNightSunLatitude = 35.0f;
        float dayNightHorizonHeight = 0.0f;
        int weatherType = 0;
        float weatherWetness = 0.0f;

        bool ssrEnabled = false;
        int ssrMaxSteps = 64;
        float ssrStepSize = 0.15f;
        float ssrThickness = 0.05f;
        float ssrIntensity = 0.85f;

        bool contactShadowsEnabled = false;
        float contactShadowsDistance = 0.25f;
        int contactShadowsRaySteps = 16;
        float contactShadowsIntensity = 0.75f;

        // Neblina Atmosférica Global (Fase 20)
        bool globalFogEnabled = true;
        float globalFogStart = 20.0f;
        float globalFogEnd = 450.0f;
        float globalFogDensity = 0.45f;
        glm::vec3 globalFogColor = glm::vec3(0.70f, 0.78f, 0.88f);
    };

    static bool saveProjectASIS(const Scene& scene, const Camera& camera,
                               const ASISSceneLoadData& projectData,
                               const std::string& filePath);

    static bool saveSceneASIS(const Scene& scene, const Camera& camera,
                              float sunAzimuth, float sunElevation, const glm::vec3& sunColor, float sunIntensity,
                              const glm::vec3& ambientColor, float ambientIntensity, bool shadowsEnabled,
                              int shadowRes, int shadowKernel, float shadowSoftness,
                              int colorProfile, float exposure, float gamma, float saturation, float contrast,
                              const std::string& filePath,
                              bool autoExposureEnabled = true, float autoExposureKey = 0.39f, float autoExposureMin = 0.50f,
                              float autoExposureMax = 3.24f, float autoExposureSpeedUp = 3.0f, float autoExposureSpeedDown = 1.0f);

    static bool loadSceneASIS(Scene& scene, Camera& camera, ASISSceneLoadData& outData, const std::string& filePath);
    static bool loadSceneASIS(Scene& scene, Camera& camera, const std::string& filePath) {
        ASISSceneLoadData dummy;
        return loadSceneASIS(scene, camera, dummy, filePath);
    }

    // Exportación a formatos universales 3D
    static bool exportSceneToOBJ(const Scene& scene, const std::string& filePath);
    static bool exportEntityToOBJ(const Entity& entity, const std::string& filePath);
    static bool exportSceneToGLTF(const Scene& scene, const std::string& filePath);

    // Utilidades
    static bool isSupported3DFormat(const std::string& filePath);
    static std::string getFileExtension(const std::string& filePath);
};

} // namespace ASIS
