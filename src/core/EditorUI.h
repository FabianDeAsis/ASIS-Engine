#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <glm/glm.hpp>

#include "Scene.h"
#include "ScriptEngine.h"
#include "Camera.h"
#include "Window.h"
#include "Framebuffer.h"
#include "OASISMeshlet.h"
#include "DayNightCycle.h"
#include <imgui.h>
#include <imgui_internal.h>

struct GLFWwindow;
struct ImFont;

namespace ASIS {

class PhysicsWorld;
class ShadowMap;
class Skybox;
class ParticleSystem;
class SSAO;
class Terrain;
class CharacterController;
class Water;

enum class EnginePlayMode {
    EDITOR,
    PLAYING,
    PAUSED
};

enum class BlenderPropertyTab {
    OBJECT,        // Propiedades del Objeto: Transformacion, Visibilidad
    MATERIAL,      // Material PBR: Albedo, Roughness, Metalness, Specular, Normal Maps, Texturas
    LIGHTING,      // Iluminacion: Sol Direccional, Sombras 4K, Luces Puntuales, Skybox/HDRI
    TERRAIN,       // Terreno 3D Procedural, Escultura y Niebla Atmosferica
    PLAYER,        // Controlador de Personaje 3D y Play-in-Editor
    AUDIO,         // Motor de Audio 3D Espacial y SFX Procedural
    PARTICLES,     // Sistema de Particulas 3D (VFX): Fuego, Humo, Chispas, Lluvia, Magia
    ANIMATION,     // Animaciones Esqueleticas: Reproduccion, Huesos, Clips, Linea de Tiempo
    PHYSICS,       // Fisica 3D Bullet: Cuerpos Rigidos, Gravedad, Masa, Rebote, Impulsos
    SCRIPTS,       // Logica y Scripts: Estado de ejecucion, Hot-reload, Lua
    IMPORT_EXPORT  // Importacion / Exportacion 3D (.obj, .gltf, .fbx, .stl) y Drag & Drop
};

enum class UITheme {
    BLENDER_DARK,
    UNREAL_5,
    OLED_MIDNIGHT,
    CYBERPUNK,
    CUSTOM
};

enum class UILayoutPreset {
    GENERAL,          // General / Completo
    LIGHTING_CINEMA,  // Iluminación & Cine
    TERRAIN_MODELING, // Terreno & Modelado
    GAME_VIEW         // Modo Juego (Inmersivo)
};

struct EntitySnapshot {
    std::string name;
    std::string primitiveType;
    std::string modelPath;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec3 color;
    bool hasCustomColor;
    bool visible;
    glm::vec3 matAlbedo;
    float matMetallic;
    float matRoughness;
    float matSpecular;
    glm::vec3 matEmission;
    float matEmissionStrength;
    float matNormalStrength;
    float matAO;
    std::string group;
    bool isTrigger = false;
    std::string triggerTag = "Default";
};

struct SceneSnapshot {
    std::vector<EntitySnapshot> entities;
    std::vector<PointLight> lights;
    bool hasTerrainData = false;
    std::vector<float> terrainHeights;
    std::vector<uint8_t> terrainSplatmap;
};

// ============================================================================
// HELPER DE PROPIEDADES DIVIDIDAS ESTILO BLENDER 4/5 (FASE 21.5)
// Sistema de 2 columnas (38% etiqueta con tooltip / 62% widget a ancho completo)
// ============================================================================
namespace UI {

enum class IconType {
    PLAY,
    STOP,
    PAUSE,
    SNAP,
    EYE_OPEN,
    EYE_CLOSED,
    LOCK,
    PHYSICS,
    SEARCH,
    PLUS,
    DUPLICATE,
    TRASH,
    RESET,
    FOLDER,
    SAVE,
    CAMERA_TARGET,
    SPEAKER,
    WAVE,
    GRID,
    SPARK,
    SUN,
    STATUS_DOT,
    SHADING_WIREFRAME,
    SHADING_SOLID,
    SHADING_OASIS,
    SHADING_RENDERED,
    OVERLAYS,
    MOUSE_LMB,
    MOUSE_MMB,
    MOUSE_RMB,
    KEY_FIRST,
    KEY_PREV,
    KEY_NEXT,
    KEYFRAME,
    AUTO_KEY,
    CURVE_GRAPH,
    CINE_CAMERA,
    CLAPPERBOARD
};

inline void DrawMiniIcon(ImDrawList* dl, IconType type, ImVec2 center, float size, ImU32 col) {
    float r = size * 0.5f;
    switch (type) {
    case IconType::PLAY: {
        ImVec2 p1(center.x - r * 0.65f, center.y - r * 0.85f);
        ImVec2 p2(center.x - r * 0.65f, center.y + r * 0.85f);
        ImVec2 p3(center.x + r * 0.90f, center.y);
        dl->AddTriangleFilled(p1, p2, p3, col);
        break;
    }
    case IconType::STOP: {
        dl->AddRectFilled(ImVec2(center.x - r * 0.70f, center.y - r * 0.70f),
                          ImVec2(center.x + r * 0.70f, center.y + r * 0.70f), col, 1.5f);
        break;
    }
    case IconType::PAUSE: {
        float barW = r * 0.42f;
        float barH = r * 0.85f;
        float gap  = r * 0.28f;
        dl->AddRectFilled(ImVec2(center.x - gap - barW, center.y - barH),
                          ImVec2(center.x - gap,        center.y + barH), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x + gap,        center.y - barH),
                          ImVec2(center.x + gap + barW, center.y + barH), col, 1.0f);
        break;
    }
    case IconType::SNAP: {
        float armW = r * 0.35f;
        float gap  = r * 0.35f;
        float topY = center.y - r * 0.85f;
        float midY = center.y - r * 0.25f;
        float botY = center.y + r * 0.75f;
        dl->AddRectFilled(ImVec2(center.x - gap - armW, topY), ImVec2(center.x - gap, midY), IM_COL32(235, 65, 65, 255), 1.0f);
        dl->AddRectFilled(ImVec2(center.x - gap - armW, midY), ImVec2(center.x - gap, botY), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x + gap, topY), ImVec2(center.x + gap + armW, midY), IM_COL32(245, 245, 245, 255), 1.0f);
        dl->AddRectFilled(ImVec2(center.x + gap, midY), ImVec2(center.x + gap + armW, botY), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x - gap - armW, botY - armW), ImVec2(center.x + gap + armW, botY), col, 1.5f);
        break;
    }
    case IconType::EYE_OPEN: {
        ImVec2 left(center.x - r * 0.90f, center.y);
        ImVec2 right(center.x + r * 0.90f, center.y);
        ImVec2 top(center.x, center.y - r * 0.55f);
        ImVec2 bot(center.x, center.y + r * 0.55f);
        dl->AddQuad(left, top, right, bot, col, 1.2f);
        dl->AddCircleFilled(center, r * 0.32f, col);
        break;
    }
    case IconType::EYE_CLOSED: {
        ImVec2 left(center.x - r * 0.80f, center.y);
        ImVec2 right(center.x + r * 0.80f, center.y);
        ImVec2 bot(center.x, center.y + r * 0.45f);
        dl->AddLine(left, bot, col, 1.2f);
        dl->AddLine(bot, right, col, 1.2f);
        dl->AddLine(ImVec2(center.x - r * 0.70f, center.y - r * 0.70f),
                    ImVec2(center.x + r * 0.70f, center.y + r * 0.70f),
                    IM_COL32(230, 80, 80, 255), 1.5f);
        break;
    }
    case IconType::LOCK: {
        dl->AddRectFilled(ImVec2(center.x - r * 0.65f, center.y - r * 0.15f),
                          ImVec2(center.x + r * 0.65f, center.y + r * 0.75f), col, 1.5f);
        dl->AddLine(ImVec2(center.x - r * 0.40f, center.y - r * 0.15f),
                    ImVec2(center.x - r * 0.40f, center.y - r * 0.65f), col, 1.5f);
        dl->AddLine(ImVec2(center.x - r * 0.40f, center.y - r * 0.65f),
                    ImVec2(center.x + r * 0.40f, center.y - r * 0.65f), col, 1.5f);
        dl->AddLine(ImVec2(center.x + r * 0.40f, center.y - r * 0.65f),
                    ImVec2(center.x + r * 0.40f, center.y - r * 0.15f), col, 1.5f);
        break;
    }
    case IconType::PHYSICS: {
        float s = r * 0.55f;
        ImVec2 f1(center.x - s, center.y);
        ImVec2 f2(center.x + s * 0.2f, center.y);
        ImVec2 f3(center.x + s * 0.2f, center.y + s * 1.2f);
        ImVec2 f4(center.x - s, center.y + s * 1.2f);
        dl->AddQuad(f1, f2, f3, f4, col, 1.2f);
        ImVec2 off(s * 0.6f, -s * 0.6f);
        dl->AddLine(f1, ImVec2(f1.x + off.x, f1.y + off.y), col, 1.2f);
        dl->AddLine(f2, ImVec2(f2.x + off.x, f2.y + off.y), col, 1.2f);
        dl->AddLine(f3, ImVec2(f3.x + off.x, f3.y + off.y), col, 1.2f);
        dl->AddLine(ImVec2(f1.x + off.x, f1.y + off.y), ImVec2(f2.x + off.x, f2.y + off.y), col, 1.2f);
        dl->AddLine(ImVec2(f2.x + off.x, f2.y + off.y), ImVec2(f3.x + off.x, f3.y + off.y), col, 1.2f);
        break;
    }
    case IconType::SEARCH: {
        float cr = r * 0.45f;
        ImVec2 cc(center.x - r * 0.22f, center.y - r * 0.22f);
        dl->AddCircle(cc, cr, col, 0, 1.5f);
        dl->AddLine(ImVec2(cc.x + cr * 0.707f, cc.y + cr * 0.707f),
                    ImVec2(center.x + r * 0.85f, center.y + r * 0.85f), col, 2.0f);
        break;
    }
    case IconType::PLUS: {
        dl->AddLine(ImVec2(center.x - r * 0.75f, center.y), ImVec2(center.x + r * 0.75f, center.y), col, 1.8f);
        dl->AddLine(ImVec2(center.x, center.y - r * 0.75f), ImVec2(center.x, center.y + r * 0.75f), col, 1.8f);
        break;
    }
    case IconType::DUPLICATE: {
        dl->AddRect(ImVec2(center.x - r * 0.25f, center.y - r * 0.80f),
                    ImVec2(center.x + r * 0.80f, center.y + r * 0.25f), col, 1.0f, 0, 1.2f);
        dl->AddRectFilled(ImVec2(center.x - r * 0.80f, center.y - r * 0.25f),
                          ImVec2(center.x + r * 0.25f, center.y + r * 0.80f), IM_COL32(40, 40, 40, 240), 1.0f);
        dl->AddRect(ImVec2(center.x - r * 0.80f, center.y - r * 0.25f),
                    ImVec2(center.x + r * 0.25f, center.y + r * 0.80f), col, 1.0f, 0, 1.2f);
        break;
    }
    case IconType::TRASH: {
        dl->AddLine(ImVec2(center.x - r * 0.75f, center.y - r * 0.55f),
                    ImVec2(center.x + r * 0.75f, center.y - r * 0.55f), col, 1.5f);
        dl->AddLine(ImVec2(center.x - r * 0.25f, center.y - r * 0.75f),
                    ImVec2(center.x + r * 0.25f, center.y - r * 0.75f), col, 1.2f);
        dl->AddRect(ImVec2(center.x - r * 0.55f, center.y - r * 0.45f),
                    ImVec2(center.x + r * 0.55f, center.y + r * 0.75f), col, 1.0f, 0, 1.2f);
        dl->AddLine(ImVec2(center.x - r * 0.20f, center.y - r * 0.30f), ImVec2(center.x - r * 0.20f, center.y + r * 0.60f), col, 1.0f);
        dl->AddLine(ImVec2(center.x + r * 0.20f, center.y - r * 0.30f), ImVec2(center.x + r * 0.20f, center.y + r * 0.60f), col, 1.0f);
        break;
    }
    case IconType::RESET: {
        dl->PathArcTo(center, r * 0.65f, 0.5f, 5.8f);
        dl->PathStroke(col, 0, 1.4f);
        ImVec2 tip(center.x + r * 0.65f * cosf(5.8f), center.y + r * 0.65f * sinf(5.8f));
        dl->AddTriangleFilled(tip, ImVec2(tip.x - r * 0.45f, tip.y - r * 0.15f),
                                   ImVec2(tip.x - r * 0.15f, tip.y - r * 0.45f), col);
        break;
    }
    case IconType::FOLDER: {
        dl->AddRectFilled(ImVec2(center.x - r * 0.85f, center.y - r * 0.70f),
                          ImVec2(center.x - r * 0.15f, center.y - r * 0.35f), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x - r * 0.85f, center.y - r * 0.35f),
                          ImVec2(center.x + r * 0.85f, center.y + r * 0.70f), col, 1.5f);
        break;
    }
    case IconType::SAVE: {
        dl->AddRectFilled(ImVec2(center.x - r * 0.75f, center.y - r * 0.75f),
                          ImVec2(center.x + r * 0.75f, center.y + r * 0.75f), col, 1.5f);
        dl->AddRectFilled(ImVec2(center.x - r * 0.45f, center.y - r * 0.75f),
                          ImVec2(center.x + r * 0.35f, center.y - r * 0.25f), IM_COL32(35, 35, 35, 255), 1.0f);
        dl->AddRectFilled(ImVec2(center.x - r * 0.50f, center.y + r * 0.15f),
                          ImVec2(center.x + r * 0.50f, center.y + r * 0.75f), IM_COL32(230, 230, 230, 255), 1.0f);
        break;
    }
    case IconType::CAMERA_TARGET: {
        dl->AddCircle(center, r * 0.65f, col, 0, 1.2f);
        dl->AddCircleFilled(center, r * 0.22f, col);
        dl->AddLine(ImVec2(center.x - r * 0.90f, center.y), ImVec2(center.x - r * 0.35f, center.y), col, 1.2f);
        dl->AddLine(ImVec2(center.x + r * 0.35f, center.y), ImVec2(center.x + r * 0.90f, center.y), col, 1.2f);
        dl->AddLine(ImVec2(center.x, center.y - r * 0.90f), ImVec2(center.x, center.y - r * 0.35f), col, 1.2f);
        dl->AddLine(ImVec2(center.x, center.y + r * 0.35f), ImVec2(center.x, center.y + r * 0.90f), col, 1.2f);
        break;
    }
    case IconType::SPEAKER: {
        dl->AddRectFilled(ImVec2(center.x - r * 0.75f, center.y - r * 0.35f),
                          ImVec2(center.x - r * 0.35f, center.y + r * 0.35f), col, 1.0f);
        ImVec2 c1(center.x - r * 0.35f, center.y - r * 0.35f);
        ImVec2 c2(center.x + r * 0.15f, center.y - r * 0.75f);
        ImVec2 c3(center.x + r * 0.15f, center.y + r * 0.75f);
        ImVec2 c4(center.x - r * 0.35f, center.y + r * 0.35f);
        dl->AddQuadFilled(c1, c2, c3, c4, col);
        dl->AddLine(ImVec2(center.x + r * 0.40f, center.y - r * 0.35f),
                    ImVec2(center.x + r * 0.55f, center.y), col, 1.2f);
        dl->AddLine(ImVec2(center.x + r * 0.55f, center.y),
                    ImVec2(center.x + r * 0.40f, center.y + r * 0.35f), col, 1.2f);
        dl->AddLine(ImVec2(center.x + r * 0.65f, center.y - r * 0.60f),
                    ImVec2(center.x + r * 0.85f, center.y), col, 1.2f);
        dl->AddLine(ImVec2(center.x + r * 0.85f, center.y),
                    ImVec2(center.x + r * 0.65f, center.y + r * 0.60f), col, 1.2f);
        break;
    }
    case IconType::WAVE: {
        float bW = r * 0.30f;
        dl->AddRectFilled(ImVec2(center.x - r * 0.70f, center.y - r * 0.35f),
                          ImVec2(center.x - r * 0.70f + bW, center.y + r * 0.75f), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x - bW * 0.5f, center.y - r * 0.85f),
                          ImVec2(center.x + bW * 0.5f, center.y + r * 0.75f), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x + r * 0.70f - bW, center.y - r * 0.55f),
                          ImVec2(center.x + r * 0.70f, center.y + r * 0.75f), col, 1.0f);
        break;
    }
    case IconType::GRID: {
        float g = r * 0.65f;
        float h = r * 0.25f;
        dl->AddRectFilled(ImVec2(center.x - g, center.y - g), ImVec2(center.x - h, center.y - h), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x + h, center.y - g), ImVec2(center.x + g, center.y - h), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x - g, center.y + h), ImVec2(center.x - h, center.y + g), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x + h, center.y + h), ImVec2(center.x + g, center.y + g), col, 1.0f);
        break;
    }
    case IconType::SPARK: {
        dl->AddTriangleFilled(ImVec2(center.x, center.y - r * 0.85f),
                              ImVec2(center.x - r * 0.25f, center.y),
                              ImVec2(center.x + r * 0.25f, center.y), col);
        dl->AddTriangleFilled(ImVec2(center.x, center.y + r * 0.85f),
                              ImVec2(center.x - r * 0.25f, center.y),
                              ImVec2(center.x + r * 0.25f, center.y), col);
        dl->AddTriangleFilled(ImVec2(center.x - r * 0.85f, center.y),
                              ImVec2(center.x, center.y - r * 0.25f),
                              ImVec2(center.x, center.y + r * 0.25f), col);
        dl->AddTriangleFilled(ImVec2(center.x + r * 0.85f, center.y),
                              ImVec2(center.x, center.y - r * 0.25f),
                              ImVec2(center.x, center.y + r * 0.25f), col);
        break;
    }
    case IconType::SUN: {
        dl->AddCircleFilled(center, r * 0.40f, col);
        for (int i = 0; i < 8; ++i) {
            float a = i * 0.785398f;
            ImVec2 p1(center.x + cosf(a) * r * 0.55f, center.y + sinf(a) * r * 0.55f);
            ImVec2 p2(center.x + cosf(a) * r * 0.85f, center.y + sinf(a) * r * 0.85f);
            dl->AddLine(p1, p2, col, 1.2f);
        }
        break;
    }
    case IconType::STATUS_DOT: {
        dl->AddCircleFilled(center, r * 0.55f, col);
        dl->AddCircle(center, r * 0.55f, IM_COL32(255, 255, 255, 120), 0, 1.0f);
        break;
    }
    case IconType::SHADING_WIREFRAME: {
        dl->AddCircle(center, r * 0.85f, col, 16, 1.2f);
        dl->AddLine(ImVec2(center.x - r * 0.85f, center.y), ImVec2(center.x + r * 0.85f, center.y), col, 1.0f);
        dl->AddLine(ImVec2(center.x, center.y - r * 0.85f), ImVec2(center.x, center.y + r * 0.85f), col, 1.0f);
        break;
    }
    case IconType::SHADING_SOLID: {
        dl->AddCircleFilled(center, r * 0.85f, col);
        dl->AddCircleFilled(ImVec2(center.x - r * 0.25f, center.y - r * 0.25f), r * 0.45f, IM_COL32(255, 255, 255, 65));
        break;
    }
    case IconType::SHADING_OASIS: {
        dl->AddCircleFilled(center, r * 0.85f, col);
        dl->PathLineTo(center);
        dl->PathArcTo(center, r * 0.85f, 0.0f, 1.5707f);
        dl->PathFillConvex(IM_COL32(50, 180, 240, 255));
        dl->PathLineTo(center);
        dl->PathArcTo(center, r * 0.85f, 1.5707f, 3.14159f);
        dl->PathFillConvex(IM_COL32(240, 60, 120, 255));
        dl->PathLineTo(center);
        dl->PathArcTo(center, r * 0.85f, 3.14159f, 4.71239f);
        dl->PathFillConvex(IM_COL32(245, 200, 40, 255));
        dl->PathLineTo(center);
        dl->PathArcTo(center, r * 0.85f, 4.71239f, 6.28318f);
        dl->PathFillConvex(IM_COL32(60, 220, 90, 255));
        dl->AddCircle(center, r * 0.85f, col, 16, 1.0f);
        break;
    }
    case IconType::SHADING_RENDERED: {
        dl->AddCircleFilled(center, r * 0.85f, col);
        dl->AddCircleFilled(ImVec2(center.x - r * 0.30f, center.y - r * 0.30f), r * 0.30f, IM_COL32(255, 255, 255, 230));
        dl->AddCircle(center, r * 0.85f, IM_COL32(255, 255, 255, 120), 16, 1.0f);
        break;
    }
    case IconType::OVERLAYS: {
        dl->AddCircle(ImVec2(center.x - r * 0.35f, center.y), r * 0.55f, col, 16, 1.2f);
        dl->AddCircle(ImVec2(center.x + r * 0.35f, center.y), r * 0.55f, col, 16, 1.2f);
        break;
    }
    case IconType::MOUSE_LMB:
    case IconType::MOUSE_MMB:
    case IconType::MOUSE_RMB: {
        ImVec2 mMin(center.x - r * 0.55f, center.y - r * 0.85f);
        ImVec2 mMax(center.x + r * 0.55f, center.y + r * 0.85f);
        dl->AddRect(mMin, mMax, col, r * 0.4f, 0, 1.0f);
        float splitY = center.y - r * 0.2f;
        dl->AddLine(ImVec2(mMin.x, splitY), ImVec2(mMax.x, splitY), col, 1.0f);
        dl->AddLine(ImVec2(center.x, mMin.y), ImVec2(center.x, splitY), col, 1.0f);
        if (type == IconType::MOUSE_LMB) {
            dl->AddRectFilled(mMin, ImVec2(center.x, splitY), col, r * 0.3f, ImDrawFlags_RoundCornersTopLeft);
        } else if (type == IconType::MOUSE_RMB) {
            dl->AddRectFilled(ImVec2(center.x, mMin.y), ImVec2(mMax.x, splitY), col, r * 0.3f, ImDrawFlags_RoundCornersTopRight);
        } else if (type == IconType::MOUSE_MMB) {
            dl->AddRectFilled(ImVec2(center.x - 1.2f, mMin.y + 2.0f), ImVec2(center.x + 1.2f, splitY - 1.0f), col);
        }
        break;
    }
    case IconType::KEY_FIRST: {
        float barW = r * 0.28f;
        dl->AddRectFilled(ImVec2(center.x - r * 0.85f, center.y - r * 0.75f),
                          ImVec2(center.x - r * 0.85f + barW, center.y + r * 0.75f), col, 1.0f);
        dl->AddTriangleFilled(ImVec2(center.x - r * 0.85f + barW, center.y),
                              ImVec2(center.x + r * 0.75f, center.y - r * 0.75f),
                              ImVec2(center.x + r * 0.75f, center.y + r * 0.75f), col);
        break;
    }
    case IconType::KEY_PREV: {
        dl->AddTriangleFilled(ImVec2(center.x - r * 0.75f, center.y),
                              ImVec2(center.x + r * 0.65f, center.y - r * 0.75f),
                              ImVec2(center.x + r * 0.65f, center.y + r * 0.75f), col);
        break;
    }
    case IconType::KEY_NEXT: {
        dl->AddTriangleFilled(ImVec2(center.x + r * 0.75f, center.y),
                              ImVec2(center.x - r * 0.65f, center.y - r * 0.75f),
                              ImVec2(center.x - r * 0.65f, center.y + r * 0.75f), col);
        break;
    }
    case IconType::KEYFRAME: {
        ImVec2 top(center.x, center.y - r * 0.85f);
        ImVec2 right(center.x + r * 0.70f, center.y);
        ImVec2 bottom(center.x, center.y + r * 0.85f);
        ImVec2 left(center.x - r * 0.70f, center.y);
        dl->AddQuadFilled(top, right, bottom, left, col);
        break;
    }
    case IconType::AUTO_KEY: {
        dl->AddCircleFilled(center, r * 0.50f, col);
        dl->AddCircle(center, r * 0.85f, col, 16, 1.2f);
        break;
    }
    case IconType::CURVE_GRAPH: {
        ImVec2 p0(center.x - r * 0.8f, center.y + r * 0.6f);
        ImVec2 cp1(center.x - r * 0.2f, center.y + r * 0.6f);
        ImVec2 cp2(center.x + r * 0.2f, center.y - r * 0.6f);
        ImVec2 p3(center.x + r * 0.8f, center.y - r * 0.6f);
        dl->AddBezierCubic(p0, cp1, cp2, p3, col, 1.5f);
        dl->AddCircleFilled(p0, 2.0f, col);
        dl->AddCircleFilled(p3, 2.0f, col);
        break;
    }
    case IconType::CINE_CAMERA: {
        dl->AddRectFilled(ImVec2(center.x - r * 0.75f, center.y - r * 0.35f),
                          ImVec2(center.x + r * 0.20f, center.y + r * 0.65f), col, 1.5f);
        dl->AddTriangleFilled(ImVec2(center.x + r * 0.20f, center.y - r * 0.15f),
                              ImVec2(center.x + r * 0.80f, center.y - r * 0.55f),
                              ImVec2(center.x + r * 0.80f, center.y + r * 0.65f), col);
        dl->AddCircleFilled(ImVec2(center.x - r * 0.45f, center.y - r * 0.60f), r * 0.28f, col);
        dl->AddCircleFilled(ImVec2(center.x - r * 0.05f, center.y - r * 0.60f), r * 0.28f, col);
        break;
    }
    case IconType::CLAPPERBOARD: {
        dl->AddRectFilled(ImVec2(center.x - r * 0.75f, center.y - r * 0.10f),
                          ImVec2(center.x + r * 0.75f, center.y + r * 0.75f), col, 1.0f);
        dl->AddRectFilled(ImVec2(center.x - r * 0.75f, center.y - r * 0.70f),
                          ImVec2(center.x + r * 0.75f, center.y - r * 0.15f), col, 1.0f);
        dl->AddLine(ImVec2(center.x - r * 0.45f, center.y - r * 0.15f),
                    ImVec2(center.x - r * 0.20f, center.y - r * 0.70f), IM_COL32(40, 40, 40, 255), 1.5f);
        dl->AddLine(ImVec2(center.x + r * 0.05f, center.y - r * 0.15f),
                    ImVec2(center.x + r * 0.30f, center.y - r * 0.70f), IM_COL32(40, 40, 40, 255), 1.5f);
        break;
    }
    }
}

