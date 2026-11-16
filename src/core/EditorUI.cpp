#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "EditorUI.h"
#include "PhysicsWorld.h"
#include "SceneIO.h"
#include "PrefabManager.h"
#include "MaterialManager.h"
#include "ShadowMap.h"
#include "Material.h"
#include "Skybox.h"
#include "ParticleSystem.h"
#include "SSAO.h"
#include "Animation.h"
#include "Terrain.h"
#include "Water.h"
#include "CharacterController.h"
#include "AudioEngine.h"
#include "Sequencer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <cstdlib>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace ASIS {

EditorUI::EditorUI()
    : m_initialized(false),
      m_visible(true),
      m_layoutBuilt(false),
      m_selectedEntityName(""),
      m_activePropertyTab(BlenderPropertyTab::OBJECT),
      m_fps(60.0f),
      m_frameTimeMs(16.6f),
      m_fpsUpdateTimer(0.0f),
      m_mainFont(nullptr),
      m_monoFont(nullptr),
      m_viewportSize(1280.0f, 720.0f),
      m_viewportPos(0.0f, 0.0f),
      m_isViewportHovered(false),
      m_isViewportFocused(false),
      m_isUsingGizmo(false),
      m_scriptsPaused(false),
      m_gizmoOperation(ImGuizmo::TRANSLATE),
      m_gizmoMode(ImGuizmo::WORLD),
      m_useSnap(false),
      m_snapTranslation(0.5f),
      m_snapRotation(15.0f),
      m_snapScale(0.1f),
      m_shadowsEnabled(true),
      m_sunAzimuth(-132.9f),
      m_sunElevation(62.7f),
      m_lightColor(1.0f, 245.0f / 255.0f, 224.0f / 255.0f),
      m_lightIntensity(1.20f),
      m_ambientColor(61.0f / 255.0f, 69.0f / 255.0f, 89.0f / 255.0f),
      m_ambientIntensity(1.00f),
      m_shadowBias(0.0001f),
      m_shadowResolution(4096),
      m_shadowKernel(3),
      m_shadowSoftness(0.57f),
      m_shadowDistance(85.0f),
      m_terrainCastsShadows(false),
      m_anisotropyLevel(16),
      m_shadowFilterMode(1),
      m_pcssLightSize(0.35f),
      m_pcssBlockerSamples(16),
      m_pcssFilterSamples(32),
      m_csmEnabled(true),
      m_csmDebugColors(false),
      m_isGenerating(false),
      m_generationStatus("Listo. Escribe una idea para que Gemini construya la escena en 3D."),
      m_generationTimer(0.0f),
      m_colorProfile(0),
      m_exposure(0.52f),
      m_gamma(2.2f),
      m_saturation(1.0f),
      m_contrast(1.0f),
      m_autoExposureEnabled(true),
      m_autoExposureKey(0.39f),
      m_autoExposureMin(0.50f),
      m_autoExposureMax(3.24f),
      m_autoExposureSpeedUp(3.0f),
      m_autoExposureSpeedDown(1.0f),
      m_currentAdaptedExposure(1.0f),
      m_measuredSceneLuminance(0.473f),
      m_showGroundPlane(true),
      m_showGroundGrid(true),
      m_selectedLightIndex(-1),
      m_maxUndoSteps(64),
      m_wasUsingGizmo(false),
      m_showPreferencesModal(false),
      m_vsyncEnabled(true),
      m_targetFPS(0),
      m_msaaLevel(4),
      m_currentTheme(UITheme::BLENDER_DARK),
      m_customAccentColor(0.95f, 0.52f, 0.18f, 1.0f),
      m_useBlenderShortcuts(false),
      m_flycamSpeed(6.0f),
      m_autoSaveEnabled(true),
      m_autoSaveIntervalMinutes(5.0f),
      m_autoSaveTimer(0.0f),
      m_openAddMenu(false),
      m_useIBL(true),
      m_showSkybox(true),
      m_skyboxExposure(0.64f),
      m_skyboxRotation(0.0f),
      m_skyboxBlur(0.0f),
      m_iblIntensity(1.0f),
      m_terrainReflectionIntensity(1.0f),
      m_flipSkyboxX(false),
      m_flipSkyboxY(false),
      m_fxaaEnabled(true),
      m_bloomEnabled(true),
      m_bloomThreshold(0.65f),
      m_bloomIntensity(0.60f),
      m_requestRenderImage(false),
      m_lastRenderPath(""),
      m_showRenderNotification(false),
      m_renderNotificationTimer(0.0f),
      m_ssaoEnabled(false),
      m_ssaoRadius(0.50f),
      m_ssaoBias(0.035f),
      m_ssaoPower(1.8f),
      m_ssaoIntensity(0.85f),
      m_dofEnabled(false),
      m_dofFocusDistance(6.0f),
      m_dofFocusRange(3.0f),
      m_dofBokehRadius(8.0f),
      m_selectedEmitterIndex(-1),
      m_playMode(EnginePlayMode::EDITOR),
      m_requestPlayStart(false),
      m_requestPlayStop(false),
      m_savedEditorCameraPos(0.0f, 3.5f, 7.5f),
      m_savedEditorCameraTarget(0.0f, 1.2f, 0.0f),
      m_savedEditorYaw(-90.0f),
      m_savedEditorPitch(-20.0f),
      m_groundPlaneDeleted(false),
      m_hasPrePlaySnapshot(false),
      m_renamingEntityName(""),
      m_renamingGroupName(""),
      m_showGroupCreationModal(false)
{
    m_promptBuffer[0] = '\0';
    m_renameEntityBuf[0] = '\0';
    m_renameGroupBuf[0] = '\0';
    m_newGroupNameBuf[0] = '\0';
    m_sceneSearchFilter[0] = '\0';
    m_luaCommandBuf[0] = '\0';
}

void EditorUI::saveEditorCamera(const Camera& camera) {
    m_savedEditorCameraPos = camera.getPosition();
    m_savedEditorCameraTarget = camera.getTarget();
    m_savedEditorYaw = camera.getYaw();
    m_savedEditorPitch = camera.getPitch();
}

void EditorUI::restoreEditorCamera(Camera& camera) {
    camera.setPosition(m_savedEditorCameraPos);
    camera.setTarget(m_savedEditorCameraTarget);
    camera.setYawPitch(m_savedEditorYaw, m_savedEditorPitch);
}

void EditorUI::selectEntity(const std::string& name) {
    m_selectedEntityName = name;
    m_selectedEntityNames.clear();
    m_selectedGroupName = "";
    if (!name.empty()) {
        m_selectedEntityNames.insert(name);
        m_selectionAnchor = name;
    }
}

bool EditorUI::isEntitySelected(const std::string& name) const {
    if (name.empty()) return false;
    return m_selectedEntityNames.find(name) != m_selectedEntityNames.end() || m_selectedEntityName == name;
}

void EditorUI::toggleEntitySelection(const std::string& name) {
    if (name.empty()) return;
    auto it = m_selectedEntityNames.find(name);
    if (it != m_selectedEntityNames.end()) {
        m_selectedEntityNames.erase(it);
        if (m_selectedEntityName == name) {
            m_selectedEntityName = m_selectedEntityNames.empty() ? "" : *m_selectedEntityNames.begin();
        }
    } else {
        m_selectedEntityNames.insert(name);
        m_selectedEntityName = name;
        m_selectionAnchor = name;
    }
}

void EditorUI::selectEntityRange(const std::string& targetName, const std::vector<std::string>& allNames) {
    if (targetName.empty()) return;
    if (m_selectionAnchor.empty()) {
        selectEntity(targetName);
        return;
    }

    int anchorIdx = -1;
    int targetIdx = -1;
    for (int i = 0; i < static_cast<int>(allNames.size()); ++i) {
        if (allNames[i] == m_selectionAnchor) anchorIdx = i;
        if (allNames[i] == targetName) targetIdx = i;
    }

    if (anchorIdx == -1 || targetIdx == -1) {
        selectEntity(targetName);
        return;
    }

    int start = (std::min)(anchorIdx, targetIdx);
    int end = (std::max)(anchorIdx, targetIdx);
    m_selectedEntityNames.clear();
    for (int i = start; i <= end; ++i) {
        m_selectedEntityNames.insert(allNames[i]);
    }
    m_selectedEntityName = targetName;
}

void EditorUI::clearEntitySelection() {
    m_selectedEntityName = "";
    m_selectedEntityNames.clear();
    m_selectionAnchor = "";
    m_selectedGroupName = "";
}

void EditorUI::drawOutlinerIcon(OutlinerIconType type, const ImVec2& screenPos, float size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float x = screenPos.x;
    float y = screenPos.y;
    float half = size * 0.5f;

    switch (type) {
        case OutlinerIconType::MESH_CUBE: {
            // Cubo 3D isométrico sombreado estilo Blender (Naranja)
            ImVec2 topCenter(x + half, y + size * 0.15f);
            ImVec2 topRight(x + size * 0.90f, y + size * 0.35f);
            ImVec2 center(x + half, y + size * 0.55f);
            ImVec2 topLeft(x + size * 0.10f, y + size * 0.35f);
            ImVec2 botLeft(x + size * 0.10f, y + size * 0.75f);
            ImVec2 botCenter(x + half, y + size * 0.95f);
            ImVec2 botRight(x + size * 0.90f, y + size * 0.75f);

            dl->AddQuadFilled(topCenter, topRight, center, topLeft, ImColor(255, 175, 70));
            dl->AddQuadFilled(topLeft, center, botCenter, botLeft, ImColor(230, 130, 30));
            dl->AddQuadFilled(center, topRight, botRight, botCenter, ImColor(195, 95, 15));

            dl->AddLine(topCenter, topRight, ImColor(20, 20, 20, 220), 1.0f);
            dl->AddLine(topRight, botRight, ImColor(20, 20, 20, 220), 1.0f);
            dl->AddLine(botRight, botCenter, ImColor(20, 20, 20, 220), 1.0f);
            dl->AddLine(botCenter, botLeft, ImColor(20, 20, 20, 220), 1.0f);
            dl->AddLine(botLeft, topLeft, ImColor(20, 20, 20, 220), 1.0f);
            dl->AddLine(topLeft, topCenter, ImColor(20, 20, 20, 220), 1.0f);
            dl->AddLine(center, topCenter, ImColor(20, 20, 20, 180), 1.0f);
            dl->AddLine(center, botCenter, ImColor(20, 20, 20, 180), 1.0f);
            dl->AddLine(center, topLeft, ImColor(20, 20, 20, 180), 1.0f);
            dl->AddLine(center, topRight, ImColor(20, 20, 20, 180), 1.0f);
            break;
        }
        case OutlinerIconType::SPHERE: {
            ImVec2 c(x + half, y + half);
            float r = size * 0.42f;
            dl->AddCircleFilled(c, r, ImColor(40, 160, 220));
            dl->AddCircleFilled(ImVec2(c.x - r * 0.28f, c.y - r * 0.28f), r * 0.45f, ImColor(130, 220, 255));
            dl->AddCircle(c, r, ImColor(20, 20, 20, 220), 16, 1.2f);
            break;
        }
        case OutlinerIconType::CYLINDER: {
            // Cilindro 3D verde esmeralda
            dl->AddRectFilled(ImVec2(x + size * 0.20f, y + size * 0.30f), ImVec2(x + size * 0.80f, y + size * 0.85f), ImColor(40, 175, 110));
            dl->AddRect(ImVec2(x + size * 0.20f, y + size * 0.30f), ImVec2(x + size * 0.80f, y + size * 0.85f), ImColor(20, 20, 20, 220), 0.0f, 0, 1.0f);
            dl->AddEllipseFilled(ImVec2(x + half, y + size * 0.30f), ImVec2(size * 0.30f, size * 0.15f), ImColor(80, 210, 140));
            dl->AddEllipse(ImVec2(x + half, y + size * 0.30f), ImVec2(size * 0.30f, size * 0.15f), ImColor(20, 20, 20, 220), 0.0f, 16, 1.0f);
            break;
        }
        case OutlinerIconType::CONE: {
            ImVec2 apex(x + half, y + size * 0.15f);
            ImVec2 bl(x + size * 0.15f, y + size * 0.85f);
            ImVec2 br(x + size * 0.85f, y + size * 0.85f);
            dl->AddTriangleFilled(apex, bl, br, ImColor(230, 160, 40));
            dl->AddTriangle(apex, bl, br, ImColor(20, 20, 20, 220), 1.0f);
            break;
        }
        case OutlinerIconType::TORUS: {
            ImVec2 c(x + half, y + half);
            dl->AddCircleFilled(c, size * 0.44f, ImColor(180, 90, 220));
            dl->AddCircleFilled(c, size * 0.20f, ImColor(30, 34, 42)); // Agujero
            dl->AddCircle(c, size * 0.44f, ImColor(20, 20, 20, 220), 16, 1.0f);
            dl->AddCircle(c, size * 0.20f, ImColor(20, 20, 20, 220), 16, 1.0f);
            break;
        }
        case OutlinerIconType::PLANE: {
            ImVec2 top(x + half, y + size * 0.25f);
            ImVec2 right(x + size * 0.92f, y + half);
            ImVec2 bot(x + half, y + size * 0.75f);
            ImVec2 left(x + size * 0.08f, y + half);
            dl->AddQuadFilled(top, right, bot, left, ImColor(70, 110, 160));
            dl->AddQuad(top, right, bot, left, ImColor(140, 180, 230), 1.2f);
            dl->AddLine(ImVec2(x + size * 0.29f, y + size * 0.375f), ImVec2(x + size * 0.71f, y + size * 0.625f), ImColor(120, 160, 210, 180));
            dl->AddLine(ImVec2(x + size * 0.71f, y + size * 0.375f), ImVec2(x + size * 0.29f, y + size * 0.625f), ImColor(120, 160, 210, 180));
            break;
        }
        case OutlinerIconType::LIGHT_POINT: {
            ImVec2 c(x + half, y + size * 0.38f);
            float r = size * 0.30f;
            dl->AddCircleFilled(c, r, ImColor(255, 220, 60));
            dl->AddCircle(c, r, ImColor(230, 180, 20), 16, 1.2f);
            dl->AddRectFilled(ImVec2(x + half - 2.5f, y + size * 0.65f), ImVec2(x + half + 2.5f, y + size * 0.88f), ImColor(180, 190, 200));
            dl->AddLine(ImVec2(x + half - 2.5f, y + size * 0.73f), ImVec2(x + half + 2.5f, y + size * 0.73f), ImColor(80, 80, 80));
            dl->AddLine(ImVec2(x + half - 2.5f, y + size * 0.81f), ImVec2(x + half + 2.5f, y + size * 0.81f), ImColor(80, 80, 80));
            dl->AddLine(ImVec2(c.x, c.y - r - 2.0f), ImVec2(c.x, c.y - r - 4.5f), ImColor(255, 235, 120), 1.2f);
            dl->AddLine(ImVec2(c.x - r - 2.0f, c.y), ImVec2(c.x - r - 4.5f, c.y), ImColor(255, 235, 120), 1.2f);
            dl->AddLine(ImVec2(c.x + r + 2.0f, c.y), ImVec2(c.x + r + 4.5f, c.y), ImColor(255, 235, 120), 1.2f);
            break;
        }
        case OutlinerIconType::LIGHT_SPOT: {
            ImVec2 apex(x + half, y + size * 0.15f);
            ImVec2 bl(x + size * 0.15f, y + size * 0.88f);
            ImVec2 br(x + size * 0.85f, y + size * 0.88f);
            dl->AddTriangleFilled(apex, bl, br, ImColor(255, 200, 60, 180));
            dl->AddLine(apex, bl, ImColor(255, 220, 80), 1.2f);
            dl->AddLine(apex, br, ImColor(255, 220, 80), 1.2f);
            dl->AddLine(bl, br, ImColor(255, 240, 140), 1.5f);
            dl->AddCircleFilled(apex, 2.5f, ImColor(255, 255, 220));
            break;
        }
        case OutlinerIconType::TERRAIN: {
            ImVec2 p1(x + size * 0.10f, y + size * 0.88f);
            ImVec2 top1(x + size * 0.42f, y + size * 0.20f);
            ImVec2 valley(x + size * 0.60f, y + size * 0.65f);
            ImVec2 top2(x + size * 0.78f, y + size * 0.35f);
            ImVec2 p2(x + size * 0.95f, y + size * 0.88f);

            dl->AddTriangleFilled(p1, top1, valley, ImColor(45, 160, 95));
            dl->AddTriangleFilled(valley, top2, p2, ImColor(35, 130, 75));
            dl->AddTriangleFilled(ImVec2(x + size * 0.33f, y + size * 0.38f), top1, ImVec2(x + size * 0.50f, y + size * 0.38f), ImColor(245, 250, 255));
            dl->AddTriangleFilled(ImVec2(x + size * 0.72f, y + size * 0.48f), top2, ImVec2(x + size * 0.84f, y + size * 0.48f), ImColor(245, 250, 255));
            break;
        }
        case OutlinerIconType::FOLDER: {
            float fx = x + 2.0f;
            float fy = y + 3.0f;
            float fw = size - 4.0f;
            float fh = size - 6.0f;
            dl->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + fw * 0.45f, fy + fh * 0.35f), ImColor(230, 180, 50), 2.0f);
            dl->AddRectFilled(ImVec2(fx, fy + fh * 0.22f), ImVec2(fx + fw, fy + fh), ImColor(245, 200, 60), 2.0f);
            dl->AddRect(ImVec2(fx, fy + fh * 0.22f), ImVec2(fx + fw, fy + fh), ImColor(180, 140, 30), 2.0f, 0, 1.0f);
            break;
        }
        case OutlinerIconType::MODEL:
        default: {
            ImVec2 topCenter(x + half, y + size * 0.15f);
            ImVec2 topRight(x + size * 0.90f, y + size * 0.35f);
            ImVec2 center(x + half, y + size * 0.55f);
            ImVec2 topLeft(x + size * 0.10f, y + size * 0.35f);
            ImVec2 botLeft(x + size * 0.10f, y + size * 0.75f);
            ImVec2 botCenter(x + half, y + size * 0.95f);
            ImVec2 botRight(x + size * 0.90f, y + size * 0.75f);

            dl->AddQuadFilled(topCenter, topRight, center, topLeft, ImColor(160, 110, 230));
            dl->AddQuadFilled(topLeft, center, botCenter, botLeft, ImColor(130, 80, 200));
            dl->AddQuadFilled(center, topRight, botRight, botCenter, ImColor(105, 60, 170));
            dl->AddLine(topCenter, topRight, ImColor(220, 190, 255), 1.0f);
            dl->AddLine(topRight, botRight, ImColor(220, 190, 255), 1.0f);
            dl->AddLine(botRight, botCenter, ImColor(220, 190, 255), 1.0f);
            dl->AddLine(botCenter, botLeft, ImColor(220, 190, 255), 1.0f);
            dl->AddLine(botLeft, topLeft, ImColor(220, 190, 255), 1.0f);
            dl->AddLine(topLeft, topCenter, ImColor(220, 190, 255), 1.0f);
            break;
        }
    }
}

void EditorUI::openRendersFolder() {
#ifdef _WIN32
    std::filesystem::path dir = std::filesystem::absolute("renders");
    std::filesystem::create_directories(dir);
    ShellExecuteA(NULL, "open", dir.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
}

void EditorUI::openFileWithDefaultApp(const std::string& path) {
#ifdef _WIN32
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
}

void EditorUI::notifyRenderComplete(const std::string& fullPath) {
    m_lastRenderPath = fullPath;
    m_showRenderNotification = true;
    m_renderNotificationTimer = 6.0f;
}

EditorUI::~EditorUI() {
    shutdown();
}

glm::vec3 EditorUI::getLightDir() const {
    if (m_dayNight.isCycleEnabled()) {
        if (m_dayNight.isNight()) {
            glm::vec3 moonDir = m_dayNight.getMoonDirection();
            // Asegurar que apunte hacia abajo con elevacion minima equivalente de 12°
            float minDownY = -std::sin(glm::radians(12.0f)); // ~ -0.20791f
            if (moonDir.y > minDownY) {
                moonDir.y = minDownY;
                moonDir = glm::normalize(moonDir);
            }
            return moonDir;
        }
        return m_dayNight.getSunDirection();
    }

    // Modo manual (ciclo deshabilitado): misma formula trigonometrica unificada con Skybox y DayNightCycle
    float azRad = glm::radians(m_sunAzimuth);
    float elRad = glm::radians(glm::clamp(m_sunElevation, 2.0f, 89.0f));
    glm::vec3 sunDir(
        std::cos(elRad) * std::cos(azRad),
        std::sin(elRad),
        std::cos(elRad) * std::sin(azRad)
    );
    return -glm::normalize(sunDir);
}

void EditorUI::init(GLFWwindow* window) {
    if (m_initialized) return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 1. Configurar fuentes vectoriales Segoe UI (Windows 11) y Consolas
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Latin básico + caracteres en español (á, é, í, ó, ú, ñ, etc.)
        0x0100, 0x017F, // Latin extendido-A
        0x2000, 0x206F, // Puntuación
        0,
    };

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    fontConfig.PixelSnapH = true;

    if (std::filesystem::exists("C:\\Windows\\Fonts\\segoeui.ttf")) {
        m_mainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.0f, &fontConfig, ranges);
    } else {
        m_mainFont = io.Fonts->AddFontDefault();
    }

    // Fallback de símbolos de Windows (Segoe UI Symbol) fusionado
    static const ImWchar symRanges[] = {
        0x2000, 0x27FF, // Puntuación general, flechas, formas geométricas, símbolos varios
        0x2B00, 0x2BFF, // Símbolos y flechas adicionales
        0,
    };
    if (std::filesystem::exists("C:\\Windows\\Fonts\\seguisym.ttf")) {
        ImFontConfig symConfig = fontConfig;
        symConfig.MergeMode = true;
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisym.ttf", 16.0f, &symConfig, symRanges);
    }

    if (std::filesystem::exists("C:\\Windows\\Fonts\\consola.ttf")) {
        m_monoFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 15.0f, &fontConfig, ranges);
    } else {
        m_monoFont = m_mainFont;
    }

    // 2. Aplicar tema oscuro profesional estilo Unreal Engine 5 / Blender
    applyDarkTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    m_initialized = true;
    std::cout << "[ASIS Engine] Editor UI con Panel de Propiedades estilo Blender inicializada.\n";
}

void EditorUI::applyBlenderDarkTheme() {
    m_currentTheme = UITheme::BLENDER_DARK;
    ImGuiStyle& style = ImGui::GetStyle();

    // Métricas compactas de alta densidad (Fase 21.5 - Estilo Blender 4/5)
    style.WindowPadding     = ImVec2(6.0f, 6.0f);
    style.FramePadding      = ImVec2(4.0f, 2.0f); // Botones e inputs compactos de 18-20px de alto
    style.ItemSpacing       = ImVec2(6.0f, 3.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 2.0f);
    style.IndentSpacing     = 12.0f;
    style.ScrollbarSize     = 10.0f;
    style.WindowRounding    = 4.0f;
    style.ChildRounding     = 3.0f;
    style.FrameRounding     = 3.0f;
    style.PopupRounding     = 3.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 3.0f;
    style.FrameBorderSize   = 0.0f;
    style.WindowBorderSize  = 1.0f;

    ImVec4* colors = style.Colors;

    // Paleta Oficial Blender Dark
    colors[ImGuiCol_WindowBg]           = ImVec4(0.157f, 0.157f, 0.157f, 1.0f); // #282828
    colors[ImGuiCol_ChildBg]            = ImVec4(0.133f, 0.133f, 0.133f, 1.0f); // #222222
    colors[ImGuiCol_PopupBg]            = ImVec4(0.157f, 0.157f, 0.157f, 0.98f);
    colors[ImGuiCol_Border]             = ImVec4(0.200f, 0.200f, 0.200f, 0.70f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_FrameBg]            = ImVec4(0.114f, 0.114f, 0.114f, 1.0f); // #1D1D1D
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.165f, 0.165f, 0.165f, 1.0f); // #2A2A2A
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.180f, 0.180f, 0.180f, 1.0f);

    colors[ImGuiCol_TitleBg]            = ImVec4(0.133f, 0.133f, 0.133f, 1.0f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.157f, 0.157f, 0.157f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.133f, 0.133f, 0.133f, 0.75f);

    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.133f, 0.133f, 0.133f, 1.0f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.133f, 0.133f, 0.133f, 0.6f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive]= ImVec4(0.50f, 0.50f, 0.50f, 1.0f);

    // Azul Blender oficial (#4772B3)
    const ImVec4 blenderBlue(0.278f, 0.447f, 0.702f, 1.0f);
    colors[ImGuiCol_CheckMark]          = blenderBlue;
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_SliderGrabActive]   = blenderBlue;

    // Botones oscuros (#383838) con hover (#484848) y activo (#4772B3)
    colors[ImGuiCol_Button]             = ImVec4(0.220f, 0.220f, 0.220f, 1.0f); // #383838
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.282f, 0.282f, 0.282f, 1.0f); // #484848
    colors[ImGuiCol_ButtonActive]       = blenderBlue;

    // Encabezados discretos (#333333) sin azul estridente por defecto
    colors[ImGuiCol_Header]             = ImVec4(0.200f, 0.200f, 0.200f, 1.0f); // #333333
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.247f, 0.247f, 0.247f, 1.0f); // #3F3F3F
    colors[ImGuiCol_HeaderActive]       = blenderBlue;

    // Pestañas (Tabs)
    colors[ImGuiCol_Tab]                = ImVec4(0.180f, 0.180f, 0.180f, 1.0f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.282f, 0.282f, 0.282f, 1.0f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.220f, 0.220f, 0.220f, 1.0f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.140f, 0.140f, 0.140f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.180f, 0.180f, 0.180f, 1.0f);

    colors[ImGuiCol_DockingPreview]     = ImVec4(blenderBlue.x, blenderBlue.y, blenderBlue.z, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

    colors[ImGuiCol_Text]               = ImVec4(0.88f, 0.88f, 0.88f, 1.0f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
}

void EditorUI::applyTheme(UITheme theme) {
    if (theme == UITheme::BLENDER_DARK) {
        applyBlenderDarkTheme();
        return;
    }
    m_currentTheme = theme;
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 5.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);

    ImVec4 accent = m_customAccentColor;

    if (theme == UITheme::UNREAL_5) {
        accent = ImVec4(0.00f, 0.48f, 0.88f, 1.00f); // Azul Unreal
        colors[ImGuiCol_WindowBg]       = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBg]        = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgActive]  = ImVec4(0.12f, 0.18f, 0.28f, 1.00f);
    } else if (theme == UITheme::OLED_MIDNIGHT) {
        accent = ImVec4(0.20f, 0.80f, 0.50f, 1.00f); // Verde OLED
        colors[ImGuiCol_WindowBg]       = ImVec4(0.02f, 0.02f, 0.03f, 1.00f);
        colors[ImGuiCol_FrameBg]        = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgActive]  = ImVec4(0.08f, 0.12f, 0.10f, 1.00f);
    } else if (theme == UITheme::CYBERPUNK) {
        accent = ImVec4(0.95f, 0.15f, 0.60f, 1.00f); // Magenta Neón
        colors[ImGuiCol_WindowBg]       = ImVec4(0.09f, 0.07f, 0.14f, 1.00f);
        colors[ImGuiCol_FrameBg]        = ImVec4(0.15f, 0.11f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBgActive]  = ImVec4(0.25f, 0.10f, 0.35f, 1.00f);
    } else {
        // CUSTOM
        colors[ImGuiCol_WindowBg]       = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
        colors[ImGuiCol_FrameBg]        = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
        colors[ImGuiCol_TitleBgActive]  = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    }

    colors[ImGuiCol_CheckMark]          = accent;
    colors[ImGuiCol_SliderGrab]         = accent;
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(accent.x * 1.15f, accent.y * 1.15f, accent.z * 1.15f, 1.0f);
    colors[ImGuiCol_ButtonActive]       = accent;
    colors[ImGuiCol_HeaderActive]       = accent;
    colors[ImGuiCol_TabActive]          = ImVec4(accent.x * 0.7f, accent.y * 0.7f, accent.z * 0.7f, 1.0f);
    colors[ImGuiCol_DockingPreview]     = accent;
    colors[ImGuiCol_Border]             = ImVec4(0.22f, 0.24f, 0.28f, 0.65f);
}

void EditorUI::applyDarkTheme() {
    applyBlenderDarkTheme();
}

static SceneSnapshot captureSceneState(const Scene& scene, Terrain* terrain) {
    SceneSnapshot snap;
    const auto& order = scene.getEntityOrder();
    const auto& entities = scene.getEntities();

    for (const auto& name : order) {
        auto it = entities.find(name);
        if (it == entities.end()) continue;
        const auto& ent = it->second;

        EntitySnapshot e;
        e.name = ent->getName();
        e.primitiveType = ent->getPrimitiveType();
        e.modelPath = ent->getModelPath();
        e.position = ent->getPosition();
        e.rotation = ent->getRotation();
        e.scale = ent->getScale();
        e.color = ent->getColor();
        e.hasCustomColor = ent->hasCustomColor();
        e.visible = ent->isVisible();
        e.group = ent->getGroup();
        e.isTrigger = ent->isTrigger();
        e.triggerTag = ent->getTriggerTag();

        auto mat = ent->getMaterial();
        if (mat) {
            e.matAlbedo = mat->albedo;
            e.matMetallic = mat->metallic;
            e.matRoughness = mat->roughness;
            e.matSpecular = mat->specular;
            e.matEmission = mat->emission;
            e.matEmissionStrength = mat->emissionStrength;
            e.matNormalStrength = mat->normalStrength;
            e.matAO = mat->ao;
        } else {
            e.matAlbedo = glm::vec3(1.0f);
            e.matMetallic = 0.0f;
            e.matRoughness = 0.5f;
            e.matSpecular = 0.5f;
            e.matEmission = glm::vec3(0.0f);
            e.matEmissionStrength = 1.0f;
            e.matNormalStrength = 1.0f;
            e.matAO = 1.0f;
        }
        snap.entities.push_back(e);
    }
    snap.lights = scene.getPointLights();

    if (terrain && terrain->isVisible()) {
        snap.hasTerrainData = true;
        snap.terrainHeights = terrain->getHeights();
        snap.terrainSplatmap = terrain->getSplatmapData();
    } else {
        snap.hasTerrainData = false;
    }
    return snap;
}

static void restoreSceneState(const SceneSnapshot& snap, Scene& scene, PhysicsWorld& physicsWorld, Terrain* terrain) {
    scene.clear();
    for (const auto& e : snap.entities) {
        std::shared_ptr<Entity> ent = nullptr;
        if (e.primitiveType == "Sphere") ent = scene.createSphere(e.name);
        else if (e.primitiveType == "Cylinder") ent = scene.createCylinder(e.name);
        else if (e.primitiveType == "Cone") ent = scene.createCone(e.name);
        else if (e.primitiveType == "Torus") ent = scene.createTorus(e.name);
        else if (e.primitiveType == "Plane") ent = scene.createPlane(e.name);
        else if (e.primitiveType == "Model" && !e.modelPath.empty()) ent = scene.createModel(e.name, e.modelPath);
        else ent = scene.createCube(e.name);

        if (ent) {
            ent->setPosition(e.position.x, e.position.y, e.position.z);
            ent->setRotation(e.rotation.x, e.rotation.y, e.rotation.z);
            ent->setScale(e.scale.x, e.scale.y, e.scale.z);
            ent->setColor(e.color.r, e.color.g, e.color.b);
            ent->setHasCustomColor(e.hasCustomColor);
            ent->setVisible(e.visible);
            ent->setGroup(e.group);
            ent->setTrigger(e.isTrigger);
            ent->setTriggerTag(e.triggerTag);
            if (e.isTrigger) {
                physicsWorld.setBodyTrigger(e.name, true);
            }
            auto mat = ent->getMaterial();
            if (mat) {
                mat->albedo = e.matAlbedo;
                mat->metallic = e.matMetallic;
                mat->roughness = e.matRoughness;
                mat->specular = e.matSpecular;
                mat->emission = e.matEmission;
                mat->emissionStrength = e.matEmissionStrength;
                mat->normalStrength = e.matNormalStrength;
                mat->ao = e.matAO;
            }
            physicsWorld.syncEntityToPhysics(ent);
        }
    }
    scene.getPointLights() = snap.lights;

    if (snap.hasTerrainData && terrain) {
        terrain->restoreHeightsAndSplatmap(snap.terrainHeights, snap.terrainSplatmap);
    }
}

void EditorUI::recordSnapshot(const Scene& scene, Terrain* terrain) {
    SceneSnapshot snap = captureSceneState(scene, terrain);
    m_undoStack.push_back(snap);
    if (m_undoStack.size() > static_cast<size_t>(m_maxUndoSteps)) {
        m_undoStack.erase(m_undoStack.begin());
    }
    m_redoStack.clear();
}

void EditorUI::performUndo(Scene& scene, PhysicsWorld& physicsWorld, Terrain* terrain) {
    if (m_undoStack.empty()) return;

    // Guardar estado actual en redo stack
    SceneSnapshot curSnap = captureSceneState(scene, terrain);
    m_redoStack.push_back(curSnap);

    // Obtener estado anterior
    SceneSnapshot snap = m_undoStack.back();
    m_undoStack.pop_back();

    // Restaurar escena y terreno
    restoreSceneState(snap, scene, physicsWorld, terrain);
    m_selectedEntityName = "";
    m_selectedGroupName = "";
    m_selectedEntityNames.clear();
    m_selectedLightIndex = -1;
}

void EditorUI::performRedo(Scene& scene, PhysicsWorld& physicsWorld, Terrain* terrain) {
    if (m_redoStack.empty()) return;

    // Guardar estado actual en undo stack
    SceneSnapshot curSnap = captureSceneState(scene, terrain);
    m_undoStack.push_back(curSnap);

    // Obtener estado siguiente
    SceneSnapshot snap = m_redoStack.back();
    m_redoStack.pop_back();

    // Restaurar escena y terreno
    restoreSceneState(snap, scene, physicsWorld, terrain);
    m_selectedEntityName = "";
    m_selectedGroupName = "";
    m_selectedEntityNames.clear();
    m_selectedLightIndex = -1;
}

void EditorUI::savePrePlaySnapshot(const Scene& scene) {
    m_prePlaySnapshot = captureSceneState(scene, nullptr);
    m_hasPrePlaySnapshot = true;
}

void EditorUI::restorePrePlaySnapshot(Scene& scene, PhysicsWorld& physicsWorld) {
    if (!m_hasPrePlaySnapshot) return;
    for (const auto& e : m_prePlaySnapshot.entities) {
        auto ent = scene.getEntity(e.name);
        if (ent) {
            ent->setPosition(e.position.x, e.position.y, e.position.z);
            ent->setRotation(e.rotation.x, e.rotation.y, e.rotation.z);
            ent->setScale(e.scale.x, e.scale.y, e.scale.z);
            ent->setGroup(e.group);
            ent->setTrigger(e.isTrigger);
            ent->setTriggerTag(e.triggerTag);
            physicsWorld.syncEntityToPhysics(ent);
            if (e.isTrigger) {
                physicsWorld.setBodyTrigger(e.name, true);
            }
        }
    }
    m_hasPrePlaySnapshot = false;
}

void EditorUI::setLayoutPreset(UILayoutPreset preset) {
    m_currentLayout = preset;
    m_layoutBuilt = false;
    if (preset == UILayoutPreset::LIGHTING_CINEMA) {
        m_activePropertyTab = BlenderPropertyTab::LIGHTING;
        ImGui::SetWindowFocus("Sequencer & Cine (Fase 22)");
    } else if (preset == UILayoutPreset::TERRAIN_MODELING) {
        m_activePropertyTab = BlenderPropertyTab::TERRAIN;
    } else if (preset == UILayoutPreset::GAME_VIEW) {
        m_activePropertyTab = BlenderPropertyTab::PLAYER;
    }
}

std::shared_ptr<Entity> EditorUI::duplicateEntity(Scene& scene, PhysicsWorld& physicsWorld, const std::string& name) {
    auto ent = scene.getEntity(name);
    if (!ent) return nullptr;

    recordSnapshot(scene);

    std::string baseName = name;
    std::string dupName = baseName + "_Copy";
    int counter = 1;
    while (scene.getEntity(dupName)) {
        dupName = baseName + "_Copy" + std::to_string(counter++);
    }

    glm::vec3 offset(1.0f, 0.0f, 1.0f);
    glm::vec3 newPos = ent->getPosition() + offset;

    std::shared_ptr<Entity> dupEnt = nullptr;
    const std::string& prim = ent->getPrimitiveType();
    if (prim == "Sphere") dupEnt = scene.createSphere(dupName, newPos.x, newPos.y, newPos.z);
    else if (prim == "Cylinder") dupEnt = scene.createCylinder(dupName, newPos.x, newPos.y, newPos.z);
    else if (prim == "Cone") dupEnt = scene.createCone(dupName, newPos.x, newPos.y, newPos.z);
    else if (prim == "Torus") dupEnt = scene.createTorus(dupName, newPos.x, newPos.y, newPos.z);
    else if (prim == "Plane") dupEnt = scene.createPlane(dupName, newPos.x, newPos.y, newPos.z);
    else if (prim == "Model" && !ent->getModelPath().empty()) dupEnt = scene.createModel(dupName, ent->getModelPath(), newPos.x, newPos.y, newPos.z);
    else dupEnt = scene.createCube(dupName, newPos.x, newPos.y, newPos.z);

    if (dupEnt) {
        dupEnt->setRotation(ent->getRotation().x, ent->getRotation().y, ent->getRotation().z);
        if (ent->usesQuaternion()) {
            dupEnt->setOrientation(ent->getOrientation());
        }
        dupEnt->setScale(ent->getScale().x, ent->getScale().y, ent->getScale().z);
        dupEnt->setColor(ent->getColor().r, ent->getColor().g, ent->getColor().b);
        dupEnt->setGroup(ent->getGroup());
        dupEnt->setVisible(ent->isVisible());

        // Copiar Material PBR completo
        if (ent->getMaterial()) {
            auto srcMat = ent->getMaterial();
            auto newMat = std::make_shared<Material>();
            newMat->albedo = srcMat->albedo;
            newMat->roughness = srcMat->roughness;
            newMat->metallic = srcMat->metallic;
            newMat->specular = srcMat->specular;
            newMat->normalStrength = srcMat->normalStrength;
            newMat->ao = srcMat->ao;
            newMat->emission = srcMat->emission;
            newMat->emissionStrength = srcMat->emissionStrength;
            newMat->albedoMap = srcMat->albedoMap;
            newMat->normalMap = srcMat->normalMap;
            newMat->roughnessMap = srcMat->roughnessMap;
            newMat->metallicMap = srcMat->metallicMap;
            newMat->specularMap = srcMat->specularMap;
            newMat->emissionMap = srcMat->emissionMap;
            newMat->aoMap = srcMat->aoMap;
            dupEnt->setMaterial(newMat);
        }

        // Copiar disparador/trigger si aplica
        dupEnt->setTrigger(ent->isTrigger());
        dupEnt->setTriggerTag(ent->getTriggerTag());

        // Copiar física Bullet si aplica
        if (ent->hasPhysics()) {
            dupEnt->setHasPhysics(true);
            dupEnt->setMass(ent->getMass());
            dupEnt->setColliderType(ent->getColliderType());
            dupEnt->setFriction(ent->getFriction());
            dupEnt->setRestitution(ent->getRestitution());
            dupEnt->setKinematic(ent->isKinematic());

            ColliderType colType = ColliderType::BOX;
            if (ent->getColliderType() == "SPHERE") colType = ColliderType::SPHERE;
            else if (ent->getColliderType() == "PLANE") colType = ColliderType::PLANE;

            physicsWorld.addRigidBody(dupEnt, ent->getMass(), colType);
        }

        // Seleccionar la nueva entidad
        selectEntity(dupName);
    }

    return dupEnt;
}

void EditorUI::deleteSelectedEntities(Scene& scene, PhysicsWorld& physicsWorld) {
    if (m_selectedEntityNames.empty() && m_selectedEntityName.empty()) return;

    recordSnapshot(scene);

    std::unordered_set<std::string> toDelete = m_selectedEntityNames;
    if (!m_selectedEntityName.empty()) {
        toDelete.insert(m_selectedEntityName);
    }

    for (const auto& name : toDelete) {
        physicsWorld.removeRigidBody(name);
        scene.removeEntity(name);
    }

    clearEntitySelection();
}

void EditorUI::focusCameraOnSelection(Camera& camera, Scene& scene) {
    if (!m_selectedGroupName.empty()) {
        camera.resetToFocus(m_groupPivot);
        return;
    }

    if (!m_selectedEntityName.empty()) {
        auto ent = scene.getEntity(m_selectedEntityName);
        if (ent) {
            camera.resetToFocus(ent->getPosition());
            return;
        }
    }

    if (!m_selectedEntityNames.empty()) {
        glm::vec3 center(0.0f);
        int count = 0;
        for (const auto& name : m_selectedEntityNames) {
            auto ent = scene.getEntity(name);
            if (ent) {
                center += ent->getPosition();
                count++;
            }
        }
        if (count > 0) {
            camera.resetToFocus(center / static_cast<float>(count));
        }
    }
}

void EditorUI::render(Scene& scene, ScriptEngine& scriptEngine, Camera& camera, Framebuffer& framebuffer, 
                      PhysicsWorld& physicsWorld, ShadowMap& shadowMap, Skybox& skybox, 
                      ParticleSystem& particleSystem, SSAO& ssao, Terrain& terrain, CharacterController& character, float deltaTime, GLuint displayTexture, Water* water) {
    if (!m_initialized || !m_visible) return;

    // Actualizar Ciclo Día / Noche de 24h y Clima (Fase 20)
    m_dayNight.update(deltaTime);
    if (m_dayNight.isCycleEnabled()) {
        m_sunAzimuth = m_dayNight.getSunAzimuth();
        m_sunElevation = m_dayNight.getSunElevation();
        m_lightColor = m_dayNight.getSunColor();
        m_lightIntensity = m_dayNight.getSunIntensity();
        m_ambientColor = m_dayNight.getAmbientColor();
        m_ambientIntensity = m_dayNight.getAmbientIntensity();
    } else {
        m_dayNight.setManualSun(m_sunAzimuth, m_sunElevation);
    }

    // Cálculo de FPS
    m_fpsUpdateTimer += deltaTime;
    if (m_fpsUpdateTimer >= 0.2f && deltaTime > 0.0001f) {
        m_fps = 1.0f / deltaTime;
        m_frameTimeMs = deltaTime * 1000.0f;
        m_fpsUpdateTimer = 0.0f;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    // 1. Contenedor de DockSpace maestro
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImVec2 dockspaceWorkSize = viewport->WorkSize;
    dockspaceWorkSize.y -= 22.0f; // Blender Status Bar 22px
    ImGui::SetNextWindowSize(dockspaceWorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockWindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    dockWindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    dockWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("ASIS_DockSpace_Master", nullptr, dockWindowFlags);
    ImGui::PopStyleVar(3);

    // Atajos de teclado globales (Fase 11, 12 y 15)
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        if (m_playMode == EnginePlayMode::PLAYING) {
            // En modo juego, únicamente procesar Escape para salir o F12 para captura
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                triggerPlayStop();
            } else if (ImGui::IsKeyPressed(ImGuiKey_F12)) {
                m_requestRenderImage = true;
            }
        } else {
            // Atajos de Edición (SOLO en modo editor)
            if ((io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_P)) || ImGui::IsKeyPressed(ImGuiKey_F5)) {
                triggerPlayStart();
            } else if (ImGui::IsKeyPressed(ImGuiKey_F12)) {
                m_requestRenderImage = true;
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
                performUndo(scene, physicsWorld, &terrain);
            } else if ((io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) || (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))) {
                performRedo(scene, physicsWorld, &terrain);
            } else if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A)) {
                m_openAddMenu = true;
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_G)) {
                if (!m_selectedEntityNames.empty()) {
                    m_showGroupCreationModal = true;
                }
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
                // Duplicar Selección (Ctrl+D - Fase 21)
                if (!m_selectedEntityName.empty()) {
                    duplicateEntity(scene, physicsWorld, m_selectedEntityName);
                } else if (!m_selectedEntityNames.empty()) {
                    auto toDup = m_selectedEntityNames;
                    for (const auto& name : toDup) {
                        duplicateEntity(scene, physicsWorld, name);
                    }
                }
            } else if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                // Eliminar Selección (Supr / Delete - Fase 21)
                deleteSelectedEntities(scene, physicsWorld);
            } else if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                // Enfocar Selección (F - Fase 21)
                focusCameraOnSelection(camera, scene);
            } else if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                if (!m_selectedEntityName.empty()) {
                    m_renamingEntityName = m_selectedEntityName;
                    strncpy_s(m_renameEntityBuf, m_selectedEntityName.c_str(), sizeof(m_renameEntityBuf) - 1);
                }
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Comma)) {
                m_showPreferencesModal = true;
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
                recordSnapshot(scene, &terrain);
                newProject(scene, camera, terrain, skybox, physicsWorld, scriptEngine, water);
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
                std::string path = SceneIO::saveFileDialog("Proyectos ASIS Engine (*.asis)\0*.asis\0Todos los archivos (*.*)\0*.*\0", "asis");
                if (!path.empty()) {
                    saveProject(path, scene, camera, terrain, skybox, water);
                    scriptEngine.addLog("[Proyecto] [OK] Proyecto guardado con exito: " + path);
                }
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
                std::string path = SceneIO::openFileDialog("Proyectos ASIS Engine (*.asis)\0*.asis\0Todos los archivos (*.*)\0*.*\0");
                if (!path.empty()) {
                    recordSnapshot(scene, &terrain);
                    loadProject(path, scene, camera, terrain, skybox, physicsWorld, scriptEngine, water);
                }
            }
        }
    }

    // Autoguardado periódico
    if (m_autoSaveEnabled && m_autoSaveIntervalMinutes > 0.0f) {
        m_autoSaveTimer += deltaTime;
        if (m_autoSaveTimer >= m_autoSaveIntervalMinutes * 60.0f) {
            saveProject("autosave.asis", scene, camera, terrain, skybox, water);
            m_autoSaveTimer = 0.0f;
        }
    }

    renderMenuBar(scene, scriptEngine, camera, physicsWorld, particleSystem, terrain, character, skybox, water);

    ImGuiID dockspaceId = ImGui::GetID("ASIS_DockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Distribución adaptable según Preset de Layouts (Fase 21.5: Columna unificada derecha estilo Blender 5)
    if (!m_layoutBuilt) {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, dockspaceWorkSize);

        ImGuiID dockMainId = dockspaceId;

        // Distribución Blender 5: Viewport 3D a la izquierda (~77%), columna unificada derecha (~23%, ~300px)
        float rightColWidth = 0.23f;
        float outlinerHeight = 0.38f;
        float bottomTrayHeight = 0.20f;

        if (m_currentLayout == UILayoutPreset::LIGHTING_CINEMA) {
            rightColWidth = 0.26f;
            outlinerHeight = 0.30f;
            bottomTrayHeight = 0.16f;
        } else if (m_currentLayout == UILayoutPreset::TERRAIN_MODELING) {
            rightColWidth = 0.28f;
            outlinerHeight = 0.32f;
            bottomTrayHeight = 0.16f;
        } else if (m_currentLayout == UILayoutPreset::GAME_VIEW) {
            rightColWidth = 0.16f;
            outlinerHeight = 0.40f;
            bottomTrayHeight = 0.12f;
        }

        // 1. Separar columna unificada derecha
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, rightColWidth, nullptr, &dockMainId);

        // 2. Dividir columna derecha verticalmente: Outliner arriba (~38%), Propiedades abajo (~62%)
        ImGuiID dockRightTop = ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Up, outlinerHeight, nullptr, &dockRight);

        // 3. Separar bandeja inferior debajo del Viewport 3D (~20% de alto)
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, bottomTrayHeight, nullptr, &dockMainId);

        // 4. Asignar ventanas a los docks
        ImGui::DockBuilderDockWindow("Jerarquia de Escena", dockRightTop);
        ImGui::DockBuilderDockWindow("Propiedades", dockRight);
        ImGui::DockBuilderDockWindow("Consola del Motor", dockBottom);
        ImGui::DockBuilderDockWindow("Generador de IA (Gemini)", dockBottom);
        ImGui::DockBuilderDockWindow("Navegador de Activos & Prefabs", dockBottom);
        ImGui::DockBuilderDockWindow("Sequencer & Cine (Fase 22)", dockBottom);
        ImGui::DockBuilderDockWindow("Viewport 3D", dockMainId);

        ImGui::DockBuilderFinish(dockspaceId);
        m_layoutBuilt = true;
    }

    ImGui::End();

    // Actualizar Sequencer y cinemáticas (Fase 22)
    Sequencer::get().update(deltaTime, scene, camera, &m_dayNight, &scriptEngine, this);

    // 2. Renderizar paneles modulares
    renderViewport(scene, camera, framebuffer, physicsWorld, terrain, character, displayTexture, &particleSystem, &scriptEngine);
    renderOutliner(scene, physicsWorld, camera);
    renderPropertiesPanel(scene, camera, physicsWorld, scriptEngine, shadowMap, skybox, particleSystem, terrain, character, water);
    renderAIPanel(scene, terrain);
    renderConsole(scriptEngine);
    renderPrefabPalette(scene, camera, physicsWorld);
    renderSequencerPanel(scene, camera, &m_dayNight, &scriptEngine);
    renderStatusBar(scene, camera, deltaTime);
    renderAboutModal();

    // Notificación flotante de Render / Captura completada (Fase 13)
    if (m_showRenderNotification && m_renderNotificationTimer > 0.0f) {
        m_renderNotificationTimer -= deltaTime;
        ImGuiViewport* mainVp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(mainVp->WorkPos.x + mainVp->WorkSize.x - 380.0f, mainVp->WorkPos.y + mainVp->WorkSize.y - 110.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.14f, 0.18f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.75f, 0.45f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        if (ImGui::Begin("##RenderToast", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.30f, 0.95f, 0.50f, 1.0f), "[Render] Guardado con Exito!");
            std::filesystem::path rp(m_lastRenderPath);
            ImGui::TextUnformatted(rp.filename().string().c_str());
            ImGui::Spacing();
            if (UI::ButtonWithIcon(UI::IconType::FOLDER, "Abrir Carpeta")) {
                openRendersFolder();
            }
            ImGui::SameLine();
            if (UI::ButtonWithIcon(UI::IconType::EYE_OPEN, "Ver Imagen")) {
                openFileWithDefaultApp(m_lastRenderPath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cerrar")) {
                m_showRenderNotification = false;
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    // Modales de Preferencias y Añadir Primitivas
    renderPreferencesModal(scene, camera, physicsWorld);
    renderAddPrimitiveMenu(scene, camera, particleSystem);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool EditorUI::saveProject(const std::string& path, const Scene& scene, const Camera& camera, const Terrain& terrain, const Skybox& skybox, const Water* water) {
    SceneIO::ASISSceneLoadData projectData;

    // 1. Iluminación Solar y Sombras
    projectData.sunAzimuth = m_sunAzimuth;
    projectData.sunElevation = m_sunElevation;
    projectData.sunColor = m_lightColor;
    projectData.sunIntensity = m_lightIntensity;
    projectData.ambientColor = m_ambientColor;
    projectData.ambientIntensity = m_ambientIntensity;
    projectData.shadowsEnabled = m_shadowsEnabled;
    projectData.shadowRes = m_shadowResolution;
    projectData.shadowKernel = m_shadowKernel;
    projectData.shadowSoftness = m_shadowSoftness;
    projectData.shadowDistance = m_shadowDistance;
    projectData.terrainCastsShadows = m_terrainCastsShadows;
    projectData.shadowFilterMode = m_shadowFilterMode;
    projectData.pcssLightSize = m_pcssLightSize;
    projectData.pcssBlockerSamples = m_pcssBlockerSamples;
    projectData.pcssFilterSamples = m_pcssFilterSamples;
    projectData.csmEnabled = m_csmEnabled;

    // 2. Color y Auto-Exposición
    projectData.colorProfile = m_colorProfile;
    projectData.exposure = m_exposure;
    projectData.gamma = m_gamma;
    projectData.saturation = m_saturation;
    projectData.contrast = m_contrast;
    projectData.autoExposureEnabled = m_autoExposureEnabled;
    projectData.autoExposureKey = m_autoExposureKey;
    projectData.autoExposureMin = m_autoExposureMin;
    projectData.autoExposureMax = m_autoExposureMax;
    projectData.autoExposureSpeedUp = m_autoExposureSpeedUp;
    projectData.autoExposureSpeedDown = m_autoExposureSpeedDown;

    // 3. Cielo HDRI y Entorno
    projectData.useIBL = m_useIBL;
    projectData.showSkybox = m_showSkybox;
    projectData.skyboxExposure = m_skyboxExposure;
    projectData.skyboxRotation = m_skyboxRotation;
    projectData.skyboxBlur = m_skyboxBlur;
    projectData.iblIntensity = m_iblIntensity;
    projectData.flipSkyboxX = m_flipSkyboxX;
    projectData.flipSkyboxY = m_flipSkyboxY;
    projectData.hdriPath = skybox.hasCustomHDRI() ? skybox.getHDRIPath() : "";

    // 4. Post-Proceso y Gráficos
    projectData.bloomEnabled = m_bloomEnabled;
    projectData.bloomThreshold = m_bloomThreshold;
    projectData.bloomIntensity = m_bloomIntensity;
    projectData.fxaaEnabled = m_fxaaEnabled;
    projectData.msaaLevel = m_msaaLevel;
    projectData.ssaoEnabled = m_ssaoEnabled;
    projectData.ssaoRadius = m_ssaoRadius;
    projectData.ssaoBias = m_ssaoBias;
    projectData.ssaoPower = m_ssaoPower;
    projectData.ssaoIntensity = m_ssaoIntensity;
    projectData.dofEnabled = m_dofEnabled;
    projectData.dofFocusDistance = m_dofFocusDistance;
    projectData.dofFocusRange = m_dofFocusRange;
    projectData.dofBokehRadius = m_dofBokehRadius;
    projectData.anisotropyLevel = m_anisotropyLevel;
    projectData.showGroundPlane = m_showGroundPlane;
    projectData.showGroundGrid = m_showGroundGrid;

    // 5. Terreno 3D y Capas PBR
    projectData.terrainVisible = terrain.isVisible();
    projectData.terrainBaseOffset = terrain.getBaseOffset();
    projectData.terrainReflectionIntensity = terrain.getReflectionIntensity();
    projectData.terrainReliefMode = static_cast<int>(terrain.getReliefMode());
    projectData.terrainTessellationScale = terrain.getTessellationDisplacementScale();
    projectData.terrainResolution = terrain.getResolution();
    projectData.terrainWorldSize = terrain.getWorldSize();
    projectData.terrainHeights = terrain.getHeights();
    projectData.terrainSplatmap = terrain.getSplatmapData();

    projectData.terrainLayers.clear();
    for (int i = 0; i < terrain.getLayerCount(); ++i) {
        const auto& layer = terrain.getLayer(i);
        SceneIO::TerrainLayerSaveData layData;
        layData.name = layer.name;
        layData.enabled = layer.enabled;
        layData.visible = layer.visible;
        layData.opacity = layer.opacity;
        layData.tiling = layer.tiling;
        layData.roughness = layer.roughness;
        layData.specular = layer.specular;
        layData.metallic = layer.metallic;
        layData.normalStrength = layer.normalStrength;
        layData.heightScale = layer.heightScale;
        layData.aoStrength = layer.aoStrength;
        layData.tint = layer.tint;
        layData.albedoPath = layer.albedoPath;
        layData.normalPath = layer.normalPath;
        layData.roughSpecPath = layer.roughSpecPath;
        layData.heightPath = layer.heightPath;
        layData.aoPath = layer.aoPath;
        projectData.terrainLayers.push_back(layData);
    }

    // 6. Configuración de OASIS
    auto oasis = terrain.getOASIS();
    if (oasis) {
        projectData.oasisEnabled = oasis->isEnabled();
        projectData.oasisFrustumCulling = oasis->isFrustumCullingEnabled();
        projectData.oasisGeomorphing = oasis->isGeomorphingEnabled();
        projectData.oasisDebugLOD = oasis->isDebugLODEnabled();
        projectData.oasisLodDistanceFactor = oasis->getLodDistanceFactor();
        projectData.oasisMaxLOD = oasis->getMaxLOD();
        projectData.oasisPatchResolution = oasis->getPatchResolution();
    }

    // OASIS Módulo 2: Meshlets
    projectData.meshletsEnabled = m_meshletConfig.enabled;
    projectData.meshletFrustumCulling = m_meshletConfig.frustumCulling;
    projectData.meshletBackfaceCulling = m_meshletConfig.backfaceCulling;
    projectData.meshletDebugColors = m_meshletConfig.debugColors;

    // 7. Océano y Cuerpos de Agua 3D (Fase 19)
    if (water) {
        projectData.waterEnabled = water->isEnabled();
        projectData.waterLevel = water->getWaterLevel();
        projectData.waterWaveAmplitude = water->getWaveAmplitude();
        projectData.waterWaveSpeed = water->getWaveSpeed();
        projectData.waterShallowColor = water->getShallowColor();
        projectData.waterDeepColor = water->getDeepColor();
        projectData.waterClarity = water->getClarity();
        projectData.waterFoamThreshold = water->getFoamThreshold();
        projectData.waterFoamIntensity = water->getFoamIntensity();
        projectData.waterRefraction = water->getRefractionStrength();
        projectData.waterRoughness = water->getRoughness();
        projectData.waterHorizonDistance = water->getHorizonDistance();
        projectData.waterHorizonBlend = water->getHorizonBlend();
    }

    // 8. Fase 20: Ciclo Día / Noche, Clima, SSR y Contact Shadows
    projectData.dayNightCycleEnabled = m_dayNight.isCycleEnabled();
    projectData.dayNightTimeOfDay = m_dayNight.getTimeOfDay();
    projectData.dayNightDuration = m_dayNight.getDayDurationSeconds();
    projectData.dayNightSunLatitude = m_dayNight.getSunLatitude();
    projectData.dayNightHorizonHeight = m_dayNight.getHorizonHeight();
    projectData.weatherType = static_cast<int>(m_dayNight.getWeather());
    projectData.weatherWetness = m_dayNight.getWetness();

    projectData.ssrEnabled = m_ssrEnabled;
    projectData.ssrMaxSteps = m_ssrMaxSteps;
    projectData.ssrStepSize = m_ssrStepSize;
    projectData.ssrThickness = m_ssrThickness;
    projectData.ssrIntensity = m_ssrIntensity;

    projectData.contactShadowsEnabled = m_contactShadowsEnabled;
    projectData.contactShadowsDistance = m_contactShadowsDistance;
    projectData.contactShadowsRaySteps = m_contactShadowsRaySteps;
    projectData.contactShadowsIntensity = m_contactShadowsIntensity;

    projectData.globalFogEnabled = m_dayNight.isFogEnabled();
    projectData.globalFogStart = m_dayNight.getFogStart();
    projectData.globalFogEnd = m_dayNight.getFogEnd();
    projectData.globalFogDensity = m_dayNight.getFogDensity();
    projectData.globalFogColor = m_dayNight.getFogColor();

    return SceneIO::saveProjectASIS(scene, camera, projectData, path);
}

bool EditorUI::loadProject(const std::string& path, Scene& scene, Camera& camera, Terrain& terrain, Skybox& skybox, PhysicsWorld& physicsWorld, ScriptEngine& scriptEngine, Water* water) {
    SceneIO::ASISSceneLoadData data;
    if (!SceneIO::loadSceneASIS(scene, camera, data, path)) {
        scriptEngine.addLog("[Proyecto ERROR] No se pudo cargar el archivo: " + path);
        return false;
    }

    // 1. Restaurar Iluminación y Sombras
    m_sunAzimuth = data.sunAzimuth;
    m_sunElevation = data.sunElevation;
    m_lightColor = data.sunColor;
    m_lightIntensity = data.sunIntensity;
    m_ambientColor = data.ambientColor;
    m_ambientIntensity = data.ambientIntensity;
    m_shadowsEnabled = data.shadowsEnabled;
    m_shadowResolution = data.shadowRes;
    m_shadowKernel = data.shadowKernel;
    m_shadowSoftness = data.shadowSoftness;
    m_shadowDistance = data.shadowDistance;
    m_terrainCastsShadows = data.terrainCastsShadows;
    m_shadowFilterMode = data.shadowFilterMode;
    m_pcssLightSize = data.pcssLightSize;
    m_pcssBlockerSamples = data.pcssBlockerSamples;
    m_pcssFilterSamples = data.pcssFilterSamples;
    m_csmEnabled = data.csmEnabled;

    // 2. Restaurar Color y Auto-Exposición
    m_colorProfile = data.colorProfile;
    m_exposure = data.exposure;
    m_gamma = data.gamma;
    m_saturation = data.saturation;
    m_contrast = data.contrast;
    m_autoExposureEnabled = data.autoExposureEnabled;
    m_autoExposureKey = data.autoExposureKey;
    m_autoExposureMin = data.autoExposureMin;
    m_autoExposureMax = data.autoExposureMax;
    m_autoExposureSpeedUp = data.autoExposureSpeedUp;
    m_autoExposureSpeedDown = data.autoExposureSpeedDown;

    // 3. Restaurar Cielo HDRI
    m_useIBL = data.useIBL;
    m_showSkybox = data.showSkybox;
    m_skyboxExposure = data.skyboxExposure;
    m_skyboxRotation = data.skyboxRotation;
    m_skyboxBlur = data.skyboxBlur;
    m_iblIntensity = data.iblIntensity;
    m_flipSkyboxX = data.flipSkyboxX;
    m_flipSkyboxY = data.flipSkyboxY;
    if (!data.hdriPath.empty() && std::filesystem::exists(data.hdriPath)) {
        skybox.loadHDRI(data.hdriPath);
    } else {
        skybox.generateProceduralSky();
    }

    // 4. Restaurar Post-Proceso y Opciones Gráficas
    m_bloomEnabled = data.bloomEnabled;
    m_bloomThreshold = data.bloomThreshold;
    m_bloomIntensity = data.bloomIntensity;
    m_fxaaEnabled = data.fxaaEnabled;
    m_msaaLevel = data.msaaLevel;
    m_ssaoEnabled = data.ssaoEnabled;
    m_ssaoRadius = data.ssaoRadius;
    m_ssaoBias = data.ssaoBias;
    m_ssaoPower = data.ssaoPower;
    m_ssaoIntensity = data.ssaoIntensity;
    m_dofEnabled = data.dofEnabled;
    m_dofFocusDistance = data.dofFocusDistance;
    m_dofFocusRange = data.dofFocusRange;
    m_dofBokehRadius = data.dofBokehRadius;
    m_anisotropyLevel = data.anisotropyLevel;
    m_showGroundPlane = data.showGroundPlane;
    m_showGroundGrid = data.showGroundGrid;

    // 5. Restaurar Terreno 3D y sus Capas PBR
    if (data.terrainResolution > 0 && (data.terrainResolution != terrain.getResolution() || std::abs(data.terrainWorldSize - terrain.getWorldSize()) > 0.1f)) {
        terrain.rebuild(data.terrainResolution, data.terrainWorldSize);
    }
    if (!data.terrainHeights.empty()) {
        terrain.restoreHeightsAndSplatmap(data.terrainHeights, data.terrainSplatmap);
    }
    terrain.setVisible(data.terrainVisible);
    terrain.setBaseOffset(data.terrainBaseOffset);
    terrain.setReflectionIntensity(data.terrainReflectionIntensity);
    terrain.setAnisotropyLevel(data.anisotropyLevel);
    terrain.setReliefMode(static_cast<TerrainReliefMode>(data.terrainReliefMode));
    terrain.setTessellationDisplacementScale(data.terrainTessellationScale);

    if (!data.terrainLayers.empty()) {
        while (terrain.getLayerCount() < static_cast<int>(data.terrainLayers.size())) {
            terrain.addLayer();
        }
        while (terrain.getLayerCount() > static_cast<int>(data.terrainLayers.size()) && terrain.getLayerCount() > 4) {
            terrain.removeLayer(terrain.getLayerCount() - 1);
        }

        for (size_t i = 0; i < data.terrainLayers.size() && i < static_cast<size_t>(terrain.getLayerCount()); ++i) {
            const auto& layData = data.terrainLayers[i];
            auto& layer = terrain.getLayer(static_cast<int>(i));
            layer.name = layData.name;
            layer.enabled = layData.enabled;
            layer.visible = layData.visible;
            layer.opacity = layData.opacity;
            layer.tiling = layData.tiling;
            layer.roughness = layData.roughness;
            layer.specular = layData.specular;
            layer.metallic = layData.metallic;
            layer.normalStrength = layData.normalStrength;
            layer.heightScale = layData.heightScale;
            layer.aoStrength = layData.aoStrength;
            layer.tint = layData.tint;

            if (!layData.albedoPath.empty() && std::filesystem::exists(layData.albedoPath)) {
                terrain.loadLayerAlbedo(static_cast<int>(i), layData.albedoPath);
            }
            if (!layData.normalPath.empty() && std::filesystem::exists(layData.normalPath)) {
                terrain.loadLayerNormal(static_cast<int>(i), layData.normalPath);
            }
            if (!layData.roughSpecPath.empty() && std::filesystem::exists(layData.roughSpecPath)) {
                terrain.loadLayerRoughSpec(static_cast<int>(i), layData.roughSpecPath);
            }
            if (!layData.heightPath.empty() && std::filesystem::exists(layData.heightPath)) {
                terrain.loadLayerHeight(static_cast<int>(i), layData.heightPath);
            }
            if (!layData.aoPath.empty() && std::filesystem::exists(layData.aoPath)) {
                terrain.loadLayerAO(static_cast<int>(i), layData.aoPath);
            }
        }
    }

    // 6. Restaurar OASIS
    auto oasis = terrain.getOASIS();
    if (oasis) {
        oasis->setEnabled(data.oasisEnabled);
        oasis->setFrustumCulling(data.oasisFrustumCulling);
        oasis->setGeomorphing(data.oasisGeomorphing);
        oasis->setDebugLOD(data.oasisDebugLOD);
        oasis->setLodDistanceFactor(data.oasisLodDistanceFactor);
        oasis->setMaxLOD(data.oasisMaxLOD);
        oasis->setPatchResolution(data.oasisPatchResolution);
    }

    // OASIS Módulo 2: Meshlets
    m_meshletConfig.enabled = data.meshletsEnabled;
    m_meshletConfig.frustumCulling = data.meshletFrustumCulling;
    m_meshletConfig.backfaceCulling = data.meshletBackfaceCulling;
    m_meshletConfig.debugColors = data.meshletDebugColors;

    // 7. Restaurar Océano y Cuerpos de Agua 3D (Fase 19)
    if (water) {
        water->setEnabled(data.waterEnabled);
        water->setWaterLevel(data.waterLevel);
        water->setWaveAmplitude(data.waterWaveAmplitude);
        water->setWaveSpeed(data.waterWaveSpeed);
        water->setShallowColor(data.waterShallowColor);
        water->setDeepColor(data.waterDeepColor);
        water->setClarity(data.waterClarity);
        water->setFoamThreshold(data.waterFoamThreshold);
        water->setFoamIntensity(data.waterFoamIntensity);
        water->setRefractionStrength(data.waterRefraction);
        water->setRoughness(data.waterRoughness);
        water->setHorizonDistance(data.waterHorizonDistance);
        water->setHorizonBlend(data.waterHorizonBlend);
    }

    // 8. Restaurar Ciclo Día / Noche 24h, Clima Atmosférico, SSR y Contact Shadows (Fase 20)
    m_dayNight.setCycleEnabled(data.dayNightCycleEnabled);
    m_dayNight.setTimeOfDay(data.dayNightTimeOfDay);
    m_dayNight.setDayDurationSeconds(data.dayNightDuration);
    m_dayNight.setSunLatitude(data.dayNightSunLatitude);
    m_dayNight.setHorizonHeight(data.dayNightHorizonHeight);
    m_dayNight.setWeather(static_cast<WeatherType>(data.weatherType));
    m_dayNight.setWetness(data.weatherWetness);
    m_dayNight.setFogEnabled(data.globalFogEnabled);
    m_dayNight.setFogStart(data.globalFogStart);
    m_dayNight.setFogEnd(data.globalFogEnd);
    m_dayNight.setFogDensity(data.globalFogDensity);
    m_dayNight.setFogColor(data.globalFogColor);

    m_ssrEnabled = data.ssrEnabled;
    m_ssrMaxSteps = data.ssrMaxSteps;
    m_ssrStepSize = data.ssrStepSize;
    m_ssrThickness = data.ssrThickness;
    m_ssrIntensity = data.ssrIntensity;

    m_contactShadowsEnabled = data.contactShadowsEnabled;
    m_contactShadowsDistance = data.contactShadowsDistance;
    m_contactShadowsRaySteps = data.contactShadowsRaySteps;
    m_contactShadowsIntensity = data.contactShadowsIntensity;

    // Sincronizar colisiones físicas de entidades de la escena
    physicsWorld.clear();
    for (const auto& [name, ent] : scene.getEntities()) {
        if (!ent || (!ent->getMesh() && !ent->getModel())) continue;
        if (ent->hasPhysics() || ent->isTrigger()) {
            ColliderType cType = ColliderType::BOX;
            std::string cTypeStr = ent->getColliderType();
            if (cTypeStr == "SPHERE" || ent->getPrimitiveType() == "Sphere") cType = ColliderType::SPHERE;
            else if (cTypeStr == "PLANE" || ent->getPrimitiveType() == "Plane") cType = ColliderType::PLANE;

            float mass = ent->isTrigger() ? 0.0f : ent->getMass();
            physicsWorld.addRigidBody(ent, mass, cType);
            if (ent->isTrigger()) {
                physicsWorld.setBodyTrigger(ent->getName(), true);
            }
            auto rbComp = physicsWorld.getRigidBodyComponent(ent->getName());
            if (rbComp) {
                rbComp->friction = ent->getFriction();
                rbComp->restitution = ent->getRestitution();
                if (rbComp->body) {
                    rbComp->body->setFriction(ent->getFriction());
                    rbComp->body->setRestitution(ent->getRestitution());
                }
            }
        }
    }
    terrain.syncPhysics();

    m_selectedEntityName = "";
    m_selectedEntityNames.clear();
    m_selectedLightIndex = -1;
    m_selectedGroupName = "";

    scriptEngine.addLog("[Proyecto] [OK] Proyecto cargado con exito con todas sus texturas y opciones: " + path);
    return true;
}

void EditorUI::newProject(Scene& scene, Camera& camera, Terrain& terrain, Skybox& skybox, PhysicsWorld& physicsWorld, ScriptEngine& scriptEngine, Water* water) {
    scene.clear();
    physicsWorld.clear();

    camera.setPosition(glm::vec3(0.0f, 3.5f, 7.5f));
    camera.setTarget(glm::vec3(0.0f, 1.2f, 0.0f));
    camera.setFOV(60.0f);
    camera.setOrthographic(false);
    camera.setNearPlane(0.1f);
    camera.setFarPlane(1000.0f);

    m_sunAzimuth = 45.0f;
    m_sunElevation = 50.0f;
    m_lightColor = glm::vec3(1.0f, 0.96f, 0.88f);
    m_lightIntensity = 1.2f;
    m_ambientColor = glm::vec3(0.24f, 0.27f, 0.35f);
    m_ambientIntensity = 1.0f;

    m_shadowsEnabled = true;
    m_shadowResolution = 4096;
    m_shadowKernel = 3;
    m_shadowSoftness = 0.57f;
    m_shadowDistance = 85.0f;
    m_terrainCastsShadows = false;
    m_shadowFilterMode = 1;
    m_pcssLightSize = 0.35f;
    m_pcssBlockerSamples = 16;
    m_pcssFilterSamples = 32;
    m_csmEnabled = true;

    m_colorProfile = 0;
    m_exposure = 0.52f;
    m_gamma = 2.2f;
    m_saturation = 1.0f;
    m_contrast = 1.0f;
    m_autoExposureEnabled = true;
    m_autoExposureKey = 0.39f;
    m_autoExposureMin = 0.50f;
    m_autoExposureMax = 3.24f;
    m_autoExposureSpeedUp = 3.0f;
    m_autoExposureSpeedDown = 1.0f;

    skybox.generateProceduralSky();
    m_useIBL = true;
    m_showSkybox = true;
    m_skyboxExposure = 0.64f;
    m_skyboxRotation = 0.0f;
    m_skyboxBlur = 0.0f;
    m_iblIntensity = 1.0f;
    m_flipSkyboxX = false;
    m_flipSkyboxY = false;

    m_bloomEnabled = true;
    m_bloomThreshold = 0.65f;
    m_bloomIntensity = 0.60f;
    m_fxaaEnabled = true;
    m_msaaLevel = 4;
    m_ssaoEnabled = false;
    m_ssaoRadius = 0.50f;
    m_ssaoBias = 0.035f;
    m_ssaoPower = 1.8f;
    m_ssaoIntensity = 0.85f;
    m_dofEnabled = false;
    m_dofFocusDistance = 6.0f;
    m_dofFocusRange = 3.0f;
    m_dofBokehRadius = 8.0f;
    m_anisotropyLevel = 16;
    m_showGroundPlane = true;
    m_showGroundGrid = true;
    m_groundPlaneDeleted = false;

    terrain.resetToDefaults();
    terrain.setReliefMode(TerrainReliefMode::OASIS_TESSELLATION);
    terrain.setTessellationDisplacementScale(1.0f);

    auto oasis = terrain.getOASIS();
    if (oasis) {
        oasis->setEnabled(true);
        oasis->setFrustumCulling(true);
        oasis->setGeomorphing(true);
        oasis->setDebugLOD(false);
        oasis->setLodDistanceFactor(1.0f);
        oasis->setMaxLOD(6);
        oasis->setPatchResolution(33);
    }

    m_meshletConfig.enabled = true;
    m_meshletConfig.frustumCulling = true;
    m_meshletConfig.backfaceCulling = true;
    m_meshletConfig.debugColors = false;

    if (water) {
        water->setEnabled(true);
        water->setWaterLevel(-3.5f);
        water->applyPreset(WaterPreset::TROPICAL_CARIBBEAN);
    }

    // 8. Fase 20: Ciclo Día / Noche, Clima, SSR y Contact Shadows por defecto
    m_dayNight.setCycleEnabled(true);
    m_dayNight.setTimeOfDay(14.0f);
    m_dayNight.setDayDurationSeconds(300.0f);
    m_dayNight.setSunLatitude(35.0f);
    m_dayNight.setHorizonHeight(0.0f);
    m_dayNight.setWeather(WeatherType::CLEAR);
    m_dayNight.setWetness(0.0f);
    m_dayNight.setFogEnabled(true);
    m_dayNight.setFogStart(20.0f);
    m_dayNight.setFogEnd(450.0f);
    m_dayNight.setFogDensity(0.45f);

    m_ssrEnabled = true;
    m_ssrMaxSteps = 32;
    m_ssrStepSize = 0.40f;
    m_ssrThickness = 0.50f;
    m_ssrIntensity = 0.85f;

    m_contactShadowsEnabled = true;
    m_contactShadowsDistance = 0.35f;
    m_contactShadowsRaySteps = 16;
    m_contactShadowsIntensity = 0.75f;

    m_selectedEntityName = "";
    m_selectedEntityNames.clear();
    m_selectedLightIndex = -1;
    m_selectedGroupName = "";
    m_undoStack.clear();
    m_redoStack.clear();

    scriptEngine.addLog("[Proyecto] [OK] Nuevo proyecto en blanco creado con todos los valores predeterminados.");
}

void EditorUI::renderMenuBar(Scene& scene, ScriptEngine& scriptEngine, Camera& camera, PhysicsWorld& physicsWorld, ParticleSystem& particleSystem, Terrain& terrain, CharacterController& character, Skybox& skybox, Water* water) {
    if (ImGui::BeginMenuBar()) {
        // Isotipo oficial de ASIS con boton interactivo (Fase 21.8)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.45f, 0.70f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.45f, 0.70f, 0.60f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
        if (ImGui::Button("##ASIS_LogoBtn", ImVec2(22.0f, 18.0f))) {
            m_showAboutModal = true;
        }
        ImVec2 btnMin = ImGui::GetItemRectMin();
        UI::DrawASISLogo(ImGui::GetWindowDrawList(), ImVec2(btnMin.x + 3.0f, btnMin.y + 1.0f), 16.0f, IM_COL32(255, 255, 255, 255), IM_COL32(30, 30, 30, 255));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Acerca de ASIS Engine (v0.22)");
        }
        ImGui::SameLine();

        if (ImGui::BeginMenu("Archivo")) {
            if (ImGui::MenuItem("Nueva Escena", "Ctrl+N")) {
                recordSnapshot(scene, &terrain);
                newProject(scene, camera, terrain, skybox, physicsWorld, scriptEngine, water);
            }
            if (ImGui::MenuItem("Abrir Escena (.asis)...", "Ctrl+O")) {
                std::string path = SceneIO::openFileDialog("Proyectos ASIS Engine (*.asis)\0*.asis\0Todos los archivos (*.*)\0*.*\0");
                if (!path.empty()) {
                    recordSnapshot(scene, &terrain);
                    loadProject(path, scene, camera, terrain, skybox, physicsWorld, scriptEngine, water);
                }
            }
            if (ImGui::MenuItem("Guardar Escena (.asis)...", "Ctrl+S")) {
                std::string path = SceneIO::saveFileDialog("Proyectos ASIS Engine (*.asis)\0*.asis\0Todos los archivos (*.*)\0*.*\0", "asis");
                if (!path.empty()) {
                    if (saveProject(path, scene, camera, terrain, skybox, water)) {
                        scriptEngine.addLog("[Proyecto] [OK] Proyecto guardado con exito: " + path);
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Importar Modelo 3D...", "Ctrl+I")) {
                std::string path = SceneIO::openFileDialog();
                if (!path.empty()) {
                    recordSnapshot(scene);
                    auto ent = SceneIO::importModel(scene, path);
                    if (ent) m_selectedEntityName = ent->getName();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exportar Escena como OBJ (.obj + .mtl)...", "Ctrl+E")) {
                std::string path = SceneIO::saveFileDialog("Wavefront OBJ (*.obj)\0*.obj\0", "obj");
                if (!path.empty()) {
                    SceneIO::exportSceneToOBJ(scene, path);
                }
            }
            if (ImGui::MenuItem("Exportar Escena como glTF 2.0 (.gltf)...")) {
                std::string path = SceneIO::saveFileDialog("glTF 2.0 (*.gltf)\0*.gltf\0", "gltf");
                if (!path.empty()) {
                    SceneIO::exportSceneToGLTF(scene, path);
                }
            }
            if (!m_selectedEntityName.empty()) {
                auto selEnt = scene.getEntity(m_selectedEntityName);
                if (selEnt) {
                    std::string optName = "Exportar '" + selEnt->getName() + "' a OBJ...";
                    if (ImGui::MenuItem(optName.c_str())) {
                        std::string path = SceneIO::saveFileDialog("Wavefront OBJ (*.obj)\0*.obj\0", "obj");
                        if (!path.empty()) {
                            SceneIO::exportEntityToOBJ(*selEnt, path);
                        }
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Limpiar Escena")) {
                recordSnapshot(scene);
                scene.clear();
                m_selectedEntityName = "";
                m_selectedLightIndex = -1;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Restablecer Disposicion de Ventanas")) {
                m_layoutBuilt = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Editar")) {
            if (ImGui::MenuItem("Deshacer", "Ctrl+Z", false, canUndo())) {
                performUndo(scene, physicsWorld, &terrain);
            }
            if (ImGui::MenuItem("Rehacer", "Ctrl+Y", false, canRedo())) {
                performRedo(scene, physicsWorld, &terrain);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Preferencias y Configuracion...", "Ctrl+,")) {
                m_showPreferencesModal = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Render")) {
            if (ImGui::MenuItem("Renderizar Imagen a PNG...", "F12")) {
                m_requestRenderImage = true;
            }
            if (ImGui::MenuItem("Abrir Carpeta de Capturas (Renders)...")) {
                openRendersFolder();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Oclusion Ambiental (SSAO)", nullptr, m_ssaoEnabled)) {
                m_ssaoEnabled = !m_ssaoEnabled;
            }
            if (ImGui::MenuItem("Profundidad de Campo (DoF / Bokeh)", nullptr, m_dofEnabled)) {
                m_dofEnabled = !m_dofEnabled;
            }
            if (ImGui::MenuItem("Reflejos en Pantalla (SSR)", nullptr, m_ssrEnabled)) {
                m_ssrEnabled = !m_ssrEnabled;
            }
            if (ImGui::MenuItem("Sombras de Contacto (Contact Shadows)", nullptr, m_contactShadowsEnabled)) {
                m_contactShadowsEnabled = !m_contactShadowsEnabled;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Bloom HDR (Resplandor)", nullptr, m_bloomEnabled)) {
                m_bloomEnabled = !m_bloomEnabled;
            }
            if (ImGui::MenuItem("Suavizado FXAA", nullptr, m_fxaaEnabled)) {
                m_fxaaEnabled = !m_fxaaEnabled;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Añadir")) {
            if (ImGui::BeginMenu("Malla (Mesh)")) {
                glm::vec3 spawnPos = camera.getPosition() + camera.getFront() * 4.0f;
                spawnPos.y = (std::max)(0.5f, spawnPos.y);

                if (ImGui::MenuItem("Cubo")) {
                    recordSnapshot(scene);
                    std::string name = "Cube_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createCube(name);
                    if (ent) {
                        ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                        m_selectedEntityName = name;
                        m_selectedLightIndex = -1;
                    }
                }
                if (ImGui::MenuItem("Esfera UV")) {
                    recordSnapshot(scene);
                    std::string name = "Sphere_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createSphere(name);
                    if (ent) {
                        ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                        m_selectedEntityName = name;
                        m_selectedLightIndex = -1;
                    }
                }
                if (ImGui::MenuItem("Cilindro")) {
                    recordSnapshot(scene);
                    std::string name = "Cylinder_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createCylinder(name);
                    if (ent) {
                        ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                        m_selectedEntityName = name;
                        m_selectedLightIndex = -1;
                    }
                }
                if (ImGui::MenuItem("Cono")) {
                    recordSnapshot(scene);
                    std::string name = "Cone_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createCone(name);
                    if (ent) {
                        ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                        m_selectedEntityName = name;
                        m_selectedLightIndex = -1;
                    }
                }
                if (ImGui::MenuItem("Toroide (Donut)")) {
                    recordSnapshot(scene);
                    std::string name = "Torus_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createTorus(name);
                    if (ent) {
                        ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                        m_selectedEntityName = name;
                        m_selectedLightIndex = -1;
                    }
                }
                if (ImGui::MenuItem("Plano")) {
                    recordSnapshot(scene);
                    std::string name = "Plane_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createPlane(name);
                    if (ent) {
                        ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                        m_selectedEntityName = name;
                        m_selectedLightIndex = -1;
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Sistema de Particulas (VFX)")) {
                glm::vec3 pPos = camera.getPosition() + camera.getFront() * 4.0f;
                pPos.y = (std::max)(0.5f, pPos.y);

                if (ImGui::MenuItem("Fuego (Fire)")) {
                    particleSystem.addEmitter("Fuego_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::FIRE, pPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Chispas (Sparks)")) {
                    particleSystem.addEmitter("Chispas_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::SPARKS, pPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Humo (Smoke)")) {
                    particleSystem.addEmitter("Humo_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::SMOKE, pPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Lluvia (Rain)")) {
                    particleSystem.addEmitter("Lluvia_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::RAIN, pPos + glm::vec3(0, 5, 0));
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Magia (Magic)")) {
                    particleSystem.addEmitter("Magia_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::MAGIC, pPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Emisor Personalizado")) {
                    particleSystem.addEmitter("Emisor_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::CUSTOM, pPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Luz Puntual")) {
                recordSnapshot(scene);
                glm::vec3 spawnPos = camera.getPosition() + camera.getFront() * 3.5f;
                auto& lights = scene.getPointLights();
                if (lights.size() < 8) {
                    PointLight l("Luz Puntual " + std::to_string(lights.size() + 1), spawnPos);
                    scene.addPointLight(l);
                    m_selectedLightIndex = static_cast<int>(lights.size() - 1);
                    m_selectedEntityName = "";
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Gizmos")) {
            if (ImGui::MenuItem("Traslacion (W)", nullptr, m_gizmoOperation == ImGuizmo::TRANSLATE)) m_gizmoOperation = ImGuizmo::TRANSLATE;
            if (ImGui::MenuItem("Rotacion (E)", nullptr, m_gizmoOperation == ImGuizmo::ROTATE)) m_gizmoOperation = ImGuizmo::ROTATE;
            if (ImGui::MenuItem("Escala (R)", nullptr, m_gizmoOperation == ImGuizmo::SCALE)) m_gizmoOperation = ImGuizmo::SCALE;
            ImGui::Separator();
            if (ImGui::MenuItem("Espacio Local", nullptr, m_gizmoMode == ImGuizmo::LOCAL)) m_gizmoMode = ImGuizmo::LOCAL;
            if (ImGui::MenuItem("Espacio Mundo (World)", nullptr, m_gizmoMode == ImGuizmo::WORLD)) m_gizmoMode = ImGuizmo::WORLD;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Iluminacion")) {
            ImGui::MenuItem("Iluminacion IBL y Reflejos", nullptr, &m_useIBL);
            ImGui::MenuItem("Mostrar Cielo Skybox en Fondo", nullptr, &m_showSkybox);
            ImGui::Separator();
            ImGui::MenuItem("Sombras Dinamicas (Shadow Mapping)", nullptr, &m_shadowsEnabled);
            ImGui::Separator();
            if (ImGui::MenuItem("Preajuste: Mediodia Cenital")) {
                m_sunAzimuth = 45.0f;
                m_sunElevation = 80.0f;
                m_lightColor = glm::vec3(1.0f, 0.98f, 0.95f);
                m_lightIntensity = 1.25f;
            }
            if (ImGui::MenuItem("Preajuste: Atardecer Dorado")) {
                m_sunAzimuth = 110.0f;
                m_sunElevation = 18.0f;
                m_lightColor = glm::vec3(1.0f, 0.65f, 0.30f);
                m_lightIntensity = 1.30f;
            }
            if (ImGui::MenuItem("Preajuste: Estudio / Diagonal")) {
                m_sunAzimuth = 45.0f;
                m_sunElevation = 45.0f;
                m_lightColor = glm::vec3(1.0f, 0.96f, 0.88f);
                m_lightIntensity = 1.15f;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Ver")) {
            if (ImGui::BeginMenu("Espacios de Trabajo / Layouts")) {
                if (ImGui::MenuItem("General / Completo", nullptr, m_currentLayout == UILayoutPreset::GENERAL)) {
                    setLayoutPreset(UILayoutPreset::GENERAL);
                }
                if (ImGui::MenuItem("Iluminacion & Cine", nullptr, m_currentLayout == UILayoutPreset::LIGHTING_CINEMA)) {
                    setLayoutPreset(UILayoutPreset::LIGHTING_CINEMA);
                }
                if (ImGui::MenuItem("Terreno & Modelado", nullptr, m_currentLayout == UILayoutPreset::TERRAIN_MODELING)) {
                    setLayoutPreset(UILayoutPreset::TERRAIN_MODELING);
                }
                if (ImGui::MenuItem("Modo Juego (Inmersivo)", nullptr, m_currentLayout == UILayoutPreset::GAME_VIEW)) {
                    setLayoutPreset(UILayoutPreset::GAME_VIEW);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            ImGui::MenuItem("Plano de Suelo Base", nullptr, &m_showGroundPlane);
            ImGui::MenuItem("Cuadricula 3D (Grid)", nullptr, &m_showGroundGrid);
            ImGui::Separator();
            if (ImGui::MenuItem("Alternar Interfaz (F1)", nullptr, m_visible)) {
                toggleVisibility();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Juego")) {
            if (!isPlaying()) {
                if (ImGui::MenuItem("Iniciar Modo Juego (PIE)", "Alt+P / F5")) {
                    triggerPlayStart();
                }
            } else {
                if (ImGui::MenuItem("Detener Modo Juego", "Esc")) {
                    triggerPlayStop();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Alternar 1ra / 3ra Persona", "V", false, isPlaying())) {
                character.toggleCameraMode();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Abrir Ajustes de Personaje...")) {
                m_activePropertyTab = BlenderPropertyTab::PLAYER;
            }
            ImGui::EndMenu();
        }

        // Pestañas de Espacio de Trabajo estilo Blender 4/5 (Fase 21.5)
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        auto renderWorkspaceTab = [&](const char* label, int tabIdx, UILayoutPreset preset, BlenderPropertyTab propTab) {
            bool isSelected = (m_activeWorkspaceTab == tabIdx);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.278f, 0.447f, 0.702f, 1.0f)); // Azul Blender oficial
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
            }
            if (ImGui::SmallButton(label)) {
                m_activeWorkspaceTab = tabIdx;
                setLayoutPreset(preset);
                m_activePropertyTab = propTab;
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
        };

        renderWorkspaceTab("Layout", 0, UILayoutPreset::GENERAL, BlenderPropertyTab::OBJECT);
        renderWorkspaceTab("Modelado/Terreno", 1, UILayoutPreset::TERRAIN_MODELING, BlenderPropertyTab::TERRAIN);
        renderWorkspaceTab("Shading/PBR", 2, UILayoutPreset::GENERAL, BlenderPropertyTab::MATERIAL);
        renderWorkspaceTab("Iluminación/Cine", 3, UILayoutPreset::LIGHTING_CINEMA, BlenderPropertyTab::LIGHTING);
        renderWorkspaceTab("Scripting/IA", 4, UILayoutPreset::GENERAL, BlenderPropertyTab::SCRIPTS);
        renderWorkspaceTab("Juego", 5, UILayoutPreset::GAME_VIEW, BlenderPropertyTab::PLAYER);

        // Extremo derecho de la barra superior: Boton Jugar y Boton Limpiar (Fase 21.8)
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float rightOffset = 180.0f;
        if (ImGui::GetCursorPosX() < viewport->WorkSize.x - rightOffset) {
            ImGui::SameLine(viewport->WorkSize.x - rightOffset);
        }

        if (!isPlaying()) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.62f, 0.30f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.78f, 0.38f, 1.0f));
            if (UI::ButtonWithIcon(UI::IconType::PLAY, "Jugar (Alt+P)", ImVec2(0.0f, 18.0f))) {
                triggerPlayStart();
            }
            ImGui::PopStyleColor(2);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
            if (UI::ButtonWithIcon(UI::IconType::STOP, "Detener (Esc)", ImVec2(0.0f, 18.0f))) {
                triggerPlayStop();
            }
            ImGui::PopStyleColor(2);
        }

        ImGui::SameLine();
        if (ImGui::Button("Limpiar (F1)", ImVec2(0.0f, 18.0f))) {
            recordSnapshot(scene);
            scene.clear();
            m_selectedEntityName = "";
            m_selectedLightIndex = -1;
        }

        ImGui::EndMenuBar();
    }
}

void EditorUI::renderViewport(Scene& scene, Camera& camera, Framebuffer& framebuffer, PhysicsWorld& physicsWorld, Terrain& terrain, CharacterController& character, GLuint displayTexture, ParticleSystem* particleSystem, ScriptEngine* scriptEngine) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Viewport 3D")) {
        m_isViewportHovered = ImGui::IsWindowHovered();
        m_isViewportFocused = ImGui::IsWindowFocused();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 0.0f && avail.y > 0.0f) {
            m_viewportSize = glm::vec2(avail.x, avail.y);
            framebuffer.resize(static_cast<int>(avail.x), static_cast<int>(avail.y), m_msaaLevel);
            terrain.setViewportAspectRatio(avail.x / avail.y);
        }

        ImVec2 screenPos = ImGui::GetCursorScreenPos();
        m_viewportPos = glm::vec2(screenPos.x, screenPos.y);

        // 1. Dibujar la textura FBO (con Bloom/FXAA si están activos) (Invertida verticalmente en UV para OpenGL)
        GLuint texToShow = (displayTexture != 0) ? displayTexture : framebuffer.getColorTexture();
        ImTextureID texID = (ImTextureID)(intptr_t)texToShow;
        ImGui::Image(texID, avail, ImVec2(0, 1), ImVec2(1, 0));

        // Drag & Drop Target en Viewport para Prefabs y Presets PBR (Fase 21.8)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASIS_PREFAB")) {
                const char* pFile = (const char*)payload->Data;
                recordSnapshot(scene);
                glm::vec3 spawnPos = camera.getTarget();
                PrefabManager::instantiatePrefab(pFile, scene, &physicsWorld, spawnPos);
                if (scriptEngine) scriptEngine->addLog("[DragDrop] [OK] Prefab instanciado en escena: " + std::string(pFile));
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASIS_MATERIAL_PRESET")) {
                const char* presetName = (const char*)payload->Data;
                if (!m_selectedEntityName.empty()) {
                    auto ent = scene.getEntity(m_selectedEntityName);
                    if (ent) {
                        MaterialManager::get().applyToEntity(*ent, presetName);
                        if (scriptEngine) scriptEngine->addLog("[DragDrop] [OK] Material '" + std::string(presetName) + "' aplicado a " + m_selectedEntityName);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // 2. Barra flotante vertical estilo T-Panel a la izquierda del Viewport (Fase 21.5)
        ImGui::SetCursorScreenPos(ImVec2(screenPos.x + 8.0f, screenPos.y + 8.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 0.88f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 3.0f));
        ImGui::BeginChild("TPanelToolbar", ImVec2(36.0f, isPlaying() ? 90.0f : 185.0f), true, 
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        if (!isPlaying()) {
            auto drawTButton = [&](const char* label, bool active, const char* tip) -> bool {
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.278f, 0.447f, 0.702f, 1.0f));
                else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 0.85f));
                bool clicked = ImGui::Button(label, ImVec2(28.0f, 22.0f));
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
                return clicked;
            };

            if (drawTButton("W", m_gizmoOperation == ImGuizmo::TRANSLATE, "Traslacion / Mover [W]")) m_gizmoOperation = ImGuizmo::TRANSLATE;
            if (drawTButton("E", m_gizmoOperation == ImGuizmo::ROTATE, "Rotacion [E]")) m_gizmoOperation = ImGuizmo::ROTATE;
            if (drawTButton("R", m_gizmoOperation == ImGuizmo::SCALE, "Escala [R]")) m_gizmoOperation = ImGuizmo::SCALE;

            ImGui::Separator();

            const char* modeLabel = (m_gizmoMode == ImGuizmo::WORLD) ? "Wld" : "Loc";
            if (drawTButton(modeLabel, false, "Espacio de Gizmo (World / Local)")) {
                m_gizmoMode = (m_gizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }

            if (UI::IconButton("##t_snap", UI::IconType::SNAP, ImVec2(28.0f, 22.0f), "Ajuste de Rejilla (Snap)", m_useSnap ? IM_COL32(71, 114, 179, 255) : 0)) {
                m_useSnap = !m_useSnap;
            }

            if (UI::IconButton("##t_pause", m_scriptsPaused ? UI::IconType::PLAY : UI::IconType::PAUSE, ImVec2(28.0f, 22.0f), "Pausar / Reanudar animaciones Lua [Espacio]", m_scriptsPaused ? IM_COL32(230, 80, 80, 255) : 0)) {
                m_scriptsPaused = !m_scriptsPaused;
            }
        } else {
            const char* vLabel = (character.getCameraMode() == CharacterCameraMode::FIRST_PERSON) ? "1P" : "3P";
            if (ImGui::Button(vLabel, ImVec2(28.0f, 22.0f))) {
                character.toggleCameraMode();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Alternar 1ra / 3ra Persona [V]");

            glm::vec3 vel = character.getVelocity();
            float speedKmh = glm::length(glm::vec2(vel.x, vel.z)) * 3.6f;
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "%.0f", speedKmh);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Velocidad: %.1f km/h", speedKmh);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        // 3. Pildora unificada de Shading Modes (Wireframe, Solid, OASIS, Rendered) + Overlays + SSR (Fase 21.8)
        float pillsWidth = isPlaying() ? 230.0f : 190.0f;
        ImGui::SetCursorScreenPos(ImVec2(screenPos.x + avail.x - pillsWidth - 8.0f, screenPos.y + 8.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 0.88f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 3.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 0.0f));
        ImGui::BeginChild("ViewportTopRightPills", ImVec2(pillsWidth, 28.0f), true, 
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        // 4 Botones Circulares de Shading (Blender style)
        bool isWire = (m_shadingMode == ViewportShadingMode::WIREFRAME || m_wireframeMode);
        if (UI::IconButton("##sh_wire", UI::IconType::SHADING_WIREFRAME, ImVec2(22.0f, 20.0f), "Modo Malla de Alambre (Wireframe)", isWire ? IM_COL32(71, 114, 179, 255) : 0)) {
            m_shadingMode = ViewportShadingMode::WIREFRAME;
            m_wireframeMode = true;
        }

        ImGui::SameLine();
        bool isSolid = (m_shadingMode == ViewportShadingMode::SOLID && !m_wireframeMode);
        if (UI::IconButton("##sh_solid", UI::IconType::SHADING_SOLID, ImVec2(22.0f, 20.0f), "Modo Solido (Unlit / Geometria)", isSolid ? IM_COL32(71, 114, 179, 255) : 0)) {
            m_shadingMode = ViewportShadingMode::SOLID;
            m_wireframeMode = false;
        }

        ImGui::SameLine();
        bool isOasis = (m_shadingMode == ViewportShadingMode::OASIS_MESHLETS && !m_wireframeMode);
        if (UI::IconButton("##sh_oasis", UI::IconType::SHADING_OASIS, ImVec2(22.0f, 20.0f), "Depuracion Meshlets OASIS (Nanite Colors)", isOasis ? IM_COL32(71, 114, 179, 255) : 0)) {
            m_shadingMode = ViewportShadingMode::OASIS_MESHLETS;
            m_wireframeMode = false;
        }

        ImGui::SameLine();
        bool isRendered = (m_shadingMode == ViewportShadingMode::RENDERED && !m_wireframeMode);
        if (UI::IconButton("##sh_rendered", UI::IconType::SHADING_RENDERED, ImVec2(22.0f, 20.0f), "Modo Rendered PBR Completo (Materiales, IBL, Sombras)", isRendered ? IM_COL32(71, 114, 179, 255) : 0)) {
            m_shadingMode = ViewportShadingMode::RENDERED;
            m_wireframeMode = false;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Boton Overlays (mostrar/ocultar textos 3D, rejilla y marcadores)
        if (UI::IconButton("##sh_overlays", UI::IconType::OVERLAYS, ImVec2(22.0f, 20.0f), "Mostrar / Ocultar Overlays (Textos 3D, Gizmos, Rejilla)", m_showOverlays ? IM_COL32(71, 114, 179, 255) : 0)) {
            m_showOverlays = !m_showOverlays;
        }

        ImGui::SameLine();
        // SSR toggle rapido
        if (UI::IconButton("##sh_ssr", UI::IconType::WAVE, ImVec2(22.0f, 20.0f), "Screen-Space Reflections (SSR)", m_ssrEnabled ? IM_COL32(71, 114, 179, 255) : 0)) {
            m_ssrEnabled = !m_ssrEnabled;
        }

        if (isPlaying()) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.22f, 0.22f, 0.9f));
            if (UI::ButtonWithIcon(UI::IconType::STOP, "Fin", ImVec2(36.0f, 20.0f), "Detener Juego [Esc]")) {
                triggerPlayStop();
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        // 3.1. Gizmo de Navegacion 3D estilo Blender (Fase 21.8)
        if (!isPlaying()) {
            ImVec2 gizmoCenter(screenPos.x + avail.x - 42.0f, screenPos.y + 82.0f);
            UI::DrawNavigationGizmo(ImGui::GetWindowDrawList(), camera, gizmoCenter, 34.0f);
        }

        // Atajos de teclado W, E, R para alternar gizmo, F para enfocar y Espacio para pausar scripts (solo en modo Editor)
        if (!isPlaying() && !ImGui::GetIO().WantTextInput && m_isViewportFocused) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOperation = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOperation = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOperation = ImGuizmo::SCALE;
            if (ImGui::IsKeyPressed(ImGuiKey_Space)) m_scriptsPaused = !m_scriptsPaused;
            if (ImGui::IsKeyPressed(ImGuiKey_I)) {
                if (!m_selectedEntityName.empty()) {
                    auto ent = scene.getEntity(m_selectedEntityName);
                    if (ent) {
                        Sequencer::get().recordEntityKeyframe(*ent);
                        char buf[96];
                        snprintf(buf, sizeof(buf), "Keyframe registrado para '%s' en %.2fs", m_selectedEntityName.c_str(), Sequencer::get().getCurrentTime());
                        setStatusBarNotification(buf);
                    }
                } else {
                    m_showKeyframeInsertPopup = true;
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                if (!m_selectedGroupName.empty() || m_selectedEntityNames.size() > 1) {
                    camera.resetToFocus(m_groupPivot);
                } else if (!m_selectedEntityName.empty()) {
                    auto entity = scene.getEntity(m_selectedEntityName);
                    if (entity) {
                        camera.resetToFocus(entity->getPosition());
                    } else {
                        camera.resetToFocus(glm::vec3(0.0f, 0.5f, 0.0f));
                    }
                } else {
                    camera.resetToFocus(glm::vec3(0.0f, 0.5f, 0.0f));
                }
            }
        }

        // 3. Renderizar Gizmo 3D (ImGuizmo) sobre Grupo / Selección Múltiple o Entidad individual o Luz (SOLO en Modo Editor)
        if (!isPlaying() && (!m_selectedGroupName.empty() || m_selectedEntityNames.size() > 1)) {
            std::vector<std::shared_ptr<Entity>> groupEnts;
            if (!m_selectedGroupName.empty()) {
                groupEnts = scene.getEntitiesInGroup(m_selectedGroupName);
            } else {
                for (const auto& name : m_selectedEntityNames) {
                    auto ent = scene.getEntity(name);
                    if (ent) groupEnts.push_back(ent);
                }
            }

            if (!groupEnts.empty()) {
                if (!m_isUsingGizmo) {
                    glm::vec3 center(0.0f);
                    for (auto& ge : groupEnts) {
                        if (ge) center += ge->getPosition();
                    }
                    m_groupPivot = center / static_cast<float>(groupEnts.size());
                }

                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(screenPos.x, screenPos.y, avail.x, avail.y);

                glm::mat4 view = camera.getViewMatrix();
                float aspect = (avail.y > 0.0f) ? avail.x / avail.y : 16.0f / 9.0f;
                glm::mat4 proj = camera.getProjectionMatrix(aspect);

                glm::mat4 groupMat = glm::translate(glm::mat4(1.0f), m_groupPivot);
                glm::mat4 deltaMatrix(1.0f);

                float snapVal[3] = { m_snapTranslation, m_snapTranslation, m_snapTranslation };
                if (m_gizmoOperation == ImGuizmo::ROTATE) snapVal[0] = m_snapRotation;
                else if (m_gizmoOperation == ImGuizmo::SCALE) snapVal[0] = m_snapScale;

                bool manipulated = ImGuizmo::Manipulate(
                    glm::value_ptr(view),
                    glm::value_ptr(proj),
                    static_cast<ImGuizmo::OPERATION>(m_gizmoOperation),
                    static_cast<ImGuizmo::MODE>(m_gizmoMode),
                    glm::value_ptr(groupMat),
                    glm::value_ptr(deltaMatrix),
                    m_useSnap ? snapVal : nullptr
                );

                if (ImGuizmo::IsUsing() && !m_wasUsingGizmo) {
                    recordSnapshot(scene);
                }
                m_isUsingGizmo = ImGuizmo::IsUsing();
                m_wasUsingGizmo = m_isUsingGizmo;

                if (manipulated) {
                    float dTrans[3], dRot[3], dScale[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(deltaMatrix), dTrans, dRot, dScale);

                    glm::vec3 deltaT(dTrans[0], dTrans[1], dTrans[2]);
                    glm::vec3 deltaR(dRot[0], dRot[1], dRot[2]);
                    glm::vec3 deltaS(dScale[0], dScale[1], dScale[2]);

                    glm::mat4 rotMat(1.0f);
                    if (glm::length(deltaR) > 0.0001f) {
                        rotMat = glm::rotate(rotMat, glm::radians(deltaR.z), glm::vec3(0, 0, 1));
                        rotMat = glm::rotate(rotMat, glm::radians(deltaR.y), glm::vec3(0, 1, 0));
                        rotMat = glm::rotate(rotMat, glm::radians(deltaR.x), glm::vec3(1, 0, 0));
                    }

                    for (auto& ge : groupEnts) {
                        if (!ge) continue;

                        glm::vec3 oldPos = ge->getPosition();
                        glm::vec3 offset = oldPos - m_groupPivot;

                        if (m_gizmoOperation == ImGuizmo::SCALE) {
                            offset *= deltaS;
                            glm::vec3 oldScale = ge->getScale();
                            ge->setScale(oldScale.x * deltaS.x, oldScale.y * deltaS.y, oldScale.z * deltaS.z);
                        } else if (m_gizmoOperation == ImGuizmo::ROTATE) {
                            offset = glm::vec3(rotMat * glm::vec4(offset, 0.0f));
                            glm::vec3 oldRot = ge->getRotation();
                            ge->setRotation(oldRot.x + deltaR.x, oldRot.y + deltaR.y, oldRot.z + deltaR.z);
                        }

                        glm::vec3 newPos = m_groupPivot + deltaT + offset;
                        ge->setPosition(newPos.x, newPos.y, newPos.z);

                        physicsWorld.syncEntityToPhysics(ge);
                    }

                    m_groupPivot += deltaT;
                }
            }
        } else if (!isPlaying() && !m_selectedEntityName.empty()) {
            auto entity = scene.getEntity(m_selectedEntityName);
            if (entity) {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(screenPos.x, screenPos.y, avail.x, avail.y);

                glm::mat4 view = camera.getViewMatrix();
                float aspect = (avail.y > 0.0f) ? avail.x / avail.y : 16.0f / 9.0f;
                glm::mat4 proj = camera.getProjectionMatrix(aspect);
                glm::mat4 model = entity->getModelMatrix();

                float snapVal[3] = { m_snapTranslation, m_snapTranslation, m_snapTranslation };
                if (m_gizmoOperation == ImGuizmo::ROTATE) snapVal[0] = m_snapRotation;
                else if (m_gizmoOperation == ImGuizmo::SCALE) snapVal[0] = m_snapScale;

                bool manipulated = ImGuizmo::Manipulate(
                    glm::value_ptr(view),
                    glm::value_ptr(proj),
                    static_cast<ImGuizmo::OPERATION>(m_gizmoOperation),
                    static_cast<ImGuizmo::MODE>(m_gizmoMode),
                    glm::value_ptr(model),
                    nullptr,
                    m_useSnap ? snapVal : nullptr
                );

                if (ImGuizmo::IsUsing() && !m_wasUsingGizmo) {
                    recordSnapshot(scene);
                }
                m_isUsingGizmo = ImGuizmo::IsUsing();
                m_wasUsingGizmo = m_isUsingGizmo;

                if (manipulated) {
                    float t[3], r[3], s[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), t, r, s);
                    entity->setPosition(t[0], t[1], t[2]);
                    entity->setRotation(r[0], r[1], r[2]);
                    entity->setScale(s[0], s[1], s[2]);
                    entity->syncQuaternionFromEuler();

                    // Sincronizar posición con Bullet Physics
                    physicsWorld.syncEntityToPhysics(entity);

                    // Auto-Keying o edición sobre fotograma clave existente estilo Blender
                    auto& seq = Sequencer::get();
                    bool hasKeyframeAtCurrent = false;
                    for (const auto& trk : seq.getEntityTracks()) {
                        if (trk.entityName == entity->getName()) {
                            for (const auto& kf : trk.keyframes) {
                                if (std::abs(kf.time - seq.getCurrentTime()) < 0.04f) {
                                    hasKeyframeAtCurrent = true;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (seq.isAutoKeyEnabled() || hasKeyframeAtCurrent) {
                        seq.recordEntityKeyframe(*entity);
                    }
                }
            }
        } else if (!isPlaying() && m_selectedLightIndex >= 0 && m_selectedLightIndex < static_cast<int>(scene.getPointLights().size())) {
            auto& pointLights = scene.getPointLights();
            auto& light = pointLights[m_selectedLightIndex];

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(screenPos.x, screenPos.y, avail.x, avail.y);

            glm::mat4 view = camera.getViewMatrix();
            float aspect = (avail.y > 0.0f) ? avail.x / avail.y : 16.0f / 9.0f;
            glm::mat4 proj = camera.getProjectionMatrix(aspect);
            glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), light.position);

            float snapVal[3] = { m_snapTranslation, m_snapTranslation, m_snapTranslation };

            bool manipulated = ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(proj),
                ImGuizmo::TRANSLATE,
                ImGuizmo::WORLD,
                glm::value_ptr(lightModel),
                nullptr,
                m_useSnap ? snapVal : nullptr
            );

            if (ImGuizmo::IsUsing() && !m_wasUsingGizmo) {
                recordSnapshot(scene);
            }
            m_isUsingGizmo = ImGuizmo::IsUsing();
            m_wasUsingGizmo = m_isUsingGizmo;

            if (manipulated) {
                light.position = glm::vec3(lightModel[3]);
            }
        } else {
            m_isUsingGizmo = false;
            m_wasUsingGizmo = false;
        }

        // 3.5. Dibujar marcadores visuales para luces puntuales en el Viewport (Solo en Modo Editor)
        const auto& pLights = scene.getPointLights();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Menú Flotante Rápido con tecla N en Viewport (Fase 21.8)
        if (m_isViewportHovered && !isPlaying() && !ImGui::GetIO().WantTextInput && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && ImGui::IsKeyPressed(ImGuiKey_N)) {
            ImGui::OpenPopup("ASIS_QuickMenu_N");
        }

        if (ImGui::BeginPopup("ASIS_QuickMenu_N")) {
            ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.0f, 1.0f), "Acciones Rapidas y Creacion [N]");
            ImGui::Separator();

            glm::vec3 spawnPos = camera.getTarget();

            if (ImGui::BeginMenu("Anadir Objeto 3D")) {
                if (ImGui::MenuItem("Cubo")) {
                    recordSnapshot(scene);
                    std::string name = "Cube_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createCube(name);
                    if (ent) { ent->setPosition(spawnPos.x, (std::max)(0.5f, spawnPos.y), spawnPos.z); m_selectedEntityName = name; }
                }
                if (ImGui::MenuItem("Esfera UV")) {
                    recordSnapshot(scene);
                    std::string name = "Sphere_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createSphere(name);
                    if (ent) { ent->setPosition(spawnPos.x, (std::max)(0.5f, spawnPos.y), spawnPos.z); m_selectedEntityName = name; }
                }
                if (ImGui::MenuItem("Cilindro")) {
                    recordSnapshot(scene);
                    std::string name = "Cylinder_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createCylinder(name);
                    if (ent) { ent->setPosition(spawnPos.x, (std::max)(0.5f, spawnPos.y), spawnPos.z); m_selectedEntityName = name; }
                }
                if (ImGui::MenuItem("Plano")) {
                    recordSnapshot(scene);
                    std::string name = "Plane_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createPlane(name);
                    if (ent) { ent->setPosition(spawnPos.x, (std::max)(0.0f, spawnPos.y), spawnPos.z); m_selectedEntityName = name; }
                }
                if (ImGui::MenuItem("Cono")) {
                    recordSnapshot(scene);
                    std::string name = "Cone_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createCone(name);
                    if (ent) { ent->setPosition(spawnPos.x, (std::max)(0.5f, spawnPos.y), spawnPos.z); m_selectedEntityName = name; }
                }
                if (ImGui::MenuItem("Toroide (Donut)")) {
                    recordSnapshot(scene);
                    std::string name = "Torus_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createTorus(name);
                    if (ent) { ent->setPosition(spawnPos.x, (std::max)(0.5f, spawnPos.y), spawnPos.z); m_selectedEntityName = name; }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Importar Modelo 3D desde Disco...")) {
                    std::string path = SceneIO::openFileDialog();
                    if (!path.empty()) {
                        recordSnapshot(scene);
                        auto ent = SceneIO::importModel(scene, path);
                        if (ent) m_selectedEntityName = ent->getName();
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Anadir Luz / Entorno")) {
                if (ImGui::MenuItem("Luz Puntual")) {
                    recordSnapshot(scene);
                    auto& lights = scene.getPointLights();
                    if (lights.size() < 8) {
                        PointLight l("Luz Puntual " + std::to_string(lights.size() + 1), spawnPos + glm::vec3(0, 1.5f, 0));
                        scene.addPointLight(l);
                        m_selectedLightIndex = static_cast<int>(lights.size() - 1);
                        m_selectedEntityName = "";
                    }
                }
                if (ImGui::MenuItem("Crear Zona Trigger (Sensor)")) {
                    recordSnapshot(scene);
                    std::string name = "Trigger_" + std::to_string(scene.getEntityCount() + 1);
                    auto ent = scene.createCube(name);
                    if (ent) {
                        ent->setPosition(spawnPos.x, spawnPos.y + 0.5f, spawnPos.z);
                        ent->setScale(2.0f, 1.0f, 2.0f);
                        ent->setTrigger(true);
                        ent->setTriggerTag("Sensor");
                        m_selectedEntityName = name;
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Anadir Efecto VFX")) {
                if (ImGui::MenuItem("Fuego (Fire)")) {
                    if (particleSystem) particleSystem->addEmitter("Fuego_" + std::to_string(particleSystem->getEmitterCount() + 1), EmitterType::FIRE, spawnPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Humo (Smoke)")) {
                    if (particleSystem) particleSystem->addEmitter("Humo_" + std::to_string(particleSystem->getEmitterCount() + 1), EmitterType::SMOKE, spawnPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Chispas (Sparks)")) {
                    if (particleSystem) particleSystem->addEmitter("Chispas_" + std::to_string(particleSystem->getEmitterCount() + 1), EmitterType::SPARKS, spawnPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Lluvia (Rain)")) {
                    if (particleSystem) particleSystem->addEmitter("Lluvia_" + std::to_string(particleSystem->getEmitterCount() + 1), EmitterType::RAIN, spawnPos + glm::vec3(0, 5, 0));
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                if (ImGui::MenuItem("Portal Magico")) {
                    if (particleSystem) particleSystem->addEmitter("Portal_" + std::to_string(particleSystem->getEmitterCount() + 1), EmitterType::MAGIC, spawnPos);
                    m_activePropertyTab = BlenderPropertyTab::PARTICLES;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Instanciar Prefab")) {
                if (m_availablePrefabs.empty()) {
                    m_availablePrefabs = PrefabManager::getAvailablePrefabs("assets/prefabs");
                }
                if (m_availablePrefabs.empty()) {
                    ImGui::TextDisabled("No hay prefabs disponibles.");
                } else {
                    for (const auto& pFile : m_availablePrefabs) {
                        std::string displayName = pFile;
                        size_t dotPos = displayName.rfind(".prefab");
                        if (dotPos != std::string::npos) displayName = displayName.substr(0, dotPos);
                        if (ImGui::MenuItem(displayName.c_str())) {
                            recordSnapshot(scene);
                            PrefabManager::instantiatePrefab(pFile, scene, &physicsWorld, spawnPos);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (!m_selectedEntityName.empty()) {
                if (ImGui::BeginMenu("Aplicar Material PBR Rapido")) {
                    auto& matMgr = MaterialManager::get();
                    auto ent = scene.getEntity(m_selectedEntityName);
                    if (ent) {
                        const char* presets[] = {
                            "Oro Pulido", "Cobre", "Acero Inoxidable", "Cromo / Espejo",
                            "Vidrio / Cristal", "Plastico Brillante", "Plastico Mate",
                            "Neon Cian Emisivo", "Neon Carmesi", "Obsidiana Negra",
                            "Marmol Blanco", "Hormigon / Concreto", "Hierro Oxidado", "Madera / Corteza"
                        };
                        for (const char* pName : presets) {
                            if (ImGui::MenuItem(pName)) {
                                matMgr.applyToEntity(*ent, pName);
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Vista Rapida / Camara")) {
                if (ImGui::MenuItem("Enfocar Seleccion", "F", false, !m_selectedEntityName.empty())) {
                    auto ent = scene.getEntity(m_selectedEntityName);
                    if (ent) camera.resetToFocus(ent->getPosition());
                }
                if (ImGui::MenuItem("Resetear Transformacion", nullptr, false, !m_selectedEntityName.empty())) {
                    auto ent = scene.getEntity(m_selectedEntityName);
                    if (ent) {
                        recordSnapshot(scene);
                        ent->setPosition(0.0f, 0.5f, 0.0f);
                        ent->setRotation(0.0f, 0.0f, 0.0f);
                        ent->setScale(1.0f, 1.0f, 1.0f);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Alternar Cuadricula (Grid)", nullptr, m_showGroundGrid)) {
                    m_showGroundGrid = !m_showGroundGrid;
                }
                if (ImGui::MenuItem("Alternar Modo Alambre (Wireframe)", nullptr, m_wireframeMode)) {
                    m_wireframeMode = !m_wireframeMode;
                }
                if (ImGui::MenuItem("Alternar Overlays", nullptr, m_showOverlays)) {
                    m_showOverlays = !m_showOverlays;
                }
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }

        if (!isPlaying() && m_showOverlays) {
            glm::mat4 curView = camera.getViewMatrix();
            float curAspect = (avail.y > 0.0f) ? avail.x / avail.y : 16.0f / 9.0f;
            glm::mat4 curProj = camera.getProjectionMatrix(curAspect);

            for (size_t i = 0; i < pLights.size(); ++i) {
                const auto& pl = pLights[i];
                glm::vec4 clip = curProj * curView * glm::vec4(pl.position, 1.0f);
                if (clip.w > 0.001f) {
                    float ndcX = clip.x / clip.w;
                    float ndcY = clip.y / clip.w;
                    if (ndcX >= -1.1f && ndcX <= 1.1f && ndcY >= -1.1f && ndcY <= 1.1f) {
                        float scrX = screenPos.x + (ndcX * 0.5f + 0.5f) * avail.x;
                        float scrY = screenPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * avail.y;

                        bool isSel = (m_selectedLightIndex == static_cast<int>(i));
                        ImU32 col = isSel ? ImColor(255, 220, 60) : (pl.enabled ? ImColor(255, 235, 120) : ImColor(140, 140, 140));

                        drawList->AddCircle(ImVec2(scrX, scrY), isSel ? 11.0f : 8.0f, col, 16, isSel ? 2.5f : 1.5f);
                        drawList->AddCircleFilled(ImVec2(scrX, scrY), 3.5f, col);
                        if (isSel) {
                            drawList->AddText(ImVec2(scrX + 14, scrY - 7), col, pl.name.c_str());
                        }
                    }
                }
            }

            // 3.55. Dibujar Marcadores y Cajas de Alambre de Zonas Trigger (Fase 17 - Solo en Modo Editor)
            glm::mat4 curViewProj = curProj * curView;

            // Plain Axes estilo Blender en el centroide / pivote del grupo o selección múltiple
            if (!m_selectedGroupName.empty() || m_selectedEntityNames.size() > 1) {
                std::string badgeLabel = !m_selectedGroupName.empty() ? m_selectedGroupName : (std::to_string(m_selectedEntityNames.size()) + " obj");
                drawPlainAxes(drawList, m_groupPivot, curViewProj, screenPos, avail, badgeLabel);
            }

            for (const auto& [entName, entPtr] : scene.getEntities()) {
                if (entPtr && entPtr->isTrigger()) {
                    drawTriggerWireframe(drawList, *entPtr, curViewProj, screenPos, avail);
                }
            }
        }

        // 3.6. Interacción y Pincel Visual de Escultura de Terreno 3D (Fase 14 - Solo en Modo Editor)
        if (!isPlaying() && m_activePropertyTab == BlenderPropertyTab::TERRAIN && terrain.isVisible() && m_isViewportHovered && !m_isUsingGizmo && !ImGuizmo::IsOver()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float relX = mousePos.x - screenPos.x;
            float relY = mousePos.y - screenPos.y;

            if (relX >= 0.0f && relX <= avail.x && relY > 48.0f && relY <= avail.y) {
                float ndcX = (2.0f * relX) / avail.x - 1.0f;
                float ndcY = 1.0f - (2.0f * relY) / avail.y;

                float aspect = (avail.y > 0.0f) ? avail.x / avail.y : 16.0f / 9.0f;
                glm::mat4 proj = camera.getProjectionMatrix(aspect);
                glm::mat4 view = camera.getViewMatrix();
                glm::mat4 invProj = glm::inverse(proj);
                glm::mat4 invView = glm::inverse(view);

                glm::vec4 rayClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 rayEye = invProj * rayClip;
                rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
                glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));
                glm::vec3 rayOrigin = camera.getPosition();

                glm::vec3 hitPos, hitNormal;
                if (terrain.raycast(rayOrigin, rayWorld, hitPos, hitNormal)) {
                    // Dibujar cursor circular proyectado sobre el relieve del terreno
                    const int circleSegs = 36;
                    ImVec2 polyPoints[36];
                    int validPts = 0;

                    glm::vec3 upRef = (std::abs(hitNormal.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                    glm::vec3 tangent = glm::normalize(glm::cross(hitNormal, upRef));
                    glm::vec3 bitangent = glm::normalize(glm::cross(hitNormal, tangent));

                    float activeRad = (terrain.activeToolMode == TerrainToolMode::PAINT_TEXTURE) ? terrain.paintRadius : terrain.brushRadius;
                    for (int s = 0; s < circleSegs; ++s) {
                        float angle = s * (6.2831853f / static_cast<float>(circleSegs));
                        glm::vec3 pt3D = hitPos + (tangent * std::cos(angle) + bitangent * std::sin(angle)) * activeRad;
                        // Amoldar altura del anillo a la elevación real del terreno
                        pt3D.y = terrain.getHeightAtWorld(pt3D.x, pt3D.z) + 0.08f;

                        glm::vec4 clipPt = proj * view * glm::vec4(pt3D, 1.0f);
                        if (clipPt.w > 0.001f) {
                            float pNdcX = clipPt.x / clipPt.w;
                            float pNdcY = clipPt.y / clipPt.w;
                            float scrX = screenPos.x + (pNdcX * 0.5f + 0.5f) * avail.x;
                            float scrY = screenPos.y + (1.0f - (pNdcY * 0.5f + 0.5f)) * avail.y;
                            polyPoints[validPts++] = ImVec2(scrX, scrY);
                        }
                    }

                    if (validPts >= 3) {
                        ImU32 brushCol = ImColor(85, 235, 130, 230); // Verde esmeralda por defecto
                        if (terrain.activeToolMode == TerrainToolMode::PAINT_TEXTURE) {
                            switch (terrain.activePaintLayer) {
                                case TerrainPaintLayer::GRASS: brushCol = ImColor(75, 230, 95, 230); break;
                                case TerrainPaintLayer::DIRT:  brushCol = ImColor(215, 150, 75, 230); break;
                                case TerrainPaintLayer::ROCK:  brushCol = ImColor(170, 180, 195, 230); break;
                                case TerrainPaintLayer::SNOW:  brushCol = ImColor(235, 245, 255, 230); break;
                            }
                        } else {
                            if (ImGui::GetIO().KeyCtrl) {
                                brushCol = ImColor(255, 90, 80, 230);    // Rojo al bajar terreno
                            } else if (ImGui::GetIO().KeyShift) {
                                brushCol = ImColor(90, 190, 255, 230);   // Azul al suavizar
                            }
                        }
                        drawList->AddPolyline(polyPoints, validPts, brushCol, ImDrawFlags_Closed, 2.2f);
                    }

                    // Marcador central en el punto de contacto
                    glm::vec4 centerClip = proj * view * glm::vec4(hitPos + glm::vec3(0.0f, 0.08f, 0.0f), 1.0f);
                    if (centerClip.w > 0.001f) {
                        float cNdcX = centerClip.x / centerClip.w;
                        float cNdcY = centerClip.y / centerClip.w;
                        float cScrX = screenPos.x + (cNdcX * 0.5f + 0.5f) * avail.x;
                        float cScrY = screenPos.y + (1.0f - (cNdcY * 0.5f + 0.5f)) * avail.y;
                        drawList->AddCircleFilled(ImVec2(cScrX, cScrY), 3.0f, ImColor(255, 255, 255, 240));
                        drawList->AddCircle(ImVec2(cScrX, cScrY), 5.5f, ImColor(80, 220, 120, 220), 12, 1.5f);
                    }

                    // Esculpir o pintar texturas continuamente mientras el botón izquierdo del ratón permanezca presionado
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        recordSnapshot(scene, &terrain);
                    }
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        if (terrain.activeToolMode == TerrainToolMode::PAINT_TEXTURE) {
                            terrain.paintTexture(hitPos, terrain.activePaintLayer, terrain.paintRadius, terrain.paintStrength, ImGui::GetIO().DeltaTime);
                        } else {
                            SculptBrushType effectiveBrush = terrain.activeBrushType;
                            if (ImGui::GetIO().KeyCtrl) {
                                effectiveBrush = SculptBrushType::LOWER;
                            } else if (ImGui::GetIO().KeyShift) {
                                effectiveBrush = SculptBrushType::SMOOTH;
                            }

                            terrain.sculpt(hitPos, terrain.brushRadius, terrain.brushStrength, effectiveBrush,
                                           terrain.activeBrushFalloff, terrain.brushTargetHeight, ImGui::GetIO().DeltaTime);
                            terrain.isSculpting = true;
                        }
                    }
                }
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && terrain.isSculpting) {
                if (terrain.isDirty()) {
                    terrain.updateNormalsAndVBO();
                }
                terrain.syncPhysics();
                terrain.isSculpting = false;
            }
        }

        // 4. Selección directa 3D con Clic Izquierdo (Raycasting en espacio de mundo - Solo en Modo Editor)
        if (!isPlaying() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_isViewportHovered && !m_isUsingGizmo && !ImGuizmo::IsOver() && m_activePropertyTab != BlenderPropertyTab::TERRAIN) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float relX = mousePos.x - screenPos.x;
            float relY = mousePos.y - screenPos.y;

            // Ignorar clics sobre la barra superior de herramientas
            if (relX >= 0.0f && relX <= avail.x && relY > 48.0f && relY <= avail.y) {
                float ndcX = (2.0f * relX) / avail.x - 1.0f;
                float ndcY = 1.0f - (2.0f * relY) / avail.y;

                float aspect = (avail.y > 0.0f) ? avail.x / avail.y : 16.0f / 9.0f;
                glm::mat4 proj = camera.getProjectionMatrix(aspect);
                glm::mat4 view = camera.getViewMatrix();

                glm::mat4 invProj = glm::inverse(proj);
                glm::mat4 invView = glm::inverse(view);

                glm::vec4 rayClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 rayEye = invProj * rayClip;
                rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

                glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));
                glm::vec3 rayOrigin = camera.getPosition();

                std::string closestEntity = "";
                float closestDist = 10000.0f;

                // Comprobar objetos 3D
                for (const auto& [name, entity] : scene.getEntities()) {
                    if (!entity || !entity->isVisible()) continue;

                    glm::vec3 center = entity->getPosition();
                    glm::vec3 scale = entity->getScale();
                    float radius = (std::max)({scale.x, scale.y, scale.z}) * 0.85f;
                    if (radius < 0.5f) radius = 0.5f;

                    glm::vec3 oc = rayOrigin - center;
                    float b = glm::dot(oc, rayWorld);
                    float c = glm::dot(oc, oc) - radius * radius;
                    float disc = b * b - c;

                    if (disc >= 0.0f) {
                        float dist = -b - std::sqrt(disc);
                        if (dist > 0.0f && dist < closestDist) {
                            closestDist = dist;
                            closestEntity = name;
                        }
                    }
                }

                // Comprobar colisión con luces puntuales
                bool clickedLight = false;
                for (size_t i = 0; i < pLights.size(); ++i) {
                    glm::vec3 oc = rayOrigin - pLights[i].position;
                    float b = glm::dot(oc, rayWorld);
                    float c = glm::dot(oc, oc) - 0.7f * 0.7f;
                    float disc = b * b - c;
                    if (disc >= 0.0f) {
                        float dist = -b - std::sqrt(disc);
                        if (dist > 0.0f && dist < closestDist) {
                            closestDist = dist;
                            m_selectedLightIndex = static_cast<int>(i);
                            m_selectedEntityName = "";
                            clickedLight = true;
                        }
                    }
                }

                if (!clickedLight && !closestEntity.empty()) {
                    m_selectedEntityName = closestEntity;
                    m_selectedLightIndex = -1;
                }
            }
        }

        // 5. HUD de Juego (Crosshair y Banner de Controles) durante Play-in-Editor (Fase 15)
        if (isPlaying()) {
            ImDrawList* vpDrawList = ImGui::GetWindowDrawList();
            ImVec2 center(screenPos.x + avail.x * 0.5f, screenPos.y + avail.y * 0.5f);

            // 5.1. Retícula central / Crosshair (Punto y 4 miras)
            ImU32 reticleCol = ImColor(255, 255, 255, 225);
            ImU32 shadowCol = ImColor(10, 15, 20, 180);

            vpDrawList->AddCircleFilled(center, 2.5f, shadowCol);
            vpDrawList->AddCircleFilled(center, 1.4f, reticleCol);

            float retLen = 8.0f;
            float retGap = 4.0f;
            // Superior
            vpDrawList->AddLine(ImVec2(center.x, center.y - retGap - retLen), ImVec2(center.x, center.y - retGap), shadowCol, 2.8f);
            vpDrawList->AddLine(ImVec2(center.x, center.y - retGap - retLen), ImVec2(center.x, center.y - retGap), reticleCol, 1.4f);
            // Inferior
            vpDrawList->AddLine(ImVec2(center.x, center.y + retGap), ImVec2(center.x, center.y + retGap + retLen), shadowCol, 2.8f);
            vpDrawList->AddLine(ImVec2(center.x, center.y + retGap), ImVec2(center.x, center.y + retGap + retLen), reticleCol, 1.4f);
            // Izquierda
            vpDrawList->AddLine(ImVec2(center.x - retGap - retLen, center.y), ImVec2(center.x - retGap, center.y), shadowCol, 2.8f);
            vpDrawList->AddLine(ImVec2(center.x - retGap - retLen, center.y), ImVec2(center.x - retGap, center.y), reticleCol, 1.4f);
            // Derecha
            vpDrawList->AddLine(ImVec2(center.x + retGap, center.y), ImVec2(center.x + retGap + retLen, center.y), shadowCol, 2.8f);
            vpDrawList->AddLine(ImVec2(center.x + retGap, center.y), ImVec2(center.x + retGap + retLen, center.y), reticleCol, 1.4f);

            // 5.2. Banner flotante inferior con guía de atajos
            float hudW = 640.0f;
            float hudH = 34.0f;
            ImVec2 hudMin(screenPos.x + (avail.x - hudW) * 0.5f, screenPos.y + avail.y - hudH - 14.0f);
            ImVec2 hudMax(hudMin.x + hudW, hudMin.y + hudH);

            vpDrawList->AddRectFilled(hudMin, hudMax, ImColor(14, 18, 26, 220), 17.0f);
            vpDrawList->AddRect(hudMin, hudMax, ImColor(50, 180, 245, 180), 17.0f, 0, 1.2f);

            const char* hudText = "[WASD] Mover | [Shift] Correr | [Espacio] Saltar | [V] 1ra/3ra Persona | [Esc] Salir";
            ImVec2 txtSz = ImGui::CalcTextSize(hudText);
            ImVec2 txtPos(hudMin.x + (hudW - txtSz.x) * 0.5f, hudMin.y + (hudH - txtSz.y) * 0.5f);
            vpDrawList->AddText(txtPos, ImColor(240, 245, 255, 245), hudText);
        }

        // 4. Overlays Cinemáticos: Anamórfico 2.39:1, Regla de Tercios y Motion Path (Fase 22)
        drawViewportSequencerOverlays(ImGui::GetWindowDrawList(), screenPos, ImVec2(screenPos.x + avail.x, screenPos.y + avail.y), camera);

        // Popup de inserción rápida de Keyframe (I)
        if (m_showKeyframeInsertPopup) {
            ImGui::OpenPopup("Insertar Clave de Animacion (I)");
            m_showKeyframeInsertPopup = false;
        }

        if (ImGui::BeginPopup("Insertar Clave de Animacion (I)")) {
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Insertar Keyframe (t = %.2fs):", Sequencer::get().getCurrentTime());
            ImGui::Separator();
            if (!m_selectedEntityName.empty()) {
                auto ent = scene.getEntity(m_selectedEntityName);
                if (ent) {
                    std::string entLabel = "Transformacion: " + m_selectedEntityName + " (LocRotScale)";
                    if (ImGui::Selectable(entLabel.c_str())) {
                        Sequencer::get().recordEntityKeyframe(*ent);
                        if (scriptEngine) scriptEngine->addLog("[Sequencer] Keyframe insertado para objeto: " + m_selectedEntityName);
                    }
                }
            } else {
                ImGui::TextDisabled("Transformacion (Sin entidad seleccionada)");
            }

            if (ImGui::Selectable("Camara de Cine (Posicion, Target, Lente y DoF)")) {
                float focal = Sequencer::fovToFocalLength(camera.getFOV());
                Sequencer::get().recordCameraKeyframe(camera, focal, m_dofFocusDistance, 2.8f);
                if (scriptEngine) scriptEngine->addLog("[Sequencer] Keyframe insertado para Camara de Cine");
            }

            if (ImGui::Selectable("Entorno y Sol 24h (Hora, Exposicion, Luz)")) {
                Sequencer::get().recordEnvironmentKeyframe(m_dayNight.getTimeOfDay(), m_exposure, m_lightIntensity, m_lightColor);
                if (scriptEngine) scriptEngine->addLog("[Sequencer] Keyframe insertado para Entorno y Sol");
            }

            ImGui::Separator();
            if (ImGui::Selectable("Todos los Canales Activos")) {
                if (!m_selectedEntityName.empty()) {
                    auto ent = scene.getEntity(m_selectedEntityName);
                    if (ent) Sequencer::get().recordEntityKeyframe(*ent);
                }
                float focal = Sequencer::fovToFocalLength(camera.getFOV());
                Sequencer::get().recordCameraKeyframe(camera, focal, m_dofFocusDistance, 2.8f);
                Sequencer::get().recordEnvironmentKeyframe(m_dayNight.getTimeOfDay(), m_exposure, m_lightIntensity, m_lightColor);
                if (scriptEngine) scriptEngine->addLog("[Sequencer] Keyframes globales insertados en t = " + std::to_string(Sequencer::get().getCurrentTime()) + "s");
            }

            ImGui::EndPopup();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorUI::renderOutliner(Scene& scene, PhysicsWorld& physicsWorld, Camera& camera) {
    if (ImGui::Begin("Jerarquia de Escena")) {
        const auto& order = scene.getEntityOrder();
        auto& pointLights = scene.getPointLights();
        auto groupNames = scene.getGroupNames();

        // Barra superior unificada: Buscador + Limpiar + Añadir (+) en la misma fila (Fase 21.5)
        float addBtnW = 24.0f;
        float clrBtnW = 20.0f;
        float searchW = ImGui::GetContentRegionAvail().x - addBtnW - clrBtnW - ImGui::GetStyle().ItemSpacing.x * 2.0f;
        ImGui::SetNextItemWidth((std::max)(50.0f, searchW));
        ImGui::InputTextWithHint("##SceneFilter", "Buscar...", m_sceneSearchFilter, sizeof(m_sceneSearchFilter));
        ImGui::SameLine();
        if (UI::IconButton("##ClearSearch", UI::IconType::RESET, ImVec2(clrBtnW, 19.0f), "Limpiar busqueda")) {
            m_sceneSearchFilter[0] = '\0';
        }
        ImGui::SameLine();
        if (UI::IconButton("##AddMenuBtn", UI::IconType::PLUS, ImVec2(addBtnW, 19.0f), "Añadir Primitiva 3D, Luz o VFX (Shift+A)")) {
            m_openAddMenu = true;
        }

        bool hasSearch = (m_sceneSearchFilter[0] != '\0');
        std::string searchLower = m_sceneSearchFilter;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), [](unsigned char c) { return (char)std::tolower(c); });

        auto stringContainsFilter = [&](const std::string& str) -> bool {
            if (!hasSearch) return true;
            std::string lower = str;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return (lower.find(searchLower) != std::string::npos);
        };

        if (hasSearch) {
            int matchCount = 0;
            for (const auto& name : order) {
                if (stringContainsFilter(name)) matchCount++;
            }
            for (const auto& pl : pointLights) {
                if (stringContainsFilter(pl.name)) matchCount++;
            }
            ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.0f, 1.0f), "%d resultado%s", matchCount, (matchCount == 1 ? "" : "s"));
        } else {
            ImGui::TextDisabled("Escena: %zu obj, %zu grp, %zu luz", order.size(), groupNames.size(), pointLights.size());
        }
        ImGui::Separator();

        ImGui::BeginChild("EntityScrollList", ImVec2(0, ImGui::GetContentRegionAvail().y - 54.0f), true);

        // Control del Plano de Suelo
        if (!m_groundPlaneDeleted && (!hasSearch || stringContainsFilter("Plano de Suelo Base"))) {
            bool showGround = m_showGroundPlane;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
            ImVec2 planeIconPos = ImGui::GetCursorScreenPos();
            drawOutlinerIcon(OutlinerIconType::PLANE, planeIconPos, 14.0f);
            ImGui::Dummy(ImVec2(16.0f, 14.0f));
            ImGui::SameLine();
            float rightIconWidth = 44.0f;
            float nameWidth = (std::max)(50.0f, ImGui::GetContentRegionAvail().x - rightIconWidth);
            ImGui::Selectable("Suelo Base", false, 0, ImVec2(nameWidth, 0));
            if (ImGui::BeginPopupContextItem("GroundPlaneContext")) {
                if (ImGui::MenuItem("Ocultar Suelo")) {
                    m_showGroundPlane = false;
                }
                if (ImGui::MenuItem("Eliminar Suelo Base")) {
                    m_groundPlaneDeleted = true;
                    m_showGroundPlane = false;
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            if (UI::IconButton("##vis_ground", showGround ? UI::IconType::EYE_OPEN : UI::IconType::EYE_CLOSED, ImVec2(20.0f, 17.0f), "Visibilidad del Plano de Suelo", showGround ? 0 : IM_COL32(140, 140, 140, 255))) {
                m_showGroundPlane = !showGround;
            }
            ImGui::PopStyleVar();
            ImGui::Separator();
        }

        // Helper lambda para mapear primitiva a tipo de icono vectorial
        auto getOutlinerIconType = [](const std::string& prim) -> OutlinerIconType {
            if (prim == "Sphere") return OutlinerIconType::SPHERE;
            if (prim == "Cylinder") return OutlinerIconType::CYLINDER;
            if (prim == "Cone") return OutlinerIconType::CONE;
            if (prim == "Torus") return OutlinerIconType::TORUS;
            if (prim == "Plane") return OutlinerIconType::PLANE;
            if (prim == "Model") return OutlinerIconType::MODEL;
            return OutlinerIconType::MESH_CUBE;
        };

        // Helper lambda para renderizar una fila de entidad con multi-selección e iconos vectoriales
        auto renderEntityRow = [&](std::shared_ptr<Entity> ent) {
            if (!ent) return;
            const std::string& name = ent->getName();

            // Filtrado por buscador
            if (hasSearch && !stringContainsFilter(name)) {
                return;
            }

            bool isSelected = isEntitySelected(name);
            bool vis = ent->isVisible();

            ImGui::PushID(name.c_str());
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));

            // Icono vectorial nítido (Cubo, Esfera, Cilindro, etc.)
            ImVec2 iconPos = ImGui::GetCursorScreenPos();
            drawOutlinerIcon(getOutlinerIconType(ent->getPrimitiveType()), iconPos, 14.0f);
            ImGui::Dummy(ImVec2(16.0f, 14.0f));
            ImGui::SameLine();

            // Selectable con nombre (dejando espacio a la derecha para Visibilidad y Enfocar)
            float rightIconWidth = 44.0f;
            float nameWidth = (std::max)(40.0f, ImGui::GetContentRegionAvail().x - rightIconWidth);

            // Renombrado inline (F2 o doble clic)
            if (m_renamingEntityName == name) {
                ImGui::SetNextItemWidth(nameWidth);
                if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##InlineRename", m_renameEntityBuf, sizeof(m_renameEntityBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                    if (m_renameEntityBuf[0] != '\0' && name != m_renameEntityBuf) {
                        recordSnapshot(scene);
                        std::string newName = m_renameEntityBuf;
                        if (scene.renameEntity(name, newName)) {
                            if (isEntitySelected(name)) {
                                m_selectedEntityNames.erase(name);
                                m_selectedEntityNames.insert(newName);
                                m_selectedEntityName = newName;
                            }
                        }
                    }
                    m_renamingEntityName = "";
                }
                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                    m_renamingEntityName = "";
                }
            } else {
                if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(nameWidth, 0))) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        m_renamingEntityName = name;
                        strncpy_s(m_renameEntityBuf, name.c_str(), sizeof(m_renameEntityBuf) - 1);
                    } else {
                        ImGuiIO& curIO = ImGui::GetIO();
                        if (curIO.KeyCtrl) {
                            toggleEntitySelection(name);
                        } else if (curIO.KeyShift) {
                            selectEntityRange(name, order);
                        } else {
                            selectEntity(name);
                        }
                        m_selectedLightIndex = -1;
                    }
                }
            }

            // Menú contextual clic derecho profesional (Fase 21)
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Renombrar (F2)")) {
                    m_renamingEntityName = name;
                    strncpy_s(m_renameEntityBuf, name.c_str(), sizeof(m_renameEntityBuf) - 1);
                }
                if (ImGui::MenuItem("Duplicar (Ctrl+D)")) {
                    duplicateEntity(scene, physicsWorld, name);
                }
                if (ImGui::MenuItem("Enfocar Camara (F)")) {
                    camera.resetToFocus(ent->getPosition());
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Agrupar Seleccion... (Ctrl+G)")) {
                    if (!isEntitySelected(name)) selectEntity(name);
                    m_showGroupCreationModal = true;
                }
                if (!ent->getGroup().empty()) {
                    if (ImGui::MenuItem("Quitar del Grupo")) {
                        ent->setGroup("");
                    }
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Asignar Preset PBR")) {
                    const char* presetNames[] = {
                        "Oro Pulido", "Cobre", "Acero Inoxidable", "Cromo / Espejo",
                        "Plastico Brillante", "Vidrio / Cristal", "Neon Cian Emisivo",
                        "Obsidiana Negra", "Marmol Blanco", "Hormigon / Concreto", "Hierro Oxidado"
                    };
                    for (const char* pName : presetNames) {
                        if (ImGui::MenuItem(pName)) {
                            MaterialManager::get().applyToEntity(*ent, pName);
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Guardar como Prefab (.prefab)...")) {
                    PrefabManager::saveEntityAsPrefab(ent, ent->getName(), &physicsWorld);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Eliminar Objeto(s) (Supr)")) {
                    deleteSelectedEntities(scene, physicsWorld);
                }
                ImGui::EndPopup();
            }

            // Iconos de estado a la derecha: Visibilidad y Enfocar
            ImGui::SameLine();
            char visBtnId[32];
            snprintf(visBtnId, sizeof(visBtnId), "##vis_%s", name.c_str());
            if (UI::IconButton(visBtnId, vis ? UI::IconType::EYE_OPEN : UI::IconType::EYE_CLOSED, ImVec2(20.0f, 17.0f), vis ? "Ocultar objeto" : "Mostrar objeto", vis ? 0 : IM_COL32(140, 140, 140, 255))) {
                ent->setVisible(!vis);
            }

            ImGui::SameLine();
            char focBtnId[32];
            snprintf(focBtnId, sizeof(focBtnId), "##foc_%s", name.c_str());
            if (UI::IconButton(focBtnId, UI::IconType::CAMERA_TARGET, ImVec2(20.0f, 17.0f), "Enfocar camara en este objeto [F]")) {
                camera.resetToFocus(ent->getPosition());
            }

            ImGui::PopStyleVar();
            ImGui::PopID();
        };

        // 1. Renderizar Entidades Agrupadas en Carpetas / Nodos Desplegables (Fase 16 & 21)
        if (!groupNames.empty()) {
            ImGui::TextColored(ImVec4(0.78f, 0.55f, 0.98f, 1.0f), "Grupos de Objetos:");
            for (const auto& grpName : groupNames) {
                auto groupEnts = scene.getEntitiesInGroup(grpName);
                if (groupEnts.empty()) continue;

                // Si hay búsqueda activa, verificar si el grupo o alguno de sus hijos coincide
                bool groupMatches = false;
                if (hasSearch) {
                    if (stringContainsFilter(grpName)) {
                        groupMatches = true;
                    } else {
                        for (const auto& e : groupEnts) {
                            if (e && stringContainsFilter(e->getName())) {
                                groupMatches = true;
                                break;
                            }
                        }
                    }
                    if (!groupMatches) continue;
                }

                ImGui::PushID(grpName.c_str());

                bool anyVis = false;
                for (auto& e : groupEnts) {
                    if (e && e->isVisible()) { anyVis = true; break; }
                }
                bool grpVis = anyVis;
                if (ImGui::Checkbox("##GrpVis", &grpVis)) {
                    scene.setGroupVisibility(grpName, grpVis);
                }
                ImGui::SameLine();

                ImVec2 fIconPos = ImGui::GetCursorScreenPos();
                drawOutlinerIcon(OutlinerIconType::FOLDER, fIconPos, 16.0f);
                ImGui::Dummy(ImVec2(18.0f, 16.0f));
                ImGui::SameLine();

                std::string headerLabel = grpName + " (" + std::to_string(groupEnts.size()) + ")";
                bool isGrpSelected = (m_selectedGroupName == grpName);
                ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (hasSearch || isGrpSelected) nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
                if (isGrpSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;
                bool nodeOpen = ImGui::TreeNodeEx(headerLabel.c_str(), nodeFlags);

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    selectGroup(grpName, scene);
                }

                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Seleccionar Todo el Grupo")) {
                        selectGroup(grpName, scene);
                    }
                    if (ImGui::MenuItem("Enfocar Grupo (F)")) {
                        camera.resetToFocus(m_groupPivot);
                    }
                    if (ImGui::MenuItem("Renombrar Grupo...")) {
                        m_renamingGroupName = grpName;
                        strncpy_s(m_renameGroupBuf, grpName.c_str(), sizeof(m_renameGroupBuf) - 1);
                    }
                    if (ImGui::MenuItem("Mostrar Todo el Grupo")) {
                        scene.setGroupVisibility(grpName, true);
                    }
                    if (ImGui::MenuItem("Ocultar Todo el Grupo")) {
                        scene.setGroupVisibility(grpName, false);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Guardar Grupo como Prefab (.prefab)...")) {
                        std::string prefabPath = "assets/prefabs/" + grpName + ".prefab";
                        PrefabManager::savePrefab(prefabPath, groupEnts, &physicsWorld);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Desagrupar Objetos")) {
                        recordSnapshot(scene);
                        scene.deleteGroup(grpName, false);
                    }
                    if (ImGui::MenuItem("Eliminar Grupo y Contenido")) {
                        recordSnapshot(scene);
                        scene.deleteGroup(grpName, true);
                        clearEntitySelection();
                    }
                    ImGui::EndPopup();
                }

                if (nodeOpen) {
                    for (auto& ent : groupEnts) {
                        renderEntityRow(ent);
                    }
                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
            ImGui::Separator();
        }

        // 2. Renderizar Entidades Sueltas (sin grupo)
        std::vector<std::shared_ptr<Entity>> ungrouped;
        for (const auto& name : order) {
            auto ent = scene.getEntity(name);
            if (ent && ent->getGroup().empty()) {
                ungrouped.push_back(ent);
            }
        }

        if (!ungrouped.empty()) {
            ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), groupNames.empty() ? "Objetos 3D:" : "Objetos Sueltos:");
            for (auto& ent : ungrouped) {
                renderEntityRow(ent);
            }
        }

        // 3. Lista de Luces Puntuales
        std::vector<size_t> matchingLights;
        for (size_t i = 0; i < pointLights.size(); ++i) {
            if (!hasSearch || stringContainsFilter(pointLights[i].name)) {
                matchingLights.push_back(i);
            }
        }

        if (!matchingLights.empty()) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Luces Puntuales (%zu/%zu):", matchingLights.size(), pointLights.size());
            for (size_t i : matchingLights) {
                auto& pl = pointLights[i];
                ImGui::PushID(static_cast<int>(1000 + i));
                if (ImGui::Checkbox("##LightVis", &pl.enabled)) {
                }
                ImGui::SameLine();
                bool isLightSelected = (m_selectedLightIndex == static_cast<int>(i));

                ImVec2 lightIconPos = ImGui::GetCursorScreenPos();
                drawOutlinerIcon(OutlinerIconType::LIGHT_POINT, lightIconPos, 16.0f);
                ImGui::Dummy(ImVec2(18.0f, 16.0f));
                ImGui::SameLine();

                if (ImGui::Selectable(pl.name.c_str(), isLightSelected)) {
                    m_selectedLightIndex = static_cast<int>(i);
                    clearEntitySelection();
                }
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Enfocar Luz (F)")) {
                        camera.resetToFocus(pl.position);
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
        }

        ImGui::EndChild();

        // Panel de Gestión de Grupo para el objeto actualmente seleccionado
        auto selEnt = scene.getEntity(m_selectedEntityName);
        if (selEnt) {
            ImGui::Separator();
            ImGui::TextDisabled("Grupo:");
            ImGui::SameLine();
            static char grpInputBuf[64] = "";
            std::string curGrp = selEnt->getGroup();
            if (curGrp.empty()) {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
                ImGui::InputTextWithHint("##GrpNameInput", "Nuevo Grupo...", grpInputBuf, IM_ARRAYSIZE(grpInputBuf));
                ImGui::SameLine();
                if (ImGui::Button("+ Asignar", ImVec2(66.0f, 0.0f)) && grpInputBuf[0] != '\0') {
                    selEnt->setGroup(grpInputBuf);
                    grpInputBuf[0] = '\0';
                }
            } else {
                ImVec2 fIconPos2 = ImGui::GetCursorScreenPos();
                drawOutlinerIcon(OutlinerIconType::FOLDER, fIconPos2, 14.0f);
                ImGui::Dummy(ImVec2(16.0f, 14.0f));
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.78f, 0.55f, 0.98f, 1.0f), "%s", curGrp.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Quitar")) {
                    selEnt->setGroup("");
                }
            }
        }

        // Barra inferior de acciones compacta estilo Blender 4/5 (Fase 21.5)
        float colBtnW = (ImGui::GetContentRegionAvail().x - 2.0f * ImGui::GetStyle().ItemSpacing.x) / 3.0f;
        if (m_selectedEntityNames.size() > 1) {
            if (UI::ButtonWithIcon(UI::IconType::FOLDER, "Agrupar", ImVec2(colBtnW, 22.0f), "Agrupar seleccion [Ctrl+G]")) {
                m_showGroupCreationModal = true;
            }
        } else {
            if (UI::ButtonWithIcon(UI::IconType::PLUS, "Añadir", ImVec2(colBtnW, 22.0f), "Añadir Primitiva o Luz (Shift+A)")) {
                m_openAddMenu = true;
            }
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::DUPLICATE, "Duplicar", ImVec2(colBtnW, 22.0f), "Duplicar seleccion [Ctrl+D]")) {
            if (!m_selectedEntityName.empty()) {
                duplicateEntity(scene, physicsWorld, m_selectedEntityName);
            }
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::TRASH, "Eliminar", ImVec2(colBtnW, 22.0f), "Eliminar seleccion [Supr]", IM_COL32(230, 80, 80, 255))) {
            deleteSelectedEntities(scene, physicsWorld);
        }

        if (m_groundPlaneDeleted) {
            if (UI::ButtonWithIcon(UI::IconType::RESET, "Restaurar Suelo Base", ImVec2(-1, 20.0f))) {
                m_groundPlaneDeleted = false;
                m_showGroundPlane = true;
            }
        }

        // Modal para crear nuevo grupo con selección
        if (m_showGroupCreationModal) {
            ImGui::OpenPopup("Crear Grupo con Selección##Modal");
        }
        if (ImGui::BeginPopupModal("Crear Grupo con Selección##Modal", &m_showGroupCreationModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nombre del nuevo Grupo o Subgrupo (ej: Castillo, Nivel/Props):");
            ImGui::InputText("##NewGroupName", m_newGroupNameBuf, sizeof(m_newGroupNameBuf));
            ImGui::Spacing();
            ImGui::TextDisabled("Se asignarán %zu objetos seleccionados a este grupo.", m_selectedEntityNames.size());
            ImGui::Separator();
            if (ImGui::Button("Crear Grupo", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                if (m_newGroupNameBuf[0] != '\0') {
                    recordSnapshot(scene);
                    for (const auto& selName : m_selectedEntityNames) {
                        auto ent = scene.getEntity(selName);
                        if (ent) ent->setGroup(m_newGroupNameBuf);
                    }
                    m_newGroupNameBuf[0] = '\0';
                    m_showGroupCreationModal = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(100, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                m_showGroupCreationModal = false;
            }
            ImGui::EndPopup();
        }

        // Modal para renombrar grupo existente
        if (!m_renamingGroupName.empty()) {
            ImGui::OpenPopup("Renombrar Grupo##Modal");
        }
        if (ImGui::BeginPopupModal("Renombrar Grupo##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nuevo nombre para el grupo '%s':", m_renamingGroupName.c_str());
            ImGui::InputText("##NewGroupNameInput", m_renameGroupBuf, sizeof(m_renameGroupBuf));
            ImGui::Spacing();
            if (ImGui::Button("Guardar", ImVec2(100, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                if (m_renameGroupBuf[0] != '\0') {
                    recordSnapshot(scene);
                    scene.renameGroup(m_renamingGroupName, m_renameGroupBuf);
                }
                m_renamingGroupName = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(100, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                m_renamingGroupName = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void EditorUI::renderPropertiesPanel(Scene& scene, Camera& camera, PhysicsWorld& physicsWorld, 
                                     ScriptEngine& scriptEngine, ShadowMap& shadowMap, Skybox& skybox, 
                                     ParticleSystem& particleSystem, Terrain& terrain, CharacterController& character, Water* water) {
    if (ImGui::Begin("Propiedades")) {
        // -------------------------------------------------------------
        // 1. Barra vertical de iconos estilo Blender en el costado izquierdo (30px de ancho compacto)
        // -------------------------------------------------------------
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 3.0f));
        ImGui::BeginChild("BlenderIconRail", ImVec2(30, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // 1. Grupo Superior: Escena y Entorno Global (Fase 21.8)
        renderVectorRailButton(BlenderPropertyTab::IMPORT_EXPORT, "##BtnIO", "Ajustes del Proyecto, Empaquetador y Modelos 3D", ImVec4(0.72f, 0.50f, 1.0f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::LIGHTING, "##BtnLuz", "Iluminacion (Sol 24h, Sombras PCSS/CSM, Luces Puntuales)", ImVec4(0.98f, 0.85f, 0.22f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::TERRAIN, "##BtnTer", "Terreno Procedural 3D, OASIS, Niebla y Oceano 3D", ImVec4(0.35f, 0.82f, 0.45f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::AUDIO, "##BtnAud", "Motor de Audio 3D Espacial y SFX", ImVec4(0.78f, 0.45f, 0.98f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::PLAYER, "##BtnPly", "Controlador de Personaje 3D (PIE)", ImVec4(0.20f, 0.75f, 0.95f, 1.0f));

        // Linea divisoria horizontal sutil entre Escena/Entorno y Objeto/Activos
        ImGui::Spacing();
        ImVec2 rSepMin = ImGui::GetCursorScreenPos();
        ImDrawList* railDl = ImGui::GetWindowDrawList();
        railDl->AddLine(ImVec2(rSepMin.x + 3.0f, rSepMin.y + 1.0f), ImVec2(rSepMin.x + 21.0f, rSepMin.y + 1.0f), IM_COL32(75, 75, 75, 255), 1.0f);
        ImGui::Dummy(ImVec2(0.0f, 3.0f));

        // 2. Grupo Inferior: Objeto y Activos
        renderVectorRailButton(BlenderPropertyTab::OBJECT, "##BtnObj", "Propiedades del Objeto (Transformacion, Visibilidad)", ImVec4(0.95f, 0.52f, 0.18f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::PHYSICS, "##BtnFis", "Fisica 3D Bullet (Cuerpos Rigidos, Gravedad, Rebote)", ImVec4(0.25f, 0.65f, 1.0f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::MATERIAL, "##BtnMat", "Materiales PBR y Presets Fisicos (.asismat)", ImVec4(0.92f, 0.30f, 0.45f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::PARTICLES, "##BtnVFX", "Sistema de Particulas 3D (VFX: Fuego, Humo, Chispas, Lluvia)", ImVec4(1.0f, 0.45f, 0.15f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::ANIMATION, "##BtnAnim", "Animaciones Esqueleticas y Huesos", ImVec4(0.35f, 0.85f, 0.95f, 1.0f));
        renderVectorRailButton(BlenderPropertyTab::SCRIPTS, "##BtnLua", "Logica y Scripts Lua en Tiempo Real", ImVec4(0.25f, 0.85f, 0.45f, 1.0f));

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::SameLine();

        // -------------------------------------------------------------
        // 2. Área de contenido principal según el icono activo
        // -------------------------------------------------------------
        ImGui::BeginChild("BlenderContentArea", ImVec2(0, 0), false);

        switch (m_activePropertyTab) {
            case BlenderPropertyTab::OBJECT:
                renderObjectProperties(scene, camera, physicsWorld);
                break;
            case BlenderPropertyTab::MATERIAL:
                renderMaterialProperties(scene);
                break;
            case BlenderPropertyTab::LIGHTING:
                renderLightingProperties(scene, shadowMap, camera, skybox);
                break;
            case BlenderPropertyTab::TERRAIN:
                renderTerrainProperties(terrain, camera, physicsWorld, water);
                break;
            case BlenderPropertyTab::PLAYER:
                renderCharacterProperties(character, camera, physicsWorld, scene);
                break;
            case BlenderPropertyTab::AUDIO:
                renderAudioProperties();
                break;
            case BlenderPropertyTab::PARTICLES:
                renderParticleProperties(particleSystem, camera);
                break;
            case BlenderPropertyTab::ANIMATION:
                renderAnimationProperties(scene);
                break;
            case BlenderPropertyTab::PHYSICS:
                renderPhysicsProperties(scene, physicsWorld);
                break;
            case BlenderPropertyTab::SCRIPTS:
                renderScriptsProperties(scriptEngine);
                break;
            case BlenderPropertyTab::IMPORT_EXPORT:
                renderImportExportProperties(scene, scriptEngine);
                break;
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void EditorUI::renderVectorRailButton(BlenderPropertyTab tab, const char* str_id, const char* tooltip, const ImVec4& themeColor) {
    bool isActive = (m_activePropertyTab == tab);

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImVec2 pMax = ImVec2(pMin.x + 24.0f, pMin.y + 24.0f);

    bool clicked = ImGui::InvisibleButton(str_id, ImVec2(24, 24));
    bool hovered = ImGui::IsItemHovered();
    if (clicked) {
        m_activePropertyTab = tab;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Fondo del botón
    ImU32 bgCol = isActive ? ImColor(0.24f, 0.28f, 0.36f, 1.0f) :
                  hovered  ? ImColor(0.22f, 0.22f, 0.22f, 1.0f) :
                             ImColor(0.14f, 0.14f, 0.14f, 1.0f);
    drawList->AddRectFilled(pMin, pMax, bgCol, 3.0f);

    // Borde e indicador activo (barra acentuada en el borde izquierdo)
    if (isActive) {
        drawList->AddRect(pMin, pMax, ImColor(themeColor), 3.0f, 0, 1.2f);
        drawList->AddRectFilled(ImVec2(pMin.x, pMin.y + 2.0f), ImVec2(pMin.x + 2.5f, pMax.y - 2.0f), ImColor(themeColor), 1.5f);
    }

    // Ilustración Vectorial según la categoría centrada y escalada a 24px
    ImVec2 c = ImVec2((pMin.x + pMax.x) * 0.5f + (isActive ? 0.5f : 0.0f), (pMin.y + pMax.y) * 0.5f);

    if (tab == BlenderPropertyTab::OBJECT) {
        // [OBJETO] -> Cubo Isométrico 3D compacto
        float s = 6.5f;
        ImVec2 top(c.x, c.y - s);
        ImVec2 tr(c.x + s * 0.866f, c.y - s * 0.5f);
        ImVec2 tl(c.x - s * 0.866f, c.y - s * 0.5f);
        ImVec2 mid(c.x, c.y);
        ImVec2 br(c.x + s * 0.866f, c.y + s * 0.5f);
        ImVec2 bl(c.x - s * 0.866f, c.y + s * 0.5f);
        ImVec2 bot(c.x, c.y + s);

        drawList->AddQuadFilled(tl, top, tr, mid, ImColor(255, 175, 75));
        drawList->AddQuadFilled(tl, mid, bot, bl, ImColor(220, 120, 45));
        drawList->AddQuadFilled(mid, tr, br, bot, ImColor(175, 80, 25));

        ImU32 edgeCol = ImColor(255, 235, 190);
        drawList->AddLine(top, tr, edgeCol, 1.0f);
        drawList->AddLine(tr, br, edgeCol, 1.0f);
        drawList->AddLine(br, bot, edgeCol, 1.0f);
        drawList->AddLine(bot, bl, edgeCol, 1.0f);
        drawList->AddLine(bl, tl, edgeCol, 1.0f);
        drawList->AddLine(tl, top, edgeCol, 1.0f);
        drawList->AddLine(mid, top, edgeCol, 1.0f);
        drawList->AddLine(mid, br, edgeCol, 1.0f);
        drawList->AddLine(mid, bl, edgeCol, 1.0f);

    } else if (tab == BlenderPropertyTab::MATERIAL) {
        // [MATERIAL PBR] -> Esfera de Materiales compacta
        drawList->AddCircleFilled(c, 6.2f, ImColor(225, 60, 95));
        drawList->AddCircleFilled(ImVec2(c.x, c.y + 1.0f), 5.0f, ImColor(175, 40, 68));

        for (int i = 0; i < 12; ++i) {
            float a1 = i * (3.14159f / 12.0f);
            float a2 = (i + 1) * (3.14159f / 12.0f);
            float x1 = c.x - 6.2f * cosf(a1);
            float y1 = c.y + 2.0f * sinf(a1);
            float x2 = c.x - 6.2f * cosf(a2);
            float y2 = c.y + 2.0f * sinf(a2);
            drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImColor(255, 170, 190), 1.0f);
        }

        drawList->AddCircleFilled(ImVec2(c.x - 2.0f, c.y - 2.0f), 1.6f, ImColor(255, 240, 245));
        drawList->AddCircleFilled(ImVec2(c.x - 2.0f, c.y - 2.0f), 0.8f, ImColor(255, 255, 255));

    } else if (tab == BlenderPropertyTab::LIGHTING) {
        // [ILUMINACIÓN] -> Sol Radiante compacto
        drawList->AddCircleFilled(c, 3.2f, ImColor(255, 215, 50));
        drawList->AddCircleFilled(c, 1.5f, ImColor(255, 255, 220));

        for (int i = 0; i < 8; ++i) {
            float a = i * 0.785398f;
            float cosA = cosf(a), sinA = sinf(a);
            ImVec2 rStart(c.x + cosA * 4.8f, c.y + sinA * 4.8f);
            ImVec2 rEnd(c.x + cosA * 7.5f, c.y + sinA * 7.5f);
            drawList->AddLine(rStart, rEnd, ImColor(255, 225, 90), 1.2f);
        }

    } else if (tab == BlenderPropertyTab::TERRAIN) {
        // [TERRENO 3D] -> Cordillera montañosa compacta
        ImU32 sunBgCol = ImColor(255, 215, 60);
        ImU32 rockCol1 = ImColor(75, 175, 105);
        ImU32 rockCol2 = ImColor(45, 125, 75);
        ImU32 snowCol  = ImColor(245, 250, 255);

        drawList->AddCircleFilled(ImVec2(c.x + 3.2f, c.y - 3.2f), 2.4f, sunBgCol);

        drawList->AddTriangleFilled(
            ImVec2(c.x - 3.2f, c.y - 1.8f),
            ImVec2(c.x - 7.0f, c.y + 5.5f),
            ImVec2(c.x + 0.5f, c.y + 5.5f),
            rockCol2
        );

        drawList->AddTriangleFilled(
            ImVec2(c.x + 1.2f, c.y - 5.5f),
            ImVec2(c.x - 3.2f, c.y + 5.5f),
            ImVec2(c.x + 6.5f, c.y + 5.5f),
            rockCol1
        );

        drawList->AddTriangleFilled(
            ImVec2(c.x + 1.2f, c.y - 5.5f),
            ImVec2(c.x - 0.2f, c.y - 1.8f),
            ImVec2(c.x + 2.8f, c.y - 1.8f),
            snowCol
        );

        drawList->AddLine(ImVec2(c.x - 7.0f, c.y + 5.5f), ImVec2(c.x + 7.0f, c.y + 5.5f), ImColor(110, 210, 135), 1.2f);

    } else if (tab == BlenderPropertyTab::PLAYER) {
        // [JUGADOR] -> Mando de videojuegos compacto
        ImVec2 boxMin(c.x - 7.0f, c.y - 4.0f);
        ImVec2 boxMax(c.x + 7.0f, c.y + 4.5f);
        drawList->AddRectFilled(boxMin, boxMax, ImColor(38, 48, 62), 3.0f);
        drawList->AddRect(boxMin, boxMax, ImColor(60, 185, 240), 3.0f, 0, 1.0f);

        drawList->AddTriangleFilled(ImVec2(c.x - 6.5f, c.y + 3.0f), ImVec2(c.x - 3.5f, c.y + 6.5f), ImVec2(c.x - 2.5f, c.y + 3.0f), ImColor(38, 48, 62));
        drawList->AddTriangleFilled(ImVec2(c.x + 6.5f, c.y + 3.0f), ImVec2(c.x + 3.5f, c.y + 6.5f), ImVec2(c.x + 2.5f, c.y + 3.0f), ImColor(38, 48, 62));

        drawList->AddCircleFilled(ImVec2(c.x + 3.8f, c.y - 1.5f), 0.9f, ImColor(255, 215, 60));
        drawList->AddCircleFilled(ImVec2(c.x + 5.4f, c.y + 0.0f), 0.9f, ImColor(255, 75, 75));
        drawList->AddCircleFilled(ImVec2(c.x + 3.8f, c.y + 1.5f), 0.9f, ImColor(75, 230, 95));

        drawList->AddTriangleFilled(ImVec2(c.x - 0.6f, c.y - 1.5f), ImVec2(c.x - 0.6f, c.y + 1.5f), ImVec2(c.x + 1.0f, c.y), ImColor(80, 220, 255));

    } else if (tab == BlenderPropertyTab::AUDIO) {
        // [AUDIO] -> Altavoz compacto
        ImU32 spkCol = ImColor(220, 175, 255);
        ImU32 waveCol = ImColor(185, 125, 255);

        drawList->AddRectFilled(ImVec2(c.x - 5.5f, c.y - 2.5f), ImVec2(c.x - 2.5f, c.y + 2.5f), spkCol, 1.0f);

        ImVec2 p1(c.x - 2.5f, c.y - 2.5f);
        ImVec2 p2(c.x + 0.8f, c.y - 5.0f);
        ImVec2 p3(c.x + 0.8f, c.y + 5.0f);
        ImVec2 p4(c.x - 2.5f, c.y + 2.5f);
        drawList->AddQuadFilled(p1, p2, p3, p4, spkCol);

        for (int i = -2; i <= 2; ++i) {
            float a1 = (float)i * 0.28f;
            float a2 = (float)(i + 1) * 0.28f;
            drawList->AddLine(
                ImVec2(c.x + 2.8f + cosf(a1) * 2.0f, c.y + sinf(a1) * 3.0f),
                ImVec2(c.x + 2.8f + cosf(a2) * 2.0f, c.y + sinf(a2) * 3.0f),
                waveCol, 1.2f
            );
            drawList->AddLine(
                ImVec2(c.x + 4.5f + cosf(a1) * 3.0f, c.y + sinf(a1) * 5.0f),
                ImVec2(c.x + 4.5f + cosf(a2) * 3.0f, c.y + sinf(a2) * 5.0f),
                waveCol, 1.2f
            );
        }

    } else if (tab == BlenderPropertyTab::PHYSICS) {
        // [FÍSICA] -> Esfera compacta con órbita
        drawList->AddCircleFilled(c, 3.6f, ImColor(65, 165, 255));
        drawList->AddCircleFilled(ImVec2(c.x - 1.0f, c.y - 1.0f), 1.0f, ImColor(220, 240, 255));

        const int numPts = 12;
        for (int i = 0; i < numPts; ++i) {
            float a1 = i * (6.283185f / numPts);
            float a2 = (i + 1) * (6.283185f / numPts);
            auto getPoint = [&](float angle) {
                float rx = 7.0f * cosf(angle);
                float ry = 2.6f * sinf(angle);
                float cosT = 0.866f, sinT = -0.5f;
                return ImVec2(c.x + rx * cosT - ry * sinT, c.y + rx * sinT + ry * cosT);
            };
            drawList->AddLine(getPoint(a1), getPoint(a2), ImColor(140, 215, 255), 1.2f);
        }

    } else if (tab == BlenderPropertyTab::SCRIPTS) {
        // [SCRIPTS] -> Símbolo < / > compacto
        ImU32 codeCol = ImColor(70, 230, 130);
        drawList->AddLine(ImVec2(c.x - 2.5f, c.y - 5.0f), ImVec2(c.x - 6.5f, c.y), codeCol, 1.5f);
        drawList->AddLine(ImVec2(c.x - 6.5f, c.y), ImVec2(c.x - 2.5f, c.y + 5.0f), codeCol, 1.5f);
        drawList->AddLine(ImVec2(c.x + 2.5f, c.y - 5.0f), ImVec2(c.x + 6.5f, c.y), codeCol, 1.5f);
        drawList->AddLine(ImVec2(c.x + 6.5f, c.y), ImVec2(c.x + 2.5f, c.y + 5.0f), codeCol, 1.5f);
        drawList->AddLine(ImVec2(c.x + 1.0f, c.y - 5.5f), ImVec2(c.x - 1.0f, c.y + 5.5f), ImColor(160, 255, 180), 1.5f);

    } else if (tab == BlenderPropertyTab::IMPORT_EXPORT) {
        // [IMPORT / EXPORT] -> Caja 3D compacta con flechas
        ImU32 boxCol = ImColor(185, 130, 255);
        ImU32 arrowCol = ImColor(90, 220, 255);

        drawList->AddLine(ImVec2(c.x - 6.0f, c.y - 0.5f), ImVec2(c.x - 6.0f, c.y + 6.0f), boxCol, 1.5f);
        drawList->AddLine(ImVec2(c.x - 6.0f, c.y + 6.0f), ImVec2(c.x + 6.0f, c.y + 6.0f), boxCol, 1.5f);
        drawList->AddLine(ImVec2(c.x + 6.0f, c.y + 6.0f), ImVec2(c.x + 6.0f, c.y - 0.5f), boxCol, 1.5f);

        drawList->AddLine(ImVec2(c.x, c.y - 6.5f), ImVec2(c.x, c.y + 2.0f), arrowCol, 1.6f);
        drawList->AddLine(ImVec2(c.x - 3.0f, c.y - 0.5f), ImVec2(c.x, c.y + 2.5f), arrowCol, 1.6f);
        drawList->AddLine(ImVec2(c.x + 3.0f, c.y - 0.5f), ImVec2(c.x, c.y + 2.5f), arrowCol, 1.6f);

    } else if (tab == BlenderPropertyTab::PARTICLES) {
        // [VFX] -> Estrella compacta
        ImU32 vfxCol = ImColor(255, 120, 30);
        ImU32 glowCol = ImColor(255, 230, 100);

        drawList->AddTriangleFilled(ImVec2(c.x, c.y - 7.5f), ImVec2(c.x - 2.0f, c.y), ImVec2(c.x + 2.0f, c.y), vfxCol);
        drawList->AddTriangleFilled(ImVec2(c.x, c.y + 7.5f), ImVec2(c.x - 2.0f, c.y), ImVec2(c.x + 2.0f, c.y), vfxCol);
        drawList->AddTriangleFilled(ImVec2(c.x - 7.5f, c.y), ImVec2(c.x, c.y - 2.0f), ImVec2(c.x, c.y + 2.0f), vfxCol);
        drawList->AddTriangleFilled(ImVec2(c.x + 7.5f, c.y), ImVec2(c.x, c.y - 2.0f), ImVec2(c.x, c.y + 2.0f), vfxCol);
        drawList->AddCircleFilled(c, 2.2f, glowCol);

    } else if (tab == BlenderPropertyTab::ANIMATION) {
        // [ANIMACIÓN] -> Claqueta compacta
        ImVec2 boxMin(c.x - 6.5f, c.y - 5.5f);
        ImVec2 boxMax(c.x + 6.5f, c.y + 5.5f);
        drawList->AddRectFilled(boxMin, boxMax, ImColor(35, 45, 60), 2.0f);
        drawList->AddRect(boxMin, boxMax, ImColor(90, 200, 240), 2.0f, 0, 1.0f);

        drawList->AddRectFilled(boxMin, ImVec2(boxMax.x, boxMin.y + 3.5f), ImColor(55, 75, 100), 1.5f);
        drawList->AddTriangleFilled(ImVec2(c.x - 1.8f, c.y - 0.5f), ImVec2(c.x - 1.8f, c.y + 3.8f), ImVec2(c.x + 2.5f, c.y + 1.6f), ImColor(90, 220, 255));
    }

    if (hovered) {
        ImGui::SetTooltip("%s", tooltip);
    }
}

void EditorUI::renderObjectProperties(Scene& scene, Camera& camera, PhysicsWorld& physicsWorld) {
    if (m_selectedEntityName.empty()) {
        ImGui::TextDisabled("Selecciona un objeto en el Viewport 3D\no en la jerarquia para ver sus propiedades.");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Atajos de Gizmo 3D:");
        ImGui::BulletText("Tecla W: Traslacion (Mover ejes)");
        ImGui::BulletText("Tecla E: Rotacion (Aros orbitales)");
        ImGui::BulletText("Tecla R: Escala (Dimensiones)");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Atajos de Navegacion:");
        ImGui::BulletText("Clic Izquierdo en 3D: Seleccionar objeto");
        ImGui::BulletText("RMB + WASD: Vuelo libre (Unreal)");
        ImGui::BulletText("MMB / Alt+LMB: Orbitar (Blender)");
        ImGui::BulletText("Tecla Espacio: Pausar/Reanudar animaciones");
        ImGui::BulletText("Tecla F: Re-enfocar vista");
    } else {
        auto entity = scene.getEntity(m_selectedEntityName);
        if (!entity) {
            m_selectedEntityName = "";
            return;
        }

        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.20f, 1.0f), "Objeto: %s", entity->getName().c_str());
        ImGui::Separator();

        // 1. Transformación (Estilo Blender 4/5: Location, Rotation, Scale vertical con X/Y/Z)
        if (ImGui::CollapsingHeader("Transformacion", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& seq = Sequencer::get();
            float curT = seq.getCurrentTime();
            int animState = 0;
            for (const auto& trk : seq.getEntityTracks()) {
                if (trk.entityName == entity->getName() && !trk.keyframes.empty()) {
                    bool exact = false;
                    for (const auto& k : trk.keyframes) {
                        if (std::abs(k.time - curT) < 0.02f) {
                            exact = true;
                            break;
                        }
                    }
                    animState = exact ? 2 : 1;
                    break;
                }
            }

            auto toggleKeyframe = [&]() {
                if (animState == 2) {
                    for (auto& trk : seq.getEntityTracks()) {
                        if (trk.entityName == entity->getName()) {
                            trk.keyframes.erase(
                                std::remove_if(trk.keyframes.begin(), trk.keyframes.end(),
                                    [&](const EntityTransformKeyframe& k) {
                                        return std::abs(k.time - curT) < 0.02f;
                                    }),
                                trk.keyframes.end()
                            );
                            break;
                        }
                    }
                    setStatusBarNotification("Keyframe eliminado para '" + entity->getName() + "'");
                } else {
                    seq.recordEntityKeyframe(*entity);
                    char buf[96];
                    snprintf(buf, sizeof(buf), "Keyframe insertado para '%s' en %.2fs", entity->getName().c_str(), curT);
                    setStatusBarNotification(buf);
                }
            };

            UI::BeginProps("ObjTransformProps");
            glm::vec3& pos = entity->getPositionRef();
            if (UI::PropVec3Vertical("Posicion", pos, 0.05f, 0.0f, 0.0f, "%.2f", nullptr, animState, toggleKeyframe)) {
                entity->syncQuaternionFromEuler();
                physicsWorld.syncEntityToPhysics(entity);
                if (seq.isAutoKeyEnabled() || animState == 2) seq.recordEntityKeyframe(*entity);
            }

            glm::vec3& rot = entity->getRotationRef();
            if (UI::PropVec3Vertical("Rotacion", rot, 1.0f, 0.0f, 0.0f, "%.2f", nullptr, animState, toggleKeyframe)) {
                rot.x = std::fmod(rot.x, 360.0f); if (rot.x < 0.0f) rot.x += 360.0f;
                rot.y = std::fmod(rot.y, 360.0f); if (rot.y < 0.0f) rot.y += 360.0f;
                rot.z = std::fmod(rot.z, 360.0f); if (rot.z < 0.0f) rot.z += 360.0f;
                entity->syncQuaternionFromEuler();
                physicsWorld.syncEntityToPhysics(entity);
                if (seq.isAutoKeyEnabled() || animState == 2) seq.recordEntityKeyframe(*entity);
            }

            glm::vec3& scale = entity->getScaleRef();
            if (UI::PropVec3Vertical("Escala", scale, 0.05f, 0.01f, 100.0f, "%.2f", nullptr, animState, toggleKeyframe)) {
                entity->syncQuaternionFromEuler();
                physicsWorld.syncEntityToPhysics(entity);
                if (seq.isAutoKeyEnabled() || animState == 2) seq.recordEntityKeyframe(*entity);
            }
            UI::EndProps();
        }

        // 2. Material y Color
        if (ImGui::CollapsingHeader("Material y Color", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& seq = Sequencer::get();
            float curT = seq.getCurrentTime();
            int colorAnimState = 0;
            for (const auto& trk : seq.getEntityTracks()) {
                if (trk.entityName == entity->getName() && !trk.keyframes.empty()) {
                    for (const auto& k : trk.keyframes) {
                        if (std::abs(k.time - curT) < 0.02f) {
                            colorAnimState = 2;
                            break;
                        }
                    }
                    if (colorAnimState == 0) colorAnimState = 1;
                    break;
                }
            }

            auto toggleColorKeyframe = [&]() {
                seq.recordEntityKeyframe(*entity);
                setStatusBarNotification("Keyframe registrado para '" + entity->getName() + "'");
            };

            UI::BeginProps("ObjColorProps");
            glm::vec3& color = entity->getColorRef();
            if (UI::PropColor3("Color RGB", &color[0], nullptr, colorAnimState, toggleColorKeyframe)) {
                entity->setHasCustomColor(true);
                if (seq.isAutoKeyEnabled()) seq.recordEntityKeyframe(*entity);
            }

            bool visible = entity->isVisible();
            if (UI::PropCheckbox("Visible", &visible)) {
                entity->setVisible(visible);
            }
            UI::EndProps();
        }

        // 3. Zona de Trigger y Disparadores de Eventos (Fase 17)
        if (ImGui::CollapsingHeader("Disparador de Eventos / Trigger (Fase 17)", ImGuiTreeNodeFlags_DefaultOpen)) {
            UI::BeginProps("ObjTriggerProps");
            bool isTrig = entity->isTrigger();
            if (UI::PropCheckbox("Es Trigger", &isTrig, "Sensor sin colision fisica")) {
                entity->setTrigger(isTrig);
                physicsWorld.setBodyTrigger(entity->getName(), isTrig);
                if (isTrig && !physicsWorld.hasRigidBody(entity->getName())) {
                    physicsWorld.addRigidBody(entity, 0.0f, ColliderType::BOX);
                    physicsWorld.setBodyTrigger(entity->getName(), true);
                }
            }
            if (isTrig) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                UI::PropLabel("Tag Trigger");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                char tagBuf[128] = { 0 };
                strncpy_s(tagBuf, entity->getTriggerTag().c_str(), sizeof(tagBuf) - 1);
                if (ImGui::InputText("##TriggerTag", tagBuf, sizeof(tagBuf))) {
                    entity->setTriggerTag(tagBuf);
                }
            }
            UI::EndProps();
            if (isTrig) {
                ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "onTriggerEnter(trigger, other)");
            }
        }

        // 4. Guardar como Prefab (Fase 17)
        if (ImGui::Button("Guardar como Prefab (.prefab)...", ImVec2(-1, 26))) {
            PrefabManager::saveEntityAsPrefab(entity, entity->getName(), &physicsWorld);
            m_availablePrefabs = PrefabManager::getAvailablePrefabs("assets/prefabs");
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Enfocar Camara (F)")) {
            camera.resetToFocus(entity->getPosition());
        }
        ImGui::SameLine();
        if (ImGui::Button("Resetear Transformacion")) {
            entity->setPosition(0.0f, 1.0f, 0.0f);
            entity->setRotation(0.0f, 0.0f, 0.0f);
            entity->setScale(1.0f, 1.0f, 1.0f);
            physicsWorld.syncEntityToPhysics(entity);
        }
    }
}

void EditorUI::renderMaterialProperties(Scene& scene) {
    auto& matMgr = MaterialManager::get();
    const auto& allMaterials = matMgr.getMaterials();

    // 0. Cabecera del Gestor de Materiales PBR (Fase 21)
    ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.25f, 1.0f), "Gestor de Materiales PBR y Biblioteca (.asismat)");
    ImGui::TextDisabled("Biblioteca global y asignacion fisica a entidades de escena");
    ImGui::Separator();

    auto entity = (!m_selectedEntityName.empty()) ? scene.getEntity(m_selectedEntityName) : nullptr;
    std::shared_ptr<Material> mat = entity ? entity->getMaterial() : nullptr;
    if (entity && !mat) {
        entity->setMaterial(std::make_shared<Material>());
        mat = entity->getMaterial();
    }

    // 1. Paleta Visual de Presets PBR Físicos (14 Preajustes en Grid de 3 Columnas)
    if (ImGui::CollapsingHeader("Paleta de Presets Fisicos PBR (Un Clic)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled(entity ? "Aplica preajustes fisicos al objeto seleccionado:" : "Selecciona un objeto para aplicarle estos presets:");

        auto applyPresetToSelection = [&](const std::string& presetName) {
            if (entity) {
                matMgr.applyToEntity(*entity, presetName);
            }
            if (!m_selectedEntityNames.empty()) {
                for (const auto& name : m_selectedEntityNames) {
                    auto ent = scene.getEntity(name);
                    if (ent) matMgr.applyToEntity(*ent, presetName);
                }
            }
        };

        struct PBRPresetItem {
            const char* id;
            const char* label;
            ImU32 dotColor;
        };
        static const PBRPresetItem s_presets[14] = {
            { "Oro Pulido", "Oro", IM_COL32(255, 215, 0, 255) },
            { "Cobre", "Cobre", IM_COL32(184, 115, 51, 255) },
            { "Acero Inoxidable", "Acero", IM_COL32(200, 200, 205, 255) },
            { "Cromo / Espejo", "Cromo", IM_COL32(230, 235, 245, 255) },
            { "Vidrio / Cristal", "Vidrio", IM_COL32(160, 220, 240, 255) },
            { "Plastico Brillante", "Plastico", IM_COL32(220, 50, 50, 255) },
            { "Plastico Mate", "Mate", IM_COL32(220, 220, 220, 255) },
            { "Neon Cian Emisivo", "Neon Cian", IM_COL32(0, 240, 255, 255) },
            { "Neon Carmesi", "Neon Carmesi", IM_COL32(255, 30, 80, 255) },
            { "Obsidiana Negra", "Obsidiana", IM_COL32(35, 35, 40, 255) },
            { "Marmol Blanco", "Marmol", IM_COL32(245, 245, 240, 255) },
            { "Hormigon / Concreto", "Hormigon", IM_COL32(140, 140, 140, 255) },
            { "Hierro Oxidado", "Oxido", IM_COL32(165, 75, 42, 255) },
            { "Madera / Corteza", "Madera", IM_COL32(139, 90, 43, 255) }
        };

        if (ImGui::BeginTable("##PBRPresetsGrid", 3, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp)) {
            for (int i = 0; i < 14; ++i) {
                ImGui::TableNextColumn();
                ImVec2 p = ImGui::GetCursorScreenPos();
                std::string btnLabel = "   " + std::string(s_presets[i].label) + "##" + std::to_string(i);
                if (ImGui::Button(btnLabel.c_str(), ImVec2(-FLT_MIN, 22.0f))) {
                    applyPresetToSelection(s_presets[i].id);
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("ASIS_MATERIAL_PRESET", s_presets[i].id, strlen(s_presets[i].id) + 1);
                    ImGui::Text("Material PBR: %s", s_presets[i].id);
                    ImGui::EndDragDropSource();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Aplicar preset: %s (Arrastrar al Viewport para asignar)", s_presets[i].id);
                }
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddCircleFilled(ImVec2(p.x + 8.0f, p.y + 11.0f), 3.5f, s_presets[i].dotColor);
            }
            ImGui::EndTable();
        }
    }

    // 2. Biblioteca de Materiales del Proyecto y Persistencia (.asismat)
    if (ImGui::CollapsingHeader("Biblioteca de Materiales (.asismat)", ImGuiTreeNodeFlags_DefaultOpen)) {
        float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (UI::ButtonWithIcon(UI::IconType::FOLDER, "Cargar...", ImVec2(halfW, 22.0f))) {
            std::string path = SceneIO::openFileDialog("Material ASIS (*.asismat)\0*.asismat\0Todos los archivos (*.*)\0*.*\0");
            if (!path.empty()) {
                auto loaded = matMgr.loadMaterialFromFile(path);
                if (loaded && entity) {
                    matMgr.applyToEntity(*entity, loaded);
                }
            }
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::RESET, "Escanear", ImVec2(halfW, 22.0f))) {
            matMgr.scanMaterialsDirectory("assets/materials");
        }

        if (entity && mat) {
            if (UI::ButtonWithIcon(UI::IconType::SAVE, "Guardar Seleccion (.asismat)...", ImVec2(-1, 22.0f))) {
                std::string defaultFile = entity->getName() + ".asismat";
                std::string path = SceneIO::saveFileDialog("Material ASIS (*.asismat)\0*.asismat\0", "asismat");
                if (!path.empty()) {
                    auto asset = matMgr.extractFromEntity(*entity, entity->getName());
                    if (asset) {
                        matMgr.saveMaterialToFile(*asset, path);
                    }
                }
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Materiales registrados (%zu activos):", allMaterials.size());

        static int selectedAssetIdx = 0;
        if (!allMaterials.empty()) {
            if (selectedAssetIdx >= (int)allMaterials.size()) selectedAssetIdx = 0;

            std::string previewName = allMaterials[selectedAssetIdx]->name;
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##MaterialAssetCombo", previewName.c_str())) {
                for (int i = 0; i < (int)allMaterials.size(); ++i) {
                    bool isSel = (selectedAssetIdx == i);
                    auto m = allMaterials[i];
                    ImVec4 chipColor(m->material->albedo.r, m->material->albedo.g, m->material->albedo.b, 1.0f);
                    ImGui::ColorButton("##chip", chipColor, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(14, 14));
                    ImGui::SameLine();
                    if (ImGui::Selectable(m->name.c_str(), isSel)) {
                        selectedAssetIdx = i;
                    }
                    if (isSel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (selectedAssetIdx >= 0 && selectedAssetIdx < (int)allMaterials.size()) {
                auto activeAsset = allMaterials[selectedAssetIdx];
                if (entity && UI::ButtonWithIcon(UI::IconType::CAMERA_TARGET, "Asignar este Material a Seleccion", ImVec2(-1, 22.0f))) {
                    matMgr.applyToEntity(*entity, activeAsset);
                }
            }
        }
    }

    // 3. Si no hay entidad seleccionada, mostrar mensaje orientativo
    if (!entity || !mat) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Selecciona un objeto en el Viewport 3D o en la jerarquia\npara inspeccionar y editar sus parametros PBR directamente.");
        return;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Propiedades PBR del Objeto: %s", entity->getName().c_str());

    // 4. Propiedades Físicas PBR (Cook-Torrance Microfacet GGX)
    if (ImGui::CollapsingHeader("Propiedades Fisicas PBR", ImGuiTreeNodeFlags_DefaultOpen)) {
        UI::BeginProps("PBRPropsTable");
        UI::PropColor3("Albedo Base", &mat->albedo[0], "Color difuso base");
        UI::PropSlider("Rugosidad", &mat->roughness, 0.04f, 1.0f, "%.2f", "0.04 = Espejo pulido, 1.0 = Mate difuso");
        UI::PropSlider("Metalicidad", &mat->metallic, 0.0f, 1.0f, "%.2f", "0.0 = Dielectrico, 1.0 = Conductor puro");
        UI::PropSlider("Especular", &mat->specular, 0.0f, 1.0f, "%.2f", "Intensidad del reflejo Fresnel");
        UI::PropSlider("Relieve Normal", &mat->normalStrength, 0.0f, 3.0f, "%.2fx", "Fuerza del mapa de normales");
        UI::PropSlider("Reflejo IBL / AO", &mat->ao, 0.0f, 1.0f, "%.2f", "Intensidad del reflejo de entorno HDRI y AO");
        UI::EndProps();
    }

    // 5. Emisión y Resplandor propio
    if (ImGui::CollapsingHeader("Emision de Luz y Resplandor", ImGuiTreeNodeFlags_DefaultOpen)) {
        UI::BeginProps("PBREmissionTable");
        UI::PropColor3("Color Emision", &mat->emission[0], "Tono de luz emitida");
        UI::PropSlider("Intensidad", &mat->emissionStrength, 0.0f, 10.0f, "%.1fx", "Multiplicador de intensidad de emision");
        UI::EndProps();
    }

    // 6. Mapas de Texturas PBR (7 Ranuras Compactas)
    if (ImGui::CollapsingHeader("Mapas de Textura PBR (7 Slots)", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* imgFilter = "Imagenes (*.png;*.jpg;*.jpeg;*.bmp;*.tga)\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0Todos los archivos (*.*)\0*.*\0";

        auto renderTexSlot = [&](const char* slotLabel, std::shared_ptr<Texture>& slot, auto setter) {
            std::string path = slot ? slot->getPath() : "";
            UI::PropTextureSlot(slotLabel, path, slot != nullptr,
                [&]() {
                    std::string p = SceneIO::openFileDialog(imgFilter);
                    if (!p.empty()) {
                        try {
                            setter(std::make_shared<Texture>(p));
                        } catch (...) {
                            std::cerr << "[Material] Error al cargar textura: " << p << std::endl;
                        }
                    }
                },
                [&]() {
                    slot = nullptr;
                }
            );
        };

        UI::BeginProps("PBRTexturesTable");
        renderTexSlot("Albedo", mat->albedoMap, [&](auto t) { mat->setAlbedoMap(t); });
        renderTexSlot("Normal", mat->normalMap, [&](auto t) { mat->setNormalMap(t); });
        renderTexSlot("Rugosidad", mat->roughnessMap, [&](auto t) { mat->setRoughnessMap(t); });
        renderTexSlot("Metálico", mat->metallicMap, [&](auto t) { mat->setMetallicMap(t); });
        renderTexSlot("Especular", mat->specularMap, [&](auto t) { mat->setSpecularMap(t); });
        renderTexSlot("Emisión", mat->emissionMap, [&](auto t) { mat->setEmissionMap(t); });
        renderTexSlot("AO", mat->aoMap, [&](auto t) { mat->setAOMap(t); });
        UI::EndProps();
    }
}

void EditorUI::renderLightingProperties(Scene& scene, ShadowMap& shadowMap, Camera& camera, Skybox& skybox) {
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Iluminacion, Sombras 4K, Multiluz y Camara");
    ImGui::Separator();

    // 0. Cielo Skybox HDRI e Iluminación IBL (Fase 12)
    if (ImGui::CollapsingHeader("Cielo Skybox HDRI e Iluminacion IBL (Fase 12)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Habilitar IBL", &m_useIBL);
        ImGui::SameLine();
        ImGui::Checkbox("Mostrar Cielo", &m_showSkybox);
        ImGui::SameLine();
        ImGui::Checkbox("Flip Y", &m_flipSkyboxY);
        ImGui::SameLine();
        ImGui::Checkbox("Flip X", &m_flipSkyboxX);

        UI::BeginProps("IBLPropsTable");
        UI::PropSlider("Intensidad IBL", &m_iblIntensity, 0.0f, 3.0f, "%.2fx", "Luz indirecta y reflejos ambientales");
        UI::PropSlider("Exposicion Cielo", &m_skyboxExposure, 0.1f, 5.0f, "%.2fx", "Brillo del fondo");
        UI::PropSlider("Rotacion 360°", &m_skyboxRotation, 0.0f, 360.0f, "%.1f deg", "Orientacion azimutal del HDRI");
        UI::PropSlider("Desenfoque", &m_skyboxBlur, 0.0f, 7.0f, "%.1f", "Desenfoque de fondo para bokeh");
        UI::EndProps();

        ImGui::Spacing();
        std::string hdriFname = std::filesystem::path(skybox.getHDRIPath()).filename().string();
        ImGui::TextDisabled("Mapa activo: %s", hdriFname.empty() ? "[Procedural]" : hdriFname.c_str());

        float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (UI::ButtonWithIcon(UI::IconType::FOLDER, "Cargar HDRI...", ImVec2(halfW, 22.0f))) {
            std::string path = SceneIO::openFileDialog("Imagenes HDRI / 360 (*.hdr;*.exr;*.png;*.jpg)\0*.hdr;*.exr;*.png;*.jpg\0Todos los archivos (*.*)\0*.*\0");
            if (!path.empty()) {
                if (skybox.loadHDRI(path)) {
                    m_showSkybox = true;
                    m_useIBL = true;
                    std::string lowerPath = path;
                    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                    m_flipSkyboxY = (lowerPath.rfind(".exr") != std::string::npos);
                }
            }
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::SUN, "Cielo Procedural", ImVec2(halfW, 22.0f))) {
            skybox.generateProceduralSky();
        }
    }

    // 0.1. Gestión de Color y Mapeo Tonal (ACES Filmic / AgX) - Fase 11 y 17
    if (ImGui::CollapsingHeader("Gestion de Color, Exposicion y Adaptacion Ocular (Fase 17)", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* profiles[] = { "ACES Filmic (Predeterminado)", "AgX (Blender 4.0)", "Reinhard", "Lineal / sRGB" };
        UI::BeginProps("ColorGradingProps");
        UI::PropCombo("Curva Tonal", &m_colorProfile, profiles, IM_ARRAYSIZE(profiles));
        UI::PropSlider("Gamma", &m_gamma, 1.0f, 3.2f, "%.2f");
        UI::PropSlider("Saturacion", &m_saturation, 0.0f, 2.5f, "%.2fx");
        UI::PropSlider("Contraste", &m_contrast, 0.5f, 2.0f, "%.2fx");
        UI::EndProps();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Adaptacion Ocular y Auto-Exposicion (Fase 17)");
        ImGui::Checkbox("Habilitar Auto-Exposicion Dinamica (Eye Adaptation)", &m_autoExposureEnabled);

        if (m_autoExposureEnabled) {
            ImGui::TextDisabled("Luminancia: %.3f cd/m2 | Exp: %.2fx", m_measuredSceneLuminance, m_currentAdaptedExposure);
            float progress = (m_autoExposureMax > m_autoExposureMin) ? (m_currentAdaptedExposure - m_autoExposureMin) / (m_autoExposureMax - m_autoExposureMin) : 0.5f;
            ImGui::ProgressBar(std::clamp(progress, 0.0f, 1.0f), ImVec2(-1, 12));

            UI::BeginProps("AutoExposureProps");
            UI::PropSlider("Compensacion EV", &m_exposure, 0.1f, 3.0f, "%.2fx");
            UI::PropSlider("Luminancia Clave", &m_autoExposureKey, 0.05f, 0.50f, "%.2f");
            UI::PropSlider("Exposicion Min.", &m_autoExposureMin, 0.05f, 2.0f, "%.2fx");
            UI::PropSlider("Exposicion Max.", &m_autoExposureMax, 1.0f, 10.0f, "%.2fx");
            UI::PropSlider("Velocidad a Luz", &m_autoExposureSpeedUp, 0.5f, 10.0f, "%.1fx /s");
            UI::PropSlider("Vel. a Oscuridad", &m_autoExposureSpeedDown, 0.1f, 5.0f, "%.1fx /s");
            UI::EndProps();

            ImGui::TextDisabled("Preajustes de Adaptacion:");
            if (ImGui::SmallButton("Ojo Humano")) {
                m_autoExposureKey = 0.18f; m_autoExposureMin = 0.20f; m_autoExposureMax = 4.50f;
                m_autoExposureSpeedUp = 3.0f; m_autoExposureSpeedDown = 1.0f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Camara DSLR")) {
                m_autoExposureKey = 0.18f; m_autoExposureMin = 0.20f; m_autoExposureMax = 5.0f;
                m_autoExposureSpeedUp = 6.0f; m_autoExposureSpeedDown = 4.0f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Cinematico")) {
                m_autoExposureKey = 0.14f; m_autoExposureMin = 0.10f; m_autoExposureMax = 6.0f;
                m_autoExposureSpeedUp = 2.0f; m_autoExposureSpeedDown = 0.6f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Cuevas/Noche")) {
                m_autoExposureKey = 0.22f; m_autoExposureMin = 0.40f; m_autoExposureMax = 8.0f;
                m_autoExposureSpeedUp = 4.0f; m_autoExposureSpeedDown = 1.5f;
            }
        } else {
            UI::BeginProps("ManualExposureProps");
            UI::PropSlider("Exposicion Fija", &m_exposure, 0.1f, 5.0f, "%.2fx");
            UI::EndProps();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Preajustes de Color:");
        if (ImGui::SmallButton("Vibrante / Rico")) {
            m_colorProfile = 0; m_exposure = 1.15f; m_gamma = 2.2f; m_saturation = 1.25f; m_contrast = 1.10f;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Cinematico AgX")) {
            m_colorProfile = 1; m_exposure = 1.10f; m_gamma = 2.2f; m_saturation = 1.10f; m_contrast = 1.05f;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Neutro / Fiel")) {
            m_colorProfile = 0; m_exposure = 1.0f; m_gamma = 2.2f; m_saturation = 1.0f; m_contrast = 1.0f;
        }
    }

    // 0.9. Ciclo Día / Noche Dinámico de 24 Horas y Clima Atmosférico (Fase 20)
    if (ImGui::CollapsingHeader("Ciclo Dia / Noche Dinamico (24h) y Clima Atmosferico (Fase 20)", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool cycleOn = m_dayNight.isCycleEnabled();
        if (ImGui::Checkbox("Habilitar Ciclo 24h en Tiempo Real", &cycleOn)) {
            m_dayNight.setCycleEnabled(cycleOn);
        }

        float timeOfDay = m_dayNight.getTimeOfDay();
        std::string timeStr = "Hora: " + m_dayNight.getTimeString() + (m_dayNight.isNight() ? " [Noche]" : " [Dia]");
        UI::BeginProps("DayNightPropsTable");
        if (UI::PropSlider("Reloj 24h", &timeOfDay, 0.0f, 24.0f, timeStr.c_str())) {
            m_dayNight.setTimeOfDay(timeOfDay);
        }
        float dayDur = m_dayNight.getDayDurationSeconds();
        if (UI::PropSlider("Duracion Dia", &dayDur, 15.0f, 600.0f, "%.0f s")) {
            m_dayNight.setDayDurationSeconds(dayDur);
        }
        float sunLat = m_dayNight.getSunLatitude();
        if (UI::PropSlider("Latitud Solar", &sunLat, -80.0f, 80.0f, "%.1f deg")) {
            m_dayNight.setSunLatitude(sunLat);
        }
        float horizH = m_dayNight.getHorizonHeight();
        if (UI::PropSlider("Horizonte Offset", &horizH, -0.25f, 0.25f, "%.3f")) {
            m_dayNight.setHorizonHeight(horizH);
        }
        UI::EndProps();

        ImGui::TextDisabled("Saltos Rapidos:");
        if (ImGui::SmallButton("Amanecer (06:30)")) m_dayNight.setTimeOfDay(6.5f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Mediodia (12:00)")) m_dayNight.setTimeOfDay(12.0f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Atardecer (18:45)")) m_dayNight.setTimeOfDay(18.75f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Noche (00:00)")) m_dayNight.setTimeOfDay(0.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Clima Atmosferico y Precipitaciones (Fase 20)");

        int weatherIdx = static_cast<int>(m_dayNight.getWeather());
        const char* weatherNames[] = { "Despejado", "Parcialmente Nublado", "Nublado", "Lluvia Ligera", "Tormenta Electrica" };
        UI::BeginProps("WeatherPropsTable");
        if (UI::PropCombo("Estado Clima", &weatherIdx, weatherNames, IM_ARRAYSIZE(weatherNames))) {
            m_dayNight.setWeather(static_cast<WeatherType>(weatherIdx));
        }
        float wetness = m_dayNight.getWetness();
        if (UI::PropSlider("Humedad PBR", &wetness, 0.0f, 1.0f, "%.2fx")) {
            m_dayNight.setWetness(wetness);
        }
        UI::EndProps();

        if (m_dayNight.getWeather() == WeatherType::STORM && m_dayNight.getLightningFlash() > 0.05f) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "[!] ¡DESTELLO DE RELAMPAGO ACTIVO!");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.8f, 1.0f), "Neblina Atmosferica Global:");

        bool fogEnabled = m_dayNight.isFogEnabled();
        if (ImGui::Checkbox("Habilitar Neblina Global (Volumetric Fog)", &fogEnabled)) {
            m_dayNight.setFogEnabled(fogEnabled);
        }

        if (fogEnabled) {
            float fogStart = m_dayNight.getFogStart();
            float fogEnd = m_dayNight.getFogEnd();
            float fogDensity = m_dayNight.getFogDensity();
            glm::vec3 fogCol = m_dayNight.getFogColor();

            UI::BeginProps("FogPropsTable");
            if (UI::PropSlider("Distancia Cerca", &fogStart, 0.0f, 150.0f, "%.1f m")) m_dayNight.setFogStart(fogStart);
            if (UI::PropSlider("Distancia Lejos", &fogEnd, 50.0f, 1200.0f, "%.1f m")) m_dayNight.setFogEnd(fogEnd);
            if (UI::PropSlider("Densidad", &fogDensity, 0.0f, 1.0f, "%.2fx")) m_dayNight.setFogDensity(fogDensity);
            if (UI::PropColor3("Color Neblina", &fogCol.r)) {
                m_dayNight.setFogColor(fogCol);
                m_dayNight.setCustomFogColor(true);
            }
            UI::EndProps();

            if (ImGui::Button("Sincronizar Color con Horizonte", ImVec2(-1, 20.0f))) {
                m_dayNight.setCustomFogColor(false);
            }
        }
    }

    // 1. Sol Direccional Principal
    if (ImGui::CollapsingHeader("Sol Direccional y Sombras", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Habilitar Sombras Dinamicas", &m_shadowsEnabled);
        UI::BeginProps("SunPropsTable");
        UI::PropSlider("Azimut Solar", &m_sunAzimuth, -180.0f, 180.0f, "%.1f deg");
        UI::PropSlider("Elevacion Solar", &m_sunElevation, 5.0f, 89.0f, "%.1f deg");
        UI::PropColor3("Color del Sol", &m_lightColor[0]);
        UI::PropSlider("Intensidad Sol", &m_lightIntensity, 0.0f, 4.0f, "%.2fx");
        UI::PropColor3("Color Ambiental", &m_ambientColor[0]);
        UI::PropSlider("Intensidad Amb.", &m_ambientIntensity, 0.0f, 2.0f, "%.2fx");
        UI::EndProps();

        ImGui::TextDisabled("Ajustes Rapidos:");
        if (ImGui::SmallButton("Mediodia Cenital")) {
            m_sunAzimuth = 45.0f; m_sunElevation = 80.0f;
            m_lightColor = glm::vec3(1.0f, 0.98f, 0.95f); m_lightIntensity = 1.25f;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Atardecer Dorado")) {
            m_sunAzimuth = 110.0f; m_sunElevation = 18.0f;
            m_lightColor = glm::vec3(1.0f, 0.65f, 0.30f); m_lightIntensity = 1.30f;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Estudio / Diagonal")) {
            m_sunAzimuth = 45.0f; m_sunElevation = 45.0f;
            m_lightColor = glm::vec3(1.0f, 0.96f, 0.88f); m_lightIntensity = 1.15f;
        }
    }

    // 2. Parámetros de Sombras Ultra-HD (1K / 2K / 4K / 8K) y Penumbra
    if (ImGui::CollapsingHeader("Calidad de Sombras & Penumbra (PCSS / CSM)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Cascaded Shadow Maps (CSM)", &m_csmEnabled);
        if (m_csmEnabled) {
            ImGui::SameLine();
            ImGui::Checkbox("Debug Cascadas", &m_csmDebugColors);
        }

        ImGui::Spacing();
        bool isPCF = (m_shadowFilterMode == 0);
        bool isPCSS = (m_shadowFilterMode == 1);
        if (ImGui::RadioButton("PCF Uniforme", isPCF)) m_shadowFilterMode = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("PCSS Fisico (Contact Hardening)", isPCSS)) m_shadowFilterMode = 1;

        if (m_shadowFilterMode == 1) {
            UI::BeginProps("PCSSPropsTable");
            UI::PropSlider("Tamano Fuente Luz", &m_pcssLightSize, 0.05f, 1.5f, "%.2fx");
            UI::PropSliderInt("Muestras Bloqueo", &m_pcssBlockerSamples, 8, 32);
            UI::PropSliderInt("Muestras Filtro", &m_pcssFilterSamples, 16, 64);
            UI::EndProps();
        } else {
            UI::BeginProps("PCFPropsTable");
            UI::PropSlider("Suavizado Penumbra", &m_shadowSoftness, 0.2f, 3.0f, "%.2fx");
            UI::EndProps();
            bool k1 = (m_shadowKernel == 1);
            bool k2 = (m_shadowKernel == 2);
            bool k3 = (m_shadowKernel == 3);
            if (ImGui::RadioButton("3x3", k1)) m_shadowKernel = 1;
            ImGui::SameLine();
            if (ImGui::RadioButton("5x5", k2)) m_shadowKernel = 2;
            ImGui::SameLine();
            if (ImGui::RadioButton("7x7", k3)) m_shadowKernel = 3;
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Resolucion de Sombra:");
        bool is1K = (m_shadowResolution == 1024);
        bool is2K = (m_shadowResolution == 2048);
        bool is4K = (m_shadowResolution == 4096);
        bool is8K = (m_shadowResolution == 8192);

        if (ImGui::RadioButton("1K", is1K)) { m_shadowResolution = 1024; shadowMap.resize(1024, 1024); }
        ImGui::SameLine();
        if (ImGui::RadioButton("2K", is2K)) { m_shadowResolution = 2048; shadowMap.resize(2048, 2048); }
        ImGui::SameLine();
        if (ImGui::RadioButton("4K HD", is4K)) { m_shadowResolution = 4096; shadowMap.resize(4096, 4096); }
        ImGui::SameLine();
        if (ImGui::RadioButton("8K Cine", is8K)) { m_shadowResolution = 8192; shadowMap.resize(8192, 8192); }

        UI::BeginProps("ShadowRangeProps");
        UI::PropFloat("Shadow Bias", &m_shadowBias, 0.0001f, 0.0001f, 0.01f, "%.4f");
        UI::PropSlider("Alcance Sombras", &m_shadowDistance, 20.0f, 600.0f, "%.1f m");
        UI::PropCheckbox("Auto-Sombra Terreno", &m_terrainCastsShadows);
        UI::EndProps();
    }

    // 3. Luces Puntuales (Point Lights)
    auto& pointLights = scene.getPointLights();
    std::string lightHeader = "Luces Puntuales Activas (" + std::to_string(pointLights.size()) + "/8)";
    if (ImGui::CollapsingHeader(lightHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (pointLights.size() < 8) {
            if (UI::ButtonWithIcon(UI::IconType::PLUS, "Anadir Luz Puntual", ImVec2(-1, 24.0f))) {
                recordSnapshot(scene);
                glm::vec3 spawnPos = camera.getPosition() + camera.getFront() * 3.0f;
                PointLight newLight("Luz Puntual " + std::to_string(pointLights.size() + 1), spawnPos);
                scene.addPointLight(newLight);
                m_selectedLightIndex = static_cast<int>(pointLights.size() - 1);
                m_selectedEntityName = "";
            }
        } else {
            ImGui::TextDisabled("Maximo de 8 luces puntuales alcanzado.");
        }

        ImGui::Spacing();
        int lightToRemove = -1;
        for (size_t i = 0; i < pointLights.size(); ++i) {
            auto& light = pointLights[i];
            ImGui::PushID(static_cast<int>(i));

            bool isSel = (m_selectedLightIndex == static_cast<int>(i));
            std::string label = (isSel ? "* " : "") + light.name + "##Tree";
            if (ImGui::TreeNode(label.c_str())) {
                ImGui::Checkbox("Habilitada", &light.enabled);
                ImGui::SameLine();
                ImGui::Checkbox("Generar Sombras", &light.castShadows);

                UI::BeginProps("PLightPropsTable");
                if (light.castShadows) {
                    UI::PropSlider("Radio Fisico", &light.lightSize, 0.02f, 1.0f, "%.2f m");
                    UI::PropFloat("Shadow Bias", &light.bias, 0.001f, 0.001f, 0.08f, "%.4f");
                }
                UI::PropVec3Vertical("Posicion", light.position);
                UI::PropColor3("Color", &light.color[0]);
                UI::PropSlider("Intensidad", &light.intensity, 0.0f, 10.0f, "%.1fx");
                UI::EndProps();

                float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
                if (UI::ButtonWithIcon(UI::IconType::CAMERA_TARGET, "Gizmo", ImVec2(halfW, 20.0f))) {
                    m_selectedLightIndex = static_cast<int>(i);
                    m_selectedEntityName = "";
                }
                ImGui::SameLine();
                if (UI::ButtonWithIcon(UI::IconType::TRASH, "Eliminar", ImVec2(halfW, 20.0f), "Eliminar luz", IM_COL32(230, 80, 80, 255))) {
                    lightToRemove = static_cast<int>(i);
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if (lightToRemove >= 0) {
            recordSnapshot(scene);
            scene.removePointLight(static_cast<size_t>(lightToRemove));
            if (m_selectedLightIndex == lightToRemove) m_selectedLightIndex = -1;
        }
    }

    // 4. Parámetros de Cámara y Lente (FOV, Ortográfica, Planos)
    if (ImGui::CollapsingHeader("Sistema de Camara y Lente", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool isOrtho = camera.isOrthographic();
        if (ImGui::Checkbox("Proyeccion Ortografica (Isometrica)", &isOrtho)) {
            camera.setOrthographic(isOrtho);
        }

        UI::BeginProps("CameraPropsTable");
        if (!isOrtho) {
            float fov = camera.getFOV();
            if (UI::PropSlider("Campo de Vision", &fov, 15.0f, 110.0f, "%.1f deg")) {
                camera.setFOV(fov);
            }
        }
        UI::PropFloat("Plano Cercano", &camera.getNearPlaneRef(), 0.01f, 0.01f, 10.0f, "%.2f m");
        UI::PropFloat("Plano Lejano", &camera.getFarPlaneRef(), 5.0f, 10.0f, 5000.0f, "%.1f m");
        UI::EndProps();

        if (!isOrtho) {
            ImGui::TextDisabled("Lentes Rapidos:");
            if (ImGui::SmallButton("30° Telefoto")) camera.setFOV(30.0f);
            ImGui::SameLine();
            if (ImGui::SmallButton("45° Normal")) camera.setFOV(45.0f);
            ImGui::SameLine();
            if (ImGui::SmallButton("75° Angular")) camera.setFOV(75.0f);
            ImGui::SameLine();
            if (ImGui::SmallButton("100° Ojo Pez")) camera.setFOV(100.0f);
        }

        ImGui::Spacing();
        if (ImGui::Button("Resetear Camara a Vista Principal", ImVec2(-1, 22.0f))) {
            camera.resetToFocus(glm::vec3(0.0f, 0.5f, 0.0f));
            camera.setFOV(45.0f);
            camera.setOrthographic(false);
        }
    }

    // Camara de Cine y Lentes Fisicos (Fase 22)
    renderCineCameraProperties(camera, scene);
}

void EditorUI::renderTerrainProperties(Terrain& terrain, Camera& camera, PhysicsWorld& physicsWorld, Water* water) {
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.50f, 1.0f), "Terreno 3D Procedural, Escultura y Niebla Atmosferica");
    ImGui::Separator();

    // 1. Estado General y Dimensiones
    bool visible = terrain.isVisible();
    if (ImGui::Checkbox("Habilitar / Mostrar Terreno 3D", &visible)) {
        terrain.setVisible(visible);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Mostrar Suelo Plano", &m_showGroundPlane);

    ImGui::TextDisabled("Dimensiones: %.0fm x %.0fm | Resolucion: %dx%d (%d vertices, %d triangulos)",
        terrain.getWorldSize(), terrain.getWorldSize(),
        terrain.getResolution(), terrain.getResolution(),
        terrain.getResolution() * terrain.getResolution(),
        (terrain.getResolution() - 1) * (terrain.getResolution() - 1) * 2);

    ImGui::TextColored(ImVec4(0.70f, 0.85f, 0.95f, 1.0f), "Rango de Altitud: [%.1fm .. %.1fm]",
        terrain.getMinHeight(), terrain.getMaxHeight());

    // Ajuste de Altura Base Vertical (Y Offset - Fase 16)
    float baseOff = terrain.getBaseOffset();
    UI::BeginProps("TerrainOffsetProps");
    if (UI::PropSlider("Y Offset Base", &baseOff, -35.0f, 15.0f, "%.1f m", "Ajusta la altura base vertical del terreno")) {
        terrain.setBaseOffset(baseOff);
    }
    UI::EndProps();

    // Resolucion, Tamano del Mundo y Reconstruccion Dinamica (Fase 16 / 17)
    if (ImGui::CollapsingHeader("Resolucion, Tamano y Reconstruccion de Malla", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int resChoice = 1;
        static int customRes = 256;
        const char* resOptions[] = {
            "64 x 64 (Borrador Ultrarrápido)",
            "128 x 128 (Equilibrado - Por Defecto)",
            "256 x 256 (Detallado HD - 65k Vértices)",
            "512 x 512 (Ultra HD - 262k Vértices)",
            "1024 x 1024 (Cinemático 1K - 1.05M Vértices)",
            "2048 x 2048 (Extremo 2K - 4.19M Vértices)",
            "Personalizado..."
        };
        UI::BeginProps("TerrainMeshRebuildProps");
        UI::PropCombo("Resolución", &resChoice, resOptions, IM_ARRAYSIZE(resOptions));

        int targetRes = 128;
        if (resChoice == 0) targetRes = 64;
        else if (resChoice == 1) targetRes = 128;
        else if (resChoice == 2) targetRes = 256;
        else if (resChoice == 3) targetRes = 512;
        else if (resChoice == 4) targetRes = 1024;
        else if (resChoice == 5) targetRes = 2048;
        else if (resChoice == 6) {
            ImGui::InputInt("Vértices Eje", &customRes, 32, 128);
            customRes = std::clamp(customRes, 16, 4096);
            targetRes = customRes;
        }

        static float newWorldSize = 120.0f;
        UI::PropSlider("Tamaño Mundo", &newWorldSize, 40.0f, 1000.0f, "%.0f m");
        UI::EndProps();

        uint64_t vertCount = static_cast<uint64_t>(targetRes) * targetRes;
        uint64_t triCount = static_cast<uint64_t>(targetRes - 1) * (targetRes - 1) * 2;
        float vramMB = (vertCount * sizeof(TerrainVertex) + triCount * 3 * sizeof(uint32_t)) / (1024.0f * 1024.0f);

        ImGui::TextDisabled("Geometria: %llu v | %llu tris (~%.1f MB VRAM)", vertCount, triCount, vramMB);

        if (UI::ButtonWithIcon(UI::IconType::RESET, "Reconstruir Malla con Nueva Resolucion", ImVec2(-1, 24.0f))) {
            terrain.rebuild(targetRes, newWorldSize);
        }
    }

    // Sistema OASIS: Occlusion and Adaptive Streaming of Infinite Surfaces (Fase 18)
    auto oasis = terrain.getOASIS();
    if (oasis && ImGui::CollapsingHeader("Sistema OASIS (Geometria Virtual CDLOD)", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool oasisEn = oasis->isEnabled();
        if (ImGui::Checkbox("Habilitar OASIS (Streaming y Teselacion)", &oasisEn)) {
            oasis->setEnabled(oasisEn);
        }

        if (oasisEn) {
            bool fc = oasis->isFrustumCullingEnabled();
            if (ImGui::Checkbox("Frustum Culling", &fc)) oasis->setFrustumCulling(fc);
            ImGui::SameLine();
            bool gm = oasis->isGeomorphingEnabled();
            if (ImGui::Checkbox("Geomorphing", &gm)) oasis->setGeomorphing(gm);
            bool dbg = oasis->isDebugLODEnabled();
            if (ImGui::Checkbox("Debug LOD", &dbg)) oasis->setDebugLOD(dbg);

            UI::BeginProps("OASISParamsProps");
            float distFactor = oasis->getLodDistanceFactor();
            if (UI::PropSlider("Distancia LOD", &distFactor, 0.4f, 4.0f, "%.2fx")) {
                oasis->setLodDistanceFactor(distFactor);
            }
            int maxLod = oasis->getMaxLOD();
            if (UI::PropSliderInt("Subdivision LOD", &maxLod, 1, 7)) {
                oasis->setMaxLOD(maxLod);
            }
            int patchRes = oasis->getPatchResolution();
            static int patchChoice = 1;
            if (patchRes == 17) patchChoice = 0;
            else if (patchRes == 33) patchChoice = 1;
            else if (patchRes == 65) patchChoice = 2;

            const char* patchOptions[] = { "17 x 17 (Bajo Costo)", "33 x 33 (Equilibrado)", "65 x 65 (Alta Fidelidad)" };
            if (UI::PropCombo("Malla por Chunk", &patchChoice, patchOptions, IM_ARRAYSIZE(patchOptions))) {
                if (patchChoice == 0) oasis->setPatchResolution(17);
                else if (patchChoice == 1) oasis->setPatchResolution(33);
                else if (patchChoice == 2) oasis->setPatchResolution(65);
            }
            UI::EndProps();

            // Panel de Telemetría en Tiempo Real Compacto (2 líneas + barra de progreso)
            const auto& telem = oasis->getTelemetry();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.16f, 0.85f));
            ImGui::BeginChild("OASISTelemetryBox", ImVec2(0, 62), true);
            ImGui::Text("Chunks: %d / %d vis (%d culls)", telem.visibleChunksCount, telem.totalEvaluatedChunks, telem.culledChunksCount);
            ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "Triangulos: %llu", static_cast<unsigned long long>(telem.renderedTriangles));
            float frac = telem.triangleReductionPercent / 100.0f;
            char barBuf[64];
            snprintf(barBuf, sizeof(barBuf), "Ahorro GPU: %.1f%%", telem.triangleReductionPercent);
            ImGui::ProgressBar(frac, ImVec2(-1, 14), barBuf);
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }

    // ========================================================================
    // Sistema OASIS - Módulo 2: Modelos 3D y Mallas (Meshlets & Cone Culling)
    // ========================================================================
    if (ImGui::CollapsingHeader("Sistema OASIS - Modulo 2: Modelos 3D y Meshlets", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Habilitar OASIS Meshlets", &m_meshletConfig.enabled);

        if (m_meshletConfig.enabled) {
            ImGui::Checkbox("Frustum Culling", &m_meshletConfig.frustumCulling);
            ImGui::SameLine();
            ImGui::Checkbox("Backface Culling", &m_meshletConfig.backfaceCulling);
            ImGui::SameLine();
            ImGui::Checkbox("Debug Clusters", &m_meshletConfig.debugColors);

            // Panel de Telemetría de Meshlets en Tiempo Real Compacto
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.16f, 0.85f));
            ImGui::BeginChild("OASISMeshletTelemBox", ImVec2(0, 48), true);
            ImGui::Text("Clusters: %llu / %llu vis | Culls: %llu F / %llu BC",
                static_cast<unsigned long long>(m_meshletTelemetry.visibleMeshletsCount),
                static_cast<unsigned long long>(m_meshletTelemetry.totalMeshletsEvaluated),
                static_cast<unsigned long long>(m_meshletTelemetry.frustumCulledMeshlets),
                static_cast<unsigned long long>(m_meshletTelemetry.backfaceCulledMeshlets));
            float frac = m_meshletTelemetry.triangleSavingsPercent / 100.0f;
            char barBuf[64];
            snprintf(barBuf, sizeof(barBuf), "Ahorro GPU: %.1f%%", m_meshletTelemetry.triangleSavingsPercent);
            ImGui::ProgressBar(frac, ImVec2(-1, 14), barBuf);
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }

    ImGui::Spacing();

    // Selector Maestro de Modo de Herramienta: Escultura vs Pintura de Texturas (50% / 50%)
    ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "Modo Activo de Herramienta:");
    bool isSculpt = (terrain.activeToolMode == TerrainToolMode::SCULPT);
    bool isPaint = (terrain.activeToolMode == TerrainToolMode::PAINT_TEXTURE);

    float halfToolW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    if (isSculpt) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.85f, 1.0f));
    if (ImGui::Button("Escultura 3D", ImVec2(halfToolW, 26.0f))) {
        terrain.activeToolMode = TerrainToolMode::SCULPT;
    }
    if (isSculpt) ImGui::PopStyleColor();

    ImGui::SameLine();
    if (isPaint) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.40f, 0.90f, 1.0f));
    if (ImGui::Button("Pintar Texturas", ImVec2(halfToolW, 26.0f))) {
        terrain.activeToolMode = TerrainToolMode::PAINT_TEXTURE;
    }
    if (isPaint) ImGui::PopStyleColor();

    ImGui::Spacing();

    // Panel de Pintura de Texturas y Capas PBR Multicapa
    if (ImGui::CollapsingHeader("Capas de Material PBR y Pintura de Terreno", ImGuiTreeNodeFlags_DefaultOpen)) {
        UI::BeginProps("TerrainReflAnisoProps");
        float terrainRefl = terrain.getReflectionIntensity();
        if (UI::PropSlider("Reflejos Terreno", &terrainRefl, 0.0f, 3.0f, "%.2fx")) {
            terrain.setReflectionIntensity(terrainRefl);
            m_terrainReflectionIntensity = terrainRefl;
        }

        const char* anisoTerrainOptions[] = { "1x (Bilineal)", "2x AF", "4x AF", "8x AF", "16x AF" };
        int anisoTerrainIdx = (m_anisotropyLevel <= 1) ? 0 : (m_anisotropyLevel == 2) ? 1 : (m_anisotropyLevel == 4) ? 2 : (m_anisotropyLevel == 8) ? 3 : 4;
        if (UI::PropCombo("Filtrado AF", &anisoTerrainIdx, anisoTerrainOptions, IM_ARRAYSIZE(anisoTerrainOptions))) {
            m_anisotropyLevel = (anisoTerrainIdx == 0) ? 1 : (anisoTerrainIdx == 1) ? 2 : (anisoTerrainIdx == 2) ? 4 : (anisoTerrainIdx == 3) ? 8 : 16;
            terrain.setAnisotropyLevel(m_anisotropyLevel);
        }

        const char* reliefOptions[] = {
            "Teselacion OASIS",
            "Parallax (POM)",
            "Plano (Normal Map)"
        };
        int currentRelief = static_cast<int>(terrain.getReliefMode());
        if (UI::PropCombo("Modo Relieve", &currentRelief, reliefOptions, IM_ARRAYSIZE(reliefOptions))) {
            terrain.setReliefMode(static_cast<TerrainReliefMode>(currentRelief));
        }

        if (terrain.getReliefMode() == TerrainReliefMode::OASIS_TESSELLATION) {
            float tessScale = terrain.getTessellationDisplacementScale();
            if (UI::PropSlider("Escala Teselacion", &tessScale, 0.0f, 5.0f, "%.2fx")) {
                terrain.setTessellationDisplacementScale(tessScale);
            }
        }
        UI::EndProps();

        ImGui::Spacing();

        if (UI::ButtonWithIcon(UI::IconType::PLUS, "Añadir Nueva Capa PBR", ImVec2(-1, 24.0f))) {
            terrain.addLayer("Nueva Capa " + std::to_string(terrain.getLayerCount() + 1));
        }

        ImGui::Spacing();

        const std::string imgFilter = "Texturas 2D (*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.bmp;*.tga)\0*.png;*.jpg;*.jpeg;*.tif;*.tiff;*.bmp;*.tga\0Todos los archivos (*.*)\0*.*\0";

        for (int i = 0; i < terrain.getLayerCount(); ++i) {
            auto& layer = terrain.getLayer(i);
            ImGui::PushID(i);

            bool isCurrent = (terrain.activeToolMode == TerrainToolMode::PAINT_TEXTURE && static_cast<int>(terrain.activePaintLayer) == i);

            if (isCurrent) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.45f, 0.70f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.55f, 0.80f, 0.9f));
            }

            char nodeHeader[128];
            snprintf(nodeHeader, sizeof(nodeHeader), "%sCapa %d: %s", isCurrent ? "[PINTANDO] " : "", i + 1, layer.name.c_str());
            bool open = ImGui::TreeNodeEx(nodeHeader, ImGuiTreeNodeFlags_Framed | (i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0));

            if (isCurrent) {
                ImGui::PopStyleColor(2);
            }

            if (open) {
                char nameBuf[64];
                strncpy(nameBuf, layer.name.c_str(), sizeof(nameBuf) - 1);
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##LayerName", nameBuf, sizeof(nameBuf))) {
                    layer.name = nameBuf;
                }

                if (i > 0) {
                    if (ImGui::SmallButton("Subir")) terrain.moveLayerUp(i);
                    ImGui::SameLine();
                }
                if (i < terrain.getLayerCount() - 1) {
                    if (ImGui::SmallButton("Bajar")) terrain.moveLayerDown(i);
                    ImGui::SameLine();
                }
                if (terrain.getLayerCount() > 1) {
                    if (ImGui::SmallButton("Eliminar")) {
                        terrain.removeLayer(i);
                        ImGui::TreePop();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::SameLine();
                }

                if (isCurrent) {
                    ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "[Activa]");
                } else {
                    if (ImGui::SmallButton("Brocha")) {
                        terrain.activeToolMode = TerrainToolMode::PAINT_TEXTURE;
                        terrain.activePaintLayer = static_cast<TerrainPaintLayer>(i);
                    }
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Rellenar")) {
                    terrain.fillSplatmapWithLayer(i);
                }

                UI::BeginProps("LayerTexSlotsProps");
                UI::PropCheckbox("Visibilidad", &layer.visible);
                UI::PropSlider("Opacidad", &layer.opacity, 0.0f, 1.0f, "%.2f");

                UI::PropTextureSlot("Albedo", layer.albedoPath, layer.albedoTex != 0,
                    [&]() { std::string p = SceneIO::openFileDialog(imgFilter.c_str()); if (!p.empty()) terrain.loadLayerAlbedo(i, p); },
                    [&]() { terrain.clearLayerTexture(i, 0); });
                UI::PropTextureSlot("Normal", layer.normalPath, layer.normalTex != 0,
                    [&]() { std::string p = SceneIO::openFileDialog(imgFilter.c_str()); if (!p.empty()) terrain.loadLayerNormal(i, p); },
                    [&]() { terrain.clearLayerTexture(i, 1); });
                UI::PropTextureSlot("Rugosidad", layer.roughSpecPath, layer.roughSpecTex != 0,
                    [&]() { std::string p = SceneIO::openFileDialog(imgFilter.c_str()); if (!p.empty()) terrain.loadLayerRoughSpec(i, p); },
                    [&]() { terrain.clearLayerTexture(i, 2); });
                UI::PropTextureSlot("Altura", layer.heightPath, layer.heightTex != 0,
                    [&]() { std::string p = SceneIO::openFileDialog(imgFilter.c_str()); if (!p.empty()) terrain.loadLayerHeight(i, p); },
                    [&]() { terrain.clearLayerTexture(i, 3); });
                UI::PropTextureSlot("AO", layer.aoPath, layer.aoTex != 0,
                    [&]() { std::string p = SceneIO::openFileDialog(imgFilter.c_str()); if (!p.empty()) terrain.loadLayerAO(i, p); },
                    [&]() { terrain.clearLayerTexture(i, 4); });
                UI::EndProps();

                UI::BeginProps("LayerPBRValuesProps");
                UI::PropSlider("Repeticion UV", &layer.tiling, 1.0f, 100.0f, "%.1fx");
                UI::PropSlider("Intensidad Normal", &layer.normalStrength, 0.0f, 3.0f, "%.2fx");
                UI::PropSlider("Rugosidad", &layer.roughness, 0.04f, 1.0f, "%.2f");
                UI::PropSlider("Metalico", &layer.metallic, 0.0f, 1.0f, "%.2f");
                if (terrain.getReliefMode() == TerrainReliefMode::OASIS_TESSELLATION) {
                    UI::PropSlider("Profundidad 3D", &layer.heightScale, 0.0f, 0.25f, "%.3f m");
                } else if (terrain.getReliefMode() == TerrainReliefMode::POM) {
                    UI::PropSlider("Profundidad 3D", &layer.heightScale, 0.0f, 0.10f, "%.3f");
                } else {
                    UI::PropSlider("Profundidad 3D", &layer.heightScale, 0.0f, 0.10f, "%.3f");
                }
                UI::PropSlider("Intensidad AO", &layer.aoStrength, 0.0f, 1.0f, "%.2f");
                UI::PropColor3("Tinte Color", &layer.tint.r);
                if (layer.albedoTex == 0) {
                    UI::PropColor3("Color Respaldo", &layer.fallbackColor.r);
                }
                UI::EndProps();

                ImGui::TreePop();
            }

            ImGui::PopID();
            ImGui::Spacing();
        }

        // Parámetros del Pincel de Pintura
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.95f, 0.8f, 0.3f, 1.0f), "Parametros del Pincel de Pintura:");
        UI::BeginProps("TerrainPaintBrushProps");
        UI::PropSlider("Radio Pincel", &terrain.paintRadius, 1.0f, 50.0f, "%.1f m");
        UI::PropSlider("Fuerza Pintado", &terrain.paintStrength, 0.05f, 3.0f, "%.2fx");
        UI::EndProps();

        if (UI::ButtonWithIcon(UI::IconType::RESET, "Restaurar Bioma Procedural Base", ImVec2(-1, 22.0f))) {
            terrain.resetTexturePaint();
        }
    }
    ImGui::Spacing();

    // 2. Preajustes Procedurales (Presets)
    if (ImGui::CollapsingHeader("Preajustes de Terreno Procedural (Presets)", ImGuiTreeNodeFlags_DefaultOpen)) {
        float btnWidth = (ImGui::GetContentRegionAvail().x - 18.0f) * 0.25f;
        if (ImGui::Button("Montanas", ImVec2(btnWidth, 24.0f))) terrain.generatePreset(TerrainPreset::MOUNTAINS);
        ImGui::SameLine();
        if (ImGui::Button("Colinas", ImVec2(btnWidth, 24.0f))) terrain.generatePreset(TerrainPreset::ROLLING_HILLS);
        ImGui::SameLine();
        if (ImGui::Button("Canones", ImVec2(btnWidth, 24.0f))) terrain.generatePreset(TerrainPreset::CANYONS);
        ImGui::SameLine();
        if (ImGui::Button("Plano", ImVec2(btnWidth, 24.0f))) terrain.generatePreset(TerrainPreset::FLAT);

        ImGui::Spacing();
        if (ImGui::TreeNode("Parametros Personalizados de Ruido Fractal (fBm)")) {
            auto& s = terrain.getSettings();
            UI::BeginProps("TerrainNoiseProps");
            UI::PropSlider("Altura Maxima", &s.maxHeight, 4.0f, 60.0f, "%.1f m");
            UI::PropSlider("Frecuencia", &s.noiseFrequency, 0.005f, 0.080f, "%.4f");
            UI::PropSliderInt("Octavas Detalle", &s.noiseOctaves, 1, 8);
            UI::PropSlider("Persistencia", &s.noisePersistence, 0.20f, 0.80f, "%.2f");
            UI::EndProps();

            ImGui::InputInt("Semilla", &s.seed);
            ImGui::SameLine();
            if (ImGui::Button("Aleatorio")) s.seed = std::rand() % 99999 + 1;

            if (UI::ButtonWithIcon(UI::IconType::RESET, "Generar Terreno con Parametros", ImVec2(-1, 24.0f))) {
                terrain.generateProcedural(s);
            }
            ImGui::TreePop();
        }
    }

    ImGui::Spacing();

    // 3. Herramientas de Escultura 3D (Sculpting Brushes)
    if (ImGui::CollapsingHeader("Herramientas de Escultura 3D (Sculpting Brushes)", ImGuiTreeNodeFlags_DefaultOpen)) {
        float pWidth = (ImGui::GetContentRegionAvail().x - 20.0f) * 0.20f;
        auto drawBrushBtn = [&](SculptBrushType type, const char* label, const ImVec4& color) {
            bool isSel = (terrain.activeBrushType == type);
            if (isSel) {
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.9f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
            }
            if (ImGui::Button(label, ImVec2(pWidth, 24.0f))) {
                terrain.activeBrushType = type;
            }
            if (isSel) {
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
            }
        };

        drawBrushBtn(SculptBrushType::RAISE, "Subir", ImVec4(0.20f, 0.65f, 0.35f, 1.0f));
        ImGui::SameLine();
        drawBrushBtn(SculptBrushType::LOWER, "Bajar", ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
        ImGui::SameLine();
        drawBrushBtn(SculptBrushType::SMOOTH, "Suave", ImVec4(0.20f, 0.50f, 0.85f, 1.0f));
        ImGui::SameLine();
        drawBrushBtn(SculptBrushType::FLATTEN, "Aplanar", ImVec4(0.70f, 0.55f, 0.15f, 1.0f));
        ImGui::SameLine();
        drawBrushBtn(SculptBrushType::NOISE, "Rugoso", ImVec4(0.55f, 0.35f, 0.75f, 1.0f));

        UI::BeginProps("SculptBrushParamsProps");
        UI::PropSlider("Radio Pincel", &terrain.brushRadius, 1.0f, 35.0f, "%.1f m");
        UI::PropSlider("Fuerza Pincel", &terrain.brushStrength, 0.10f, 4.0f, "%.2fx");

        const char* falloffNames[] = { "Suave (Coseno)", "Esferico", "Lineal", "Plano (Flat)" };
        int falloffIdx = static_cast<int>(terrain.activeBrushFalloff);
        if (UI::PropCombo("Atenuacion", &falloffIdx, falloffNames, IM_ARRAYSIZE(falloffNames))) {
            terrain.activeBrushFalloff = static_cast<SculptFalloff>(falloffIdx);
        }

        if (terrain.activeBrushType == SculptBrushType::FLATTEN) {
            UI::PropSlider("Altura Objetivo", &terrain.brushTargetHeight, -5.0f, 40.0f, "%.2f m");
        }
        UI::EndProps();

        if (terrain.activeBrushType == SculptBrushType::FLATTEN) {
            if (UI::ButtonWithIcon(UI::IconType::CAMERA_TARGET, "Tomar Altura Frente a Camara", ImVec2(-1, 20.0f))) {
                glm::vec3 cPos = camera.getPosition() + camera.getFront() * 5.0f;
                terrain.brushTargetHeight = terrain.getHeightAtWorld(cPos.x, cPos.z);
            }
        }

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.16f, 0.20f, 0.70f));
        ImGui::BeginChild("SculptTips", ImVec2(0, 48), true);
        ImGui::TextDisabled("LMB: Esculpe | Ctrl+LMB: Bajar | Shift+LMB: Suavizar");
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    // 4. Física 3D Bullet del Terreno
    if (ImGui::CollapsingHeader("Fisica 3D Bullet del Terreno", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool physEnabled = terrain.isPhysicsEnabled();
        if (ImGui::Checkbox("Colision Fisica de Terreno", &physEnabled)) {
            terrain.setPhysicsEnabled(physEnabled);
        }

        bool groundPhys = physicsWorld.isGroundPlaneEnabled();
        if (ImGui::Checkbox("Habilitar Suelo Plano Fisico en Y = 0", &groundPhys)) {
            physicsWorld.enableGroundPlane(groundPhys);
        }

        if (UI::ButtonWithIcon(UI::IconType::RESET, "Sincronizar Malla Fisica con Terreno", ImVec2(-1, 22.0f))) {
            terrain.syncPhysics();
        }
    }

    ImGui::Spacing();

    // -------------------------------------------------------------
    // 7. Océano y Cuerpos de Agua 3D (Fase 19)
    // -------------------------------------------------------------
    if (water && ImGui::CollapsingHeader("Oceano y Cuerpos de Agua 3D (Fase 19)", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool wEnabled = water->isEnabled();
        if (ImGui::Checkbox("Habilitar Oceano / Superficie de Agua 3D", &wEnabled)) {
            water->setEnabled(wEnabled);
        }

        if (wEnabled) {
            float wLevel = water->getWaterLevel();
            UI::BeginProps("OceanLevelProps");
            if (UI::PropSlider("Nivel del Mar", &wLevel, -25.0f, 25.0f, "%.2f m")) {
                water->setWaterLevel(wLevel);
            }
            UI::EndProps();

            ImGui::TextDisabled("Preajustes Rapidos:");
            float availW = ImGui::GetContentRegionAvail().x;
            float btnW = (availW - 18.0f) / 4.0f;
            if (btnW < 50.0f) btnW = 50.0f;

            if (ImGui::Button("Lago", ImVec2(btnW, 22.0f))) water->applyPreset(WaterPreset::CALM_LAKE);
            ImGui::SameLine();
            if (ImGui::Button("Caribe", ImVec2(btnW, 22.0f))) water->applyPreset(WaterPreset::TROPICAL_CARIBBEAN);
            ImGui::SameLine();
            if (ImGui::Button("Oceano", ImVec2(btnW, 22.0f))) water->applyPreset(WaterPreset::OPEN_OCEAN);
            ImGui::SameLine();
            if (ImGui::Button("Tormenta", ImVec2(btnW, 22.0f))) water->applyPreset(WaterPreset::STORMY_SEA);

            ImGui::Spacing();
            ImGui::TextDisabled("Resolucion de Malla:");
            int curRes = water->getResolution();
            bool r64 = (curRes == 64);
            bool r128 = (curRes == 128);
            bool r256 = (curRes == 256);
            bool r512 = (curRes == 512);

            if (ImGui::RadioButton("64", r64)) water->setGridResolution(64);
            ImGui::SameLine();
            if (ImGui::RadioButton("128", r128)) water->setGridResolution(128);
            ImGui::SameLine();
            if (ImGui::RadioButton("256 HD", r256)) water->setGridResolution(256);
            ImGui::SameLine();
            if (ImGui::RadioButton("512 Ultra", r512)) water->setGridResolution(512);

            UI::BeginProps("OceanWaveProps");
            float wAmp = water->getWaveAmplitude();
            if (UI::PropSlider("Amplitud Olas", &wAmp, 0.0f, 3.0f, "%.2fx")) water->setWaveAmplitude(wAmp);
            float wSpeed = water->getWaveSpeed();
            if (UI::PropSlider("Velocidad Oleaje", &wSpeed, 0.0f, 4.0f, "%.2fx")) water->setWaveSpeed(wSpeed);

            glm::vec3 shallowCol = water->getShallowColor();
            if (UI::PropColor3("Color Costa", glm::value_ptr(shallowCol))) water->setShallowColor(shallowCol);
            glm::vec3 deepCol = water->getDeepColor();
            if (UI::PropColor3("Color Abisal", glm::value_ptr(deepCol))) water->setDeepColor(deepCol);

            float clarity = water->getClarity();
            if (UI::PropSlider("Claridad Agua", &clarity, 0.01f, 1.0f, "%.3f")) water->setClarity(clarity);
            float refract = water->getRefractionStrength();
            if (UI::PropSlider("Refraccion", &refract, 0.0f, 0.15f, "%.3f")) water->setRefractionStrength(refract);
            float rough = water->getRoughness();
            if (UI::PropSlider("Rugosidad", &rough, 0.01f, 0.50f, "%.2f")) water->setRoughness(rough);
            float spec = water->getSpecular();
            if (UI::PropSlider("Brillo Solar", &spec, 0.0f, 3.0f, "%.2fx")) water->setSpecular(spec);

            float foamThresh = water->getFoamThreshold();
            if (UI::PropSlider("Rango Espuma", &foamThresh, 0.1f, 6.0f, "%.2f m")) water->setFoamThreshold(foamThresh);
            float foamIntens = water->getFoamIntensity();
            if (UI::PropSlider("Int. Espuma", &foamIntens, 0.0f, 4.0f, "%.2fx")) water->setFoamIntensity(foamIntens);
            glm::vec3 foamCol = water->getFoamColor();
            if (UI::PropColor3("Color Espuma", glm::value_ptr(foamCol))) water->setFoamColor(foamCol);
            UI::EndProps();

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "OASIS Oceano (Streaming CDLOD):");
            bool oasisWater = water->isOASISEnabled();
            if (ImGui::Checkbox("Habilitar OASIS Oceano", &oasisWater)) water->setOASISEnabled(oasisWater);
            if (oasisWater) {
                bool frustumWater = water->isFrustumCullingEnabled();
                if (ImGui::Checkbox("Frustum Culling Oceano", &frustumWater)) water->setFrustumCullingEnabled(frustumWater);

                UI::BeginProps("OceanOASISProps");
                float lodDist = water->getOASISLodFactor();
                if (UI::PropSlider("Distancia LOD", &lodDist, 0.1f, 2.0f, "%.2fx")) water->setOASISLodFactor(lodDist);
                UI::EndProps();

                const auto& tel = water->getOASISTelemetry();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.16f, 0.85f));
                ImGui::BeginChild("OASISOceanTelemBox", ImVec2(0, 48), true);
                ImGui::Text("Chunks: %d / %d vis (%d culls) | Tris: %zu",
                    tel.visibleChunksCount, tel.totalEvaluatedChunks, tel.culledChunksCount, tel.renderedTriangles);
                ImGui::ProgressBar(tel.triangleReductionPercent / 100.0f, ImVec2(-1, 14),
                    ("Ahorro GPU: " + std::to_string(static_cast<int>(tel.triangleReductionPercent)) + "%").c_str());
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
        }
    }
}

void EditorUI::renderCharacterProperties(CharacterController& character, Camera& camera, PhysicsWorld& physicsWorld, Scene& scene) {
    ImGui::TextColored(ImVec4(0.20f, 0.75f, 0.95f, 1.0f), "Controlador de Personaje 3D y Play-in-Editor");
    ImGui::Separator();

    // 1. Boton Maestro de Play / Stop
    if (!isPlaying()) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.65f, 0.32f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.80f, 0.40f, 1.0f));
        if (UI::ButtonWithIcon(UI::IconType::PLAY, "INICIAR MODO JUEGO (Play-in-Editor) [Alt+P]", ImVec2(-1, 32.0f))) {
            triggerPlayStart();
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.22f, 0.22f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.32f, 0.32f, 1.0f));
        if (UI::ButtonWithIcon(UI::IconType::STOP, "DETENER MODO JUEGO [Esc]", ImVec2(-1, 32.0f))) {
            triggerPlayStop();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();

    // 2. Modo de Camara (1ra vs 3ra Persona)
    bool isFirstPerson = (character.getCameraMode() == CharacterCameraMode::FIRST_PERSON);
    const char* camLabel = isFirstPerson ? "Camara: Primera Persona (FPS)" : "Camara: Tercera Persona (TPS)";
    if (UI::ButtonWithIcon(isFirstPerson ? UI::IconType::EYE_OPEN : UI::IconType::CAMERA_TARGET, camLabel, ImVec2(-1, 26.0f), "Alternar camara [V]")) {
        character.toggleCameraMode();
    }

    bool bobEnabled = character.isHeadBobEnabled();
    UI::BeginProps("CharBobbingProps");
    if (UI::PropCheckbox("Bamboleo (Head Bob)", &bobEnabled)) {
        character.setHeadBobEnabled(bobEnabled);
    }
    UI::EndProps();
    ImGui::TextDisabled("Altura Dinamica de Vista: %.2f m (Agachado: C / Ctrl)", character.getCurrentEyeHeight());

    ImGui::Spacing();

    // 3. Parametros de Locomocion y Fisicas (2-Column Split)
    if (ImGui::CollapsingHeader("Locomocion y Parametros Fisicos", ImGuiTreeNodeFlags_DefaultOpen)) {
        float walk = character.getWalkSpeed();
        float sprint = character.getSprintSpeed();
        float crouch = character.getCrouchSpeed();
        float jump = character.getJumpForce();
        float sens = character.getMouseSensitivity();

        UI::BeginProps("CharMovementProps");
        if (UI::PropSlider("Marcha (WASD)", &walk, 1.0f, 15.0f, "%.1f m/s")) character.setWalkSpeed(walk);
        if (UI::PropSlider("Sprint (Shift)", &sprint, 2.0f, 25.0f, "%.1f m/s")) character.setSprintSpeed(sprint);
        if (UI::PropSlider("Agachado (C)", &crouch, 0.5f, 6.0f, "%.1f m/s")) character.setCrouchSpeed(crouch);
        if (UI::PropSlider("Salto (Espacio)", &jump, 2.0f, 20.0f, "%.1f N")) character.setJumpForce(jump);
        if (UI::PropSlider("Sensibilidad", &sens, 0.02f, 0.50f, "%.3f")) character.setMouseSensitivity(sens);
        UI::EndProps();

        ImGui::Spacing();
        ImGui::TextDisabled("Dimensiones de la Capsula:");
        ImGui::BulletText("Masa: %.1f kg | Radio: %.2f m | Altura: %.2f m", character.getMass(), character.getCapsuleRadius(), character.getCapsuleHeight());
    }

    ImGui::Spacing();

    // 4. Parametros de Camara en Tercera Persona (Spring Arm)
    if (ImGui::CollapsingHeader("Camara Tercera Persona (Spring Arm)", ImGuiTreeNodeFlags_DefaultOpen)) {
        float dist = character.getThirdPersonDistance();
        float offset = character.getShoulderOffset();

        UI::BeginProps("CharSpringArmProps");
        if (UI::PropSlider("Distancia Brazo", &dist, 1.0f, 12.0f, "%.1f m")) character.setThirdPersonDistance(dist);
        if (UI::PropSlider("Offset Hombro", &offset, -1.0f, 1.0f, "%.2f m")) character.setShoulderOffset(offset);
        UI::EndProps();

        ImGui::TextDisabled("Raycast de colision integrado: el brazo se acorta si choca.");
    }

    ImGui::Spacing();

    // 5. Telemetria en Tiempo Real (con Badges)
    if (ImGui::CollapsingHeader("Telemetria en Tiempo Real", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool spawned = character.isSpawned();
        if (!spawned) {
            UI::DrawStatusBadge("Estado: En espera en editor", IM_COL32(180, 180, 180, 255));
        } else if (isPlaying()) {
            UI::DrawStatusBadge("Estado: Jugando en directo", IM_COL32(75, 210, 110, 255));
        } else {
            UI::DrawStatusBadge("Estado: En pausa", IM_COL32(230, 190, 60, 255));
        }

        glm::vec3 pos = character.getPosition();
        ImGui::Text("Posicion 3D: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("Orientacion: Yaw %.1f deg | Pitch %.1f deg", character.getYaw(), character.getPitch());

        if (character.isGrounded()) {
            UI::DrawStatusBadge("Suelo: Pisando firme", IM_COL32(75, 210, 110, 255));
        } else {
            UI::DrawStatusBadge("Suelo: En el aire", IM_COL32(230, 80, 80, 255));
        }

        ImGui::Text("Corriendo: %s | Agachado: %s", character.isSprinting() ? "SI" : "NO", character.isCrouching() ? "SI" : "NO");

        glm::vec3 vel = character.getVelocity();
        float speedKmh = glm::length(glm::vec2(vel.x, vel.z)) * 3.6f;
        ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "Velocidad Horizontal: %.1f km/h", speedKmh);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 6. Guia de Atajos
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Guia de Atajos en Modo Juego:");
    ImGui::BulletText("W, A, S, D: Desplazamiento omnidireccional");
    ImGui::BulletText("Shift Izq: Correr / Sprint a alta velocidad");
    ImGui::BulletText("Ctrl Izq o C: Agacharse / Deslizarse");
    ImGui::BulletText("Espacio: Salto con impulso fisico Bullet");
    ImGui::BulletText("Tecla V: Conmutar entre 1ra y 3ra persona");
    ImGui::BulletText("Tecla Esc: Salir del juego y devolver el cursor");
}

void EditorUI::renderAudioProperties() {
    auto& audio = AudioEngine::get();

    ImGui::TextColored(ImVec4(0.78f, 0.45f, 0.98f, 1.0f), "Motor de Audio 3D Espacial y SFX Procedural (Fase 16)");
    ImGui::Separator();

    // 1. Controles Maestros de Volumen (2-Column Split)
    if (ImGui::CollapsingHeader("Controles Generales de Volumen", ImGuiTreeNodeFlags_DefaultOpen)) {
        float masterVol = audio.getMasterVolume();
        float sfxVol = audio.getSFXVolume();
        float footVol = audio.getFootstepsVolume();
        float musVol = audio.getMusicVolume();
        bool muted = audio.isMuted();
        bool amb = audio.isAmbientMusicEnabled();

        UI::BeginProps("AudioPropsTable");
        if (UI::PropSlider("Master", &masterVol, 0.0f, 1.0f, "%.2fx")) audio.setMasterVolume(masterVol);
        if (UI::PropSlider("Efectos SFX", &sfxVol, 0.0f, 1.0f, "%.2fx")) audio.setSFXVolume(sfxVol);
        if (UI::PropSlider("Pisadas", &footVol, 0.0f, 1.0f, "%.2fx")) audio.setFootstepsVolume(footVol);
        if (UI::PropSlider("Musica", &musVol, 0.0f, 1.0f, "%.2fx")) audio.setMusicVolume(musVol);
        if (UI::PropCheckbox("Silenciar Todo", &muted)) audio.setMuted(muted);
        if (UI::PropCheckbox("Tono Ambiental", &amb)) audio.toggleAmbientMusic(amb);
        UI::EndProps();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Pruebas de Efectos de Sonido (SFX Triggers) con Iconos Vectoriales
    if (ImGui::CollapsingHeader("Pruebas de SFX y Locomocion Procedural", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Prueba las formas de onda sintetizadas en tiempo real:");

        float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        if (UI::ButtonWithIcon(UI::IconType::SPEAKER, "Pisada Cesped", ImVec2(halfW, 26.0f))) {
            audio.playFootstep(FootstepSurface::GRASS);
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::SPEAKER, "Pisada Roca", ImVec2(halfW, 26.0f))) {
            audio.playFootstep(FootstepSurface::ROCK);
        }

        if (UI::ButtonWithIcon(UI::IconType::SPEAKER, "Pisada Nieve", ImVec2(halfW, 26.0f))) {
            audio.playFootstep(FootstepSurface::SNOW);
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::SPEAKER, "Pisada Suelo", ImVec2(halfW, 26.0f))) {
            audio.playFootstep(FootstepSurface::CONCRETE);
        }

        ImGui::Spacing();
        if (UI::ButtonWithIcon(UI::IconType::SPEAKER, "Salto", ImVec2(halfW, 26.0f))) {
            audio.playJump();
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::SPEAKER, "Aterrizaje", ImVec2(halfW, 26.0f))) {
            audio.playLand(FootstepSurface::GRASS);
        }

        if (UI::ButtonWithIcon(UI::IconType::SPEAKER, "Impacto Fisico 3D", ImVec2(-1, 26.0f))) {
            audio.playImpact(glm::vec3(0.0f), 2.5f);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Informacion de Diagnostico del Motor de Audio
    ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "Diagnostico de Audio:");
    ImGui::BulletText("Backend: Miniaudio 3D Espacial Multiplataforma");
    ImGui::BulletText("Frecuencia de Muestreo: 44100 Hz (16-bit PCM Mono/Estereo)");
    ImGui::BulletText("Canales: Estereo con Listener 3D HRTF Orientado");
    ImGui::BulletText("Sintesis Autonoma: 100%% Activa sin dependencias externas");
}

void EditorUI::renderParticleProperties(ParticleSystem& particleSystem, Camera& camera) {
    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.15f, 1.0f), "Sistema de Particulas 3D (VFX)");
    ImGui::Separator();

    auto& emitters = particleSystem.getEmitters();

    // Controles superiores en 3 columnas iguales
    float availW = ImGui::GetContentRegionAvail().x;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float pBtnW = (availW - 2.0f * spacing) / 3.0f;
    if (UI::ButtonWithIcon(UI::IconType::PLUS, "Añadir", ImVec2(pBtnW, 22.0f))) {
        ImGui::OpenPopup("AddEmitterPopup");
    }
    ImGui::SameLine();
    if (UI::ButtonWithIcon(UI::IconType::RESET, "Reiniciar", ImVec2(pBtnW, 22.0f))) {
        for (auto& em : emitters) em->reset();
    }
    ImGui::SameLine();
    if (UI::ButtonWithIcon(UI::IconType::TRASH, "Limpiar", ImVec2(pBtnW, 22.0f))) {
        particleSystem.clear();
        m_selectedEmitterIndex = -1;
    }

    if (ImGui::BeginPopup("AddEmitterPopup")) {
        glm::vec3 spawnPos = camera.getPosition() + camera.getFront() * 4.0f;
        spawnPos.y = (std::max)(0.5f, spawnPos.y);

        if (ImGui::MenuItem("Fuego (Fire)")) {
            particleSystem.addEmitter("Fuego_" + std::to_string(emitters.size() + 1), EmitterType::FIRE, spawnPos);
            m_selectedEmitterIndex = static_cast<int>(emitters.size() - 1);
        }
        if (ImGui::MenuItem("Chispas (Sparks)")) {
            particleSystem.addEmitter("Chispas_" + std::to_string(emitters.size() + 1), EmitterType::SPARKS, spawnPos);
            m_selectedEmitterIndex = static_cast<int>(emitters.size() - 1);
        }
        if (ImGui::MenuItem("Humo (Smoke)")) {
            particleSystem.addEmitter("Humo_" + std::to_string(emitters.size() + 1), EmitterType::SMOKE, spawnPos);
            m_selectedEmitterIndex = static_cast<int>(emitters.size() - 1);
        }
        if (ImGui::MenuItem("Lluvia (Rain)")) {
            particleSystem.addEmitter("Lluvia_" + std::to_string(emitters.size() + 1), EmitterType::RAIN, spawnPos + glm::vec3(0, 5, 0));
            m_selectedEmitterIndex = static_cast<int>(emitters.size() - 1);
        }
        if (ImGui::MenuItem("Magia (Magic)")) {
            particleSystem.addEmitter("Magia_" + std::to_string(emitters.size() + 1), EmitterType::MAGIC, spawnPos);
            m_selectedEmitterIndex = static_cast<int>(emitters.size() - 1);
        }
        if (ImGui::MenuItem("Emisor Personalizado")) {
            particleSystem.addEmitter("Emisor_" + std::to_string(emitters.size() + 1), EmitterType::CUSTOM, spawnPos);
            m_selectedEmitterIndex = static_cast<int>(emitters.size() - 1);
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Emisores Activos (%zu):", emitters.size());

    if (emitters.empty()) {
        ImGui::TextDisabled("No hay emisores de particulas activos en la escena.");
        return;
    }

    if (m_selectedEmitterIndex < 0 || m_selectedEmitterIndex >= static_cast<int>(emitters.size())) {
        m_selectedEmitterIndex = 0;
    }

    // Lista desplegable de emisores compacta de 65px
    if (ImGui::BeginListBox("##EmittersList", ImVec2(-1, 65.0f))) {
        for (size_t i = 0; i < emitters.size(); ++i) {
            auto em = emitters[i];
            bool isSelected = (m_selectedEmitterIndex == static_cast<int>(i));
            std::string label = (em->isActive() ? "* " : "o ") + em->getName() + " (" + std::to_string(em->getAliveCount()) + " p)";
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                m_selectedEmitterIndex = static_cast<int>(i);
            }
        }
        ImGui::EndListBox();
    }

    auto currentEmitter = emitters[m_selectedEmitterIndex];
    if (!currentEmitter) return;

    ImGui::Spacing();
    ImGui::Separator();

    bool active = currentEmitter->isActive();
    if (ImGui::Checkbox("Activo", &active)) {
        currentEmitter->setActive(active);
    }
    ImGui::SameLine();
    if (UI::ButtonWithIcon(UI::IconType::TRASH, "Eliminar Emitter", ImVec2(0, 20.0f), nullptr, IM_COL32(230, 80, 80, 255))) {
        particleSystem.removeEmitter(static_cast<size_t>(m_selectedEmitterIndex));
        m_selectedEmitterIndex = (std::max)(0, m_selectedEmitterIndex - 1);
        return;
    }

    char nameBuf[128];
    strncpy(nameBuf, currentEmitter->getName().c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##EmitterName", nameBuf, sizeof(nameBuf))) {
        currentEmitter->setName(nameBuf);
    }

    glm::vec3 pos = currentEmitter->getPosition();
    UI::BeginProps("EmitterPosTable");
    if (UI::PropVec3Vertical("Posicion", pos, 0.1f)) {
        currentEmitter->setPosition(pos);
    }
    const char* presetNames[] = { "Fuego", "Chispas", "Humo", "Lluvia", "Magia", "Personalizado" };
    int currentType = static_cast<int>(currentEmitter->getType());
    if (UI::PropCombo("Preajuste VFX", &currentType, presetNames, IM_ARRAYSIZE(presetNames))) {
        currentEmitter->setType(static_cast<EmitterType>(currentType));
    }
    UI::EndProps();

    if (UI::ButtonWithIcon(UI::IconType::CAMERA_TARGET, "Colocar Frente a Camara", ImVec2(-1, 20.0f))) {
        glm::vec3 cPos = camera.getPosition() + camera.getFront() * 4.0f;
        cPos.y = (std::max)(0.5f, cPos.y);
        currentEmitter->setPosition(cPos);
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Emision y Dinamica", ImGuiTreeNodeFlags_DefaultOpen)) {
        float rate = currentEmitter->getEmissionRate();
        int maxP = currentEmitter->getMaxParticles();
        float life = currentEmitter->getLifetime();
        glm::vec3 vel = currentEmitter->getVelocity();
        glm::vec3 spread = currentEmitter->getVelocitySpread();
        glm::vec3 grav = currentEmitter->getGravity();

        UI::BeginProps("EmitterDynamicsProps");
        if (UI::PropSlider("Tasa Emision", &rate, 1.0f, 1000.0f, "%.0f p/s")) currentEmitter->setEmissionRate(rate);
        if (UI::PropSliderInt("Particulas Max", &maxP, 10, 5000)) currentEmitter->setMaxParticles(maxP);
        if (UI::PropSlider("Tiempo Vida", &life, 0.1f, 10.0f, "%.2f s")) currentEmitter->setLifetime(life);
        if (UI::PropVec3Vertical("Velocidad", vel, 0.1f, -50.0f, 50.0f)) currentEmitter->setVelocity(vel);
        if (UI::PropVec3Vertical("Dispersion", spread, 0.1f, 0.0f, 30.0f)) currentEmitter->setVelocitySpread(spread);
        if (UI::PropVec3Vertical("Gravedad", grav, 0.1f, -50.0f, 50.0f)) currentEmitter->setGravity(grav);
        UI::EndProps();
    }

    if (ImGui::CollapsingHeader("Apariencia y Mezcla", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec4 startCol = currentEmitter->getStartColor();
        glm::vec4 endCol = currentEmitter->getEndColor();
        float startSz = currentEmitter->getStartSize();
        float endSz = currentEmitter->getEndSize();
        bool additive = currentEmitter->isAdditive();

        UI::BeginProps("EmitterAppearanceProps");
        if (UI::PropColor4("Color Inicial", glm::value_ptr(startCol))) currentEmitter->setStartColor(startCol);
        if (UI::PropColor4("Color Final", glm::value_ptr(endCol))) currentEmitter->setEndColor(endCol);
        if (UI::PropSlider("Tamano Inicial", &startSz, 0.01f, 3.0f, "%.2f")) currentEmitter->setStartSize(startSz);
        if (UI::PropSlider("Tamano Final", &endSz, 0.01f, 3.0f, "%.2f")) currentEmitter->setEndSize(endSz);
        if (UI::PropCheckbox("Mezcla Aditiva", &additive, "Resplandor aditivo (Fuego/Magia) vs Alfa suave")) currentEmitter->setAdditive(additive);
        UI::EndProps();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("GPU: %zu / %d particulas vivas", currentEmitter->getAliveCount(), currentEmitter->getMaxParticles());
}

void EditorUI::renderAnimationProperties(Scene& scene) {
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "Animaciones Esqueleticas y Huesos");
    ImGui::Separator();

    if (m_selectedEntityName.empty()) {
        ImGui::TextDisabled("Ninguna entidad seleccionada en la escena.");
        ImGui::TextDisabled("Haz clic en un modelo 3D (FBX, GLTF o DAE) en el Viewport o la Jerarquia para inspeccionar sus huesos.");
        return;
    }

    auto ent = scene.getEntity(m_selectedEntityName);
    if (!ent) {
        ImGui::TextDisabled("Entidad no encontrada.");
        return;
    }

    auto model = ent->getModel();
    if (!model) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Entidad: %s (Primitiva Procedural)", ent->getName().c_str());
        ImGui::TextDisabled("Esta entidad es una primitiva basica sin esqueleto.");
        return;
    }

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Modelo: %s", ent->getName().c_str());
    ImGui::TextDisabled("Ruta: %s", model->getPath().c_str());
    ImGui::Separator();

    Animator* animator = model->getAnimator();
    if (!animator || !animator->hasActiveAnimation()) {
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "Sin animaciones esqueleticas detectadas.");
        ImGui::TextWrapped("El archivo del modelo no contiene tracks de animacion de huesos (aiAnimation).");
        return;
    }

    auto anim = animator->getCurrentAnimation();
    if (!anim) return;

    ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "Clip Activo: %s", anim->getName().empty() ? "Default Animation" : anim->getName().c_str());
    ImGui::Text("Huesos en Armadura: %d", anim->getBoneCount());
    ImGui::Text("Duracion: %.2f ticks (%.2f s a %.1f fps)", anim->getDuration(), anim->getDuration() / (anim->getTicksPerSecond() > 0 ? anim->getTicksPerSecond() : 25.0f), anim->getTicksPerSecond());

    ImGui::Spacing();
    ImGui::Separator();

    // Controles de Reproduccion con Iconos
    bool isPlaying = animator->isPlaying();
    if (isPlaying) {
        if (UI::ButtonWithIcon(UI::IconType::PAUSE, "Pausar", ImVec2(90, 26))) {
            animator->setPlaying(false);
        }
    } else {
        if (UI::ButtonWithIcon(UI::IconType::PLAY, "Reproducir", ImVec2(90, 26))) {
            animator->setPlaying(true);
        }
    }
    ImGui::SameLine();
    if (UI::ButtonWithIcon(UI::IconType::STOP, "Detener", ImVec2(100, 26))) {
        animator->setPlaying(false);
        animator->setTime(0.0f);
    }
    ImGui::SameLine();
    bool loop = animator->isLoop();
    if (ImGui::Checkbox("Loop", &loop)) {
        animator->setLoop(loop);
    }

    // Scrubber y velocidad en 2 Columnas
    float curTime = animator->getCurrentTime();
    float maxTime = animator->getDuration();
    float speed = animator->getPlaybackSpeed();

    UI::BeginProps("AnimControlsProps");
    if (UI::PropSlider("Linea Tiempo", &curTime, 0.0f, maxTime, "%.1f ticks")) {
        animator->setTime(curTime);
    }
    if (UI::PropSlider("Velocidad", &speed, 0.1f, 3.0f, "%.2fx")) {
        animator->setPlaybackSpeed(speed);
    }
    UI::EndProps();

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Jerarquia de Huesos (Armature)", ImGuiTreeNodeFlags_None)) {
        const auto& boneMap = anim->getBoneInfoMap();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Total de huesos vinculados: %zu", boneMap.size());
        if (ImGui::BeginTable("BoneTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 160))) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Nombre del Hueso");
            ImGui::TableHeadersRow();

            for (const auto& pair : boneMap) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", pair.second.id);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(pair.first.c_str());
            }
            ImGui::EndTable();
        }
    }
}

void EditorUI::renderPhysicsProperties(Scene& scene, PhysicsWorld& physicsWorld) {
    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Fisica 3D en Tiempo Real (Bullet Physics)");
    ImGui::Separator();

    // 1. Simulacion Global en 2 Columnas
    bool simEnabled = physicsWorld.isSimulationEnabled();
    glm::vec3 gravity = physicsWorld.getGravity();

    UI::BeginProps("PhysicsWorldProps");
    UI::PropCheckbox("Simulacion Activa", &simEnabled);
    if (UI::PropVec3Vertical("Gravedad", gravity, 0.1f)) {
        physicsWorld.setGravity(gravity);
    }
    UI::EndProps();

    ImGui::TextDisabled("Ajustes Rapidos de Gravedad:");
    if (ImGui::SmallButton("Tierra (-9.81)")) physicsWorld.setGravity(glm::vec3(0, -9.81f, 0));
    ImGui::SameLine();
    if (ImGui::SmallButton("Luna (-1.62)")) physicsWorld.setGravity(glm::vec3(0, -1.62f, 0));
    ImGui::SameLine();
    if (ImGui::SmallButton("Jupiter (-24.79)")) physicsWorld.setGravity(glm::vec3(0, -24.79f, 0));
    ImGui::SameLine();
    if (ImGui::SmallButton("Cero (Espacio)")) physicsWorld.setGravity(glm::vec3(0, 0, 0));

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Fisica de la Entidad Seleccionada en 2 Columnas
    if (m_selectedEntityName.empty()) {
        ImGui::TextDisabled("Selecciona un objeto en la escena 3D\npara configurarle masa, colisiones y rebote.");
    } else {
        auto entity = scene.getEntity(m_selectedEntityName);
        if (!entity) return;

        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Entidad: %s", entity->getName().c_str());

        bool hasRB = physicsWorld.hasRigidBody(entity->getName());
        bool enableRB = hasRB;

        UI::BeginProps("EntityPhysicsProps");
        if (UI::PropCheckbox("Cuerpo Rigido", &enableRB)) {
            if (enableRB && !hasRB) {
                physicsWorld.addRigidBody(entity, 1.0f, ColliderType::BOX);
            } else if (!enableRB && hasRB) {
                physicsWorld.removeRigidBody(entity->getName());
            }
        }

        if (enableRB) {
            auto rbComp = physicsWorld.getRigidBodyComponent(entity->getName());
            if (rbComp) {
                float mass = rbComp->mass;
                if (UI::PropSlider("Masa", &mass, 0.0f, 100.0f, "%.1f kg")) {
                    physicsWorld.addRigidBody(entity, mass, rbComp->colliderType);
                }
                if (UI::PropSlider("Rebote", &rbComp->restitution, 0.0f, 1.0f, "%.2f")) {
                    if (rbComp->body) rbComp->body->setRestitution(rbComp->restitution);
                }
                if (UI::PropSlider("Friccion", &rbComp->friction, 0.0f, 1.0f, "%.2f")) {
                    if (rbComp->body) rbComp->body->setFriction(rbComp->friction);
                }
            }
        }
        UI::EndProps();

        if (enableRB) {
            auto rbComp = physicsWorld.getRigidBodyComponent(entity->getName());
            if (rbComp) {
                if (rbComp->mass == 0.0f) {
                    UI::DrawStatusBadge("Estado: ESTATICO (Inamovible / Suelo)", IM_COL32(120, 180, 255, 255));
                } else {
                    UI::DrawStatusBadge("Estado: DINAMICO (Cae con gravedad)", IM_COL32(75, 210, 110, 255));
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Acciones Fisicas:");
                if (UI::ButtonWithIcon(UI::IconType::PHYSICS, "Lanzar arriba (+Y)", ImVec2(0, 22.0f))) {
                    physicsWorld.applyImpulse(entity->getName(), glm::vec3(0, 10.0f * (rbComp->mass > 0 ? rbComp->mass : 1.0f), 0));
                }
                ImGui::SameLine();
                if (UI::ButtonWithIcon(UI::IconType::PHYSICS, "Empujar adelante (+Z)", ImVec2(0, 22.0f))) {
                    physicsWorld.applyImpulse(entity->getName(), glm::vec3(0, 2.0f, 10.0f * (rbComp->mass > 0 ? rbComp->mass : 1.0f)));
                }

                if (UI::ButtonWithIcon(UI::IconType::RESET, "Detener Inercia (Reset)", ImVec2(-1, 22.0f))) {
                    physicsWorld.resetVelocity(entity->getName());
                }
            }
        }
    }
}

void EditorUI::renderScriptsProperties(ScriptEngine& scriptEngine) {
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Lógica y Scripts en Tiempo Real (Lua 5.4)");
    ImGui::Separator();

    ImGui::Text("Tiempo Total Simulado: %.2f s", scriptEngine.getTotalTime());
    ImGui::Spacing();

    if (UI::ButtonWithIcon(m_scriptsPaused ? UI::IconType::PLAY : UI::IconType::PAUSE, m_scriptsPaused ? "Reanudar Ejecucion de Scripts" : "Pausar Scripts [Espacio]", ImVec2(-1, 30.0f))) {
        m_scriptsPaused = !m_scriptsPaused;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Vigilancia Hot-Reload: ACTIVA en 'scripts/main.lua'");
    ImGui::TextDisabled("Cualquier cambio guardado en disco se compila y recarga al vuelo.");
}

void EditorUI::renderImportExportProperties(Scene& scene, ScriptEngine& scriptEngine) {
    ImGui::TextColored(ImVec4(0.75f, 0.55f, 1.0f, 1.0f), "Importacion y Exportacion 3D (Fase 9)");
    ImGui::Separator();
    ImGui::Spacing();

    // 1. Sección Importación
    if (ImGui::CollapsingHeader("Importar Modelos 3D a la Escena", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("Importa archivos 3D directamente a la escena activa. El motor normaliza el tamano y reposa la base sobre la cuadricula.");
        ImGui::Spacing();

        if (UI::ButtonWithIcon(UI::IconType::FOLDER, "Importar Modelo 3D...", ImVec2(-1, 32.0f))) {
            std::string path = SceneIO::openFileDialog();
            if (!path.empty()) {
                auto ent = SceneIO::importModel(scene, path);
                if (ent) {
                    m_selectedEntityName = ent->getName();
                    scriptEngine.addLog("[Importador] [OK] Modelo importado: " + ent->getName());
                }
            }
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.14f, 0.18f, 0.9f));
        ImGui::BeginChild("DragDropBoxInfo", ImVec2(0, 68), true);
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Arrastrar y Soltar (Drag & Drop):");
        ImGui::TextWrapped("Puedes arrastrar cualquier archivo .obj, .gltf, .glb, .fbx, .stl o .ply desde Windows directamente dentro del Viewport 3D.");
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("Formatos compatibles: .obj, .gltf, .glb, .fbx, .stl, .ply, .dae, .3ds, .blend");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 2. Sección Exportación
    if (ImGui::CollapsingHeader("Exportar Escena / Creaciones", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("Exporta tus construcciones Voxel, esculturas generadas por IA o composiciones a formatos universales para abrirlos en Blender, Unreal Engine, Maya o Unity.");
        ImGui::Spacing();

        // Exportar a OBJ
        if (UI::ButtonWithIcon(UI::IconType::SAVE, "Exportar Escena a OBJ (.obj + .mtl)", ImVec2(-1, 30.0f))) {
            std::string path = SceneIO::saveFileDialog("Wavefront OBJ (*.obj)\0*.obj\0", "obj");
            if (!path.empty()) {
                if (SceneIO::exportSceneToOBJ(scene, path)) {
                    scriptEngine.addLog("[Exportador] [OK] Escena exportada exitosamente a OBJ: " + path);
                }
            }
        }

        ImGui::Spacing();

        // Exportar a glTF 2.0
        if (UI::ButtonWithIcon(UI::IconType::SAVE, "Exportar Escena a glTF 2.0 (.gltf)", ImVec2(-1, 30.0f))) {
            std::string path = SceneIO::saveFileDialog("glTF 2.0 (*.gltf)\0*.gltf\0", "gltf");
            if (!path.empty()) {
                if (SceneIO::exportSceneToGLTF(scene, path)) {
                    scriptEngine.addLog("[Exportador] [OK] Escena exportada exitosamente a glTF 2.0: " + path);
                }
            }
        }

        ImGui::Spacing();

        // Exportar entidad seleccionada
        if (!m_selectedEntityName.empty()) {
            auto selEnt = scene.getEntity(m_selectedEntityName);
            if (selEnt) {
                std::string btnText = "Exportar solo '" + selEnt->getName() + "' como OBJ...";
                if (UI::ButtonWithIcon(UI::IconType::SAVE, btnText.c_str(), ImVec2(-1, 28.0f))) {
                    std::string path = SceneIO::saveFileDialog("Wavefront OBJ (*.obj)\0*.obj\0", "obj");
                    if (!path.empty()) {
                        if (SceneIO::exportEntityToOBJ(*selEnt, path)) {
                            scriptEngine.addLog("[Exportador] [OK] Objeto '" + selEnt->getName() + "' exportado a OBJ: " + path);
                        }
                    }
                }
            }
        } else {
            ImGui::TextDisabled("Selecciona un objeto para exportarlo individualmente.");
        }
    }
}

void EditorUI::renderAIPanel(Scene& scene, Terrain& terrain) {
    if (ImGui::Begin("Generador de IA (Gemini)")) {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Constructor 3D en Tiempo Real con Inteligencia Artificial");
        ImGui::Separator();

        // Estado en vivo
        {
            std::lock_guard<std::mutex> lock(m_statusMutex);
            if (m_isGenerating) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Sintetizando escena con Gemini API... Por favor espera (~1.5s)");
            } else {
                ImGui::TextDisabled("%s", m_generationStatus.c_str());
            }
        }

        ImGui::Spacing();

        // Barra de entrada de prompt
        ImGui::SetNextItemWidth(-140.0f);
        bool enterPressed = ImGui::InputTextWithHint("##AIPromptInput", "Escribe tu idea (ej: una torre de cubos que caigan con fisica, un auto, un creeper)...", m_promptBuffer, sizeof(m_promptBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();

        bool btnPressed = ImGui::Button("Generar (Enter)", ImVec2(130, 0));

        if ((enterPressed || btnPressed) && !m_isGenerating) {
            std::string promptText(m_promptBuffer);
            if (!promptText.empty()) {
                recordSnapshot(scene, &terrain);
                generateFromPrompt(promptText);
                m_promptBuffer[0] = '\0';
            }
        }

        ImGui::Spacing();

        // Presets rápidos
        ImGui::TextDisabled("Ideas rapidas con un clic:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Auto Deportivo")) { recordSnapshot(scene, &terrain); generateFromPrompt("un auto deportivo rojo con ruedas y aleron"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Castillo")) { recordSnapshot(scene, &terrain); generateFromPrompt("un castillo medieval con 4 torres y muralla"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Creeper")) { recordSnapshot(scene, &terrain); generateFromPrompt("un creeper de minecraft verde"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Barco Pirata")) { recordSnapshot(scene, &terrain); generateFromPrompt("un barco pirata con velas y canones"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("T-Rex")) { recordSnapshot(scene, &terrain); generateFromPrompt("un dinosaurio T-Rex verde"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Domino")) { recordSnapshot(scene, &terrain); generateFromPrompt("una partida de fichas de domino que caen con fisica"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Torre Fisica")) { recordSnapshot(scene, &terrain); generateFromPrompt("una torre de bloques de colores con fisica y gravedad que caen"); }
    }
    ImGui::End();
}

void EditorUI::renderConsole(ScriptEngine& scriptEngine) {
    if (ImGui::Begin("Consola del Motor")) {
        // 1. Barra de Filtros de Logs y Control Superior (Fase 21)
        auto renderFilterButton = [this](int category, const char* label) {
            bool isActive = (m_consoleFilterCategory == category);
            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.50f, 0.82f, 1.0f));
            }
            if (ImGui::SmallButton(label)) {
                m_consoleFilterCategory = category;
            }
            if (isActive) {
                ImGui::PopStyleColor();
            }
        };

        renderFilterButton(0, "Todos");
        ImGui::SameLine();
        renderFilterButton(1, "Info");
        ImGui::SameLine();
        renderFilterButton(2, "Avisos");
        ImGui::SameLine();
        renderFilterButton(3, "Errores");
        ImGui::SameLine();
        renderFilterButton(4, "Scripts / Lua");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::TRASH, "Limpiar Logs", ImVec2(0, 18.0f))) {
            scriptEngine.clearLogs();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("| Servidor IPC: 127.0.0.1:5555");

        ImGui::Separator();

        if (m_monoFont) ImGui::PushFont(m_monoFont);

        // 2. Área de visualización de Logs filtrados (dejando espacio para el REPL abajo)
        ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, -34.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

        auto logs = scriptEngine.getLogs();
        for (const auto& log : logs) {
            bool isError = (log.find("[ERROR") != std::string::npos || log.find("[EXCEPTION") != std::string::npos || log.find("[Lua REPL ERROR]") != std::string::npos);
            bool isWarn = (log.find("[ADVERTENCIA]") != std::string::npos || log.find("[WARN]") != std::string::npos);
            bool isScript = (log.find("[Script]") != std::string::npos || log.find("[Lua") != std::string::npos || log.find("[REPL") != std::string::npos);
            bool isInfo = (log.find("[Sistema]") != std::string::npos || log.find("[Proyecto]") != std::string::npos || log.find("[Physics") != std::string::npos || log.find("[Info]") != std::string::npos);

            // Filtrado activo
            if (m_consoleFilterCategory == 1 && !isInfo && !isScript && !isWarn && !isError) {
                // Dejar pasar si es texto estándar
            } else if (m_consoleFilterCategory == 1 && (isWarn || isError)) {
                continue;
            } else if (m_consoleFilterCategory == 2 && !isWarn) {
                continue;
            } else if (m_consoleFilterCategory == 3 && !isError) {
                continue;
            } else if (m_consoleFilterCategory == 4 && !isScript) {
                continue;
            }

            if (isError) {
                ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.40f, 1.0f), "%s", log.c_str());
            } else if (isWarn) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "%s", log.c_str());
            } else if (log.find("[Lua REPL >]") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.0f, 1.0f), "%s", log.c_str());
            } else if (log.find("[Lua REPL]") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.50f, 1.0f), "%s", log.c_str());
            } else if (log.find("[IA") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.50f, 1.0f), "%s", log.c_str());
            } else if (log.find("[Physics") != std::string::npos || log.find("[ASIS Physics") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", log.c_str());
            } else if (log.find("[Sistema") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f), "%s", log.c_str());
            } else {
                ImGui::TextUnformatted(log.c_str());
            }
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        // 3. Consola Lua Interactiva (REPL en tiempo real - Fase 21)
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.30f, 0.85f, 1.0f, 1.0f), "Lua >");
        ImGui::SameLine();

        auto historyCallback = [](ImGuiInputTextCallbackData* data) -> int {
            EditorUI* ui = static_cast<EditorUI*>(data->UserData);
            if (!ui || ui->m_luaHistory.empty()) return 0;

            if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
                int prevHistory = ui->m_luaHistoryIndex;
                if (data->EventKey == ImGuiKey_UpArrow) {
                    if (ui->m_luaHistoryIndex == -1) {
                        ui->m_luaHistoryIndex = static_cast<int>(ui->m_luaHistory.size()) - 1;
                    } else if (ui->m_luaHistoryIndex > 0) {
                        ui->m_luaHistoryIndex--;
                    }
                } else if (data->EventKey == ImGuiKey_DownArrow) {
                    if (ui->m_luaHistoryIndex != -1) {
                        ui->m_luaHistoryIndex++;
                        if (ui->m_luaHistoryIndex >= static_cast<int>(ui->m_luaHistory.size())) {
                            ui->m_luaHistoryIndex = -1;
                        }
                    }
                }

                if (prevHistory != ui->m_luaHistoryIndex) {
                    const char* text = (ui->m_luaHistoryIndex >= 0) ? ui->m_luaHistory[ui->m_luaHistoryIndex].c_str() : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, text);
                }
            }
            return 0;
        };

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
        bool executeEnter = ImGui::InputTextWithHint("##LuaREPLInput", "Comando Lua (ej: print('Hola'), spawnPrefab('CajaMadera_Fisica', 0, 5, 0))...",
                                                     m_luaCommandBuf, sizeof(m_luaCommandBuf),
                                                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
                                                     historyCallback, this);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        bool executeClick = UI::ButtonWithIcon(UI::IconType::PLAY, "Ejecutar", ImVec2(76.0f, 0.0f));
        ImGui::SameLine();
        if (UI::IconButton("##ClearCmd", UI::IconType::RESET, ImVec2(22.0f, 0.0f), "Limpiar comando")) {
            m_luaCommandBuf[0] = '\0';
        }

        if (executeEnter || executeClick) {
            std::string cmd = m_luaCommandBuf;
            // Trim whitespace
            cmd.erase(0, cmd.find_first_not_of(" \t\n\r"));
            cmd.erase(cmd.find_last_not_of(" \t\n\r") + 1);

            if (!cmd.empty()) {
                m_luaHistory.push_back(cmd);
                m_luaHistoryIndex = -1;

                scriptEngine.addLog("[Lua REPL >] " + cmd);
                auto res = scriptEngine.executeString(cmd);
                if (res.success) {
                    scriptEngine.addLog("[Lua REPL] [OK] Ejecutado correctamente.");
                } else {
                    scriptEngine.addLog("[Lua REPL ERROR] " + res.errorMessage);
                }

                m_luaCommandBuf[0] = '\0';
                ImGui::SetKeyboardFocusHere(-1);
            }
        }

        if (m_monoFont) ImGui::PopFont();
    }
    ImGui::End();
}

void EditorUI::generateFromPrompt(const std::string& prompt) {
    if (prompt.empty() || m_isGenerating) return;

    m_isGenerating = true;
    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_generationStatus = "Enviando solicitud a Gemini API...";
    }

    std::thread([this, prompt]() {
        std::string scriptPath = "";

#ifdef _WIN32
        char exePathBuf[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(NULL, exePathBuf, MAX_PATH) > 0) {
            std::filesystem::path exeP(exePathBuf);
            std::vector<std::filesystem::path> absCandidates = {
                exeP.parent_path() / "src/ai_module/ai_cli.py",
                exeP.parent_path().parent_path() / "src/ai_module/ai_cli.py",
                exeP.parent_path().parent_path().parent_path() / "src/ai_module/ai_cli.py",
                exeP.parent_path().parent_path().parent_path() / "motor_grafico/src/ai_module/ai_cli.py",
                "C:/Users/Fabian/Desktop/AntiGravity/ASIS_Engine/motor_grafico/src/ai_module/ai_cli.py"
            };
            for (const auto& ap : absCandidates) {
                if (std::filesystem::exists(ap)) {
                    try {
                        scriptPath = std::filesystem::canonical(ap).string();
                    } catch (...) {
                        scriptPath = std::filesystem::absolute(ap).string();
                    }
                    break;
                }
            }
        }
#endif

        if (scriptPath.empty()) {
            std::vector<std::string> candidates = {
                "src/ai_module/ai_cli.py",
                "motor_grafico/src/ai_module/ai_cli.py",
                "../../src/ai_module/ai_cli.py",
                "../src/ai_module/ai_cli.py"
            };
            for (const auto& p : candidates) {
                if (std::filesystem::exists(p)) {
                    try {
                        scriptPath = std::filesystem::canonical(p).string();
                    } catch (...) {
                        scriptPath = std::filesystem::absolute(p).string();
                    }
                    break;
                }
            }
        }

        if (scriptPath.empty()) {
            scriptPath = "C:/Users/Fabian/Desktop/AntiGravity/ASIS_Engine/motor_grafico/src/ai_module/ai_cli.py";
        }

        std::string cleanPrompt = prompt;
        for (char& c : cleanPrompt) {
            if (c == '\"') c = '\'';
        }

        std::string command = "python \"" + scriptPath + "\" \"" + cleanPrompt + "\"";
        int result = std::system(command.c_str());

        {
            std::lock_guard<std::mutex> lock(m_statusMutex);
            if (result == 0) {
                m_generationStatus = "¡Escena '" + cleanPrompt + "' creada exitosamente en pantalla!";
            } else {
                m_generationStatus = "Aviso: finalizada ejecucion de la IA (codigo " + std::to_string(result) + ").";
            }
        }
        m_isGenerating = false;
    }).detach();
}

void EditorUI::renderPreferencesModal(Scene& scene, Camera& camera, PhysicsWorld&) {
    if (!m_showPreferencesModal) return;

    ImGui::SetNextWindowSize(ImVec2(620, 520), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Preferencias y Configuracion de ASIS Engine", &m_showPreferencesModal, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("PrefTabs")) {
            if (ImGui::BeginTabItem("Graficos y Render")) {
                ImGui::Spacing();
                UI::BeginProps("PrefGeneralRenderProps");
                if (UI::PropCheckbox("VSync", &m_vsyncEnabled)) {
                    glfwSwapInterval(m_vsyncEnabled ? 1 : 0);
                }
                const char* fpsOptions[] = { "Ilimitado", "30 FPS", "60 FPS", "120 FPS", "144 FPS", "240 FPS" };
                int currentFPSIndex = (m_targetFPS == 0) ? 0 : (m_targetFPS == 30) ? 1 : (m_targetFPS == 60) ? 2 : (m_targetFPS == 120) ? 3 : (m_targetFPS == 144) ? 4 : 5;
                if (UI::PropCombo("Limite FPS", &currentFPSIndex, fpsOptions, IM_ARRAYSIZE(fpsOptions))) {
                    if (currentFPSIndex == 0) m_targetFPS = 0;
                    else if (currentFPSIndex == 1) m_targetFPS = 30;
                    else if (currentFPSIndex == 2) m_targetFPS = 60;
                    else if (currentFPSIndex == 3) m_targetFPS = 120;
                    else if (currentFPSIndex == 4) m_targetFPS = 144;
                    else if (currentFPSIndex == 5) m_targetFPS = 240;
                }

                const char* msaaOptions[] = { "Desactivado (1x)", "2x MSAA", "4x MSAA (Recomendado)", "8x MSAA (Ultra)" };
                int msaaIdx = (m_msaaLevel == 1) ? 0 : (m_msaaLevel == 2) ? 1 : (m_msaaLevel == 4) ? 2 : 3;
                if (UI::PropCombo("Antialiasing MSAA", &msaaIdx, msaaOptions, IM_ARRAYSIZE(msaaOptions))) {
                    m_msaaLevel = (msaaIdx == 0) ? 1 : (msaaIdx == 1) ? 2 : (msaaIdx == 2) ? 4 : 8;
                }
                UI::PropCheckbox("Filtro FXAA", &m_fxaaEnabled);

                const char* anisoOptions[] = { "1x (Bilineal)", "2x AF", "4x AF", "8x AF", "16x AF (Ultra)" };
                int anisoIdx = (m_anisotropyLevel <= 1) ? 0 : (m_anisotropyLevel == 2) ? 1 : (m_anisotropyLevel == 4) ? 2 : (m_anisotropyLevel == 8) ? 3 : 4;
                if (UI::PropCombo("Filtrado Aniso", &anisoIdx, anisoOptions, IM_ARRAYSIZE(anisoOptions))) {
                    m_anisotropyLevel = (anisoIdx == 0) ? 1 : (anisoIdx == 1) ? 2 : (anisoIdx == 2) ? 4 : (anisoIdx == 3) ? 8 : 16;
                }
                UI::EndProps();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.45f, 1.0f, 1.0f), "Oclusion Ambiental (SSAO):");
                UI::BeginProps("PrefSSAOProps");
                UI::PropCheckbox("Habilitar SSAO", &m_ssaoEnabled);
                if (m_ssaoEnabled) {
                    UI::PropSlider("Radio Muestreo", &m_ssaoRadius, 0.05f, 2.5f, "%.2f m");
                    UI::PropSlider("Sesgo (Bias)", &m_ssaoBias, 0.005f, 0.15f, "%.3f");
                    UI::PropSlider("Exponente (Power)", &m_ssaoPower, 0.5f, 4.0f, "%.1f");
                    UI::PropSlider("Intensidad SSAO", &m_ssaoIntensity, 0.0f, 2.5f, "%.2fx");
                }
                UI::EndProps();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.95f, 1.0f), "Profundidad de Campo (DoF / Bokeh):");
                UI::BeginProps("PrefDoFProps");
                UI::PropCheckbox("Habilitar DoF", &m_dofEnabled);
                if (m_dofEnabled) {
                    UI::PropSlider("Distancia Focal", &m_dofFocusDistance, 0.1f, 50.0f, "%.2f m");
                    UI::PropSlider("Rango Nitido", &m_dofFocusRange, 0.1f, 25.0f, "%.2f m");
                    UI::PropSlider("Radio Bokeh", &m_dofBokehRadius, 1.0f, 25.0f, "%.1f px");
                }
                UI::EndProps();
                if (m_dofEnabled) {
                    if (UI::ButtonWithIcon(UI::IconType::CAMERA_TARGET, "Enfocar en Objeto Seleccionado", ImVec2(-1, 22.0f))) {
                        if (!m_selectedEntityName.empty()) {
                            auto ent = scene.getEntity(m_selectedEntityName);
                            if (ent) {
                                m_dofFocusDistance = glm::distance(camera.getPosition(), ent->getPosition());
                            }
                        }
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Bloom HDR (Resplandor):");
                UI::BeginProps("PrefBloomProps");
                UI::PropCheckbox("Habilitar Bloom", &m_bloomEnabled);
                if (m_bloomEnabled) {
                    UI::PropSlider("Umbral (Threshold)", &m_bloomThreshold, 0.1f, 1.5f, "%.2f");
                    UI::PropSlider("Intensidad Bloom", &m_bloomIntensity, 0.05f, 3.0f, "%.2fx");
                }
                UI::EndProps();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "Reflejos Screen-Space (SSR):");
                UI::BeginProps("PrefSSRProps");
                UI::PropCheckbox("Habilitar SSR", &m_ssrEnabled);
                if (m_ssrEnabled) {
                    UI::PropSliderInt("Pasos Raymarching", &m_ssrMaxSteps, 16, 96);
                    UI::PropSlider("Tamano Paso", &m_ssrStepSize, 0.05f, 0.50f, "%.2f");
                    UI::PropSlider("Tolerancia Grosor", &m_ssrThickness, 0.01f, 0.20f, "%.3f");
                    UI::PropSlider("Intensidad SSR", &m_ssrIntensity, 0.1f, 2.0f, "%.2fx");
                }
                UI::EndProps();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.35f, 1.0f), "Sombras de Contacto (Contact Shadows):");
                UI::BeginProps("PrefContactProps");
                UI::PropCheckbox("Habilitar Sombras Contacto", &m_contactShadowsEnabled);
                if (m_contactShadowsEnabled) {
                    UI::PropSlider("Distancia Rayo", &m_contactShadowsDistance, 0.05f, 1.0f, "%.2f m");
                    UI::PropSliderInt("Muestras Rayo", &m_contactShadowsRaySteps, 4, 32);
                    UI::PropSlider("Intensidad Sombras", &m_contactShadowsIntensity, 0.1f, 1.5f, "%.2fx");
                }
                UI::EndProps();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Mapeo Tonal y Color:");
                const char* profileNames[] = { "ACES Filmic (Predeterminado)", "AgX (Blender 4)", "Reinhard", "Lineal / sRGB" };
                UI::BeginProps("PrefToneProps");
                UI::PropCombo("Curva Tonal", &m_colorProfile, profileNames, IM_ARRAYSIZE(profileNames));
                UI::EndProps();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Adaptacion Ocular y Auto-Exposicion:");
                UI::BeginProps("PrefAutoExpProps");
                UI::PropCheckbox("Habilitar Auto-Exp", &m_autoExposureEnabled);
                if (m_autoExposureEnabled) {
                    UI::PropSlider("Luminancia Obj.", &m_autoExposureKey, 0.05f, 0.50f, "%.2f");
                    UI::PropSlider("Exp. Minima", &m_autoExposureMin, 0.05f, 2.0f, "%.2fx");
                    UI::PropSlider("Exp. Maxima", &m_autoExposureMax, 1.0f, 10.0f, "%.2fx");
                    UI::PropSlider("Vel. a Luz", &m_autoExposureSpeedUp, 0.5f, 10.0f, "%.1f/s");
                    UI::PropSlider("Vel. a Oscuridad", &m_autoExposureSpeedDown, 0.5f, 5.0f, "%.1f/s");
                }
                UI::EndProps();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Tema e Interfaz")) {
                ImGui::Spacing();
                const char* themeNames[] = { "Blender 4 Dark (Naranja)", "Unreal Engine 5 (Azul)", "OLED Midnight (Verde)", "Cyberpunk (Magenta)", "Personalizado" };
                int themeIdx = static_cast<int>(m_currentTheme);
                if (ImGui::Combo("Tema Visual de la UI", &themeIdx, themeNames, IM_ARRAYSIZE(themeNames))) {
                    applyTheme(static_cast<UITheme>(themeIdx));
                }
                if (m_currentTheme == UITheme::CUSTOM) {
                    if (ImGui::ColorEdit4("Color de Acento", &m_customAccentColor.x)) {
                        applyTheme(UITheme::CUSTOM);
                    }
                }
                ImGui::Spacing();
                ImGui::SliderFloat("Velocidad de Camara de Vuelo (Flycam)", &m_flycamSpeed, 1.0f, 30.0f, "%.1f m/s");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Atajos de Teclado")) {
                ImGui::Spacing();
                ImGui::Checkbox("Utilizar Mapeo de Atajos Estilo Blender", &m_useBlenderShortcuts);
                ImGui::Separator();
                ImGui::Columns(2, "ShortcutCols", false);
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Accion"); ImGui::NextColumn();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Atajo de Teclado"); ImGui::NextColumn();
                ImGui::Separator();
                ImGui::Text("Deshacer"); ImGui::NextColumn(); ImGui::Text("Ctrl + Z"); ImGui::NextColumn();
                ImGui::Text("Rehacer"); ImGui::NextColumn(); ImGui::Text("Ctrl + Y / Ctrl+Shift+Z"); ImGui::NextColumn();
                ImGui::Text("Añadir Primitiva / Luz"); ImGui::NextColumn(); ImGui::Text("Shift + A"); ImGui::NextColumn();
                ImGui::Text("Preferencias"); ImGui::NextColumn(); ImGui::Text("Ctrl + Comma (,)"); ImGui::NextColumn();
                ImGui::Text("Guardar Escena (.asis)"); ImGui::NextColumn(); ImGui::Text("Ctrl + S"); ImGui::NextColumn();
                ImGui::Text("Abrir Escena (.asis)"); ImGui::NextColumn(); ImGui::Text("Ctrl + O"); ImGui::NextColumn();
                ImGui::Text("Nueva Escena"); ImGui::NextColumn(); ImGui::Text("Ctrl + N"); ImGui::NextColumn();
                ImGui::Text("Gizmo: Mover / Rotar / Escalar"); ImGui::NextColumn(); ImGui::Text("W / E / R"); ImGui::NextColumn();
                ImGui::Text("Enfocar Camara en Seleccion"); ImGui::NextColumn(); ImGui::Text("F / Teclado Num ."); ImGui::NextColumn();
                ImGui::Text("Pausar / Reanudar Animacion"); ImGui::NextColumn(); ImGui::Text("Espacio"); ImGui::NextColumn();
                ImGui::Text("Ocultar / Mostrar Interfaz"); ImGui::NextColumn(); ImGui::Text("F1"); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Sistema y Deshacer")) {
                ImGui::Spacing();
                ImGui::SliderInt("Profundidad de Historial (Deshacer/Rehacer)", &m_maxUndoSteps, 16, 256, "%d pasos");
                ImGui::Spacing();
                ImGui::Checkbox("Habilitar Autoguardado Periodico", &m_autoSaveEnabled);
                if (m_autoSaveEnabled) {
                    ImGui::SliderFloat("Intervalo de Autoguardado (minutos)", &m_autoSaveIntervalMinutes, 1.0f, 30.0f, "%.1f min");
                    ImGui::TextDisabled("Las escenas se respaldan automaticamente en 'autosave.asis'");
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Cerrar", ImVec2(100, 28))) {
            m_showPreferencesModal = false;
        }
    }
    ImGui::End();
}

void EditorUI::renderAddPrimitiveMenu(Scene& scene, Camera& camera, ParticleSystem& particleSystem) {
    if (m_openAddMenu) {
        ImGui::OpenPopup("AddPrimitivePopup");
        m_openAddMenu = false;
    }

    if (ImGui::BeginPopup("AddPrimitivePopup")) {
        ImGui::TextColored(ImVec4(0.95f, 0.6f, 0.2f, 1.0f), "Añadir a la Escena");
        ImGui::Separator();

        if (m_groundPlaneDeleted) {
            if (ImGui::MenuItem("Restaurar Plano de Suelo Base")) {
                m_groundPlaneDeleted = false;
                m_showGroundPlane = true;
            }
            ImGui::Separator();
        }

        glm::vec3 spawnPos = camera.getPosition() + camera.getFront() * 4.0f;
        spawnPos.y = (std::max)(0.5f, spawnPos.y);

        if (ImGui::BeginMenu("Malla (Mesh)")) {
            if (ImGui::MenuItem("Cubo")) {
                recordSnapshot(scene);
                std::string name = "Cube_" + std::to_string(scene.getEntityCount() + 1);
                auto ent = scene.createCube(name);
                if (ent) {
                    ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                    m_selectedEntityName = name;
                    m_selectedLightIndex = -1;
                }
            }
            if (ImGui::MenuItem("Esfera UV")) {
                recordSnapshot(scene);
                std::string name = "Sphere_" + std::to_string(scene.getEntityCount() + 1);
                auto ent = scene.createSphere(name);
                if (ent) {
                    ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                    m_selectedEntityName = name;
                    m_selectedLightIndex = -1;
                }
            }
            if (ImGui::MenuItem("Cilindro")) {
                recordSnapshot(scene);
                std::string name = "Cylinder_" + std::to_string(scene.getEntityCount() + 1);
                auto ent = scene.createCylinder(name);
                if (ent) {
                    ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                    m_selectedEntityName = name;
                    m_selectedLightIndex = -1;
                }
            }
            if (ImGui::MenuItem("Cono")) {
                recordSnapshot(scene);
                std::string name = "Cone_" + std::to_string(scene.getEntityCount() + 1);
                auto ent = scene.createCone(name);
                if (ent) {
                    ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                    m_selectedEntityName = name;
                    m_selectedLightIndex = -1;
                }
            }
            if (ImGui::MenuItem("Toroide (Donut)")) {
                recordSnapshot(scene);
                std::string name = "Torus_" + std::to_string(scene.getEntityCount() + 1);
                auto ent = scene.createTorus(name);
                if (ent) {
                    ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                    m_selectedEntityName = name;
                    m_selectedLightIndex = -1;
                }
            }
            if (ImGui::MenuItem("Plano")) {
                recordSnapshot(scene);
                std::string name = "Plane_" + std::to_string(scene.getEntityCount() + 1);
                auto ent = scene.createPlane(name);
                if (ent) {
                    ent->setPosition(spawnPos.x, spawnPos.y, spawnPos.z);
                    m_selectedEntityName = name;
                    m_selectedLightIndex = -1;
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Particulas (VFX)")) {
            if (ImGui::MenuItem("Fuego (Fire)")) {
                particleSystem.addEmitter("Fuego_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::FIRE, spawnPos);
                m_activePropertyTab = BlenderPropertyTab::PARTICLES;
            }
            if (ImGui::MenuItem("Chispas (Sparks)")) {
                particleSystem.addEmitter("Chispas_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::SPARKS, spawnPos);
                m_activePropertyTab = BlenderPropertyTab::PARTICLES;
            }
            if (ImGui::MenuItem("Humo (Smoke)")) {
                particleSystem.addEmitter("Humo_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::SMOKE, spawnPos);
                m_activePropertyTab = BlenderPropertyTab::PARTICLES;
            }
            if (ImGui::MenuItem("Lluvia (Rain)")) {
                particleSystem.addEmitter("Lluvia_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::RAIN, spawnPos + glm::vec3(0, 5, 0));
                m_activePropertyTab = BlenderPropertyTab::PARTICLES;
            }
            if (ImGui::MenuItem("Magia (Magic)")) {
                particleSystem.addEmitter("Magia_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::MAGIC, spawnPos);
                m_activePropertyTab = BlenderPropertyTab::PARTICLES;
            }
            if (ImGui::MenuItem("Emisor Personalizado")) {
                particleSystem.addEmitter("Emisor_" + std::to_string(particleSystem.getEmitterCount() + 1), EmitterType::CUSTOM, spawnPos);
                m_activePropertyTab = BlenderPropertyTab::PARTICLES;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Luz")) {
            if (ImGui::MenuItem("Luz Puntual")) {
                recordSnapshot(scene);
                auto& lights = scene.getPointLights();
                if (lights.size() < 8) {
                    PointLight l("Luz Puntual " + std::to_string(lights.size() + 1), spawnPos);
                    scene.addPointLight(l);
                    m_selectedLightIndex = static_cast<int>(lights.size() - 1);
                    m_selectedEntityName = "";
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
}

void EditorUI::drawTriggerWireframe(ImDrawList* drawList, const Entity& entity, const glm::mat4& viewProj, const ImVec2& screenPos, const ImVec2& avail) {
    if (!drawList) return;
    glm::mat4 model = entity.getModelMatrix();
    glm::vec3 corners[8] = {
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f),
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f,  0.5f)
    };

    ImVec2 scrPts[8];
    bool validPts[8] = { false };
    int validCount = 0;

    for (int i = 0; i < 8; ++i) {
        glm::vec4 worldPt = model * glm::vec4(corners[i], 1.0f);
        glm::vec4 clip = viewProj * worldPt;
        if (clip.w > 0.01f) {
            float ndcX = clip.x / clip.w;
            float ndcY = clip.y / clip.w;
            scrPts[i] = ImVec2(
                screenPos.x + (ndcX * 0.5f + 0.5f) * avail.x,
                screenPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * avail.y
            );
            validPts[i] = true;
            validCount++;
        }
    }

    if (validCount < 4) return;

    ImU32 trigCol = ImColor(0, 230, 255, 220); // Cyan luminoso para triggers

    const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    for (int e = 0; e < 12; ++e) {
        int i0 = edges[e][0];
        int i1 = edges[e][1];
        if (validPts[i0] && validPts[i1]) {
            drawList->AddLine(scrPts[i0], scrPts[i1], trigCol, 1.8f);
        }
    }

    // Texto de Badge de Trigger en el centro
    glm::vec4 centerClip = viewProj * glm::vec4(entity.getPosition(), 1.0f);
    if (centerClip.w > 0.01f) {
        float ndcX = centerClip.x / centerClip.w;
        float ndcY = centerClip.y / centerClip.w;
        if (ndcX >= -1.1f && ndcX <= 1.1f && ndcY >= -1.1f && ndcY <= 1.1f) {
            float cx = screenPos.x + (ndcX * 0.5f + 0.5f) * avail.x;
            float cy = screenPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * avail.y;
            std::string label = "[TRIGGER: " + entity.getName() + " (" + entity.getTriggerTag() + ")]";
            drawList->AddText(ImVec2(cx - 30.0f, cy - 8.0f), ImColor(0, 255, 255, 240), label.c_str());
        }
    }
}

void EditorUI::drawPlainAxes(ImDrawList* drawList, const glm::vec3& pivot, const glm::mat4& viewProj, const ImVec2& screenPos, const ImVec2& avail, const std::string& label) {
    if (!drawList) return;

    auto project = [&](const glm::vec3& p) -> std::pair<ImVec2, bool> {
        glm::vec4 clip = viewProj * glm::vec4(p, 1.0f);
        if (clip.w <= 0.01f) return { ImVec2(0, 0), false };
        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;
        if (ndcX < -1.8f || ndcX > 1.8f || ndcY < -1.8f || ndcY > 1.8f) return { ImVec2(0, 0), false };
        return {
            ImVec2(
                screenPos.x + (ndcX * 0.5f + 0.5f) * avail.x,
                screenPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * avail.y
            ),
            true
        };
    };

    auto [cPt, cValid] = project(pivot);
    if (!cValid) return;

    float axisLen = 1.6f;
    glm::vec3 pXpos = pivot + glm::vec3(axisLen, 0.0f, 0.0f);
    glm::vec3 pXneg = pivot - glm::vec3(axisLen, 0.0f, 0.0f);
    glm::vec3 pYpos = pivot + glm::vec3(0.0f, axisLen, 0.0f);
    glm::vec3 pYneg = pivot - glm::vec3(0.0f, axisLen, 0.0f);
    glm::vec3 pZpos = pivot + glm::vec3(0.0f, 0.0f, axisLen);
    glm::vec3 pZneg = pivot - glm::vec3(0.0f, 0.0f, axisLen);

    auto [ptXp, vXp] = project(pXpos);
    auto [ptXn, vXn] = project(pXneg);
    auto [ptYp, vYp] = project(pYpos);
    auto [ptYn, vYn] = project(pYneg);
    auto [ptZp, vZp] = project(pZpos);
    auto [ptZn, vZn] = project(pZneg);

    ImU32 colX = ImColor(245, 65, 65, 255);   // Rojo
    ImU32 colY = ImColor(65, 230, 65, 255);   // Verde
    ImU32 colZ = ImColor(65, 120, 245, 255);  // Azul

    // Líneas completas perpendiculares cruzadas estilo Plain Axes de Blender
    if (vXp && vXn) drawList->AddLine(ptXn, ptXp, colX, 2.0f);
    else if (vXp)   drawList->AddLine(cPt, ptXp, colX, 2.0f);
    else if (vXn)   drawList->AddLine(cPt, ptXn, colX, 2.0f);

    if (vYp && vYn) drawList->AddLine(ptYn, ptYp, colY, 2.0f);
    else if (vYp)   drawList->AddLine(cPt, ptYp, colY, 2.0f);
    else if (vYn)   drawList->AddLine(cPt, ptYn, colY, 2.0f);

    if (vZp && vZn) drawList->AddLine(ptZn, ptZp, colZ, 2.0f);
    else if (vZp)   drawList->AddLine(cPt, ptZp, colZ, 2.0f);
    else if (vZn)   drawList->AddLine(cPt, ptZn, colZ, 2.0f);

    // Punto central del pivote
    drawList->AddCircleFilled(cPt, 4.0f, ImColor(255, 255, 255, 240));

    // Etiquetas en los extremos positivos
    if (vXp) drawList->AddText(ImVec2(ptXp.x + 3, ptXp.y - 7), colX, "+X");
    if (vYp) drawList->AddText(ImVec2(ptYp.x + 3, ptYp.y - 7), colY, "+Y");
    if (vZp) drawList->AddText(ImVec2(ptZp.x + 3, ptZp.y - 7), colZ, "+Z");

    // Etiqueta del grupo
    if (!label.empty()) {
        std::string badge = "[Grupo: " + label + "]";
        drawList->AddText(ImVec2(cPt.x + 8.0f, cPt.y - 18.0f), ImColor(255, 220, 80, 240), badge.c_str());
    }
}

void EditorUI::selectGroup(const std::string& groupName, Scene& scene) {
    m_selectedGroupName = groupName;
    m_selectedEntityNames.clear();
    auto groupEnts = scene.getEntitiesInGroup(groupName);
    glm::vec3 center(0.0f);
    int count = 0;
    for (auto& ge : groupEnts) {
        if (ge) {
            m_selectedEntityNames.insert(ge->getName());
            center += ge->getPosition();
            count++;
        }
    }
    if (count > 0) {
        m_groupPivot = center / static_cast<float>(count);
        m_selectedEntityName = groupEnts[0]->getName();
    } else {
        m_groupPivot = glm::vec3(0.0f);
        m_selectedEntityName = "";
    }
    m_selectedLightIndex = -1;
}

void EditorUI::renderPrefabPalette(Scene& scene, Camera& camera, PhysicsWorld& physicsWorld) {
    if (ImGui::Begin("Navegador de Activos & Prefabs")) {
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Navegador de Activos & Prefabs");
        ImGui::Separator();

        if (m_availablePrefabs.empty()) {
            m_availablePrefabs = PrefabManager::getAvailablePrefabs("assets/prefabs");
        }

        if (UI::ButtonWithIcon(UI::IconType::RESET, "Actualizar", ImVec2(0.0f, 20.0f))) {
            m_availablePrefabs = PrefabManager::getAvailablePrefabs("assets/prefabs");
        }
        ImGui::SameLine();
        if (UI::ButtonWithIcon(UI::IconType::PLUS, "Crear Prefabs de Ejemplo", ImVec2(0.0f, 20.0f))) {
            PrefabManager::createDefaultPrefabs("assets/prefabs");
            m_availablePrefabs = PrefabManager::getAvailablePrefabs("assets/prefabs");
        }

        // Seccion: Guardar entidad seleccionada como prefab
        if (!m_selectedEntityName.empty()) {
            auto selEnt = scene.getEntity(m_selectedEntityName);
            if (selEnt) {
                ImGui::Spacing();
                ImGui::TextDisabled("Guardar seleccion como Prefab:");
                ImGui::SetNextItemWidth(140.0f);
                ImGui::InputText("##NewPrefabName", m_newPrefabNameBuf, sizeof(m_newPrefabNameBuf));
                ImGui::SameLine();
                if (UI::ButtonWithIcon(UI::IconType::SAVE, "Guardar Prefab", ImVec2(0.0f, 20.0f))) {
                    std::string pName = m_newPrefabNameBuf;
                    if (!pName.empty()) {
                        PrefabManager::saveEntityAsPrefab(selEnt, pName, &physicsWorld);
                        m_availablePrefabs = PrefabManager::getAvailablePrefabs("assets/prefabs");
                    }
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.80f, 0.80f, 0.80f, 1.0f), "Activos Disponibles (%zu):", m_availablePrefabs.size());
        ImGui::Spacing();

        ImGui::BeginChild("PrefabGridRegion", ImVec2(0, 0), false);

        float availWidth = ImGui::GetContentRegionAvail().x;
        float cardWidth = 76.0f;
        float cardHeight = 86.0f;
        int cols = (std::max)(1, (int)(availWidth / (cardWidth + 8.0f)));

        if (ImGui::BeginTable("##PrefabCardGrid", cols, ImGuiTableFlags_NoSavedSettings)) {
            for (size_t i = 0; i < m_availablePrefabs.size(); ++i) {
                ImGui::TableNextColumn();
                const std::string& pFile = m_availablePrefabs[i];
                ImGui::PushID(static_cast<int>(i));

                std::string displayName = pFile;
                size_t dotPos = displayName.rfind(".prefab");
                if (dotPos != std::string::npos) displayName = displayName.substr(0, dotPos);

                ImVec2 cardPos = ImGui::GetCursorScreenPos();
                bool clicked = ImGui::InvisibleButton("##CardBtn", ImVec2(cardWidth, cardHeight));
                bool hovered = ImGui::IsItemHovered();

                ImDrawList* dl = ImGui::GetWindowDrawList();

                // Fondo de la tarjeta
                ImU32 cardBg = hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(28, 28, 28, 255);
                dl->AddRectFilled(cardPos, ImVec2(cardPos.x + cardWidth, cardPos.y + cardHeight), cardBg, 4.0f);
                dl->AddRect(cardPos, ImVec2(cardPos.x + cardWidth, cardPos.y + cardHeight), hovered ? IM_COL32(71, 114, 179, 255) : IM_COL32(42, 42, 42, 255), 4.0f, 0, 1.0f);

                // Thumbnail superior (64x50 px)
                ImVec2 thumbPos(cardPos.x + (cardWidth - 64.0f) * 0.5f, cardPos.y + 4.0f);
                UI::DrawAssetThumbnail(dl, displayName, thumbPos, ImVec2(64.0f, 50.0f));

                // Nombre recortado centrado debajo
                std::string shortName = displayName;
                if (shortName.size() > 10) shortName = shortName.substr(0, 9) + ".";
                ImVec2 tsz = ImGui::CalcTextSize(shortName.c_str());
                ImVec2 textPos(cardPos.x + (cardWidth - tsz.x) * 0.5f, cardPos.y + 60.0f);
                dl->AddText(textPos, hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255), shortName.c_str());

                // Tooltip
                if (hovered) {
                    ImGui::SetTooltip("%s\nDoble clic: Spawn en foco\nArrastrar al Viewport para instanciar", displayName.c_str());
                }

                // Doble clic: Spawn en foco
                if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    recordSnapshot(scene);
                    glm::vec3 spawnPos = camera.getTarget();
                    PrefabManager::instantiatePrefab(pFile, scene, &physicsWorld, spawnPos);
                }

                // Clic derecho: Menú contextual
                if (ImGui::BeginPopupContextItem("##CardCtxMenu")) {
                    if (ImGui::MenuItem("Spawnear en Foco")) {
                        recordSnapshot(scene);
                        glm::vec3 spawnPos = camera.getTarget();
                        PrefabManager::instantiatePrefab(pFile, scene, &physicsWorld, spawnPos);
                    }
                    if (ImGui::MenuItem("Spawnear en (0, 1, 0)")) {
                        recordSnapshot(scene);
                        PrefabManager::instantiatePrefab(pFile, scene, &physicsWorld, glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Eliminar Prefab")) {
                        std::filesystem::remove("assets/prefabs/" + pFile);
                        m_availablePrefabs = PrefabManager::getAvailablePrefabs("assets/prefabs");
                    }
                    ImGui::EndPopup();
                }

                // Drag & Drop Source
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("ASIS_PREFAB", pFile.c_str(), pFile.size() + 1);
                    ImGui::Text("Prefab: %s", displayName.c_str());
                    ImGui::EndDragDropSource();
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void EditorUI::updateAutoExposure(float measuredLuminance, float deltaTime) {
    if (!m_autoExposureEnabled || deltaTime <= 0.0f) return;

    // Calcular la luminancia absoluta des-escalando por la exposición adaptada previa
    float rawSceneLum = measuredLuminance / std::max(m_currentAdaptedExposure, 0.001f);
    m_measuredSceneLuminance = rawSceneLum;

    // Fórmula fotográfica: Target Exposure = Key / Raw Scene Luminance
    float targetExp = m_autoExposureKey / std::max(rawSceneLum, 0.0001f);
    targetExp = std::clamp(targetExp, m_autoExposureMin, m_autoExposureMax);

    // Adaptación temporal independiente del framerate (asimetría luz vs oscuridad)
    float rate = (targetExp < m_currentAdaptedExposure) ? m_autoExposureSpeedUp : m_autoExposureSpeedDown;
    float factor = 1.0f - std::exp(-deltaTime * rate);
    m_currentAdaptedExposure += (targetExp - m_currentAdaptedExposure) * factor;

    // Protección estricta contra NaNs o valores anómalos
    if (std::isnan(m_currentAdaptedExposure) || std::isinf(m_currentAdaptedExposure) || m_currentAdaptedExposure <= 0.0f) {
        m_currentAdaptedExposure = 1.0f;
    }
}

void EditorUI::shutdown() {
    if (!m_initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void EditorUI::renderStatusBar(Scene& scene, Camera& camera, float deltaTime) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 22.0f));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 22.0f));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 2.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.114f, 0.114f, 0.114f, 1.0f)); // #1D1D1D
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.20f, 0.20f, 1.0f)); // #333333

    if (ImGui::Begin("##ASIS_StatusBar", nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // 1. Izquierda: Notificación contextual o atajos de raton y teclado
        if (m_statusBarMessageTimer > 0.0f && !m_statusBarMessage.empty()) {
            m_statusBarMessageTimer -= deltaTime;
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "[*] %s", m_statusBarMessage.c_str());
        } else if (!isPlaying()) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            float midY = p.y + 9.0f;
            
            // LMB
            UI::DrawMiniIcon(dl, UI::IconType::MOUSE_LMB, ImVec2(p.x + 8.0f, midY), 12.0f, IM_COL32(200, 200, 200, 255));
            ImGui::SetCursorScreenPos(ImVec2(p.x + 18.0f, p.y));
            ImGui::TextUnformatted("Seleccionar");

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            // MMB
            p = ImGui::GetCursorScreenPos();
            UI::DrawMiniIcon(dl, UI::IconType::MOUSE_MMB, ImVec2(p.x + 8.0f, midY), 12.0f, IM_COL32(200, 200, 200, 255));
            ImGui::SetCursorScreenPos(ImVec2(p.x + 18.0f, p.y));
            ImGui::TextUnformatted("Orbitar (Blender)");

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            // RMB
            p = ImGui::GetCursorScreenPos();
            UI::DrawMiniIcon(dl, UI::IconType::MOUSE_RMB, ImVec2(p.x + 8.0f, midY), 12.0f, IM_COL32(200, 200, 200, 255));
            ImGui::SetCursorScreenPos(ImVec2(p.x + 18.0f, p.y));
            ImGui::TextUnformatted("+WASD Vuelo Libre");

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.0f, 1.0f), "[N]");
            ImGui::SameLine();
            ImGui::TextUnformatted("Menu Rapido");
        } else {
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "[WASD]");
            ImGui::SameLine();
            ImGui::TextUnformatted("Mover");
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "[Shift]");
            ImGui::SameLine();
            ImGui::TextUnformatted("Sprint");
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "[Espacio]");
            ImGui::SameLine();
            ImGui::TextUnformatted("Saltar");
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "[C]");
            ImGui::SameLine();
            ImGui::TextUnformatted("Agacharse");
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "[V]");
            ImGui::SameLine();
            ImGui::TextUnformatted("1ra/3ra Persona");
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "[Esc]");
            ImGui::SameLine();
            ImGui::TextUnformatted("Salir");
        }

        // 2. Derecha: Telemetria en vivo
        size_t totalTris = 0;
        for (const auto& [name, ent] : scene.getEntities()) {
            if (ent && ent->getModel()) {
                for (const auto& mesh : ent->getModel()->getMeshes()) {
                    if (mesh) {
                        totalTris += mesh->getIndices().size() / 3;
                    }
                }
            }
        }

        char teleBuf[256];
        snprintf(teleBuf, sizeof(teleBuf),
            "Objetos: %zu | Luces: %zu | Meshlets OASIS: %zu vis | Triangulos: %zu | FPS: %.0f (%.1f ms) | ASIS v0.22",
            scene.getEntityCount(),
            scene.getPointLights().size(),
            m_meshletTelemetry.visibleMeshletsCount,
            totalTris,
            m_fps,
            m_frameTimeMs);

        float textWidth = ImGui::CalcTextSize(teleBuf).x;
        float rightPos = viewport->WorkSize.x - textWidth - 12.0f;
        if (ImGui::GetCursorPosX() < rightPos) {
            ImGui::SameLine(rightPos);
            ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.70f, 1.0f), "%s", teleBuf);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void EditorUI::renderAboutModal() {
    if (!m_showAboutModal) return;

    ImGui::OpenPopup("Acerca de ASIS Engine v0.22");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(490.0f, 0.0f));

    if (ImGui::BeginPopupModal("Acerca de ASIS Engine v0.22", &m_showAboutModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Dibujar Isotipo grande a 84x84 px centrado en blanco puro sobre fondo oscuro #1D1D1D (Fase 22.5)
        float logoSize = 84.0f;
        float winWidth = ImGui::GetWindowWidth();
        ImVec2 curPos = ImGui::GetCursorScreenPos();
        ImVec2 logoPos(curPos.x + (winWidth - logoSize) * 0.5f - 8.0f, curPos.y + 4.0f);
        UI::DrawASISLogo(ImGui::GetWindowDrawList(), logoPos, logoSize, IM_COL32(255, 255, 255, 255), IM_COL32(29, 29, 29, 255));
        ImGui::Dummy(ImVec2(0.0f, logoSize + 8.0f));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.80f, 1.0f, 1.0f));
        float titleW = ImGui::CalcTextSize("ASIS Engine v0.22 (C++20)").x;
        ImGui::SetCursorPosX((winWidth - titleW) * 0.5f);
        ImGui::TextUnformatted("ASIS Engine v0.22 (C++20)");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BulletText("Sistema de Animacion Cinematica / Sequencer y Curvas Temporales (Fase 22)");
        ImGui::BulletText("Camaras de Cine con Lentes Fisicos (18mm-135mm), DoF Bokeh y Shake");
        ImGui::BulletText("Arquitectura de Geometria Virtual OASIS (CDLOD + Meshlets)");
        ImGui::BulletText("Pipeline PBR Avanzado (IBL Cook-Torrance, Sombras PCSS/CSM)");
        ImGui::BulletText("Reflejos SSR, Contact Shadows, SSAO y Niebla Volumetrica");
        ImGui::BulletText("Simulacion Atmosferica Dinamica 24h & Oceano 3D con Ondas Gerstner");
        ImGui::BulletText("Fisica 3D Bullet, Audio Espacial 3D y Consola Interactiva Lua 5.4");
        ImGui::BulletText("Interfaz de Alta Densidad Visual Estilo Blender 5");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX((winWidth - 110.0f) * 0.5f);
        if (ImGui::Button("Cerrar", ImVec2(110.0f, 24.0f))) {
            m_showAboutModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// ============================================================================
// FASE 22: Cámara de Cine, Óptica y Lentes Físicos
// ============================================================================
void EditorUI::renderCineCameraProperties(Camera& camera, Scene& scene) {
    if (ImGui::CollapsingHeader("Camara de Cine y Lentes Fisicos (Fase 22)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& seq = Sequencer::get();

        // 1. Barra de Controles Rapidos y Pilotaje
        bool piloting = seq.isPilotCineCamera();
        if (UI::IconToggle("cine_pilot_toggle", UI::IconType::CINE_CAMERA, piloting, ImVec2(0.0f, 22.0f), "Pilotar Camara de Cine: Toma el control del Viewport 3D con las pistas del Sequencer")) {
            seq.setPilotCineCamera(!piloting);
        }
        ImGui::SameLine();
        ImGui::TextColored(piloting ? ImVec4(0.35f, 0.85f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                           piloting ? "PILOTANDO CAMARA CINE" : "Modo Libre / Editor");

        ImGui::Spacing();
        bool scope = seq.isAnamorphicScope();
        if (ImGui::Checkbox("Anamorfico 2.39:1 (Scope)", &scope)) {
            seq.setAnamorphicScope(scope);
        }
        ImGui::SameLine();
        bool thirds = seq.isRuleOfThirds();
        if (ImGui::Checkbox("Regla de Tercios", &thirds)) {
            seq.setRuleOfThirds(thirds);
        }
        ImGui::SameLine();
        bool mpath = seq.isMotionPathVisible();
        if (ImGui::Checkbox("Ruta 3D (Motion Path)", &mpath)) {
            seq.setMotionPathVisible(mpath);
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Lentes Fisicos de Cine (Distancia Focal):");

        // Botones de Lentes Rapidos en 1 Clic
        auto lensButton = [&](const char* label, float mm) {
            if (ImGui::Button(label, ImVec2(0.0f, 20.0f))) {
                float fov = Sequencer::focalLengthToFOV(mm);
                camera.setFOV(fov);
                if (seq.isAutoKeyEnabled()) {
                    seq.recordCameraKeyframe(camera, mm, m_dofFocusDistance, 2.8f);
                }
            }
        };

        lensButton("18mm Ultra-Angular", 18.0f); ImGui::SameLine();
        lensButton("24mm Angular", 24.0f); ImGui::SameLine();
        lensButton("35mm Estandar", 35.0f);
        lensButton("50mm Normal", 50.0f); ImGui::SameLine();
        lensButton("85mm Retrato", 85.0f); ImGui::SameLine();
        lensButton("135mm Teleobjetivo", 135.0f);

        ImGui::Spacing();

        // Tabla de 2 Columnas para Parametros Opticos
        UI::BeginProps("CineLensPropsTable");

        float currentFocal = Sequencer::fovToFocalLength(camera.getFOV());
        if (UI::PropSlider("Distancia Focal", &currentFocal, 12.0f, 200.0f, "%.1f mm", "Distancia focal equivalente en sensor Full-Frame 35mm")) {
            camera.setFOV(Sequencer::focalLengthToFOV(currentFocal));
            if (seq.isAutoKeyEnabled()) {
                seq.recordCameraKeyframe(camera, currentFocal, m_dofFocusDistance, 2.8f);
            }
        }

        float curFOV = camera.getFOV();
        UI::PropFloat("FOV Vertical", &curFOV, 0.1f, 10.0f, 130.0f, "%.1f deg", "Angulo vertical de vision equivalente");

        if (UI::PropSlider("Distancia Enfoque (DoF)", &m_dofFocusDistance, 0.1f, 50.0f, "%.2f m", "Punto nitido exacto para profundidad de campo")) {
            m_dofEnabled = true;
            if (seq.isAutoKeyEnabled()) {
                seq.recordCameraKeyframe(camera, currentFocal, m_dofFocusDistance, 2.8f);
            }
        }

        float aperture = 2.8f;
        if (!seq.getCameraTracks().empty() && !seq.getCameraTracks()[0].keyframes.empty()) {
            aperture = seq.getCameraTracks()[0].keyframes[0].aperture_fstop;
        }
        if (UI::PropSlider("Apertura (f-stop)", &aperture, 1.2f, 22.0f, "f/%.1f", "Apertura del diafragma: valores bajos producen mayor desenfoque Bokeh")) {
            m_dofBokehRadius = glm::clamp(10.0f / (std::max)(aperture, 0.5f), 0.5f, 10.0f);
            m_dofEnabled = true;
        }

        UI::EndProps();

        ImGui::Spacing();

        // Acciones de Enfoque y Grabacion
        if (UI::ButtonWithIcon(UI::IconType::CAMERA_TARGET, "Enfocar en Objeto Seleccionado", ImVec2(-1, 22.0f))) {
            if (!m_selectedEntityName.empty()) {
                auto ent = scene.getEntity(m_selectedEntityName);
                if (ent) {
                    m_dofFocusDistance = glm::distance(camera.getPosition(), ent->getPosition());
                    m_dofEnabled = true;
                }
            }
        }

        if (UI::ButtonWithIcon(UI::IconType::KEYFRAME, "Grabar Clave de Camara de Cine (I)", ImVec2(-1, 22.0f))) {
            seq.recordCameraKeyframe(camera, currentFocal, m_dofFocusDistance, aperture);
        }

        if (ImGui::Button("Disparar Impulso Shake (Test)", ImVec2(-1, 20.0f))) {
            seq.triggerImpulseShake(0.35f, 0.6f);
        }
    }
}

// ============================================================================
// FASE 22: Overlays Cinemáticos en Viewport 3D
// ============================================================================
void EditorUI::drawViewportSequencerOverlays(ImDrawList* dl, ImVec2 pMin, ImVec2 pMax, Camera& camera) {
    if (!dl) return;
    auto& seq = Sequencer::get();
    float viewW = pMax.x - pMin.x;
    float viewH = pMax.y - pMin.y;
    if (viewW <= 1.0f || viewH <= 1.0f) return;

    // 1. Mascara Anamorfica 2.39:1 Scope (Barras Negras Cinematograficas)
    float barH = 0.0f;
    if (seq.isAnamorphicScope()) {
        float targetAspect = 2.39f;
        float currentAspect = viewW / viewH;
        if (currentAspect < targetAspect) {
            float activeH = viewW / targetAspect;
            barH = (viewH - activeH) * 0.5f;
            if (barH > 0.0f) {
                // Barra superior
                dl->AddRectFilled(pMin, ImVec2(pMax.x, pMin.y + barH), IM_COL32(0, 0, 0, 255));
                dl->AddLine(ImVec2(pMin.x, pMin.y + barH), ImVec2(pMax.x, pMin.y + barH), IM_COL32(255, 255, 255, 45), 1.0f);
                // Barra inferior
                dl->AddRectFilled(ImVec2(pMin.x, pMax.y - barH), pMax, IM_COL32(0, 0, 0, 255));
                dl->AddLine(ImVec2(pMin.x, pMax.y - barH), ImVec2(pMax.x, pMax.y - barH), IM_COL32(255, 255, 255, 45), 1.0f);

                // Badge de relacion de aspecto
                dl->AddText(ImVec2(pMin.x + 8.0f, pMin.y + barH + 4.0f), IM_COL32(180, 200, 230, 180), "2.39:1 SCOPE ANAMORFICO");
            }
        }
    }

    // 2. Guias de Composicion: Regla de Tercios
    if (seq.isRuleOfThirds()) {
        float topY = pMin.y + barH;
        float botY = pMax.y - barH;
        float activeH = botY - topY;

        float y1 = topY + activeH * 0.33333f;
        float y2 = topY + activeH * 0.66667f;
        float x1 = pMin.x + viewW * 0.33333f;
        float x2 = pMin.x + viewW * 0.66667f;

        ImU32 guideCol = IM_COL32(255, 255, 255, 45);
        dl->AddLine(ImVec2(pMin.x, y1), ImVec2(pMax.x, y1), guideCol, 1.0f);
        dl->AddLine(ImVec2(pMin.x, y2), ImVec2(pMax.x, y2), guideCol, 1.0f);
        dl->AddLine(ImVec2(x1, topY), ImVec2(x1, botY), guideCol, 1.0f);
        dl->AddLine(ImVec2(x2, topY), ImVec2(x2, botY), guideCol, 1.0f);

        // Puntos de interseccion dorados
        ImU32 dotCol = IM_COL32(255, 215, 60, 140);
        dl->AddCircleFilled(ImVec2(x1, y1), 3.0f, dotCol);
        dl->AddCircleFilled(ImVec2(x2, y1), 3.0f, dotCol);
        dl->AddCircleFilled(ImVec2(x1, y2), 3.0f, dotCol);
        dl->AddCircleFilled(ImVec2(x2, y2), 3.0f, dotCol);
    }

    // 3. Ruta de Movimiento 3D (Motion Path) en el Viewport
    if (seq.isMotionPathVisible() && !seq.isPilotCineCamera() && !seq.getCameraTracks().empty()) {
        const auto& camTrack = seq.getCameraTracks()[0];
        if (!camTrack.muted && camTrack.keyframes.size() >= 2) {
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(viewW / viewH);

            auto worldToScreen = [&](const glm::vec3& wpos, ImVec2& outPt) -> bool {
                glm::vec4 clip = proj * view * glm::vec4(wpos, 1.0f);
                if (clip.w <= 0.05f) return false;
                glm::vec3 ndc = glm::vec3(clip) / clip.w;
                if (ndc.z < -1.0f || ndc.z > 1.0f) return false;
                outPt.x = pMin.x + (ndc.x * 0.5f + 0.5f) * viewW;
                outPt.y = pMin.y + (-ndc.y * 0.5f + 0.5f) * viewH;
                return true;
            };

            // Dibujar spline suave muestreando 40 puntos a lo largo de la duracion
            float dur = seq.getDuration();
            int sampleCount = 40;
            ImVec2 prevPt;
            bool hasPrev = false;

            for (int i = 0; i <= sampleCount; ++i) {
                float t = (float)i / (float)sampleCount * dur;
                glm::vec3 pos(0.0f), tgt(0.0f);
                float f_mm = 35.0f, fdist = 5.0f, ap = 2.8f, shInt = 0.0f, shFreq = 1.5f;
                if (camTrack.sample(t, pos, tgt, f_mm, fdist, ap, shInt, shFreq)) {
                    ImVec2 curPt;
                    if (worldToScreen(pos, curPt)) {
                        if (hasPrev) {
                            dl->AddLine(prevPt, curPt, IM_COL32(245, 205, 45, 200), 2.0f);
                        }
                        prevPt = curPt;
                        hasPrev = true;
                    } else {
                        hasPrev = false;
                    }
                }
            }

            // Dibujar marcadores en cada Keyframe
            for (size_t k = 0; k < camTrack.keyframes.size(); ++k) {
                const auto& kf = camTrack.keyframes[k];
                ImVec2 kfPt;
                if (worldToScreen(kf.position, kfPt)) {
                    UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, kfPt, 12.0f, IM_COL32(255, 230, 80, 255));
                    char kfLabel[32];
                    snprintf(kfLabel, sizeof(kfLabel), "K%zu (%.1fs)", k + 1, kf.time);
                    dl->AddText(ImVec2(kfPt.x + 8.0f, kfPt.y - 6.0f), IM_COL32(255, 255, 255, 220), kfLabel);
                }
            }

            // Dibujar cono / frustum de la camara en el tiempo actual
            glm::vec3 curPos(0.0f), curTgt(0.0f);
            float curFOV = 45.0f, curDist = 5.0f, curAp = 2.8f;
            if (seq.getCineCameraState(curPos, curTgt, curFOV, curDist, curAp)) {
                ImVec2 posPt, tgtPt;
                if (worldToScreen(curPos, posPt) && worldToScreen(curTgt, tgtPt)) {
                    dl->AddLine(posPt, tgtPt, IM_COL32(75, 210, 110, 180), 1.5f);
                    dl->AddCircleFilled(tgtPt, 3.5f, IM_COL32(75, 210, 110, 255));
                    dl->AddCircleFilled(posPt, 5.0f, IM_COL32(71, 114, 179, 255));
                    dl->AddCircle(posPt, 5.0f, IM_COL32(255, 255, 255, 200), 16, 1.2f);
                }
            }
        }
    }
}

// ============================================================================
// FASE 22: Panel Principal Sequencer & Cine (Timeline, Dope Sheet, Graph Editor)
// ============================================================================
void EditorUI::renderSequencerPanel(Scene& scene, Camera& camera, DayNightCycle* dayNight, ScriptEngine* scriptEngine) {
    if (ImGui::Begin("Sequencer & Cine (Fase 22)")) {
        auto& seq = Sequencer::get();
        float curTime = seq.getCurrentTime();
        float duration = seq.getDuration();
        int fps = seq.getFPS();
        int currentFrame = static_cast<int>(std::round(curTime * (float)fps));
        int totalFrames = static_cast<int>(std::round(duration * (float)fps));

        // ====================================================================
        // 1. Barra de Transporte Superior (26px)
        // ====================================================================
        if (UI::IconButton("seq_btn_first", UI::IconType::KEY_FIRST, ImVec2(24.0f, 22.0f), "Primer Fotograma [Inicio]")) {
            seq.setTime(0.0f);
        }
        ImGui::SameLine();
        if (UI::IconButton("seq_btn_prev", UI::IconType::KEY_PREV, ImVec2(24.0f, 22.0f), "Fotograma Anterior [<]")) {
            seq.setTime((std::max)(0.0f, curTime - 1.0f / (float)fps));
        }
        ImGui::SameLine();

        // Boton Play / Pause con colores dinamicos
        bool playing = seq.isPlaying();
        ImU32 playCol = playing ? IM_COL32(240, 180, 50, 255) : IM_COL32(75, 210, 110, 255);
        if (UI::IconButton("seq_btn_play", playing ? UI::IconType::PAUSE : UI::IconType::PLAY, ImVec2(28.0f, 22.0f), playing ? "Pausar [Espacio]" : "Reproducir [Espacio]", playCol)) {
            if (playing) seq.pause(); else seq.play();
        }
        ImGui::SameLine();

        if (UI::IconButton("seq_btn_stop", UI::IconType::STOP, ImVec2(24.0f, 22.0f), "Detener y Rebobinar a 0s")) {
            seq.stop();
        }
        ImGui::SameLine();
        if (UI::IconButton("seq_btn_next", UI::IconType::KEY_NEXT, ImVec2(24.0f, 22.0f), "Siguiente Fotograma [>]")) {
            seq.setTime((std::min)(duration, curTime + 1.0f / (float)fps));
        }
        ImGui::SameLine();

        // Auto-Keying Button (Rojo #E03C3C cuando activo)
        bool autoKey = seq.isAutoKeyEnabled();
        if (UI::IconToggle("seq_btn_autokey", UI::IconType::AUTO_KEY, autoKey, ImVec2(24.0f, 22.0f), "Auto-Keying: Graba claves automaticamente al mover camara u objetos", IM_COL32(224, 60, 60, 255))) {
            seq.setAutoKeyEnabled(!autoKey);
        }
        ImGui::SameLine();

        if (UI::ButtonWithIcon(UI::IconType::KEYFRAME, "+ Clave (I)", ImVec2(0.0f, 22.0f), "Insertar Keyframe en tiempo actual")) {
            m_showKeyframeInsertPopup = true;
        }
        ImGui::SameLine();

        // Separador vertical
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Alternador de Vista: Dope Sheet vs Graph Editor
        bool isDope = (seq.getViewMode() == SequencerViewMode::DOPE_SHEET);
        if (UI::IconToggle("seq_view_dope", UI::IconType::KEYFRAME, isDope, ImVec2(24.0f, 22.0f), "Modo Dope Sheet (Claves en rombos estilo Blender)")) {
            seq.setViewMode(SequencerViewMode::DOPE_SHEET);
        }
        ImGui::SameLine();
        if (UI::IconToggle("seq_view_graph", UI::IconType::CURVE_GRAPH, !isDope, ImVec2(24.0f, 22.0f), "Modo Graph Editor (Curvas F-Curve Bezier continuas)")) {
            seq.setViewMode(SequencerViewMode::GRAPH_EDITOR);
        }
        ImGui::SameLine();

        // Boton Pilotar Camara
        bool piloting = seq.isPilotCineCamera();
        if (UI::IconToggle("seq_tb_pilot", UI::IconType::CINE_CAMERA, piloting, ImVec2(24.0f, 22.0f), "Pilotar Camara de Cine en Viewport 3D", IM_COL32(71, 114, 179, 255))) {
            seq.setPilotCineCamera(!piloting);
        }
        ImGui::SameLine();

        // Boton Mascara Scope
        bool scope = seq.isAnamorphicScope();
        if (UI::IconToggle("seq_tb_scope", UI::IconType::CLAPPERBOARD, scope, ImVec2(24.0f, 22.0f), "Mascara Anamorfica 2.39:1 Scope")) {
            seq.setAnamorphicScope(!scope);
        }
        ImGui::SameLine();

        // Contador de Tiempo y Fotogramas (Pill elegante)
        int min = static_cast<int>(curTime) / 60;
        float sec = std::fmod(curTime, 60.0f);
        char timeBuf[64];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%05.2f (F%d / %d)", min, sec, currentFrame, totalFrames);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.85f, 1.0f, 1.0f));
        ImGui::TextUnformatted(timeBuf);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Control de Duracion
        ImGui::SetNextItemWidth(70.0f);
        float durVal = duration;
        if (ImGui::DragFloat("##seq_dur", &durVal, 0.5f, 1.0f, 300.0f, "%.1fs")) {
            seq.setDuration(durVal);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duracion total de la secuencia");
        ImGui::SameLine();

        // FPS (68px para evitar cortes como '30 fp')
        ImGui::SetNextItemWidth(68.0f);
        int curFPS = fps;
        const char* fpsItems[] = { "24 fps", "30 fps", "60 fps" };
        int fpsIdx = (curFPS == 24) ? 0 : (curFPS == 60 ? 2 : 1);
        if (ImGui::Combo("##seq_fps", &fpsIdx, fpsItems, IM_ARRAYSIZE(fpsItems))) {
            seq.setFPS((fpsIdx == 0) ? 24 : (fpsIdx == 2 ? 60 : 30));
        }
        ImGui::SameLine();

        // Loop
        bool loop = seq.isLooping();
        if (ImGui::Checkbox("Loop", &loop)) {
            seq.setLooping(loop);
        }
        ImGui::SameLine();

        // Presets de Cinematica en 1 Clic
        if (ImGui::Button("Presets Demo...")) {
            ImGui::OpenPopup("SeqDemoPresetsPopup");
        }
        if (ImGui::BeginPopup("SeqDemoPresetsPopup")) {
            if (ImGui::Selectable("Preset 1: Hero Shot 360 con Bokeh DoF")) {
                seq.createDemoSequence(0, scene, camera);
                if (scriptEngine) scriptEngine->addLog("[Sequencer] Preset 0 cargado: Hero Shot 360 con Bokeh DoF.");
            }
            if (ImGui::Selectable("Preset 2: Travelling Steadycam + Amanecer Rapido")) {
                seq.createDemoSequence(1, scene, camera);
                if (scriptEngine) scriptEngine->addLog("[Sequencer] Preset 1 cargado: Dolly In Steadycam + Amanecer Dorado.");
            }
            if (ImGui::Selectable("Preset 3: Secuencia Accion Multi-Corte (3 Tomas + Lua)")) {
                seq.createDemoSequence(2, scene, camera);
                if (scriptEngine) scriptEngine->addLog("[Sequencer] Preset 2 cargado: Accion Multi-Corte con eventos Lua.");
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();

        // Guardar / Cargar .asisseq
        if (ImGui::SmallButton("Guardar .asisseq")) {
            seq.saveToFile(m_sequenceFilePathBuf);
            if (scriptEngine) scriptEngine->addLog("[Sequencer] Secuencia guardada en: " + std::string(m_sequenceFilePathBuf));
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Cargar .asisseq")) {
            seq.loadFromFile(m_sequenceFilePathBuf);
            if (scriptEngine) scriptEngine->addLog("[Sequencer] Secuencia cargada desde: " + std::string(m_sequenceFilePathBuf));
        }
        ImGui::SameLine();

        // Boton Capturar / Bake IA & Lua (Fase 22.5)
        if (UI::ButtonWithIcon(UI::IconType::AUTO_KEY, "Bake IA & Lua", ImVec2(0.0f, 22.0f), "Capturar/Bake animaciones procedurales de Lua, IA y Fisica a Keyframes")) {
            seq.bakeScene(scene, 10.0f, 0.5f);
            if (scriptEngine) scriptEngine->addLog("[Sequencer] Animaciones de IA, Lua y Fisica capturadas en Keyframes (10s @ 0.5s).");
            setStatusBarNotification("Bake completado: Pistas y Keyframes generados en Sequencer");
        }

        // Sincronizar automáticamente objetos animados de la escena solo en el primer frame al iniciar el motor
        if (!m_initialSequencerSynced) {
            seq.syncSceneAnimationsToSequencer(scene);
            m_initialSequencerSynced = true;
        }

        ImGui::Separator();

        // ====================================================================
        // 2. Area Dividida: Lista de Pistas (Izquierda ~22%) y Linea de Tiempo (Derecha ~78%)
        // ====================================================================
        float totalWidth = ImGui::GetContentRegionAvail().x;
        float trackListWidth = glm::clamp(totalWidth * 0.22f, 180.0f, 280.0f);
        float timelineWidth = totalWidth - trackListWidth - 8.0f;
        float contentH = ImGui::GetContentRegionAvail().y;

        float trackListScrollY = 0.0f;
        // --- Panel Izquierdo: Lista de Pistas ---
        ImGui::BeginChild("SeqTrackListPanel", ImVec2(trackListWidth, contentH), true);
        {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Pistas");
            ImGui::SameLine();
            float availR = ImGui::GetContentRegionAvail().x;
            if (availR > 70.0f) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availR - 70.0f);
            }
            if (ImGui::SmallButton("+ Pista")) {
                ImGui::OpenPopup("AddSeqTrackPopup");
            }
            if (ImGui::BeginPopup("AddSeqTrackPopup")) {
                if (!m_selectedEntityName.empty()) {
                    std::string addEntLabel = "Entidad: " + m_selectedEntityName;
                    if (ImGui::Selectable(addEntLabel.c_str())) {
                        seq.addEntityTrack(m_selectedEntityName);
                    }
                }
                if (ImGui::Selectable("Camara de Cine")) {
                    seq.addCameraTrack("Camara " + std::to_string(seq.getCameraTracks().size() + 1));
                }
                if (ImGui::Selectable("Entorno y Sol 24h")) {
                    seq.addEnvironmentTrack("Entorno Dinamico");
                }
                if (ImGui::Selectable("Pista de Eventos Lua")) {
                    seq.addLuaTrack("Eventos Gameplay");
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            // Altura fija de cada fila de pista
            const float trackRowH = 28.0f;

            // Fila de encabezado vacia para emparejar con la regla de tiempo (22px)
            ImGui::Dummy(ImVec2(0.0f, 22.0f));
            ImGui::Separator();

            // Listar Pistas de Camara
            for (size_t t = 0; t < seq.getCameraTracks().size(); ++t) {
                auto& trk = seq.getCameraTracks()[t];
                ImGui::PushID(static_cast<int>(t * 100 + 1));
                ImVec2 rowStart = ImGui::GetCursorScreenPos();
                ImVec2 iconCenter(rowStart.x + 10.0f, rowStart.y + trackRowH * 0.5f);
                UI::DrawMiniIcon(ImGui::GetWindowDrawList(), UI::IconType::CINE_CAMERA, iconCenter, 14.0f, IM_COL32(71, 114, 179, 255));
                ImGui::SetCursorScreenPos(ImVec2(rowStart.x + 22.0f, rowStart.y + 4.0f));
                ImGui::TextUnformatted(trk.name.c_str());

                ImGui::SameLine(trackListWidth - 52.0f);
                if (ImGui::SmallButton(trk.muted ? "[M]" : " M ")) trk.muted = !trk.muted;
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) { seq.removeCameraTrack(t); ImGui::PopID(); break; }

                ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + trackRowH));
                ImGui::Separator();
                ImGui::PopID();
            }

            // Listar Pistas de Entorno
            for (size_t t = 0; t < seq.getEnvironmentTracks().size(); ++t) {
                auto& trk = seq.getEnvironmentTracks()[t];
                ImGui::PushID(static_cast<int>(t * 100 + 20));
                ImVec2 rowStart = ImGui::GetCursorScreenPos();
                ImVec2 iconCenter(rowStart.x + 10.0f, rowStart.y + trackRowH * 0.5f);
                UI::DrawMiniIcon(ImGui::GetWindowDrawList(), UI::IconType::SUN, iconCenter, 14.0f, IM_COL32(245, 180, 40, 255));
                ImGui::SetCursorScreenPos(ImVec2(rowStart.x + 22.0f, rowStart.y + 4.0f));
                ImGui::TextUnformatted(trk.name.c_str());

                ImGui::SameLine(trackListWidth - 52.0f);
                if (ImGui::SmallButton(trk.muted ? "[M]" : " M ")) trk.muted = !trk.muted;
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) { seq.removeEnvironmentTrack(t); ImGui::PopID(); break; }

                ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + trackRowH));
                ImGui::Separator();
                ImGui::PopID();
            }

            // Listar Pistas de Entidades 3D
            const float subRowH = 20.0f;
            for (size_t t = 0; t < seq.getEntityTracks().size(); ++t) {
                auto& trk = seq.getEntityTracks()[t];
                ImGui::PushID(static_cast<int>(t * 100 + 40));
                ImVec2 rowStart = ImGui::GetCursorScreenPos();

                // Flecha desplegable estilo Blender
                ImGui::SetCursorScreenPos(ImVec2(rowStart.x + 2.0f, rowStart.y + 3.0f));
                char expBtnId[32];
                snprintf(expBtnId, sizeof(expBtnId), "##exp_%zu", t);
                if (ImGui::SmallButton(trk.expanded ? "v" : ">")) {
                    trk.expanded = !trk.expanded;
                }

                ImVec2 iconCenter(rowStart.x + 28.0f, rowStart.y + trackRowH * 0.5f);
                UI::DrawMiniIcon(ImGui::GetWindowDrawList(), UI::IconType::PHYSICS, iconCenter, 14.0f, IM_COL32(60, 210, 110, 255));
                ImGui::SetCursorScreenPos(ImVec2(rowStart.x + 40.0f, rowStart.y + 4.0f));
                ImGui::TextUnformatted(trk.entityName.c_str());

                ImGui::SameLine(trackListWidth - 52.0f);
                if (ImGui::SmallButton(trk.muted ? "[M]" : " M ")) trk.muted = !trk.muted;
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) { seq.removeEntityTrack(t); ImGui::PopID(); break; }

                ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + trackRowH));
                ImGui::Separator();

                // Sub-canales si está expandida
                if (trk.expanded) {
                    auto drawSubChannelLabel = [&](const char* subLabel, ImU32 col) {
                        ImVec2 subPos = ImGui::GetCursorScreenPos();
                        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(subPos.x + 12.0f, subPos.y + 4.0f), ImVec2(subPos.x + 15.0f, subPos.y + subRowH - 4.0f), col);
                        ImGui::SetCursorScreenPos(ImVec2(subPos.x + 20.0f, subPos.y + 2.0f));
                        ImGui::TextDisabled("%s", subLabel);
                        ImGui::SetCursorScreenPos(ImVec2(subPos.x, subPos.y + subRowH));
                        ImGui::Separator();
                    };
                    drawSubChannelLabel("Posicion (Location)", IM_COL32(224, 60, 60, 255));
                    drawSubChannelLabel("Rotacion (Rotation)", IM_COL32(71, 179, 71, 255));
                    drawSubChannelLabel("Escala (Scale)",       IM_COL32(71, 114, 179, 255));
                }

                ImGui::PopID();
            }

            // Listar Pistas de Eventos Lua
            for (size_t t = 0; t < seq.getLuaTracks().size(); ++t) {
                auto& trk = seq.getLuaTracks()[t];
                ImGui::PushID(static_cast<int>(t * 100 + 60));
                ImVec2 rowStart = ImGui::GetCursorScreenPos();
                ImVec2 iconCenter(rowStart.x + 10.0f, rowStart.y + trackRowH * 0.5f);
                UI::DrawMiniIcon(ImGui::GetWindowDrawList(), UI::IconType::SPARK, iconCenter, 14.0f, IM_COL32(220, 80, 220, 255));
                ImGui::SetCursorScreenPos(ImVec2(rowStart.x + 22.0f, rowStart.y + 4.0f));
                ImGui::TextUnformatted(trk.name.c_str());

                ImGui::SameLine(trackListWidth - 52.0f);
                if (ImGui::SmallButton(trk.muted ? "[M]" : " M ")) trk.muted = !trk.muted;
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) { seq.removeLuaTrack(t); ImGui::PopID(); break; }

                ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + trackRowH));
                ImGui::Separator();
                ImGui::PopID();
            }
            trackListScrollY = ImGui::GetScrollY();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // --- Panel Derecho: Linea de Tiempo (Ruler, Dope Sheet / Graph, Playhead) ---
        ImGui::BeginChild("SeqTimelineCanvasPanel", ImVec2(timelineWidth, contentH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 canvasMin = ImGui::GetCursorScreenPos();
            ImVec2 canvasMax(canvasMin.x + timelineWidth, canvasMin.y + contentH);
            ImGuiIO& io = ImGui::GetIO();

            // Fondo del lienzo
            dl->AddRectFilled(canvasMin, canvasMax, IM_COL32(22, 24, 28, 255));

            bool isHovered = ImGui::IsWindowHovered();

            // Desplazamiento horizontal con Boton Central (MMB drag) o Shift + Rueda
            if (isHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                m_sequencerScrollX -= io.MouseDelta.x;
                if (m_sequencerScrollX < 0.0f) m_sequencerScrollX = 0.0f;
            }
            if (isHovered && io.KeyShift && io.MouseWheel != 0.0f) {
                m_sequencerScrollX -= io.MouseWheel * 30.0f;
                if (m_sequencerScrollX < 0.0f) m_sequencerScrollX = 0.0f;
            }

            // Zoom horizontal con rueda del raton si se presiona Ctrl
            if (isHovered && io.KeyCtrl && io.MouseWheel != 0.0f) {
                m_sequencerTimeScale = glm::clamp(m_sequencerTimeScale + io.MouseWheel * 10.0f, 20.0f, 300.0f);
            }

            float timeScale = m_sequencerTimeScale;
            float rulerHeight = 22.0f;
            const float trackRowH = 28.0f;
            float canvasOriginX = canvasMin.x - m_sequencerScrollX;

            // Interaccion: Scrubbing solo en la regla superior o si ya se estaba arrastrando
            bool onRuler = (isHovered && io.MousePos.y >= canvasMin.y && io.MousePos.y <= canvasMin.y + rulerHeight);
            if ((onRuler && io.MouseDown[0]) || m_isScrubbingPlayhead) {
                float mouseX = io.MousePos.x - canvasOriginX;
                if (mouseX >= 0.0f) {
                    float newT = glm::clamp(mouseX / timeScale, 0.0f, duration);
                    seq.setTime(newT);
                    m_isScrubbingPlayhead = io.MouseDown[0];
                }
            } else if (!io.MouseDown[0]) {
                m_isScrubbingPlayhead = false;
            }

            // Shortcuts de Teclado estilo Blender dentro del Sequencer (Fase 22.5)
            if (isHovered || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_D)) {
                    seq.duplicateSelectedKeyframes(0.5f);
                    seq.requestSample();
                    setStatusBarNotification("Keyframes duplicados (+0.5s)");
                } else if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_X)) {
                    seq.deleteSelectedKeyframes();
                    seq.requestSample();
                    setStatusBarNotification("Keyframes seleccionados eliminados");
                } else if (ImGui::IsKeyPressed(ImGuiKey_G)) {
                    m_seqIsMovingKeys = !m_seqIsMovingKeys;
                    m_seqMoveKeysLastMouseX = io.MousePos.x;
                    if (m_seqIsMovingKeys) setStatusBarNotification("Mover Keyframes [G]: Desplaza el raton y haz clic para fijar");
                    else {
                        seq.sortAllKeyframes();
                        seq.requestSample();
                        setStatusBarNotification("Keyframes fijados");
                    }
                } else if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                    ImGui::OpenPopup("SeqInterpModePopup");
                } else if (ImGui::IsKeyPressed(ImGuiKey_A)) {
                    if (seq.hasSelectedKeyframes()) seq.clearSelection();
                    else seq.selectAllKeyframes(true);
                }
            }

            if (ImGui::BeginPopup("SeqInterpModePopup")) {
                ImGui::TextDisabled("Modo de Interpolacion (Blender):");
                if (ImGui::MenuItem("Curva Suave Bezier (Hermite / S-Curve)")) {
                    seq.setSelectedKeyframesInterp(InterpolationMode::CUBIC_BEZIER);
                    seq.requestSample();
                    setStatusBarNotification("Interpolacion: Curva Bezier Suave");
                }
                if (ImGui::MenuItem("Lineal (Linear)")) {
                    seq.setSelectedKeyframesInterp(InterpolationMode::LINEAR);
                    seq.requestSample();
                    setStatusBarNotification("Interpolacion: Lineal");
                }
                if (ImGui::MenuItem("Constante (Step / Mantener)")) {
                    seq.setSelectedKeyframesInterp(InterpolationMode::CONSTANT);
                    seq.requestSample();
                    setStatusBarNotification("Interpolacion: Constante / Step");
                }
                ImGui::EndPopup();
            }

            // Menu contextual / edicion numerica al hacer clic derecho en un keyframe
            if (ImGui::BeginPopup("SeqKeyframeContextPopup")) {
                if (m_selectedSeqTrackType == 0 && m_selectedSeqTrackIndex >= 0 && m_selectedSeqTrackIndex < (int)seq.getEntityTracks().size()) {
                    auto& trk = seq.getEntityTracks()[m_selectedSeqTrackIndex];
                    if (m_selectedSeqKeyIndex >= 0 && m_selectedSeqKeyIndex < (int)trk.keyframes.size()) {
                        auto& kf = trk.keyframes[m_selectedSeqKeyIndex];
                        ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "Edicion Keyframe: %s", trk.entityName.c_str());
                        ImGui::Separator();
                        float kTime = kf.time;
                        if (ImGui::DragFloat("Tiempo (s)", &kTime, 0.05f, 0.0f, duration, "%.2f s")) {
                            kf.time = glm::clamp(kTime, 0.0f, duration);
                            seq.requestSample();
                        }

                        int interpIdx = static_cast<int>(kf.interp);
                        const char* interpNames[] = { "Lineal", "Constante (Step)", "Curva Bezier Suave" };
                        if (ImGui::Combo("Interpolacion", &interpIdx, interpNames, 3)) {
                            kf.interp = static_cast<InterpolationMode>(interpIdx);
                            seq.requestSample();
                        }

                        if (ImGui::DragFloat3("Posicion", &kf.position.x, 0.05f)) {
                            seq.requestSample();
                        }

                        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(kf.rotation));
                        if (ImGui::DragFloat3("Rotacion (Euler)", &eulerDeg.x, 0.5f)) {
                            glm::vec3 rad = glm::radians(eulerDeg);
                            glm::mat4 R = glm::rotate(glm::mat4(1.0f), rad.y, glm::vec3(0, 1, 0)) *
                                         glm::rotate(glm::mat4(1.0f), rad.x, glm::vec3(1, 0, 0)) *
                                         glm::rotate(glm::mat4(1.0f), rad.z, glm::vec3(0, 0, 1));
                            kf.rotation = glm::quat_cast(R);
                            seq.requestSample();
                        }

                        if (ImGui::DragFloat3("Escala", &kf.scale.x, 0.05f, 0.001f, 100.0f)) {
                            seq.requestSample();
                        }

                        ImGui::Separator();
                        auto entPtr = scene.getEntity(trk.entityName);
                        if (entPtr) {
                            if (ImGui::Button("Capturar Pose Actual")) {
                                entPtr->syncQuaternionFromEuler();
                                kf.position = entPtr->getPosition();
                                kf.rotation = entPtr->getOrientation();
                                kf.scale = entPtr->getScale();
                                seq.requestSample();
                                setStatusBarNotification("Pose actual capturada en el keyframe");
                            }
                        }
                        if (ImGui::Button("Duplicar Keyframe")) {
                            trk.duplicateSelectedKeyframes(0.5f);
                            seq.requestSample();
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Eliminar Keyframe")) {
                            trk.removeKeyframe(m_selectedSeqKeyIndex);
                            m_selectedSeqKeyIndex = -1;
                            seq.requestSample();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                } else if (m_selectedSeqTrackType == 1 && m_selectedSeqTrackIndex >= 0 && m_selectedSeqTrackIndex < (int)seq.getCameraTracks().size()) {
                    auto& trk = seq.getCameraTracks()[m_selectedSeqTrackIndex];
                    if (m_selectedSeqKeyIndex >= 0 && m_selectedSeqKeyIndex < (int)trk.keyframes.size()) {
                        auto& kf = trk.keyframes[m_selectedSeqKeyIndex];
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Edicion Keyframe: %s", trk.name.c_str());
                        ImGui::Separator();
                        float kTime = kf.time;
                        if (ImGui::DragFloat("Tiempo (s)", &kTime, 0.05f, 0.0f, duration, "%.2f s")) {
                            kf.time = glm::clamp(kTime, 0.0f, duration);
                            seq.requestSample();
                        }
                        if (ImGui::DragFloat3("Posicion Camara", &kf.position.x, 0.05f)) seq.requestSample();
                        if (ImGui::DragFloat3("Target LookAt", &kf.target.x, 0.05f)) seq.requestSample();
                        if (ImGui::DragFloat("Lente (mm)", &kf.focalLength_mm, 0.5f, 12.0f, 300.0f)) seq.requestSample();
                        if (ImGui::DragFloat("Distancia Enfoque (m)", &kf.focusDistance, 0.1f, 0.1f, 100.0f)) seq.requestSample();
                        if (ImGui::DragFloat("Apertura (f-stop)", &kf.aperture_fstop, 0.1f, 1.0f, 22.0f)) seq.requestSample();
                        if (ImGui::DragFloat("Vibracion Shake", &kf.shakeIntensity, 0.01f, 0.0f, 1.0f)) seq.requestSample();
                        if (ImGui::Button("Capturar Camara Actual")) {
                            kf.position = camera.getPosition();
                            kf.target = camera.getPosition() + camera.getFront() * 5.0f;
                            seq.requestSample();
                            setStatusBarNotification("Camara actual capturada en el keyframe");
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Eliminar Keyframe")) {
                            trk.removeKeyframe(m_selectedSeqKeyIndex);
                            m_selectedSeqKeyIndex = -1;
                            seq.requestSample();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndPopup();
            }

            // Desplazamiento activo con G o arrastre
            if (m_seqIsMovingKeys && seq.hasSelectedKeyframes()) {
                float dx = io.MousePos.x - m_seqMoveKeysLastMouseX;
                if (std::abs(dx) > 0.5f) {
                    float dt = dx / timeScale;
                    seq.moveSelectedKeyframes(dt);
                    m_seqMoveKeysLastMouseX = io.MousePos.x;
                    seq.requestSample();
                }
                if (ImGui::IsMouseClicked(0) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    m_seqIsMovingKeys = false;
                    seq.sortAllKeyframes();
                    seq.requestSample();
                    setStatusBarNotification("Keyframes fijados");
                }
            }

            // Arrastre interactivo con clic izquierdo mantenido (LMB Drag)
            if (m_draggingWithMouseHold) {
                if (io.MouseDown[0]) {
                    float newTime = (io.MousePos.x - canvasOriginX) / timeScale;
                    newTime = glm::clamp(newTime, 0.0f, duration);
                    if (m_draggedKeyTrackType == 0 && m_draggedKeyTrackIndex >= 0 && m_draggedKeyTrackIndex < (int)seq.getEntityTracks().size()) {
                        auto& trk = seq.getEntityTracks()[m_draggedKeyTrackIndex];
                        if (m_draggedKeyIndex >= 0 && m_draggedKeyIndex < (int)trk.keyframes.size()) {
                            trk.keyframes[m_draggedKeyIndex].time = newTime;
                            seq.requestSample();
                        }
                    } else if (m_draggedKeyTrackType == 1 && m_draggedKeyTrackIndex >= 0 && m_draggedKeyTrackIndex < (int)seq.getCameraTracks().size()) {
                        auto& trk = seq.getCameraTracks()[m_draggedKeyTrackIndex];
                        if (m_draggedKeyIndex >= 0 && m_draggedKeyIndex < (int)trk.keyframes.size()) {
                            trk.keyframes[m_draggedKeyIndex].time = newTime;
                            seq.requestSample();
                        }
                    } else if (m_draggedKeyTrackType == 2 && m_draggedKeyTrackIndex >= 0 && m_draggedKeyTrackIndex < (int)seq.getEnvironmentTracks().size()) {
                        auto& trk = seq.getEnvironmentTracks()[m_draggedKeyTrackIndex];
                        if (m_draggedKeyIndex >= 0 && m_draggedKeyIndex < (int)trk.keyframes.size()) {
                            trk.keyframes[m_draggedKeyIndex].time = newTime;
                            seq.requestSample();
                        }
                    } else if (m_draggedKeyTrackType == 3 && m_draggedKeyTrackIndex >= 0 && m_draggedKeyTrackIndex < (int)seq.getLuaTracks().size()) {
                        auto& trk = seq.getLuaTracks()[m_draggedKeyTrackIndex];
                        if (m_draggedKeyIndex >= 0 && m_draggedKeyIndex < (int)trk.keyframes.size()) {
                            trk.keyframes[m_draggedKeyIndex].time = newTime;
                            seq.requestSample();
                        }
                    }
                } else {
                    m_draggingWithMouseHold = false;
                    m_draggedKeyTrackType = -1;
                    m_draggedKeyTrackIndex = -1;
                    m_draggedKeyIndex = -1;
                    seq.sortAllKeyframes();
                    seq.requestSample();
                }
            }

            // Region de recorte para las pistas y curvas (debajo de la regla superior)
            dl->PushClipRect(ImVec2(canvasMin.x, canvasMin.y + rulerHeight), canvasMax, true);

            // Lineas verticales de cuadricula de tiempo
            int maxSec = static_cast<int>(std::ceil(duration)) + 2;
            for (int s = 0; s <= maxSec; ++s) {
                float x = canvasOriginX + s * timeScale;
                if (x >= canvasMin.x - 2.0f && x <= canvasMax.x + 2.0f) {
                    dl->AddLine(ImVec2(x, canvasMin.y + rulerHeight), ImVec2(x, canvasMax.y), IM_COL32(40, 44, 52, 255), 1.0f);
                    if (timeScale >= 40.0f) {
                        for (int f = 1; f < fps; ++f) {
                            float fx = x + ((float)f / (float)fps) * timeScale;
                            if (fx > canvasMax.x) break;
                            if (fx >= canvasMin.x) {
                                dl->AddLine(ImVec2(fx, canvasMin.y + rulerHeight), ImVec2(fx, canvasMin.y + rulerHeight + 6.0f), IM_COL32(50, 55, 65, 255), 1.0f);
                            }
                        }
                    }
                }
            }

            // Renderizado segun el modo de visualizacion (Dope Sheet vs Graph Editor)
            if (seq.getViewMode() == SequencerViewMode::DOPE_SHEET) {
                bool clickedAnyKey = false;
                float currentY = canvasMin.y + rulerHeight - trackListScrollY;

                // Dibujar Pistas de Camara
                for (size_t t = 0; t < seq.getCameraTracks().size(); ++t) {
                    auto& trk = seq.getCameraTracks()[t];
                    if (currentY + trackRowH >= canvasMin.y + rulerHeight && currentY <= canvasMax.y) {
                        dl->AddRectFilled(ImVec2(canvasMin.x, currentY), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(26, 28, 34, 160));
                        dl->AddLine(ImVec2(canvasMin.x, currentY + trackRowH), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(40, 44, 52, 255), 1.0f);
                        float centerY = currentY + trackRowH * 0.5f;

                        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
                            auto& kf = trk.keyframes[k];
                            float kx = canvasOriginX + kf.time * timeScale;
                            if (kx >= canvasMin.x - 10.0f && kx <= canvasMax.x + 10.0f) {
                                float dist = std::hypot(io.MousePos.x - kx, io.MousePos.y - centerY);
                                bool hov = (isHovered && dist <= 8.0f && io.MousePos.y > canvasMin.y + rulerHeight);
                                if (hov && ImGui::IsMouseClicked(0)) {
                                    clickedAnyKey = true;
                                    if (ImGui::IsMouseDoubleClicked(0)) {
                                        seq.setTime(kf.time);
                                    } else {
                                        m_draggingWithMouseHold = true;
                                        m_draggedKeyTrackType = 1;
                                        m_draggedKeyTrackIndex = (int)t;
                                        m_draggedKeyIndex = (int)k;
                                        if (!io.KeyShift && !io.KeyCtrl) seq.clearSelection();
                                        kf.selected = true;
                                    }
                                }
                                if (hov && ImGui::IsMouseClicked(1)) {
                                    m_selectedSeqTrackType = 1;
                                    m_selectedSeqTrackIndex = (int)t;
                                    m_selectedSeqKeyIndex = (int)k;
                                    ImGui::OpenPopup("SeqKeyframeContextPopup");
                                }
                                ImU32 kCol = kf.selected ? IM_COL32(255, 235, 100, 255) : (hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(245, 215, 60, 255));
                                UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, ImVec2(kx, centerY), 11.0f, kCol);
                            }
                        }
                    }
                    currentY += trackRowH;
                }

                // Dibujar Pistas de Entorno
                for (size_t t = 0; t < seq.getEnvironmentTracks().size(); ++t) {
                    auto& trk = seq.getEnvironmentTracks()[t];
                    if (currentY + trackRowH >= canvasMin.y + rulerHeight && currentY <= canvasMax.y) {
                        dl->AddRectFilled(ImVec2(canvasMin.x, currentY), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(26, 28, 34, 160));
                        dl->AddLine(ImVec2(canvasMin.x, currentY + trackRowH), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(40, 44, 52, 255), 1.0f);
                        float centerY = currentY + trackRowH * 0.5f;

                        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
                            auto& kf = trk.keyframes[k];
                            float kx = canvasOriginX + kf.time * timeScale;
                            if (kx >= canvasMin.x - 10.0f && kx <= canvasMax.x + 10.0f) {
                                float dist = std::hypot(io.MousePos.x - kx, io.MousePos.y - centerY);
                                bool hov = (isHovered && dist <= 8.0f && io.MousePos.y > canvasMin.y + rulerHeight);
                                if (hov && ImGui::IsMouseClicked(0)) {
                                    clickedAnyKey = true;
                                    if (ImGui::IsMouseDoubleClicked(0)) {
                                        seq.setTime(kf.time);
                                    } else {
                                        m_draggingWithMouseHold = true;
                                        m_draggedKeyTrackType = 2;
                                        m_draggedKeyTrackIndex = (int)t;
                                        m_draggedKeyIndex = (int)k;
                                        if (!io.KeyShift && !io.KeyCtrl) seq.clearSelection();
                                        kf.selected = true;
                                    }
                                }
                                if (hov && ImGui::IsMouseClicked(1)) {
                                    m_selectedSeqTrackType = 2;
                                    m_selectedSeqTrackIndex = (int)t;
                                    m_selectedSeqKeyIndex = (int)k;
                                    ImGui::OpenPopup("SeqKeyframeContextPopup");
                                }
                                ImU32 kCol = kf.selected ? IM_COL32(255, 235, 100, 255) : (hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(245, 160, 40, 255));
                                UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, ImVec2(kx, centerY), 11.0f, kCol);
                            }
                        }
                    }
                    currentY += trackRowH;
                }

                // Dibujar Pistas de Entidades 3D (con sub-canales y Hold Bars - Fase 22.5)
                for (size_t t = 0; t < seq.getEntityTracks().size(); ++t) {
                    auto& trk = seq.getEntityTracks()[t];
                    if (currentY + trackRowH >= canvasMin.y + rulerHeight && currentY <= canvasMax.y) {
                        dl->AddRectFilled(ImVec2(canvasMin.x, currentY), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(26, 28, 34, 160));
                        dl->AddLine(ImVec2(canvasMin.x, currentY + trackRowH), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(40, 44, 52, 255), 1.0f);
                        float centerY = currentY + trackRowH * 0.5f;

                        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
                            auto& kf = trk.keyframes[k];
                            float kx = canvasOriginX + kf.time * timeScale;
                            if (kx >= canvasMin.x - 10.0f && kx <= canvasMax.x + 10.0f) {
                                float dist = std::hypot(io.MousePos.x - kx, io.MousePos.y - centerY);
                                bool hov = (isHovered && dist <= 8.0f && io.MousePos.y > canvasMin.y + rulerHeight);
                                if (hov && ImGui::IsMouseClicked(0)) {
                                    clickedAnyKey = true;
                                    if (ImGui::IsMouseDoubleClicked(0)) {
                                        seq.setTime(kf.time);
                                        selectEntity(trk.entityName);
                                    } else {
                                        m_draggingWithMouseHold = true;
                                        m_draggedKeyTrackType = 0;
                                        m_draggedKeyTrackIndex = (int)t;
                                        m_draggedKeyIndex = (int)k;
                                        if (!io.KeyShift && !io.KeyCtrl) seq.clearSelection();
                                        kf.selected = true;
                                    }
                                }
                                if (hov && ImGui::IsMouseClicked(1)) {
                                    m_selectedSeqTrackType = 0;
                                    m_selectedSeqTrackIndex = (int)t;
                                    m_selectedSeqKeyIndex = (int)k;
                                    ImGui::OpenPopup("SeqKeyframeContextPopup");
                                }
                                ImU32 kCol = kf.selected ? IM_COL32(255, 235, 100, 255) : (hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(80, 210, 110, 255));
                                UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, ImVec2(kx, centerY), 11.0f, kCol);
                            }
                        }
                    }
                    currentY += trackRowH;

                    // Sub-canales si esta expandida
                    if (trk.expanded) {
                        const float subRowH = 20.0f;

                        // Sub-canal Posicion (Rojo)
                        if (currentY + subRowH >= canvasMin.y + rulerHeight && currentY <= canvasMax.y) {
                            dl->AddRectFilled(ImVec2(canvasMin.x, currentY), ImVec2(canvasMax.x, currentY + subRowH), IM_COL32(20, 22, 26, 200));
                            dl->AddLine(ImVec2(canvasMin.x, currentY + subRowH), ImVec2(canvasMax.x, currentY + subRowH), IM_COL32(34, 38, 46, 255), 1.0f);

                            for (size_t i = 0; i + 1 < trk.keyframes.size(); ++i) {
                                if (glm::distance(trk.keyframes[i].position, trk.keyframes[i + 1].position) < 0.001f) {
                                    float x1 = canvasOriginX + trk.keyframes[i].time * timeScale;
                                    float x2 = canvasOriginX + trk.keyframes[i + 1].time * timeScale;
                                    if (x2 >= canvasMin.x && x1 <= canvasMax.x) {
                                        float cx1 = (std::max)(canvasMin.x, x1);
                                        float cx2 = (std::min)(canvasMax.x, x2);
                                        dl->AddRectFilled(ImVec2(cx1, currentY + 8.0f), ImVec2(cx2, currentY + 12.0f), IM_COL32(220, 160, 50, 150));
                                    }
                                }
                            }
                            for (const auto& kf : trk.keyframes) {
                                float kx = canvasOriginX + kf.time * timeScale;
                                if (kx >= canvasMin.x && kx <= canvasMax.x) {
                                    ImU32 col = kf.selected ? IM_COL32(255, 235, 100, 255) : IM_COL32(224, 60, 60, 255);
                                    UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, ImVec2(kx, currentY + subRowH * 0.5f), 9.0f, col);
                                }
                            }
                        }
                        currentY += subRowH;

                        // Sub-canal Rotacion (Verde)
                        if (currentY + subRowH >= canvasMin.y + rulerHeight && currentY <= canvasMax.y) {
                            dl->AddRectFilled(ImVec2(canvasMin.x, currentY), ImVec2(canvasMax.x, currentY + subRowH), IM_COL32(20, 22, 26, 200));
                            dl->AddLine(ImVec2(canvasMin.x, currentY + subRowH), ImVec2(canvasMax.x, currentY + subRowH), IM_COL32(34, 38, 46, 255), 1.0f);

                            for (size_t i = 0; i + 1 < trk.keyframes.size(); ++i) {
                                if (std::abs(glm::dot(trk.keyframes[i].rotation, trk.keyframes[i + 1].rotation)) > 0.9999f) {
                                    float x1 = canvasOriginX + trk.keyframes[i].time * timeScale;
                                    float x2 = canvasOriginX + trk.keyframes[i + 1].time * timeScale;
                                    if (x2 >= canvasMin.x && x1 <= canvasMax.x) {
                                        float cx1 = (std::max)(canvasMin.x, x1);
                                        float cx2 = (std::min)(canvasMax.x, x2);
                                        dl->AddRectFilled(ImVec2(cx1, currentY + 8.0f), ImVec2(cx2, currentY + 12.0f), IM_COL32(220, 160, 50, 150));
                                    }
                                }
                            }
                            for (const auto& kf : trk.keyframes) {
                                float kx = canvasOriginX + kf.time * timeScale;
                                if (kx >= canvasMin.x && kx <= canvasMax.x) {
                                    ImU32 col = kf.selected ? IM_COL32(255, 235, 100, 255) : IM_COL32(71, 179, 71, 255);
                                    UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, ImVec2(kx, currentY + subRowH * 0.5f), 9.0f, col);
                                }
                            }
                        }
                        currentY += subRowH;

                        // Sub-canal Escala (Azul)
                        if (currentY + subRowH >= canvasMin.y + rulerHeight && currentY <= canvasMax.y) {
                            dl->AddRectFilled(ImVec2(canvasMin.x, currentY), ImVec2(canvasMax.x, currentY + subRowH), IM_COL32(20, 22, 26, 200));
                            dl->AddLine(ImVec2(canvasMin.x, currentY + subRowH), ImVec2(canvasMax.x, currentY + subRowH), IM_COL32(34, 38, 46, 255), 1.0f);

                            for (size_t i = 0; i + 1 < trk.keyframes.size(); ++i) {
                                if (glm::distance(trk.keyframes[i].scale, trk.keyframes[i + 1].scale) < 0.001f) {
                                    float x1 = canvasOriginX + trk.keyframes[i].time * timeScale;
                                    float x2 = canvasOriginX + trk.keyframes[i + 1].time * timeScale;
                                    if (x2 >= canvasMin.x && x1 <= canvasMax.x) {
                                        float cx1 = (std::max)(canvasMin.x, x1);
                                        float cx2 = (std::min)(canvasMax.x, x2);
                                        dl->AddRectFilled(ImVec2(cx1, currentY + 8.0f), ImVec2(cx2, currentY + 12.0f), IM_COL32(220, 160, 50, 150));
                                    }
                                }
                            }
                            for (const auto& kf : trk.keyframes) {
                                float kx = canvasOriginX + kf.time * timeScale;
                                if (kx >= canvasMin.x && kx <= canvasMax.x) {
                                    ImU32 col = kf.selected ? IM_COL32(255, 235, 100, 255) : IM_COL32(71, 114, 179, 255);
                                    UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, ImVec2(kx, currentY + subRowH * 0.5f), 9.0f, col);
                                }
                            }
                        }
                        currentY += subRowH;
                    }
                }

                // Dibujar Pistas de Lua
                for (size_t t = 0; t < seq.getLuaTracks().size(); ++t) {
                    auto& trk = seq.getLuaTracks()[t];
                    if (currentY + trackRowH >= canvasMin.y + rulerHeight && currentY <= canvasMax.y) {
                        dl->AddRectFilled(ImVec2(canvasMin.x, currentY), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(26, 28, 34, 160));
                        dl->AddLine(ImVec2(canvasMin.x, currentY + trackRowH), ImVec2(canvasMax.x, currentY + trackRowH), IM_COL32(40, 44, 52, 255), 1.0f);
                        float centerY = currentY + trackRowH * 0.5f;

                        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
                            auto& kf = trk.keyframes[k];
                            float kx = canvasOriginX + kf.time * timeScale;
                            if (kx >= canvasMin.x - 10.0f && kx <= canvasMax.x + 10.0f) {
                                float dist = std::hypot(io.MousePos.x - kx, io.MousePos.y - centerY);
                                bool hov = (isHovered && dist <= 8.0f && io.MousePos.y > canvasMin.y + rulerHeight);
                                if (hov && ImGui::IsMouseClicked(0)) {
                                    clickedAnyKey = true;
                                    if (ImGui::IsMouseDoubleClicked(0)) {
                                        seq.setTime(kf.time);
                                    } else {
                                        m_draggingWithMouseHold = true;
                                        m_draggedKeyTrackType = 3;
                                        m_draggedKeyTrackIndex = (int)t;
                                        m_draggedKeyIndex = (int)k;
                                        if (!io.KeyShift && !io.KeyCtrl) seq.clearSelection();
                                        kf.selected = true;
                                    }
                                }
                                if (hov && ImGui::IsMouseClicked(1)) {
                                    m_selectedSeqTrackType = 3;
                                    m_selectedSeqTrackIndex = (int)t;
                                    m_selectedSeqKeyIndex = (int)k;
                                    ImGui::OpenPopup("SeqKeyframeContextPopup");
                                }
                                ImU32 kCol = kf.selected ? IM_COL32(255, 235, 100, 255) : (hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(220, 80, 220, 255));
                                UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, ImVec2(kx, centerY), 11.0f, kCol);
                            }
                        }
                    }
                    currentY += trackRowH;
                }

                // Box Selection (arrastre en fondo de lienzo)
                if (isHovered && io.MousePos.y > canvasMin.y + rulerHeight) {
                    if (ImGui::IsMouseClicked(0) && !clickedAnyKey && !m_seqIsMovingKeys && !m_draggingWithMouseHold) {
                        m_seqIsBoxSelecting = true;
                        m_seqBoxSelectStart = io.MousePos;
                        if (!io.KeyShift && !io.KeyCtrl) seq.clearSelection();
                    }
                }

                if (m_seqIsBoxSelecting) {
                    if (io.MouseDown[0]) {
                        ImVec2 bMin((std::min)(m_seqBoxSelectStart.x, io.MousePos.x), (std::min)(m_seqBoxSelectStart.y, io.MousePos.y));
                        ImVec2 bMax((std::max)(m_seqBoxSelectStart.x, io.MousePos.x), (std::max)(m_seqBoxSelectStart.y, io.MousePos.y));
                        dl->AddRect(bMin, bMax, IM_COL32(245, 180, 50, 220), 0.0f, 0, 1.2f);
                        dl->AddRectFilled(bMin, bMax, IM_COL32(245, 180, 50, 40));
                    } else {
                        ImVec2 bMin((std::min)(m_seqBoxSelectStart.x, io.MousePos.x), (std::min)(m_seqBoxSelectStart.y, io.MousePos.y));
                        ImVec2 bMax((std::max)(m_seqBoxSelectStart.x, io.MousePos.x), (std::max)(m_seqBoxSelectStart.y, io.MousePos.y));
                        float tMin = (bMin.x - canvasOriginX) / timeScale;
                        float tMax = (bMax.x - canvasOriginX) / timeScale;
                        for (auto& trk : seq.getEntityTracks()) {
                            for (auto& k : trk.keyframes) {
                                if (k.time >= tMin && k.time <= tMax) k.selected = true;
                            }
                        }
                        for (auto& trk : seq.getCameraTracks()) {
                            for (auto& k : trk.keyframes) {
                                if (k.time >= tMin && k.time <= tMax) k.selected = true;
                            }
                        }
                        for (auto& trk : seq.getEnvironmentTracks()) {
                            for (auto& k : trk.keyframes) {
                                if (k.time >= tMin && k.time <= tMax) k.selected = true;
                            }
                        }
                        for (auto& trk : seq.getLuaTracks()) {
                            for (auto& k : trk.keyframes) {
                                if (k.time >= tMin && k.time <= tMax) k.selected = true;
                            }
                        }
                        m_seqIsBoxSelecting = false;
                    }
                }

            } else {
                // Modo Graph Editor: Curvas Bezier F-Curves Interactivas con Auto-Scaling
                EntityTransformTrack* activeEntTrack = nullptr;
                if (!m_selectedEntityName.empty()) {
                    for (auto& trk : seq.getEntityTracks()) {
                        if (trk.entityName == m_selectedEntityName) {
                            activeEntTrack = &trk;
                            break;
                        }
                    }
                }
                if (!activeEntTrack && !seq.getEntityTracks().empty()) {
                    activeEntTrack = &seq.getEntityTracks()[0];
                }

                // Selector de Canales del Graph Editor
                ImGui::SetCursorScreenPos(ImVec2(canvasMin.x + 10.0f, canvasMin.y + rulerHeight + 6.0f));
                if (ImGui::Button(m_graphChannelSelection == 0 ? "[ Posicion ]" : "Posicion")) m_graphChannelSelection = 0;
                ImGui::SameLine();
                if (ImGui::Button(m_graphChannelSelection == 1 ? "[ Rotacion ]" : "Rotacion")) m_graphChannelSelection = 1;
                ImGui::SameLine();
                if (ImGui::Button(m_graphChannelSelection == 2 ? "[ Escala ]" : "Escala")) m_graphChannelSelection = 2;
                ImGui::SameLine();
                if (activeEntTrack) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Entidad: %s", activeEntTrack->entityName.c_str());
                } else {
                    ImGui::TextDisabled("Camara Principal (F-Curve)");
                }

                float graphTop = canvasMin.y + rulerHeight + 35.0f;
                float graphBottom = canvasMax.y - 15.0f;
                float graphH = (std::max)(40.0f, graphBottom - graphTop);

                if (activeEntTrack && !activeEntTrack->keyframes.empty()) {
                    // 1. Calcular Rango Min/Max para Auto-Scaling
                    float minVal = 1e9f, maxVal = -1e9f;
                    for (const auto& kf : activeEntTrack->keyframes) {
                        if (m_graphChannelSelection == 0) { // Posicion
                            minVal = (std::min)({minVal, kf.position.x, kf.position.y, kf.position.z});
                            maxVal = (std::max)({maxVal, kf.position.x, kf.position.y, kf.position.z});
                        } else if (m_graphChannelSelection == 1) { // Rotacion Euler
                            glm::vec3 eu = glm::degrees(glm::eulerAngles(kf.rotation));
                            minVal = (std::min)({minVal, eu.x, eu.y, eu.z});
                            maxVal = (std::max)({maxVal, eu.x, eu.y, eu.z});
                        } else { // Escala
                            minVal = (std::min)({minVal, kf.scale.x, kf.scale.y, kf.scale.z});
                            maxVal = (std::max)({maxVal, kf.scale.x, kf.scale.y, kf.scale.z});
                        }
                    }
                    if (std::abs(maxVal - minVal) < 0.2f) {
                        minVal -= 1.0f;
                        maxVal += 1.0f;
                    }
                    float pad = (maxVal - minVal) * 0.15f;
                    minVal -= pad;
                    maxVal += pad;

                    auto valToY = [&](float v) -> float {
                        float norm = (v - minVal) / (maxVal - minVal);
                        return graphBottom - glm::clamp(norm, 0.0f, 1.0f) * graphH;
                    };
                    auto yToVal = [&](float y) -> float {
                        float norm = (graphBottom - y) / graphH;
                        return minVal + norm * (maxVal - minVal);
                    };

                    // Linea de Base cero si esta visible
                    if (minVal <= 0.0f && maxVal >= 0.0f) {
                        float zeroY = valToY(0.0f);
                        dl->AddLine(ImVec2(canvasMin.x, zeroY), ImVec2(canvasMax.x, zeroY), IM_COL32(70, 75, 85, 180), 1.0f);
                    }

                    // 2. Trazar Curvas de los 3 Ejes (X: Rojo, Y: Verde, Z: Azul)
                    const int curveSteps = 70;
                    ImVec2 prevPts[3];
                    bool hasPrev = false;

                    for (int step = 0; step <= curveSteps; ++step) {
                        float t = (float)step / (float)curveSteps * duration;
                        glm::vec3 pos(0.0f), scl(1.0f);
                        glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);

                        if (activeEntTrack->sample(t, pos, rot, scl)) {
                            float px = canvasOriginX + t * timeScale;
                            glm::vec3 vals(0.0f);
                            if (m_graphChannelSelection == 0) vals = pos;
                            else if (m_graphChannelSelection == 1) vals = glm::degrees(glm::eulerAngles(rot));
                            else vals = scl;

                            ImVec2 curPts[3] = {
                                ImVec2(px, valToY(vals.x)),
                                ImVec2(px, valToY(vals.y)),
                                ImVec2(px, valToY(vals.z))
                            };

                            if (hasPrev) {
                                dl->AddLine(prevPts[0], curPts[0], IM_COL32(235, 75, 75, 230), 1.8f);
                                dl->AddLine(prevPts[1], curPts[1], IM_COL32(75, 220, 90, 230), 1.8f);
                                dl->AddLine(prevPts[2], curPts[2], IM_COL32(65, 140, 245, 230), 1.8f);
                            }
                            prevPts[0] = curPts[0]; prevPts[1] = curPts[1]; prevPts[2] = curPts[2];
                            hasPrev = true;
                        }
                    }

                    // 3. Nodos y Puntos de Control Arrastrables en cada Keyframe
                    for (size_t i = 0; i < activeEntTrack->keyframes.size(); ++i) {
                        auto& kf = activeEntTrack->keyframes[i];
                        float kx = canvasOriginX + kf.time * timeScale;

                        glm::vec3 vals(0.0f);
                        if (m_graphChannelSelection == 0) vals = kf.position;
                        else if (m_graphChannelSelection == 1) vals = glm::degrees(glm::eulerAngles(kf.rotation));
                        else vals = kf.scale;

                        ImU32 colors[3] = { IM_COL32(235, 75, 75, 255), IM_COL32(75, 220, 90, 255), IM_COL32(65, 140, 245, 255) };

                        for (int axis = 0; axis < 3; ++axis) {
                            float ky = valToY(axis == 0 ? vals.x : (axis == 1 ? vals.y : vals.z));
                            float dist = std::hypot(io.MousePos.x - kx, io.MousePos.y - ky);
                            bool hov = (isHovered && dist <= 7.0f);

                            if (hov && ImGui::IsMouseClicked(0)) {
                                m_draggedGraphPointIndex = (int)i;
                                m_draggedGraphPointAxis = axis;
                            }

                            // Comprobar arrastre activo del nodo
                            if (m_draggedGraphPointIndex == (int)i && m_draggedGraphPointAxis == axis) {
                                if (io.MouseDown[0]) {
                                    kf.time = glm::clamp((io.MousePos.x - canvasOriginX) / timeScale, 0.0f, duration);
                                    float newVal = yToVal(io.MousePos.y);
                                    if (m_graphChannelSelection == 0) {
                                        if (axis == 0) kf.position.x = newVal;
                                        else if (axis == 1) kf.position.y = newVal;
                                        else kf.position.z = newVal;
                                    } else if (m_graphChannelSelection == 1) {
                                        glm::vec3 euler = glm::degrees(glm::eulerAngles(kf.rotation));
                                        if (axis == 0) euler.x = newVal;
                                        else if (axis == 1) euler.y = newVal;
                                        else euler.z = newVal;
                                        glm::vec3 rad = glm::radians(euler);
                                        glm::mat4 R = glm::rotate(glm::mat4(1.0f), rad.y, glm::vec3(0, 1, 0)) *
                                                     glm::rotate(glm::mat4(1.0f), rad.x, glm::vec3(1, 0, 0)) *
                                                     glm::rotate(glm::mat4(1.0f), rad.z, glm::vec3(0, 0, 1));
                                        kf.rotation = glm::quat_cast(R);
                                    } else {
                                        if (axis == 0) kf.scale.x = (std::max)(0.001f, newVal);
                                        else if (axis == 1) kf.scale.y = (std::max)(0.001f, newVal);
                                        else kf.scale.z = (std::max)(0.001f, newVal);
                                    }
                                    seq.requestSample();
                                } else {
                                    m_draggedGraphPointIndex = -1;
                                    m_draggedGraphPointAxis = -1;
                                    activeEntTrack->sortKeyframes();
                                    seq.requestSample();
                                }
                            }

                            dl->AddCircleFilled(ImVec2(kx, ky), 4.5f, colors[axis]);
                            dl->AddCircle(ImVec2(kx, ky), 5.5f, hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(30, 30, 30, 200), 0, 1.2f);
                        }
                    }
                } else if (!seq.getCameraTracks().empty() && !seq.getCameraTracks()[0].keyframes.empty()) {
                    // Fallback a F-Curves de Camara
                    const auto& trk = seq.getCameraTracks()[0];
                    int curveSegments = 60;
                    ImVec2 prevX, prevY, prevZ;
                    bool hasPrev = false;

                    for (int i = 0; i <= curveSegments; ++i) {
                        float t = (float)i / (float)curveSegments * duration;
                        glm::vec3 pos(0.0f), tgt(0.0f);
                        float f_mm = 35.0f, fdist = 5.0f, ap = 2.8f, shInt = 0.0f, shFreq = 1.5f;

                        if (trk.sample(t, pos, tgt, f_mm, fdist, ap, shInt, shFreq)) {
                            float px = canvasOriginX + t * timeScale;
                            float midY = graphTop + graphH * 0.5f;
                            ImVec2 curX(px, midY - pos.x * 12.0f);
                            ImVec2 curY(px, midY - pos.y * 12.0f);
                            ImVec2 curZ(px, midY - pos.z * 12.0f);

                            if (hasPrev) {
                                dl->AddLine(prevX, curX, IM_COL32(235, 75, 75, 230), 1.8f);
                                dl->AddLine(prevY, curY, IM_COL32(75, 220, 90, 230), 1.8f);
                                dl->AddLine(prevZ, curZ, IM_COL32(65, 140, 245, 230), 1.8f);
                            }
                            prevX = curX; prevY = curY; prevZ = curZ;
                            hasPrev = true;
                        }
                    }
                }

                // Leyenda de Canales F-Curve
                dl->AddText(ImVec2(canvasMin.x + 280.0f, canvasMin.y + rulerHeight + 8.0f), IM_COL32(235, 75, 75, 255), "X");
                dl->AddText(ImVec2(canvasMin.x + 305.0f, canvasMin.y + rulerHeight + 8.0f), IM_COL32(75, 220, 90, 255), "Y");
                dl->AddText(ImVec2(canvasMin.x + 330.0f, canvasMin.y + rulerHeight + 8.0f), IM_COL32(65, 140, 245, 255), "Z");
            }

            dl->PopClipRect();

            // Fondo y Marcas de la Regla de Tiempo Superior (Siempre visible encima de las pistas)
            dl->AddRectFilled(canvasMin, ImVec2(canvasMax.x, canvasMin.y + rulerHeight), IM_COL32(30, 34, 40, 240));
            dl->AddLine(ImVec2(canvasMin.x, canvasMin.y + rulerHeight), ImVec2(canvasMax.x, canvasMin.y + rulerHeight), IM_COL32(60, 65, 75, 255), 1.0f);

            for (int s = 0; s <= maxSec; ++s) {
                float x = canvasOriginX + s * timeScale;
                if (x >= canvasMin.x - 20.0f && x <= canvasMax.x + 20.0f) {
                    dl->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMin.y + rulerHeight), IM_COL32(80, 85, 95, 255), 1.2f);
                    char secLabel[16];
                    snprintf(secLabel, sizeof(secLabel), "%ds", s);
                    dl->AddText(ImVec2(x + 3.0f, canvasMin.y + 3.0f), IM_COL32(160, 170, 185, 255), secLabel);
                }
            }

            // Playhead (Cabezal Azul Brillante #4772B3)
            float playheadX = canvasOriginX + curTime * timeScale;
            if (playheadX >= canvasMin.x && playheadX <= canvasMax.x) {
                dl->AddLine(ImVec2(playheadX, canvasMin.y), ImVec2(playheadX, canvasMax.y), IM_COL32(71, 114, 179, 255), 1.8f);

                ImVec2 tTop1(playheadX - 6.0f, canvasMin.y);
                ImVec2 tTop2(playheadX + 6.0f, canvasMin.y);
                ImVec2 tBot(playheadX, canvasMin.y + rulerHeight);
                dl->AddTriangleFilled(tTop1, tTop2, tBot, IM_COL32(71, 114, 179, 255));
                dl->AddTriangle(tTop1, tTop2, tBot, IM_COL32(255, 255, 255, 220), 1.0f);
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace ASIS