inline bool IconToggle(const char* id, IconType icon, bool active, ImVec2 size = ImVec2(22.0f, 20.0f), const char* tooltip = nullptr, ImU32 activeBg = IM_COL32(71, 114, 179, 255)) {
    ImGui::PushID(id);
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, activeBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeBg);
    }
    bool pressed = ImGui::Button("", size);
    if (active) {
        ImGui::PopStyleColor(2);
    }
    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    ImVec2 center((pMin.x + pMax.x) * 0.5f, (pMin.y + pMax.y) * 0.5f);
    float iconSz = (std::min)(size.x, size.y) * 0.65f;
    ImU32 col = active ? IM_COL32(255, 255, 255, 255) : ImGui::GetColorU32(ImGuiCol_Text);
    DrawMiniIcon(ImGui::GetWindowDrawList(), icon, center, iconSz, col);
    if (tooltip && tooltip[0] != '\0' && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
    return pressed;
}

inline bool IconButton(const char* id, IconType icon, ImVec2 size = ImVec2(22.0f, 20.0f), const char* tooltip = nullptr, ImU32 iconCol = 0) {
    ImGui::PushID(id);
    bool pressed = ImGui::Button("", size);
    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    ImVec2 center((pMin.x + pMax.x) * 0.5f, (pMin.y + pMax.y) * 0.5f);
    float iconSz = (std::min)(size.x, size.y) * 0.65f;
    ImU32 col = (iconCol != 0) ? iconCol : ImGui::GetColorU32(ImGuiCol_Text);
    DrawMiniIcon(ImGui::GetWindowDrawList(), icon, center, iconSz, col);
    if (tooltip && tooltip[0] != '\0' && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
    return pressed;
}

inline bool ButtonWithIcon(IconType icon, const char* label, ImVec2 size = ImVec2(0.0f, 22.0f), const char* tooltip = nullptr, ImU32 iconCol = 0) {
    ImGui::PushID(label);
    std::string paddedLabel = "    " + std::string(label);
    bool pressed = ImGui::Button(paddedLabel.c_str(), size);
    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    ImVec2 iconCenter(pMin.x + 11.0f, (pMin.y + pMax.y) * 0.5f);
    ImU32 col = (iconCol != 0) ? iconCol : ImGui::GetColorU32(ImGuiCol_Text);
    DrawMiniIcon(ImGui::GetWindowDrawList(), icon, iconCenter, 11.0f, col);
    if (tooltip && tooltip[0] != '\0' && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
    return pressed;
}

inline void DrawStatusBadge(const char* text, ImU32 dotColor = IM_COL32(75, 210, 110, 255)) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float lineH = ImGui::GetTextLineHeight();
    ImVec2 dotCenter(p.x + 6.0f, p.y + lineH * 0.5f);
    DrawMiniIcon(ImGui::GetWindowDrawList(), IconType::STATUS_DOT, dotCenter, 8.0f, dotColor);
    ImGui::SetCursorScreenPos(ImVec2(p.x + 15.0f, p.y));
    ImGui::TextUnformatted(text);
}

inline bool BeginProps(const char* table_id = "PropsTable") {
    if (ImGui::BeginTable(table_id, 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.38f);
        ImGui::TableSetupColumn("Widget", ImGuiTableColumnFlags_WidthStretch, 0.62f);
        return true;
    }
    return false;
}

inline void EndProps() {
    ImGui::EndTable();
}

inline void PropLabel(const char* label, const char* tooltip = nullptr) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered()) {
        if (tooltip && tooltip[0] != '\0') {
            ImGui::SetTooltip("%s", tooltip);
        } else if (ImGui::CalcTextSize(label).x > ImGui::GetColumnWidth() - 4.0f) {
            ImGui::SetTooltip("%s", label);
        }
    }
}

inline bool PropFloat(const char* label, float* val, float speed = 0.05f, float minVal = 0.0f, float maxVal = 0.0f, const char* format = "%.2f", const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::DragFloat(idBuf, val, speed, minVal, maxVal, format);

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropSlider(const char* label, float* val, float minVal, float maxVal, const char* format = "%.2f", const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::SliderFloat(idBuf, val, minVal, maxVal, format);

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropSliderInt(const char* label, int* val, int minVal, int maxVal, const char* format = "%d", const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::SliderInt(idBuf, val, minVal, maxVal, format);

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropInt(const char* label, int* val, float speed = 1.0f, int minVal = 0, int maxVal = 0, const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::DragInt(idBuf, val, speed, minVal, maxVal);

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropCheckbox(const char* label, bool* val, const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::Checkbox(idBuf, val);

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropColor3(const char* label, float* col, const char* tooltip = nullptr, int animState = 0, const std::function<void()>& onToggleKeyframe = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    float availW = ImGui::GetContentRegionAvail().x;
    float btnW = (onToggleKeyframe != nullptr) ? 18.0f : 0.0f;
    float inputW = (btnW > 0.0f) ? (availW - btnW - 4.0f) : -FLT_MIN;

    if (animState == 2) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.36f, 0.29f, 0.11f, 1.0f));
    } else if (animState == 1) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.28f, 0.15f, 1.0f));
    }

    ImGui::SetNextItemWidth(inputW);
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::ColorEdit3(idBuf, col);

    if (animState > 0) ImGui::PopStyleColor();

    if (onToggleKeyframe) {
        ImGui::SameLine(0.0f, 4.0f);
        char btnId[64];
        snprintf(btnId, sizeof(btnId), "##anim_%s", label);
        ImVec2 bPos = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton(btnId, ImVec2(18.0f, 18.0f))) {
            onToggleKeyframe();
        }
        bool hov = ImGui::IsItemHovered();
        ImU32 dCol = (animState == 2) ? IM_COL32(245, 200, 50, 255) :
                     (animState == 1) ? IM_COL32(80, 210, 110, 255) :
                     (hov ? IM_COL32(220, 220, 220, 255) : IM_COL32(130, 130, 130, 200));
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 dCenter(bPos.x + 9.0f, bPos.y + 9.0f);
        if (animState == 2) {
            UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, dCenter, 10.0f, dCol);
        } else {
            ImVec2 pts[4] = {
                ImVec2(dCenter.x, dCenter.y - 4.5f),
                ImVec2(dCenter.x + 4.5f, dCenter.y),
                ImVec2(dCenter.x, dCenter.y + 4.5f),
                ImVec2(dCenter.x - 4.5f, dCenter.y)
            };
            dl->AddPolyline(pts, 4, dCol, ImDrawFlags_Closed, 1.2f);
        }
        if (hov) {
            ImGui::SetTooltip((animState == 2) ? "Eliminar Keyframe en Playhead [I]" : "Insertar Keyframe en Playhead [I]");
        }
    }

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropColor4(const char* label, float* col, const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::ColorEdit4(idBuf, col);

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropCombo(const char* label, int* current, const char* const items[], int count, const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "##%s", label);
    bool changed = ImGui::Combo(idBuf, current, items, count);

    if (!insideTable) EndProps();
    return changed;
}

inline bool PropVec3Vertical(const char* label, glm::vec3& vec, float speed = 0.05f, float minVal = 0.0f, float maxVal = 0.0f, const char* format = "%.2f", const char* tooltip = nullptr, int animState = 0, const std::function<void()>& onToggleKeyframe = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return false;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    bool changed = false;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));

    float availW = ImGui::GetContentRegionAvail().x;
    float btnW = (onToggleKeyframe != nullptr) ? 18.0f : 0.0f;
    float inputW = (btnW > 0.0f) ? (availW - btnW - 4.0f) : -FLT_MIN;

    // Tintes de Keyframe estilo Blender 4/5
    if (animState == 2) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.36f, 0.29f, 0.11f, 1.0f)); // Blender Gold #5C4B1B
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.46f, 0.38f, 0.16f, 1.0f));
    } else if (animState == 1) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.28f, 0.15f, 1.0f)); // Blender Olive Green #2E4725
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.24f, 0.36f, 0.20f, 1.0f));
    }

    // Axis X (Rojo #E03C3C)
    {
        ImVec2 pMin = ImGui::GetCursorScreenPos();
        ImGui::SetNextItemWidth(inputW);
        char idBuf[64];
        snprintf(idBuf, sizeof(idBuf), "##X_%s", label);
        if (ImGui::DragFloat(idBuf, &vec.x, speed, minVal, maxVal, format)) changed = true;
        ImVec2 pMax = ImGui::GetItemRectMax();
        dl->AddRectFilled(pMin, ImVec2(pMin.x + 3.0f, pMax.y), IM_COL32(224, 60, 60, 255), 2.0f);

        // Boton de Animacion / Rombo estilo Blender a la derecha
        if (onToggleKeyframe) {
            ImGui::SameLine(0.0f, 4.0f);
            char btnId[64];
            snprintf(btnId, sizeof(btnId), "##anim_%s", label);
            ImVec2 bPos = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton(btnId, ImVec2(18.0f, 18.0f))) {
                onToggleKeyframe();
            }
            bool hov = ImGui::IsItemHovered();
            ImU32 dCol = (animState == 2) ? IM_COL32(245, 200, 50, 255) :
                         (animState == 1) ? IM_COL32(80, 210, 110, 255) :
                         (hov ? IM_COL32(220, 220, 220, 255) : IM_COL32(130, 130, 130, 200));
            ImVec2 dCenter(bPos.x + 9.0f, bPos.y + 9.0f);
            if (animState == 2) {
                UI::DrawMiniIcon(dl, UI::IconType::KEYFRAME, dCenter, 10.0f, dCol);
            } else {
                ImVec2 pts[4] = {
                    ImVec2(dCenter.x, dCenter.y - 4.5f),
                    ImVec2(dCenter.x + 4.5f, dCenter.y),
                    ImVec2(dCenter.x, dCenter.y + 4.5f),
                    ImVec2(dCenter.x - 4.5f, dCenter.y)
                };
                dl->AddPolyline(pts, 4, dCol, ImDrawFlags_Closed, 1.2f);
            }
            if (hov) {
                ImGui::SetTooltip((animState == 2) ? "Eliminar Keyframe en Playhead [I]" : "Insertar Keyframe en Playhead [I]");
            }
        }
    }
    // Axis Y (Verde #47B347)
    {
        ImVec2 pMin = ImGui::GetCursorScreenPos();
        ImGui::SetNextItemWidth(inputW);
        char idBuf[64];
        snprintf(idBuf, sizeof(idBuf), "##Y_%s", label);
        if (ImGui::DragFloat(idBuf, &vec.y, speed, minVal, maxVal, format)) changed = true;
        ImVec2 pMax = ImGui::GetItemRectMax();
        dl->AddRectFilled(pMin, ImVec2(pMin.x + 3.0f, pMax.y), IM_COL32(71, 179, 71, 255), 2.0f);
    }
    // Axis Z (Azul #4772B3)
    {
        ImVec2 pMin = ImGui::GetCursorScreenPos();
        ImGui::SetNextItemWidth(inputW);
        char idBuf[64];
        snprintf(idBuf, sizeof(idBuf), "##Z_%s", label);
        if (ImGui::DragFloat(idBuf, &vec.z, speed, minVal, maxVal, format)) changed = true;
        ImVec2 pMax = ImGui::GetItemRectMax();
        dl->AddRectFilled(pMin, ImVec2(pMin.x + 3.0f, pMax.y), IM_COL32(71, 114, 179, 255), 2.0f);
    }

    if (animState > 0) {
        ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleVar();

    if (!insideTable) EndProps();
    return changed;
}

inline void PropTextureSlot(const char* label, const std::string& texPath, bool hasTexture, 
                            const std::function<void()>& onLoad, 
                            const std::function<void()>& onRemove = nullptr, 
                            const char* tooltip = nullptr) {
    bool insideTable = (ImGui::GetCurrentTable() != nullptr);
    if (!insideTable && !BeginProps(label)) return;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    PropLabel(label, tooltip);

    ImGui::TableNextColumn();
    char btnId[64];
    snprintf(btnId, sizeof(btnId), "##load_%s", label);
    if (IconButton(btnId, IconType::FOLDER, ImVec2(22.0f, 20.0f), "Cargar textura...")) {
        if (onLoad) onLoad();
    }

    ImGui::SameLine();
    if (hasTexture) {
        std::string fname = std::filesystem::path(texPath).filename().string();
        if (fname.empty()) fname = "Textura";
        std::string dispName = fname;
        if (dispName.length() > 14) {
            dispName = dispName.substr(0, 11) + "...";
        }
        ImGui::TextColored(ImVec4(0.40f, 0.85f, 0.45f, 1.0f), "%s", dispName.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", texPath.c_str());
        }

        if (onRemove) {
            ImGui::SameLine();
            char rmId[64];
            snprintf(rmId, sizeof(rmId), "##rm_%s", label);
            if (IconButton(rmId, IconType::TRASH, ImVec2(20.0f, 18.0f), "Quitar textura", IM_COL32(230, 80, 80, 255))) {
                onRemove();
            }
        }
    } else {
        ImGui::TextDisabled("Sin asignar");
    }

    if (!insideTable) EndProps();
}

/**
 * @brief Renderizador Vectorial Exacto del Isotipo Oficial de ASIS (Fase 22.5)
 * Basado con exactitud matemática en asisengine.svg: Triángulo blanco puro (#FFFFFF)
 * con 4 cortes horizontales negativos en la base izquierda.
 */
inline void DrawASISLogo(ImDrawList* dl, ImVec2 pos, float size, ImU32 col = 0, ImU32 bgCol = 0) {
    if (!dl) return;
    if (col == 0) col = IM_COL32(255, 255, 255, 255);
    if (bgCol == 0) bgCol = IM_COL32(29, 29, 29, 255); // Fondo oscuro Blender

    float scaleX = size * (390.0f / 419.0f);
    float scaleY = size;
    float offsetX = (size - scaleX) * 0.5f;

    // Vértices del triángulo principal blanco:
    // Apex: (0.50f, 0.00f)
    // Inferior derecho: (1.00f, 1.00f)
    // Inferior izquierdo: (0.00f, 1.00f)
    ImVec2 pApex(pos.x + offsetX + 0.50f * scaleX, pos.y);
    ImVec2 pBottomRight(pos.x + offsetX + scaleX, pos.y + scaleY);
    ImVec2 pBottomLeft(pos.x + offsetX, pos.y + scaleY);

    // 1. Dibujar el triángulo blanco sólido sin líneas internas ni costuras
    dl->AddTriangleFilled(pApex, pBottomRight, pBottomLeft, col);

    // 2. Dibujar los 4 trapecios de corte negativo rellenos con bgCol
    // Ecuación del borde derecho de los cortes:
    // x_right(y) = 0.370f + (y - 0.674f) * 0.46f
    // Rango de los cortes en Y normalizado:
    // Corte 1: y de 0.674f a 0.708f
    // Corte 2: y de 0.752f a 0.786f
    // Corte 3: y de 0.830f a 0.864f
    // Corte 4: y de 0.908f a 0.942f
    static const float cuts[4][2] = {
        { 0.674f, 0.708f },
        { 0.752f, 0.786f },
        { 0.830f, 0.864f },
        { 0.908f, 0.942f }
    };

    float minThickness = (size <= 22.0f) ? 1.6f : 1.0f;

    for (int i = 0; i < 4; ++i) {
        float y0 = cuts[i][0];
        float y1 = cuts[i][1];

        float py0 = pos.y + y0 * scaleY;
        float py1 = pos.y + y1 * scaleY;

        // Garantizar grosor mínimo en píxeles para tamaños pequeños
        if ((py1 - py0) < minThickness) {
            float center = (py0 + py1) * 0.5f;
            py0 = center - minThickness * 0.5f;
            py1 = center + minThickness * 0.5f;
        }

        // Recalcular y0, y1 efectivos para el borde derecho
        float eff_y0 = (py0 - pos.y) / scaleY;
        float eff_y1 = (py1 - pos.y) / scaleY;

        float xr0 = 0.370f + (eff_y0 - 0.674f) * 0.46f;
        float xr1 = 0.370f + (eff_y1 - 0.674f) * 0.46f;

        float px_right0 = pos.x + offsetX + xr0 * scaleX;
        float px_right1 = pos.x + offsetX + xr1 * scaleX;

        // El corte se extiende completamente hacia la izquierda fuera del triángulo
        float px_left = pos.x + offsetX - 2.0f;

        dl->AddQuadFilled(
            ImVec2(px_left, py0),
            ImVec2(px_right0, py0),
            ImVec2(px_right1, py1),
            ImVec2(px_left, py1),
            bgCol
        );
    }
}

/**
 * @brief Gizmo de Navegacion 3D estilo Blender (Fase 21.8)
 * Dibuja los ejes X, Y, Z proyectados segun la orientacion de la camara,
 * ordenados por profundidad (Z-sort) con interactividad de clic para orientar cardinalmente.
 */
inline bool DrawNavigationGizmo(ImDrawList* dl, Camera& camera, ImVec2 center, float radius) {
    if (!dl) return false;

    // Fondo circular sutil translucido
    dl->AddCircleFilled(center, radius, IM_COL32(28, 28, 28, 160));
    dl->AddCircle(center, radius, IM_COL32(55, 55, 55, 200), 24, 1.0f);

    glm::vec3 right = camera.getRight();
    glm::vec3 up = camera.getUp();
    glm::vec3 front = camera.getFront();

    struct AxisInfo {
        char name;
        glm::vec3 dir;
        ImU32 color;
        float depth;
        ImVec2 screenPos;
        bool isPositive;
        float targetYaw;
        float targetPitch;
    };

    float armLen = radius * 0.72f;

    AxisInfo axes[6] = {
        { 'X', glm::vec3( 1, 0, 0), IM_COL32(230,  59,  59, 255), 0.0f, ImVec2(0,0), true,  180.0f,   0.0f },
        { 'Y', glm::vec3( 0, 1, 0), IM_COL32( 71, 179,  71, 255), 0.0f, ImVec2(0,0), true,  -90.0f, -89.0f },
        { 'Z', glm::vec3( 0, 0, 1), IM_COL32( 71, 114, 179, 255), 0.0f, ImVec2(0,0), true,  -90.0f,   0.0f },
        { 'X', glm::vec3(-1, 0, 0), IM_COL32(120,  60,  60, 180), 0.0f, ImVec2(0,0), false,    0.0f,   0.0f },
        { 'Y', glm::vec3( 0,-1, 0), IM_COL32( 60, 120,  60, 180), 0.0f, ImVec2(0,0), false,  -90.0f,  89.0f },
        { 'Z', glm::vec3( 0, 0,-1), IM_COL32( 60,  80, 120, 180), 0.0f, ImVec2(0,0), false,   90.0f,   0.0f }
    };

    for (int i = 0; i < 6; ++i) {
        float sx = glm::dot(axes[i].dir, right);
        float sy = -glm::dot(axes[i].dir, up);
        axes[i].depth = -glm::dot(axes[i].dir, front);
        axes[i].screenPos = ImVec2(center.x + sx * armLen, center.y + sy * armLen);
    }

    // Ordenar de atras hacia adelante (depth menor a mayor)
    int order[6] = { 0, 1, 2, 3, 4, 5 };
    std::sort(order, order + 6, [&](int a, int b) {
        return axes[a].depth < axes[b].depth;
    });

    ImGuiIO& io = ImGui::GetIO();
    bool clickedAny = false;
    int clickedAxisIdx = -1;

    for (int k = 0; k < 6; ++k) {
        int idx = order[k];
        const auto& ax = axes[idx];

        if (!ax.isPositive) {
            // Eje negativo: linea tenue y punto gris
            dl->AddLine(center, ax.screenPos, IM_COL32(100, 100, 100, 120), 1.2f);
            dl->AddCircleFilled(ax.screenPos, 3.0f, IM_COL32(130, 130, 130, 180));
        } else {
            // Eje positivo: linea gruesa coloreada y chapa circular con letra
            dl->AddLine(center, ax.screenPos, ax.color, 2.5f);

            float tipRadius = 7.5f;
            float distToMouse = glm::length(glm::vec2(io.MousePos.x - ax.screenPos.x, io.MousePos.y - ax.screenPos.y));
            bool isHovered = (distToMouse <= tipRadius + 2.0f);

            if (isHovered && io.MouseClicked[0]) {
                clickedAny = true;
                clickedAxisIdx = idx;
            }

            ImU32 circleCol = isHovered ? IM_COL32(255, 255, 255, 255) : ax.color;
            dl->AddCircleFilled(ax.screenPos, tipRadius, circleCol);

            char lbl[2] = { ax.name, 0 };
            ImVec2 tsz = ImGui::CalcTextSize(lbl);
            ImU32 textCol = isHovered ? IM_COL32(20, 20, 20, 255) : IM_COL32(255, 255, 255, 255);
            dl->AddText(ImVec2(ax.screenPos.x - tsz.x * 0.5f, ax.screenPos.y - tsz.y * 0.5f), textCol, lbl);
        }
    }

    if (clickedAny && clickedAxisIdx >= 0) {
        const auto& cAx = axes[clickedAxisIdx];
        glm::vec3 tgt = camera.getTarget();
        float dist = glm::length(camera.getPosition() - tgt);
        if (dist < 1.0f) dist = 6.0f;
        camera.setYawPitch(cAx.targetYaw, cAx.targetPitch);
        camera.setPosition(tgt - camera.getFront() * dist);
        camera.syncOrbitDistance();
        return true;
    }

    return false;
}

/**
 * @brief Ilustraciones vectoriales de activos/prefabs para el Navegador de Activos (Fase 21.8)
 */
inline void DrawAssetThumbnail(ImDrawList* dl, const std::string& name, ImVec2 pos, ImVec2 size) {
    if (!dl) return;
    ImVec2 pMin = pos;
    ImVec2 pMax = ImVec2(pos.x + size.x, pos.y + size.y);

    dl->AddRectFilled(pMin, pMax, IM_COL32(29, 29, 29, 255), 4.0f);
    dl->AddRect(pMin, pMax, IM_COL32(48, 48, 48, 255), 4.0f, 0, 1.0f);

    ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    float h = size.y * 0.32f;
    float w = size.x * 0.28f;

    std::string lower = name;
    for (char& c : lower) c = (char)::tolower((unsigned char)c);

    if (lower.find("barril") != std::string::npos || lower.find("barrel") != std::string::npos || lower.find("explosiv") != std::string::npos) {
        // Barril explosivo rojo/naranja con bandas y franja de peligro
        ImVec2 bMin(center.x - w * 0.75f, center.y - h);
        ImVec2 bMax(center.x + w * 0.75f, center.y + h);
        dl->AddRectFilled(bMin, bMax, IM_COL32(195, 45, 35, 255), 3.0f);
        dl->AddRectFilled(ImVec2(bMin.x, center.y - h * 0.25f), ImVec2(bMax.x, center.y + h * 0.25f), IM_COL32(245, 195, 30, 255));
        dl->AddLine(ImVec2(bMin.x, center.y - h * 0.25f), ImVec2(bMax.x, center.y - h * 0.25f), IM_COL32(20, 20, 20, 255), 1.0f);
        dl->AddLine(ImVec2(bMin.x, center.y + h * 0.25f), ImVec2(bMax.x, center.y + h * 0.25f), IM_COL32(20, 20, 20, 255), 1.0f);
        dl->AddLine(ImVec2(bMin.x, bMin.y + 4.0f), ImVec2(bMax.x, bMin.y + 4.0f), IM_COL32(40, 40, 40, 255), 1.5f);
        dl->AddLine(ImVec2(bMin.x, bMax.y - 4.0f), ImVec2(bMax.x, bMax.y - 4.0f), IM_COL32(40, 40, 40, 255), 1.5f);
    } else if (lower.find("caja") != std::string::npos || lower.find("madera") != std::string::npos || lower.find("crate") != std::string::npos) {
        // Caja de madera con cruceta X
        ImVec2 cMin(center.x - w, center.y - h * 0.95f);
        ImVec2 cMax(center.x + w, center.y + h * 0.95f);
        dl->AddRectFilled(cMin, cMax, IM_COL32(150, 95, 45, 255), 2.0f);
        dl->AddRect(cMin, cMax, IM_COL32(95, 55, 25, 255), 2.0f, 0, 1.5f);
        dl->AddLine(cMin, cMax, IM_COL32(95, 55, 25, 255), 1.5f);
        dl->AddLine(ImVec2(cMin.x, cMax.y), ImVec2(cMax.x, cMin.y), IM_COL32(95, 55, 25, 255), 1.5f);
    } else if (lower.find("portal") != std::string::npos || lower.find("anillo") != std::string::npos || lower.find("ring") != std::string::npos) {
        // Anillo de portal cian brillante
        dl->AddCircleFilled(center, h * 0.9f, IM_COL32(15, 30, 50, 255));
        dl->AddCircle(center, h * 0.9f, IM_COL32(30, 210, 240, 255), 20, 2.5f);
        dl->AddCircle(center, h * 0.55f, IM_COL32(80, 240, 255, 200), 16, 1.5f);
        dl->AddCircleFilled(center, h * 0.3f, IM_COL32(180, 255, 255, 255));
    } else if (lower.find("moneda") != std::string::npos || lower.find("coin") != std::string::npos || lower.find("oro") != std::string::npos) {
        // Moneda dorada
        dl->AddCircleFilled(center, h * 0.9f, IM_COL32(235, 185, 30, 255));
        dl->AddCircle(center, h * 0.9f, IM_COL32(255, 225, 80, 255), 20, 1.5f);
        dl->AddCircle(center, h * 0.65f, IM_COL32(195, 145, 20, 255), 16, 1.0f);
        dl->AddCircleFilled(ImVec2(center.x - h * 0.3f, center.y - h * 0.3f), h * 0.25f, IM_COL32(255, 255, 255, 150));
    } else {
        // Cubo 3D isometrico generico
        float r = h * 0.85f;
        ImVec2 top[4] = {
            ImVec2(center.x, center.y - r),
            ImVec2(center.x + r * 0.866f, center.y - r * 0.5f),
            ImVec2(center.x, center.y),
            ImVec2(center.x - r * 0.866f, center.y - r * 0.5f)
        };
        ImVec2 left[4] = {
            ImVec2(center.x - r * 0.866f, center.y - r * 0.5f),
            ImVec2(center.x, center.y),
            ImVec2(center.x, center.y + r),
            ImVec2(center.x - r * 0.866f, center.y + r * 0.5f)
        };
        ImVec2 right[4] = {
            ImVec2(center.x, center.y),
            ImVec2(center.x + r * 0.866f, center.y - r * 0.5f),
            ImVec2(center.x + r * 0.866f, center.y + r * 0.5f),
            ImVec2(center.x, center.y + r)
        };
        dl->AddQuadFilled(top[0], top[1], top[2], top[3], IM_COL32(110, 140, 190, 255));
        dl->AddQuadFilled(left[0], left[1], left[2], left[3], IM_COL32(70, 95, 140, 255));
        dl->AddQuadFilled(right[0], right[1], right[2], right[3], IM_COL32(50, 70, 110, 255));
        dl->AddQuad(top[0], top[1], top[2], top[3], IM_COL32(140, 170, 220, 255), 1.0f);
        dl->AddQuad(left[0], left[1], left[2], left[3], IM_COL32(140, 170, 220, 255), 1.0f);
        dl->AddQuad(right[0], right[1], right[2], right[3], IM_COL32(140, 170, 220, 255), 1.0f);
    }
}

} // namespace UI

enum class ViewportShadingMode {
    WIREFRAME,
    SOLID,
    OASIS_MESHLETS,
    RENDERED
};

class EditorUI {
public:
    EditorUI();
    ~EditorUI();

    // Inicializar ImGui, fuentes vectoriales Segoe UI y sistema de Docking
    void init(GLFWwindow* window);

    // Renderizar la interfaz completa del editor con Viewport FBO, Gizmos y Panel Blender
    void render(Scene& scene, ScriptEngine& scriptEngine, Camera& camera, Framebuffer& framebuffer, 
                PhysicsWorld& physicsWorld, ShadowMap& shadowMap, Skybox& skybox, 
                ParticleSystem& particleSystem, SSAO& ssao, Terrain& terrain, CharacterController& character, float deltaTime, GLuint displayTexture = 0, Water* water = nullptr);

    // Control del Modo de Juego (Play-in-Editor / PIE - Fase 15)
    bool isPlaying() const { return m_playMode == EnginePlayMode::PLAYING; }
    bool isPlayPaused() const { return m_playMode == EnginePlayMode::PAUSED; }
    void setPlayMode(EnginePlayMode mode) { m_playMode = mode; }
    EnginePlayMode getPlayMode() const { return m_playMode; }

    bool requestPlayStart() const { return m_requestPlayStart; }
    bool requestPlayStop() const { return m_requestPlayStop; }
    void clearPlayRequests() { m_requestPlayStart = false; m_requestPlayStop = false; }
    void triggerPlayStart() { m_requestPlayStart = true; m_requestPlayStop = false; }
    void triggerPlayStop() { m_requestPlayStop = true; m_requestPlayStart = false; }

    void saveEditorCamera(const Camera& camera);
    void restoreEditorCamera(Camera& camera);

    // Liberar recursos de ImGui
    void shutdown();

    bool isVisible() const { return m_visible; }
    void toggleVisibility() { m_visible = !m_visible; }

    const std::string& getSelectedEntityName() const { return m_selectedEntityName; }
    void selectEntity(const std::string& name);
    bool isEntitySelected(const std::string& name) const;
    void toggleEntitySelection(const std::string& name);
    void selectEntityRange(const std::string& targetName, const std::vector<std::string>& allNames);
    void clearEntitySelection();
    const std::unordered_set<std::string>& getSelectedEntityNames() const { return m_selectedEntityNames; }

    bool isGroundPlaneDeleted() const { return m_groundPlaneDeleted; }
    void setGroundPlaneDeleted(bool del) { m_groundPlaneDeleted = del; }

    // Generar escena desde prompt de lenguaje natural (en segundo plano asíncrono)
    void generateFromPrompt(const std::string& prompt);

    // Consultas de estado del Viewport FBO y Gizmos
    bool isViewportHovered() const { return m_isViewportHovered; }
    bool isViewportFocused() const { return m_isViewportFocused; }
    bool isUsingGizmo() const { return m_isUsingGizmo; }
    glm::vec2 getViewportSize() const { return m_viewportSize; }

    // Control de ejecución de Scripts / Animaciones (Play / Pause)
    bool isScriptsPaused() const { return m_scriptsPaused; }
    void setScriptsPaused(bool paused) { m_scriptsPaused = paused; }

    // Parámetros de Iluminación y Sombras (Fase 7 y 10)
    bool getShadowsEnabled() const { return m_shadowsEnabled; }
    glm::vec3 getLightDir() const;
    glm::vec3 getLightColor() const { return m_lightColor * m_lightIntensity; }
    void setLightIntensity(float intensity) { m_lightIntensity = intensity; }
    void setLightColor(const glm::vec3& color) { m_lightColor = color; }
    glm::vec3 getAmbientColor() const { return m_ambientColor * m_ambientIntensity; }
    float getShadowBias() const { return m_shadowBias; }
    int getShadowResolution() const { return m_shadowResolution; }
    int getShadowKernel() const { return m_shadowKernel; }
    float getShadowSoftness() const { return m_shadowSoftness; }
    float getShadowDistance() const { return m_shadowDistance; }
    void setShadowDistance(float d) { m_shadowDistance = d; }
    bool getTerrainCastsShadows() const { return m_terrainCastsShadows; }
    void setTerrainCastsShadows(bool enable) { m_terrainCastsShadows = enable; }
    int getAnisotropyLevel() const { return m_anisotropyLevel; }
    void setAnisotropyLevel(int level) { m_anisotropyLevel = level; }

    // Sombras Suaves Físicas PCSS y Cascadas CSM
    int getShadowFilterMode() const { return m_shadowFilterMode; }
    void setShadowFilterMode(int mode) { m_shadowFilterMode = mode; }
    float getPCSSLightSize() const { return m_pcssLightSize; }
    void setPCSSLightSize(float s) { m_pcssLightSize = s; }
    int getPCSSBlockerSamples() const { return m_pcssBlockerSamples; }
    void setPCSSBlockerSamples(int s) { m_pcssBlockerSamples = s; }
    int getPCSSFilterSamples() const { return m_pcssFilterSamples; }
    void setPCSSFilterSamples(int s) { m_pcssFilterSamples = s; }

    bool isCSMEnabled() const { return m_csmEnabled; }
    void setCSMEnabled(bool e) { m_csmEnabled = e; }
    bool isCSMDebugColors() const { return m_csmDebugColors; }
    void setCSMDebugColors(bool d) { m_csmDebugColors = d; }

    // Gestión de Color y Mapeo Tonal (Fase 11)
    int getColorProfile() const { return m_colorProfile; }
    float getExposure() const {
        if (m_autoExposureEnabled) {
            return m_currentAdaptedExposure * m_exposure;
        }
        return m_exposure;
    }
    void setExposure(float exp) { m_exposure = exp; }
    float getGamma() const { return m_gamma; }
    float getSaturation() const { return m_saturation; }

    // Modos de Shading y Overlays (Fase 21.8)
    bool isWireframeMode() const { return m_wireframeMode || m_shadingMode == ViewportShadingMode::WIREFRAME; }
    void setWireframeMode(bool w) { m_wireframeMode = w; }
    ViewportShadingMode getShadingMode() const { return m_shadingMode; }
    void setShadingMode(ViewportShadingMode m) { m_shadingMode = m; }
    bool getShowOverlays() const { return m_showOverlays; }
    void setShowOverlays(bool o) { m_showOverlays = o; }
    float getContrast() const { return m_contrast; }

    // Adaptación Ocular y Auto-Exposición Cinemática (Fase 17)
    bool isAutoExposureEnabled() const { return m_autoExposureEnabled; }
    void setAutoExposureEnabled(bool enabled) { m_autoExposureEnabled = enabled; }
    float getAutoExposureKey() const { return m_autoExposureKey; }
    void setAutoExposureKey(float key) { m_autoExposureKey = key; }
    float getAutoExposureMin() const { return m_autoExposureMin; }
    void setAutoExposureMin(float minVal) { m_autoExposureMin = minVal; }
    float getAutoExposureMax() const { return m_autoExposureMax; }
    void setAutoExposureMax(float maxVal) { m_autoExposureMax = maxVal; }
    float getAutoExposureSpeedUp() const { return m_autoExposureSpeedUp; }
    void setAutoExposureSpeedUp(float speed) { m_autoExposureSpeedUp = speed; }
    float getAutoExposureSpeedDown() const { return m_autoExposureSpeedDown; }
    void setAutoExposureSpeedDown(float speed) { m_autoExposureSpeedDown = speed; }
    float getCurrentAdaptedExposure() const { return m_currentAdaptedExposure; }
    float getMeasuredSceneLuminance() const { return m_measuredSceneLuminance; }
    void updateAutoExposure(float measuredLuminance, float deltaTime);

    // Visualización de Suelo / Cuadrícula (Fase 11)
    bool getShowGroundPlane() const { return m_showGroundPlane && !m_groundPlaneDeleted; }
    bool getShowGroundGrid() const { return m_showGroundGrid; }
    void setShowGroundPlane(bool v) { m_showGroundPlane = v; }
    void setShowGroundGrid(bool v) { m_showGroundGrid = v; }

    // Sistema de Deshacer / Rehacer (Fase 11 y 16)
    void recordSnapshot(const Scene& scene, Terrain* terrain = nullptr);
    void performUndo(Scene& scene, PhysicsWorld& physicsWorld, Terrain* terrain = nullptr);
    void performRedo(Scene& scene, PhysicsWorld& physicsWorld, Terrain* terrain = nullptr);
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    // Instantánea previa al Modo Juego (Play-in-Editor) para restaurar posiciones
    void savePrePlaySnapshot(const Scene& scene);
    void restorePrePlaySnapshot(Scene& scene, PhysicsWorld& physicsWorld);

    // Selección de Luces en Jerarquía (Fase 11)
    int getSelectedLightIndex() const { return m_selectedLightIndex; }
    void selectLight(int index) { m_selectedLightIndex = index; m_selectedEntityName = ""; }

    // VSync y Rendimiento
    bool isVSyncEnabled() const { return m_vsyncEnabled; }
    int getTargetFPS() const { return m_targetFPS; }

    // Iluminación Basada en Imágenes (IBL) y Skybox HDRI (Fase 12)
    bool getUseIBL() const { return m_useIBL; }
    bool getShowSkybox() const { return m_showSkybox; }
    float getSkyboxExposure() const { return m_skyboxExposure; }
    float getSkyboxRotation() const { return m_skyboxRotation; }
    float getSkyboxBlur() const { return m_skyboxBlur; }
    float getIBLIntensity() const { return m_iblIntensity; }
    float getTerrainReflectionIntensity() const { return m_terrainReflectionIntensity; }
    void setTerrainReflectionIntensity(float val) { m_terrainReflectionIntensity = val; }
    bool getFlipSkyboxX() const { return m_flipSkyboxX; }
    bool getFlipSkyboxY() const { return m_flipSkyboxY; }
    void setFlipSkyboxX(bool v) { m_flipSkyboxX = v; }
    void setFlipSkyboxY(bool v) { m_flipSkyboxY = v; }

    // Sistema OASIS Módulo 2: Meshlets de Geometría Virtual (Fase 18)
    OASISMeshletConfig& getMeshletConfig() { return m_meshletConfig; }
    const OASISMeshletConfig& getMeshletConfig() const { return m_meshletConfig; }
    OASISMeshletTelemetry& getMeshletTelemetry() { return m_meshletTelemetry; }
    const OASISMeshletTelemetry& getMeshletTelemetry() const { return m_meshletTelemetry; }

    // Post-procesado (Bloom HDR y Antialiasing Real) (Fase 12)
    int getMSAALevel() const { return m_msaaLevel; }
    bool isFXAAEnabled() const { return m_fxaaEnabled; }
    bool isBloomEnabled() const { return m_bloomEnabled; }
    float getBloomThreshold() const { return m_bloomThreshold; }
    float getBloomIntensity() const { return m_bloomIntensity; }

    // Oclusión Ambiental en Espacio de Pantalla (SSAO) (Fase 13)
    bool isSSAOEnabled() const { return m_ssaoEnabled; }
    float getSSAORadius() const { return m_ssaoRadius; }
    float getSSAOBias() const { return m_ssaoBias; }
    float getSSAOPower() const { return m_ssaoPower; }
    float getSSAOIntensity() const { return m_ssaoIntensity; }
    void setSSAOEnabled(bool enabled) { m_ssaoEnabled = enabled; }

    // Profundidad de Campo Fotográfica (Depth of Field / Bokeh) (Fase 13)
    bool isDoFEnabled() const { return m_dofEnabled; }
    float getDoFFocusDistance() const { return m_dofFocusDistance; }
    float getDoFFocusRange() const { return m_dofFocusRange; }
    float getDoFBokehRadius() const { return m_dofBokehRadius; }
    void setDoFEnabled(bool enabled) { m_dofEnabled = enabled; }
    void setDoFFocusDistance(float dist) { m_dofFocusDistance = dist; }
    void setDoFBokehRadius(float radius) { m_dofBokehRadius = radius; }

    // Ciclo Día / Noche Dinámico y Clima (Fase 20)
    DayNightCycle& getDayNightCycle() { return m_dayNight; }
    const DayNightCycle& getDayNightCycle() const { return m_dayNight; }

    // Screen-Space Reflections (SSR - Fase 20)
    bool isSSREnabled() const { return m_ssrEnabled; }
    void setSSREnabled(bool e) { m_ssrEnabled = e; }
    int getSSRMaxSteps() const { return m_ssrMaxSteps; }
    void setSSRMaxSteps(int steps) { m_ssrMaxSteps = steps; }
    float getSSRStepSize() const { return m_ssrStepSize; }
    void setSSRStepSize(float s) { m_ssrStepSize = s; }
    float getSSRThickness() const { return m_ssrThickness; }
    void setSSRThickness(float t) { m_ssrThickness = t; }
    float getSSRIntensity() const { return m_ssrIntensity; }
    void setSSRIntensity(float i) { m_ssrIntensity = i; }

    // Contact Shadows (Fase 20)
    bool isContactShadowsEnabled() const { return m_contactShadowsEnabled; }
    void setContactShadowsEnabled(bool e) { m_contactShadowsEnabled = e; }
    float getContactShadowsDistance() const { return m_contactShadowsDistance; }
    void setContactShadowsDistance(float d) { m_contactShadowsDistance = d; }
    int getContactShadowsRaySteps() const { return m_contactShadowsRaySteps; }
    void setContactShadowsRaySteps(int steps) { m_contactShadowsRaySteps = steps; }
    float getContactShadowsIntensity() const { return m_contactShadowsIntensity; }
    void setContactShadowsIntensity(float i) { m_contactShadowsIntensity = i; }

    // Captura de Pantalla / Render a Imagen (F12) (Fase 12 y 13)
    bool takeRenderImageRequest() { bool r = m_requestRenderImage; m_requestRenderImage = false; return r; }
    void triggerRenderImage() { m_requestRenderImage = true; }
    void notifyRenderComplete(const std::string& fullPath);
    static void openRendersFolder();
    static void openFileWithDefaultApp(const std::string& path);

    // Gestión Integral de Proyectos y Persistencia Completa (.asis)
    bool saveProject(const std::string& path, const Scene& scene, const Camera& camera, const Terrain& terrain, const Skybox& skybox, const Water* water = nullptr);
    bool loadProject(const std::string& path, Scene& scene, Camera& camera, Terrain& terrain, Skybox& skybox, PhysicsWorld& physicsWorld, ScriptEngine& scriptEngine, Water* water = nullptr);
    void newProject(Scene& scene, Camera& camera, Terrain& terrain, Skybox& skybox, PhysicsWorld& physicsWorld, ScriptEngine& scriptEngine, Water* water = nullptr);

    // Acceso al modal de preferencias y añadir
    void openPreferences() { m_showPreferencesModal = true; }
    void triggerAddMenu() { m_openAddMenu = true; }

    // Presets de Layouts de UI (Fase 21)
    void setLayoutPreset(UILayoutPreset preset);
    UILayoutPreset getLayoutPreset() const { return m_currentLayout; }

    // Duplicación y Atajos Profesionales (Fase 21)
    std::shared_ptr<Entity> duplicateEntity(Scene& scene, PhysicsWorld& physicsWorld, const std::string& name);
    void deleteSelectedEntities(Scene& scene, PhysicsWorld& physicsWorld);
    void focusCameraOnSelection(Camera& camera, Scene& scene);

private:
    bool m_initialized;
    bool m_visible;
    bool m_layoutBuilt;
    std::string m_selectedEntityName;
    std::unordered_set<std::string> m_selectedEntityNames;
    std::string m_selectionAnchor;
    bool m_groundPlaneDeleted;

    // Estados de renombrado en Outliner
    std::string m_renamingEntityName;
    char m_renameEntityBuf[128];
    std::string m_renamingGroupName;
    char m_renameGroupBuf[128];

    // Modal de creación de grupo
    bool m_showGroupCreationModal;
    char m_newGroupNameBuf[128];

    // Pestaña activa en la barra vertical estilo Blender
    BlenderPropertyTab m_activePropertyTab;

    // Métricas de rendimiento
    float m_fps;
    float m_frameTimeMs;
    float m_fpsUpdateTimer;

    // Fuentes
    ImFont* m_mainFont;
    ImFont* m_monoFont;

    // Estado del Viewport FBO
    glm::vec2 m_viewportSize;
    glm::vec2 m_viewportPos;
    bool m_isViewportHovered;
    bool m_isViewportFocused;
    bool m_isUsingGizmo;

    // Control de Scripts / Animaciones
    bool m_scriptsPaused;

    // Configuración de Gizmos 3D (ImGuizmo)
    int m_gizmoOperation; // TRANSLATE, ROTATE, SCALE
    int m_gizmoMode;      // LOCAL, WORLD
    bool m_useSnap;
    float m_snapTranslation;
    float m_snapRotation;
    float m_snapScale;

    // Parámetros de Iluminación y Sombras Dinámicas (Fase 7 y 10)
    bool m_shadowsEnabled;
    float m_sunAzimuth;
    float m_sunElevation;
    glm::vec3 m_lightColor;
    float m_lightIntensity;
    glm::vec3 m_ambientColor;
    float m_ambientIntensity;
    float m_shadowBias;
    int m_shadowResolution;  // 1024, 2048, 4096
    int m_shadowKernel;      // 1 = 3x3, 2 = 5x5, 3 = 7x7
    float m_shadowSoftness;  // Radio de dispersión de penumbra
    float m_shadowDistance;  // Alcance de sombras ortográficas en metros
    bool m_terrainCastsShadows; // Proyección de sombra propia del terreno
    int m_anisotropyLevel;   // Nivel de filtrado anisotrópico (1, 2, 4, 8, 16)
    int m_shadowFilterMode;       // 0 = Tradicional PCF, 1 = PCSS
    float m_pcssLightSize;        // Tamaño físico aparente de la fuente de luz
    int m_pcssBlockerSamples;     // Muestras para búsqueda de bloqueador
    int m_pcssFilterSamples;      // Muestras para filtro de penumbra
    bool m_csmEnabled;            // Cascaded Shadow Maps
    bool m_csmDebugColors;        // Visualizar cascadas en rojo/verde/azul

    // Estado del Asistente de IA nativo
    char m_promptBuffer[512];
    std::atomic<bool> m_isGenerating;
    std::string m_generationStatus;
    std::mutex m_statusMutex;
    float m_generationTimer;

    // Gestión de Color y Mapeo Tonal (Fase 11)
    int m_colorProfile;   // 0 = ACES Filmic, 1 = AgX, 2 = Reinhard, 3 = Linear
    float m_exposure;
    float m_gamma;
    float m_saturation;
    float m_contrast;

    // Adaptación Ocular y Auto-Exposición Cinemática (Fase 17)
    bool m_autoExposureEnabled = true;
    float m_autoExposureKey = 0.39f;
    float m_autoExposureMin = 0.50f;
    float m_autoExposureMax = 3.24f;
    float m_autoExposureSpeedUp = 3.0f;
    float m_autoExposureSpeedDown = 1.0f;
    float m_currentAdaptedExposure = 1.0f;
    float m_measuredSceneLuminance = 0.473f;

    // Suelo y Cuadrícula (Fase 11)
    bool m_showGroundPlane;
    bool m_showGroundGrid;

    // Selección de Luces en Jerarquía (Fase 11)
    int m_selectedLightIndex;

    // Historial de Deshacer y Rehacer (Fase 11)
    std::vector<SceneSnapshot> m_undoStack;
    std::vector<SceneSnapshot> m_redoStack;
    int m_maxUndoSteps;
    bool m_wasUsingGizmo;
    SceneSnapshot m_prePlaySnapshot;
    bool m_hasPrePlaySnapshot;

    // Modal de Preferencias y Configuración (Fase 11)
    bool m_showPreferencesModal;
    bool m_vsyncEnabled;
    int m_targetFPS;
    int m_msaaLevel;
    UITheme m_currentTheme;
    ImVec4 m_customAccentColor;
    bool m_useBlenderShortcuts;
    float m_flycamSpeed;
    bool m_autoSaveEnabled;
    float m_autoSaveIntervalMinutes;
    float m_autoSaveTimer;

    // Modal de Añadir Primitiva (Shift+A)
    bool m_openAddMenu;

    // Iluminación Basada en Imágenes (IBL) y Skybox HDRI (Fase 12)
    bool m_useIBL;
    bool m_showSkybox;
    float m_skyboxExposure;
    float m_skyboxRotation;
    float m_skyboxBlur;
    float m_iblIntensity;
    float m_terrainReflectionIntensity;
    bool m_flipSkyboxX;
    bool m_flipSkyboxY;

    // Post-procesado (Bloom HDR y Antialiasing Real) (Fase 12)
    bool m_fxaaEnabled;
    bool m_bloomEnabled;
    float m_bloomThreshold;
    float m_bloomIntensity;

    // Captura de Pantalla / Render a Imagen (F12) (Fase 12 y 13)
    bool m_requestRenderImage;
    std::string m_lastRenderPath;
    bool m_showRenderNotification;
    float m_renderNotificationTimer;

    // Notificaciones de Status Bar (Fase 22.5)
    std::string m_statusBarMessage;
    float m_statusBarMessageTimer = 0.0f;
    void setStatusBarNotification(const std::string& msg, float duration = 4.0f) {
        m_statusBarMessage = msg;
        m_statusBarMessageTimer = duration;
    }

    // Oclusión Ambiental en Espacio de Pantalla (SSAO) (Fase 13)
    bool m_ssaoEnabled = false;
    float m_ssaoRadius;
    float m_ssaoBias;
    float m_ssaoPower;
    float m_ssaoIntensity;

    // Sistema OASIS Módulo 2: Meshlets de Geometría Virtual (Fase 18)
    OASISMeshletConfig m_meshletConfig;
    OASISMeshletTelemetry m_meshletTelemetry;

    // Ciclo Día / Noche Dinámico y Clima (Fase 20)
    DayNightCycle m_dayNight;

    // Screen-Space Reflections (SSR) (Fase 20)
    bool m_ssrEnabled = false;
    int m_ssrMaxSteps = 64;
    float m_ssrStepSize = 0.15f;
    float m_ssrThickness = 0.05f;
    float m_ssrIntensity = 0.85f;

    // Screen-Space Contact Shadows (Fase 20)
    bool m_contactShadowsEnabled = false;
    float m_contactShadowsDistance = 0.25f;
    int m_contactShadowsRaySteps = 16;
    float m_contactShadowsIntensity = 0.75f;

    // Profundidad de Campo (Depth of Field / Bokeh) (Fase 13)
    bool m_dofEnabled;
    float m_dofFocusDistance;
    float m_dofFocusRange;
    float m_dofBokehRadius;

    // Sistema de Partículas VFX (Fase 13)
    int m_selectedEmitterIndex;

    // Control de Modo Juego (PIE) y Guardado de Cámara del Editor
    EnginePlayMode m_playMode;
    bool m_requestPlayStart;
    bool m_requestPlayStop;
    glm::vec3 m_savedEditorCameraPos;
    glm::vec3 m_savedEditorCameraTarget;
    float m_savedEditorYaw;
    float m_savedEditorPitch;

    enum class OutlinerIconType {
        MESH_CUBE,
        SPHERE,
        CYLINDER,
        CONE,
        TORUS,
        PLANE,
        MODEL,
        LIGHT_POINT,
        LIGHT_SPOT,
        TERRAIN,
        FOLDER
    };
    void drawOutlinerIcon(OutlinerIconType type, const ImVec2& screenPos, float size = 16.0f);

    void applyTheme(UITheme theme);
    void renderPreferencesModal(Scene& scene, Camera& camera, PhysicsWorld& physicsWorld);
    void renderAddPrimitiveMenu(Scene& scene, Camera& camera, ParticleSystem& particleSystem);

    void applyDarkTheme();
    void renderMenuBar(Scene& scene, ScriptEngine& scriptEngine, Camera& camera, PhysicsWorld& physicsWorld, ParticleSystem& particleSystem, Terrain& terrain, CharacterController& character, Skybox& skybox, Water* water = nullptr);
    void renderViewport(Scene& scene, Camera& camera, Framebuffer& framebuffer, PhysicsWorld& physicsWorld, Terrain& terrain, CharacterController& character, GLuint displayTexture = 0, ParticleSystem* particleSystem = nullptr, ScriptEngine* scriptEngine = nullptr);
    void renderOutliner(Scene& scene, PhysicsWorld& physicsWorld, Camera& camera);
    
    // Panel de Propiedades unificado con riel vertical de iconos estilo Blender 4
    void renderPropertiesPanel(Scene& scene, Camera& camera, PhysicsWorld& physicsWorld, 
                               ScriptEngine& scriptEngine, ShadowMap& shadowMap, Skybox& skybox, 
                               ParticleSystem& particleSystem, Terrain& terrain, CharacterController& character, Water* water = nullptr);
    void renderVectorRailButton(BlenderPropertyTab tab, const char* str_id, const char* tooltip, const ImVec4& themeColor);
    void renderObjectProperties(Scene& scene, Camera& camera, PhysicsWorld& physicsWorld);
    void renderMaterialProperties(Scene& scene);
    void renderLightingProperties(Scene& scene, ShadowMap& shadowMap, Camera& camera, Skybox& skybox);
    void renderTerrainProperties(Terrain& terrain, Camera& camera, PhysicsWorld& physicsWorld, Water* water = nullptr);
    void renderCharacterProperties(CharacterController& character, Camera& camera, PhysicsWorld& physicsWorld, Scene& scene);
    void renderAudioProperties();
    void renderParticleProperties(ParticleSystem& particleSystem, Camera& camera);
    void renderAnimationProperties(Scene& scene);
    void renderPhysicsProperties(Scene& scene, PhysicsWorld& physicsWorld);
    void renderScriptsProperties(ScriptEngine& scriptEngine);
    void renderImportExportProperties(Scene& scene, ScriptEngine& scriptEngine);

    void renderConsole(ScriptEngine& scriptEngine);
    void renderAIPanel(Scene& scene, Terrain& terrain);

    // Sistema de Prefabs y Paleta de Entidades (Fase 17)
    void renderPrefabPalette(Scene& scene, Camera& camera, PhysicsWorld& physicsWorld);
    void renderStatusBar(Scene& scene, Camera& camera, float deltaTime);
    void renderAboutModal();
    void drawTriggerWireframe(ImDrawList* drawList, const Entity& entity, const glm::mat4& viewProj, const ImVec2& screenPos, const ImVec2& avail);
    void drawPlainAxes(ImDrawList* drawList, const glm::vec3& pivot, const glm::mat4& viewProj, const ImVec2& screenPos, const ImVec2& avail, const std::string& label = "");
    void selectGroup(const std::string& groupName, Scene& scene);

    std::string m_selectedGroupName;
    glm::vec3 m_groupPivot = glm::vec3(0.0f);
    std::vector<std::string> m_availablePrefabs;
    char m_newPrefabNameBuf[128] = "MiPrefab";

    // Fase 21: Presets de Layouts, Barra de Búsqueda y Consola Lua Interactiva
    UILayoutPreset m_currentLayout = UILayoutPreset::GENERAL;
    int m_activeWorkspaceTab = 0; // 0=Layout, 1=Modelado, 2=Shading, 3=Iluminacion, 4=Scripting, 5=Juego
    char m_sceneSearchFilter[128] = "";
    int m_consoleFilterCategory = 0; // 0 = Todos, 1 = Info/Sistema, 2 = Advertencias, 3 = Errores, 4 = Scripts
    char m_luaCommandBuf[512] = "";
    std::vector<std::string> m_luaHistory;
    int m_luaHistoryIndex = -1;

    // Fase 21.8: Shading, Overlays, Isotipo y Status Bar
    bool m_showAboutModal = false;
    bool m_wireframeMode = false;
    bool m_showOverlays = true;
    ViewportShadingMode m_shadingMode = ViewportShadingMode::RENDERED;

    // Fase 22: Sequencer, Cámaras de Cine y Animación Temporal
    void renderSequencerPanel(Scene& scene, Camera& camera, DayNightCycle* dayNight, ScriptEngine* scriptEngine);
    void renderCineCameraProperties(Camera& camera, Scene& scene);
    void drawViewportSequencerOverlays(ImDrawList* dl, ImVec2 pMin, ImVec2 pMax, Camera& camera);

    float m_sequencerTimeScale = 60.0f; // px por segundo en el editor
    float m_sequencerScrollX = 0.0f;
    bool m_isScrubbingPlayhead = false;
    bool m_seqIsBoxSelecting = false;
    ImVec2 m_seqBoxSelectStart = ImVec2(0.0f, 0.0f);
    bool m_seqIsMovingKeys = false;
    float m_seqMoveKeysLastMouseX = 0.0f;
    bool m_draggingWithMouseHold = false;
    bool m_initialSequencerSynced = false;
    int m_graphChannelSelection = 0; // 0=Posicion, 1=Rotacion, 2=Escala
    int m_draggedGraphPointIndex = -1;
    int m_draggedGraphPointAxis = -1; // 0=X, 1=Y, 2=Z
    bool m_showInterpPopup = false;
    int m_selectedSeqTrackType = -1; // 0=Entity, 1=Camera, 2=Env, 3=Lua
    int m_selectedSeqTrackIndex = -1;
    int m_selectedSeqKeyIndex = -1;
    int m_draggedKeyTrackType = -1;
    int m_draggedKeyTrackIndex = -1;
    int m_draggedKeyIndex = -1;
    char m_newSeqEntityNameBuf[64] = "";
    char m_sequenceFilePathBuf[128] = "assets/sequences/demo_hero.asisseq";
    bool m_showKeyframeInsertPopup = false;

    void applyBlenderDarkTheme();
};

} // namespace ASIS
