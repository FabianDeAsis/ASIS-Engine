#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "Window.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "Model.h"
#include "Renderer.h"
#include "Scene.h"
#include "PhysicsWorld.h"
#include "ScriptEngine.h"
#include "NetworkServer.h"
#include "Framebuffer.h"
#include "ShadowMap.h"
#include "OmniShadowMap.h"
#include "CascadedShadowMap.h"
#include "SceneIO.h"
#include "EditorUI.h"
#include "Skybox.h"
#include "PostProcess.h"
#include "ParticleSystem.h"
#include "SSAO.h"
#include "Terrain.h"
#include "CharacterController.h"
#include "AudioEngine.h"
#include "PrefabManager.h"
#include "MaterialManager.h"
#include "Water.h"

int main(int argc, char* argv[]) {
    // Si se pasa un archivo en línea de comandos ("Abrir con..."), resolver ruta absoluta antes de cambiar directorio
    std::string startupFile = "";
    if (argc > 1 && argv[1] != nullptr) {
        try {
            startupFile = std::filesystem::absolute(argv[1]).string();
        } catch (...) {
            startupFile = argv[1];
        }
    }

    // Fijar directorio de trabajo al directorio del ejecutable para cargar shaders y assets correctamente
#ifdef _WIN32
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) > 0) {
        std::filesystem::path p(exePath);
        std::filesystem::current_path(p.parent_path());
    }
#endif

    try {
        std::cout << "===========================================================================================================\n";
        std::cout << ">>> ASIS Engine v0.22 - FASE 22.5: Sequencer, Blender UX Polish, Isotipo ASIS & Sincronizacion IA/Lua <<<\n";
        std::cout << "===========================================================================================================\n";

        // 1. Crear ventana principal
        const int WINDOW_WIDTH = 1366;
        const int WINDOW_HEIGHT = 768;
        ASIS::Window window(WINDOW_WIDTH, WINDOW_HEIGHT, "ASIS Engine v0.22");

        // 2. Inicializar renderizador
        ASIS::Renderer renderer;
        renderer.printGPUInfo();

        // 3. Inicializar Framebuffer para el Viewport 3D (Multimuestreo MSAA + Textura HDR GL_RGBA16F)
        ASIS::Framebuffer framebuffer;
        if (!framebuffer.init(WINDOW_WIDTH, WINDOW_HEIGHT, 4)) {
            std::cerr << "[ASIS Engine ERROR FATAL] No se pudo inicializar el Framebuffer para el Viewport 3D.\n";
            return -1;
        }

        // 4. Inicializar Framebuffer de Sombras Dinámicas 4K (Shadow Mapping)
        ASIS::ShadowMap shadowMap;
        if (!shadowMap.init(4096, 4096)) {
            std::cerr << "[ASIS Engine ERROR FATAL] No se pudo inicializar el Shadow Map FBO 4K.\n";
            return -1;
        }

        // 4.2. Inicializar Sombras en Cascada (Cascaded Shadow Maps - CSM)
        ASIS::CascadedShadowMap csm;
        if (!csm.init(2048, 3)) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar Cascaded Shadow Maps.\n";
        }

        // 4.5. Inicializar Framebuffer de Sombras Omnidireccionales para Luces Puntuales (Cubemap 1024x1024)
        ASIS::OmniShadowMap omniShadowMap;
        if (!omniShadowMap.init(1024)) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el Omni Shadow Map Cubemap.\n";
        }

        // 5. Inicializar Sistema de Skybox e Iluminación Basada en Imágenes (IBL)
        ASIS::Skybox skybox;
        if (!skybox.init()) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el Skybox e IBL.\n";
        } else {
            // Cargar HDRI por defecto solicitado si existe
            std::string defaultHdri = "C:/Users/Fabian/Pictures/Texturas/HDRI/golden_gate_hills_2k.exr";
            if (std::filesystem::exists(defaultHdri)) {
                skybox.loadHDRI(defaultHdri);
            }
        }

        // 6. Inicializar Pipeline de Post-Procesado (Bloom HDR y FXAA)
        ASIS::PostProcess postProcess;
        if (!postProcess.init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el Pipeline de Post-Procesado.\n";
        }

        // 7. Inicializar Sistema de Partículas 3D (VFX) (Fase 13)
        ASIS::ParticleSystem particleSystem;
        if (!particleSystem.init()) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el Sistema de Particulas.\n";
        }
        particleSystem.addEmitter("Fuego_Central", ASIS::EmitterType::FIRE, glm::vec3(0.0f, 0.5f, 0.0f));
        particleSystem.addEmitter("Lluvia_Atmosferica", ASIS::EmitterType::RAIN, glm::vec3(0.0f, 15.0f, 0.0f));
        auto rainInit = particleSystem.getEmitterByName("Lluvia_Atmosferica");
        if (rainInit) rainInit->setActive(false);

        // 8. Inicializar SSAO (Screen-Space Ambient Occlusion) (Fase 13)
        ASIS::SSAO ssao;
        if (!ssao.init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el sistema SSAO.\n";
        }

        // 9. Inicializar Motor de Física 3D (Bullet Physics)
        ASIS::PhysicsWorld physicsWorld;
        physicsWorld.init();

        // 10. Inicializar Terreno 3D Procedural con Escultura y Colisiones (Fase 14)
        ASIS::Terrain terrain;
        if (!terrain.init(128, 120.0f)) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el Terreno 3D.\n";
        }
        terrain.initPhysics(&physicsWorld);

        // 11. Inicializar Controlador de Personaje 3D (Fase 15)
        ASIS::CharacterController character;
        if (!character.init()) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar la geometria del Avatar 3D.\n";
        }

        // 11.5. Inicializar Motor de Audio 3D Espacial y SFX Procedural (Fase 16)
        if (!ASIS::AudioEngine::get().init()) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el motor de audio 3D.\n";
        }

        // 11.6. Inicializar Sistema de Prefabs y Templates por Defecto (Fase 17)
        ASIS::PrefabManager::createDefaultPrefabs("assets/prefabs");

        // 11.65. Inicializar Gestor de Materiales PBR y Presets Físicos (Fase 21)
        ASIS::MaterialManager::get().init();

        // 11.7. Inicializar Sistema de Océano y Cuerpos de Agua 3D (Fase 19)
        ASIS::Water water;
        if (!water.init(128, 700.0f)) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo inicializar el Sistema de Océano 3D.\n";
        }

        // 12. Inicializar Editor UI (Dear ImGui + ImGuizmo + Panel Blender)
        ASIS::EditorUI editorUI;
        editorUI.init(window.getNativeWindow());

        // 13. Sistema de cámara híbrido (Blender Orbit / Unreal Flycam)
        ASIS::Camera camera(glm::vec3(0.0f, 3.5f, 7.5f), glm::vec3(0.0f, 1.2f, 0.0f));

        // Control suave del cursor y separación con ImGui
        static double lastMouseX = WINDOW_WIDTH / 2.0;
        static double lastMouseY = WINDOW_HEIGHT / 2.0;
        static bool isInteractingWithMouse = false;
        static bool justCapturedMouse = false;

        window.setCursorPosCallback([&](double xpos, double ypos) {
            if (!isInteractingWithMouse) {
                lastMouseX = xpos;
                lastMouseY = ypos;
                return;
            }

            if (justCapturedMouse) {
                lastMouseX = xpos;
                lastMouseY = ypos;
                justCapturedMouse = false;
                return;
            }

            float xoffset = static_cast<float>(xpos - lastMouseX);
            float yoffset = static_cast<float>(lastMouseY - ypos);
            lastMouseX = xpos;
            lastMouseY = ypos;

            // Protección contra saltos súbitos de cursor por captura o desanclaje de GLFW
            if (std::abs(xoffset) > 120.0f || std::abs(yoffset) > 120.0f) {
                return;
            }

            if (editorUI.isPlaying()) {
                character.processMouseLook(xoffset, yoffset);
            } else {
                camera.processMouseMovement(xoffset, yoffset);
            }
        });

        window.setMouseButtonCallback([&](int button, int action, int mods) {
            if (editorUI.isPlaying()) {
                // Durante el modo juego, clic en el viewport vuelve a capturar el cursor
                if (action == GLFW_PRESS && (!editorUI.isVisible() || editorUI.isViewportHovered())) {
                    glfwSetInputMode(window.getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    isInteractingWithMouse = true;
                    justCapturedMouse = true;
                }
                return;
            }

            // Si el usuario está manipulando un Gizmo 3D (W/E/R), la cámara NO debe moverse
            if (editorUI.isUsingGizmo()) {
                return;
            }

            if (action == GLFW_PRESS) {
                // Solo permitir iniciar navegación si la UI está oculta (F1) o el cursor está dentro del Viewport 3D
                bool canInteract = !editorUI.isVisible() || editorUI.isViewportHovered();
                if (!canInteract) {
                    return;
                }

                double curX, curY;
                glfwGetCursorPos(window.getNativeWindow(), &curX, &curY);
                lastMouseX = curX;
                lastMouseY = curY;
                isInteractingWithMouse = true;
                justCapturedMouse = true;

                if (button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
                    glfwSetInputMode(window.getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            else if (action == GLFW_RELEASE) {
                bool rmbPressed = (glfwGetMouseButton(window.getNativeWindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
                bool mmbPressed = (glfwGetMouseButton(window.getNativeWindow(), GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
                bool lmbPressed = (glfwGetMouseButton(window.getNativeWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

                if (!rmbPressed && !mmbPressed) {
                    glfwSetInputMode(window.getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }

                if (!rmbPressed && !mmbPressed && !lmbPressed) {
                    isInteractingWithMouse = false;
                }
            }

            if (isInteractingWithMouse || action == GLFW_RELEASE) {
                camera.onMouseButton(button, action, mods);
            }
        });

        window.setScrollCallback([&](double, double yoffset) {
            if (editorUI.isPlaying()) {
                float newDist = character.getThirdPersonDistance() - static_cast<float>(yoffset) * 0.4f;
                character.setThirdPersonDistance(newDist);
                return;
            }

            // Zoom solo si la UI está oculta o si el cursor está sobre el Viewport 3D
            if (editorUI.isVisible() && !editorUI.isViewportHovered()) {
                return;
            }

            camera.processMouseScroll(static_cast<float>(yoffset));
        });

        // 8. Shaders de la escena
        std::cout << "[ASIS Engine] Compilando shaders...\n";
        ASIS::Shader modelShader("shaders/model.vert", "shaders/model.frag");
        ASIS::Shader gridShader("shaders/grid.vert", "shaders/grid.frag");
        ASIS::Shader shadowDepthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
        ASIS::Shader pointShadowDepthShader("shaders/point_shadow_depth.vert", "shaders/point_shadow_depth.frag");
        ASIS::Shader terrainShader("shaders/terrain.vert", "shaders/terrain.frag");
        ASIS::Shader waterShader("shaders/water.vert", "shaders/water.frag");

        // 9. Cargar recursos globales
        std::cout << "[ASIS Engine] Cargando texturas y geometrias globales...\n";
        auto checkerTex = std::make_shared<ASIS::Texture>("textures/checkerboard.png");
        auto gridMesh = ASIS::Mesh::createGrid(40.0f, 40);
        auto groundPlane = ASIS::Mesh::createPlane(80.0f);

        // Geometria de Cuadricula Infinita Analitica Anti-Aliased estilo Blender 4 (Fase 16)
        GLuint infiniteGridVAO = 0;
        GLuint infiniteGridVBO = 0;
        {
            float quadVertices[] = {
                -1.0f,  1.0f, 0.0f,
                -1.0f, -1.0f, 0.0f,
                 1.0f, -1.0f, 0.0f,
                -1.0f,  1.0f, 0.0f,
                 1.0f, -1.0f, 0.0f,
                 1.0f,  1.0f, 0.0f
            };
            glGenVertexArrays(1, &infiniteGridVAO);
            glGenBuffers(1, &infiniteGridVBO);
            glBindVertexArray(infiniteGridVAO);
            glBindBuffer(GL_ARRAY_BUFFER, infiniteGridVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glBindVertexArray(0);
        }

        // 10. Gestor de Escena
        ASIS::Scene scene;
        scene.setDefaultTexture(checkerTex);
        // Luces puntuales predeterminadas (Multiluz PBR Fase 10)
        scene.addPointLight(ASIS::PointLight("Luz Clave Calida", glm::vec3(4.0f, 4.5f, 3.5f), glm::vec3(1.0f, 0.92f, 0.80f), 3.0f));
        scene.addPointLight(ASIS::PointLight("Luz Relleno Fria", glm::vec3(-4.0f, 3.0f, -2.5f), glm::vec3(0.45f, 0.70f, 1.0f), 1.8f));

        // 11. Motor de Scripting (Lua 5.4 + sol2 con Sandbox y Física)
        std::cout << "[ASIS Engine] Inicializando entorno Lua seguro con soporte de fisica...\n";
        ASIS::ScriptEngine scriptEngine;
        scriptEngine.init(&scene, &physicsWorld);
        scriptEngine.setEditorUI(&editorUI);
        scriptEngine.setCharacterController(&character);

        // 12. Arrastrar y Soltar (Drag & Drop) de archivos 3D, escenas .asis y mapas HDRI directamente
        window.setDropCallback([&](const std::vector<std::string>& paths) {
            for (const auto& path : paths) {
                std::filesystem::path p(path);
                std::string ext = p.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".asis") {
                    if (editorUI.loadProject(path, scene, camera, terrain, skybox, physicsWorld, scriptEngine, &water)) {
                        scriptEngine.addLog("[Drag&Drop] Proyecto .asis cargado: " + p.filename().string());
                    }
                } else if (ext == ".hdr" || ext == ".exr") {
                    if (skybox.loadHDRI(path)) {
                        scriptEngine.addLog("[Drag&Drop] ✓ Mapa HDRI cargado: " + p.filename().string());
                    }
                } else if (ASIS::SceneIO::isSupported3DFormat(path)) {
                    auto ent = ASIS::SceneIO::importModel(scene, path);
                    if (ent) {
                        editorUI.selectEntity(ent->getName());
                        scriptEngine.addLog("[Drag&Drop] ✓ Modelo importado con exito: " + ent->getName());
                    }
                } else {
                    std::cerr << "[Drag&Drop Advertencia] Formato no soportado: " << path << std::endl;
                    scriptEngine.addLog("[Drag&Drop Advertencia] Formato no soportado: " + path);
                }
            }
        });

        // Cargar script inicial
        std::vector<std::string> luaPaths = {
            "../../src/scripts/main.lua",
            "../src/scripts/main.lua",
            "src/scripts/main.lua",
            "scripts/main.lua"
        };

        for (const auto& path : luaPaths) {
            if (std::filesystem::exists(path)) {
                scriptEngine.loadScript(path);
                break;
            }
        }

        // 13. Carga automática de archivo por línea de comandos / "Abrir con..." (Fase 12)
        if (!startupFile.empty()) {
            std::filesystem::path filePath(startupFile);
            std::string ext = filePath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".asis") {
                if (editorUI.loadProject(startupFile, scene, camera, terrain, skybox, physicsWorld, scriptEngine, &water)) {
                    scriptEngine.addLog("[Inicio] Proyecto .asis cargado con exito: " + filePath.filename().string());
                } else {
                    scriptEngine.addLog("[Inicio ERROR] No se pudo cargar el proyecto .asis: " + startupFile);
                }
            } else if (ASIS::SceneIO::isSupported3DFormat(startupFile)) {
                auto ent = ASIS::SceneIO::importModel(scene, startupFile);
                if (ent) {
                    editorUI.selectEntity(ent->getName());
                    scriptEngine.addLog("[Inicio] ✓ Modelo 3D importado con exito: " + ent->getName());
                } else {
                    scriptEngine.addLog("[Inicio ERROR] No se pudo importar el modelo: " + startupFile);
                }
            } else if (ext == ".hdr" || ext == ".exr") {
                if (skybox.loadHDRI(startupFile)) {
                    scriptEngine.addLog("[Inicio] ✓ Mapa HDRI cargado con exito: " + filePath.filename().string());
                } else {
                    scriptEngine.addLog("[Inicio ERROR] No se pudo cargar el mapa HDRI: " + startupFile);
                }
            }
        }

        // 14. Servidor Socket Local (IPC) para comunicación bidireccional con el Módulo de IA (Python)
        std::cout << "[ASIS Engine] Iniciando servidor IPC bidireccional en puerto 5555...\n";
        ASIS::NetworkServer networkServer(5555);
        if (!networkServer.start()) {
            std::cerr << "[ASIS Engine ADVERTENCIA] No se pudo iniciar el servidor socket en el puerto 5555.\n";
        }

        // 15. Variables de tiempo
        float lastFrameTime = static_cast<float>(glfwGetTime());
        static bool f1KeyPressed = false;

        std::cout << "\n=========================================================\n";
        std::cout << " SERVIDOR DE IA ACTIVO EN: 127.0.0.1:5555\n";
        std::cout << " - Barra Lateral Derecha: Riel vertical de iconos estilo Blender 4\n";
        std::cout << " - Fisica 3D: Cuerpos rigidos, gravedad terrestre y colisiones activas\n";
        std::cout << " - Sombras Dinamicas: Mapa de profundidad 2K con filtro PCF 3x3\n";
        std::cout << " - Seleccion 3D: Clic izquierdo sobre cualquier objeto en el Viewport\n";
        std::cout << " - Pausa de Scripts / Fisica: Boton en toolbar o Tecla Espacio\n";
        std::cout << "=========================================================\n\n";

        // 14. Bucle principal de Renderizado
        while (!window.shouldClose()) {
            float currentFrameTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

            // Transiciones del Modo Juego (Play-in-Editor - Fase 15 y 16)
            if (editorUI.requestPlayStart()) {
                editorUI.clearPlayRequests();
                editorUI.savePrePlaySnapshot(scene);
                editorUI.saveEditorCamera(camera);

                // Calcular posición de aparición inteligente frente a la cámara o sobre el terreno
                glm::vec3 spawnPos = camera.getPosition() + camera.getFront() * 2.5f;
                float floorY = 0.0f;
                if (terrain.isVisible()) {
                    floorY = (std::max)(floorY, terrain.getHeightAtWorld(spawnPos.x, spawnPos.z));
                }
                spawnPos.y = (std::max)(spawnPos.y, floorY + 1.2f);

                character.spawn(physicsWorld, spawnPos, camera.getYaw());
                editorUI.setPlayMode(ASIS::EnginePlayMode::PLAYING);
                glfwSetInputMode(window.getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                isInteractingWithMouse = true;
                justCapturedMouse = true;
                scriptEngine.addLog("[PIE] ▶ Modo Juego iniciado. Presiona [Esc] para volver al editor.");
            }

            if (editorUI.requestPlayStop()) {
                editorUI.clearPlayRequests();
                character.despawn(physicsWorld);
                editorUI.restorePrePlaySnapshot(scene, physicsWorld);
                editorUI.restoreEditorCamera(camera);
                editorUI.setPlayMode(ASIS::EnginePlayMode::EDITOR);
                glfwSetInputMode(window.getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                isInteractingWithMouse = false;
                scriptEngine.addLog("[PIE] ⏹ Modo Juego detenido. Objetos y camara del editor restaurados.");
            }

            // Sincronizar plano de suelo de Bullet Physics con el estado del Outliner / Editor
            physicsWorld.enableGroundPlane(editorUI.getShowGroundPlane());

            // Tecla Escape para salir de Play o cerrar ventana
            static bool escPressed = false;
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                if (!escPressed) {
                    if (editorUI.isPlaying()) {
                        editorUI.triggerPlayStop();
                    } else {
                        window.close();
                    }
                    escPressed = true;
                }
            } else {
                escPressed = false;
            }

            // Tecla V para alternar 1ra / 3ra persona durante el juego
            static bool vKeyPressed = false;
            if (editorUI.isPlaying()) {
                if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_V) == GLFW_PRESS) {
                    if (!vKeyPressed) {
                        character.toggleCameraMode();
                        vKeyPressed = true;
                    }
                } else {
                    vKeyPressed = false;
                }
            }

            // Alternar visibilidad de la interfaz con F1
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_F1) == GLFW_PRESS) {
                if (!f1KeyPressed) {
                    editorUI.toggleVisibility();
                    f1KeyPressed = true;
                }
            } else {
                f1KeyPressed = false;
            }

            // Captura de pantalla / Render con F12 (detección directa por hardware)
            static bool f12KeyPressed = false;
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_F12) == GLFW_PRESS) {
                if (!f12KeyPressed) {
                    editorUI.triggerRenderImage();
                    f12KeyPressed = true;
                }
            } else {
                f12KeyPressed = false;
            }

            // Control de teclado y actualización de locomoción:
            if (editorUI.isPlaying()) {
                character.update(physicsWorld, window.getNativeWindow(), deltaTime, &terrain);
                character.updateCamera(camera, physicsWorld);
            } else {
                // Control de teclado para la cámara del editor:
                // - Si estamos en vuelo activo con RMB (isFlying), procesar siempre WASD.
                // - Si la UI está oculta (F1), procesar libremente.
                // - Si la UI está visible, procesar solo cuando el Viewport tiene el foco, sin escribir en inputs y sin manipular gizmos.
                ImGuiIO& io = ImGui::GetIO();
                if (camera.isFlying() || !editorUI.isVisible() || (editorUI.isViewportFocused() && !io.WantTextInput && !editorUI.isUsingGizmo())) {
                    camera.processKeyboard(window.getNativeWindow(), deltaTime);
                }

                // Si la UI está oculta, permitir re-enfocar con la tecla F al centro
                if (!editorUI.isVisible() && glfwGetKey(window.getNativeWindow(), GLFW_KEY_F) == GLFW_PRESS) {
                    camera.resetToFocus(glm::vec3(0.0f, 0.5f, 0.0f));
                }
            }

            // Actualización de escucha espacial 3D y ambiente sonoro (Fase 16)
            ASIS::AudioEngine::get().update(deltaTime, camera.getPosition(), camera.getFront(), camera.getUp());

            // Actualizar audio 3D posicional de fuego si hay un emisor de fuego activo
            for (const auto& em : particleSystem.getEmitters()) {
                if (em && em->getType() == ASIS::EmitterType::FIRE && em->isActive()) {
                    float dist = glm::distance(camera.getPosition(), em->getPosition());
                    ASIS::AudioEngine::get().updateFireAudio(em->getPosition(), dist < 18.0f);
                    break;
                }
            }

            // Procesar comandos Lua inyectados desde el Módulo de IA con Feedback Bidireccional
            auto incomingLuaCommands = networkServer.pollIncomingCommands();
            for (const auto& luaCode : incomingLuaCommands) {
                std::cout << "[ASIS Engine] >> Ejecutando script generado por la IA en tiempo real...\n";
                auto result = scriptEngine.executeString(luaCode);
                
                // Responder al cliente Python con el resultado exacto
                if (result.success) {
                    networkServer.broadcast("RESULT:OK:Entities=" + std::to_string(result.entityCount));
                } else {
                    networkServer.broadcast("RESULT:ERROR:" + result.errorMessage);
                }
            }

            // Actualizar entidades de la escena (animaciones esqueléticas de modelos) (Fase 13)
            scene.update(deltaTime);

            // Actualizar partículas 3D (VFX) (Fase 13)
            auto rainEmitter = particleSystem.getEmitterByName("Lluvia_Atmosferica");
            if (rainEmitter) {
                ASIS::WeatherType curWeather = editorUI.getDayNightCycle().getWeather();
                bool isRaining = (curWeather == ASIS::WeatherType::RAIN || curWeather == ASIS::WeatherType::STORM);
                rainEmitter->setActive(isRaining);
                if (isRaining) {
                    glm::vec3 cPos = camera.getPosition();
                    rainEmitter->setPosition(glm::vec3(cPos.x, cPos.y + 12.0f, cPos.z));
                }
            }
            particleSystem.update(deltaTime);

            // Actualizar simulación de oleaje de agua (Fase 19)
            water.update(deltaTime);

            // Actualizar lógica de Lua y simulación física (siempre activa al jugar o cuando no esté en pausa)
            if (editorUI.isPlaying() || !editorUI.isScriptsPaused()) {
                scriptEngine.update(deltaTime);
                
                // Aplicar flotabilidad hidrodinámica (Principio de Arquímedes) en cuerpos sumergidos
                if (water.isEnabled()) {
                    physicsWorld.applyBuoyancy([&](float x, float z) { return water.getWaterHeightAt(x, z); }, deltaTime);
                }

                // Actualizar simulación de física Bullet 3D y sincronizar con entidades
                physicsWorld.update(deltaTime);
                physicsWorld.syncPhysicsToEntities(scene.getEntitiesMutable());

                // Comprobar eventos y disparadores de colisión Gameplay (Fase 17)
                physicsWorld.checkTriggerEvents(&scriptEngine);
            }

            // =========================================================
            // CÁLCULO DE MATRICES DE ILUMINACIÓN SOLAR (Light Space Matrix)
            // =========================================================
            glm::vec3 lightDir = editorUI.getLightDir();
            float shadowDist = editorUI.getShadowDistance();

            // Centrar el frustum ortográfico de sombras alrededor de la cámara (en el plano de suelo XZ)
            glm::vec3 camPos = camera.getPosition();
            glm::vec3 shadowCenter = glm::vec3(camPos.x, 0.0f, camPos.z);

            // Texel Snapping para eliminar el parpadeo / centelleo al mover la cámara
            unsigned int shadowRes = editorUI.getShadowResolution();
            if (shadowRes > 0) {
                float worldTexelSize = (2.0f * shadowDist) / static_cast<float>(shadowRes);
                shadowCenter.x = std::floor(shadowCenter.x / worldTexelSize) * worldTexelSize;
                shadowCenter.z = std::floor(shadowCenter.z / worldTexelSize) * worldTexelSize;
            }

            glm::vec3 lightPos = shadowCenter - lightDir * (shadowDist * 1.5f);
            glm::mat4 lightView = glm::lookAt(lightPos, shadowCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightProjection = glm::ortho(-shadowDist, shadowDist, -shadowDist, shadowDist, 1.0f, shadowDist * 3.5f);
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;

            // =========================================================
            // PASO 1: PASE DE SOMBRAS (CSM en Cascada o Monolítico 1K/2K/4K)
            // =========================================================
            if (editorUI.getShadowsEnabled()) {
                if (editorUI.isCSMEnabled()) {
                    float currentAspect = editorUI.isVisible() ? framebuffer.getAspectRatio() : (static_cast<float>(window.getWidth()) / static_cast<float>(window.getHeight() > 0 ? window.getHeight() : 1));
                    csm.update(camera, lightDir, editorUI.getShadowDistance(), currentAspect);
                    for (int c = 0; c < csm.getCascadeCount(); ++c) {
                        csm.bindCascade(c);
                        glEnable(GL_DEPTH_TEST);
                        glClear(GL_DEPTH_BUFFER_BIT);

                        shadowDepthShader.use();
                        shadowDepthShader.setMat4("u_LightSpaceMatrix", csm.getCascadeMatrices()[c]);

                        scene.draw(shadowDepthShader);

                        if (editorUI.getTerrainCastsShadows() && terrain.isVisible()) {
                            terrain.draw(shadowDepthShader, &camera);
                        }

                        if (editorUI.isPlaying() && character.getCameraMode() == ASIS::CharacterCameraMode::THIRD_PERSON) {
                            character.drawAvatar(shadowDepthShader);
                        }
                    }
                    csm.unbind();
                } else {
                    if (editorUI.getShadowResolution() != shadowMap.getWidth()) {
                        unsigned int res = editorUI.getShadowResolution();
                        shadowMap.resize(res, res);
                    }

                    shadowMap.bind();
                    glEnable(GL_DEPTH_TEST);
                    glClear(GL_DEPTH_BUFFER_BIT);

                    shadowDepthShader.use();
                    shadowDepthShader.setMat4("u_LightSpaceMatrix", lightSpaceMatrix);

                    // Dibujar todas las entidades de la escena para capturar su profundidad
                    scene.draw(shadowDepthShader);

                    // Sombra propia del terreno si está habilitada (renderizada con OASIS CDLOD ultrarrápido)
                    if (editorUI.getTerrainCastsShadows() && terrain.isVisible()) {
                        terrain.draw(shadowDepthShader, &camera);
                    }

                    // El avatar del jugador proyecta sombra dinámica en tercera persona (Fase 15)
                    if (editorUI.isPlaying() && character.getCameraMode() == ASIS::CharacterCameraMode::THIRD_PERSON) {
                        character.drawAvatar(shadowDepthShader);
                    }

                    shadowMap.unbind();
                }
            }

            // =========================================================
            // PASO 1.5: PASE DE SOMBRAS PARA LUZ PUNTUAL (Omni Shadow Pass)
            // =========================================================
            int pointShadowCasterIndex = -1;
            const auto& curPointLights = scene.getPointLights();
            for (size_t i = 0; i < curPointLights.size(); ++i) {
                if (curPointLights[i].enabled && curPointLights[i].castShadows) {
                    pointShadowCasterIndex = static_cast<int>(i);
                    break;
                }
            }

            if (pointShadowCasterIndex >= 0) {
                glm::vec3 plPos = curPointLights[pointShadowCasterIndex].position;
                std::vector<glm::mat4> omniTransforms = omniShadowMap.calculateShadowMatrices(plPos);

                pointShadowDepthShader.use();
                pointShadowDepthShader.setVec3("u_LightPos", plPos);
                pointShadowDepthShader.setFloat("u_FarPlane", omniShadowMap.getFarPlane());

                glEnable(GL_DEPTH_TEST);
                for (unsigned int f = 0; f < 6; ++f) {
                    omniShadowMap.bindFace(f);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    pointShadowDepthShader.setMat4("u_LightSpaceMatrix", omniTransforms[f]);
                    scene.draw(pointShadowDepthShader);
                    if (editorUI.getTerrainCastsShadows() && terrain.isVisible()) {
                        terrain.draw(pointShadowDepthShader, &camera);
                    }
                    if (editorUI.isPlaying() && character.getCameraMode() == ASIS::CharacterCameraMode::THIRD_PERSON) {
                        character.drawAvatar(pointShadowDepthShader);
                    }
                }
                omniShadowMap.unbind();
            }

            // =========================================================
            // PASO 2: PASE DE ESCENA 3D (Viewport FBO o Ventana Principal)
            // =========================================================
            auto setupLightingUniforms = [&](ASIS::Shader& targetShader, const glm::mat4& proj, const glm::mat4& view) {
                targetShader.use();
                targetShader.setMat4("u_Projection", proj);
                targetShader.setMat4("u_View", view);
                targetShader.setVec3("u_ViewPos", camera.getPosition());
                targetShader.setMat4("u_LightSpaceMatrix", lightSpaceMatrix);
                targetShader.setVec3("u_LightDir", lightDir);
                targetShader.setVec3("u_LightColor", editorUI.getLightColor());

                // Soporte para Modo SOLID estilo Blender (sin sombras ni IBL, luz ambiente difusa)
                auto curShading = editorUI.getShadingMode();
                if (curShading == ASIS::ViewportShadingMode::SOLID) {
                    targetShader.setVec3("u_AmbientColor", glm::vec3(0.45f));
                    targetShader.setInt("u_ShadowsEnabled", 0);
                } else {
                    targetShader.setVec3("u_AmbientColor", editorUI.getAmbientColor());
                    targetShader.setInt("u_ShadowsEnabled", editorUI.getShadowsEnabled() ? 1 : 0);
                }
                targetShader.setFloat("u_ShadowBias", editorUI.getShadowBias());
                targetShader.setInt("u_ShadowKernel", editorUI.getShadowKernel());
                targetShader.setFloat("u_ShadowSoftness", editorUI.getShadowSoftness());

                // Sombras Suaves Físicas PCSS y Cascadas CSM
                targetShader.setInt("u_ShadowFilterMode", editorUI.getShadowFilterMode());
                targetShader.setFloat("u_PCSS_LightSize", editorUI.getPCSSLightSize());
                targetShader.setInt("u_PCSS_BlockerSamples", editorUI.getPCSSBlockerSamples());
                targetShader.setInt("u_PCSS_FilterSamples", editorUI.getPCSSFilterSamples());

                targetShader.setInt("u_CSMEnabled", editorUI.isCSMEnabled() ? 1 : 0);
                targetShader.setInt("u_CSMDebugColors", editorUI.isCSMDebugColors() ? 1 : 0);

                if (editorUI.isCSMEnabled()) {
                    csm.bindTexture(25);
                    targetShader.setInt("u_CascadeShadowMaps", 25);
                    const auto& matrices = csm.getCascadeMatrices();
                    const auto& splits = csm.getCascadeSplits();
                    for (int c = 0; c < 3; ++c) {
                        targetShader.setMat4("u_CascadeLightSpaceMatrices[" + std::to_string(c) + "]", matrices[c]);
                        targetShader.setFloat("u_CascadeSplits[" + std::to_string(c) + "]", splits[c]);
                    }
                }

                // Gestión de Color y Mapeo Tonal (Fase 11: ACES Filmic, AgX, Reinhard)
                targetShader.setInt("u_ColorProfile", editorUI.getColorProfile());
                targetShader.setFloat("u_Exposure", editorUI.getExposure());
                targetShader.setFloat("u_Gamma", editorUI.getGamma());
                targetShader.setFloat("u_Saturation", editorUI.getSaturation());
                targetShader.setFloat("u_Contrast", editorUI.getContrast());
                targetShader.setFloat("u_Wetness", editorUI.getDayNightCycle().getWetness());

                // Neblina Atmosférica Global (Fase 20)
                const auto& dayNight = editorUI.getDayNightCycle();
                targetShader.setInt("u_GlobalFogEnabled", dayNight.isFogEnabled() ? 1 : 0);
                targetShader.setFloat("u_GlobalFogStart", dayNight.getFogStart());
                targetShader.setFloat("u_GlobalFogEnd", dayNight.getFogEnd());
                targetShader.setFloat("u_GlobalFogDensity", dayNight.getFogDensity());
                targetShader.setVec3("u_GlobalFogColor", dayNight.getFogColor());

                // Iluminación Basada en Imágenes (IBL) y Reflexiones de Entorno (Fase 12)
                skybox.bindEnvironmentMap(7);
                targetShader.setInt("u_EnvMap", 7);
                targetShader.setInt("u_UseIBL", (curShading == ASIS::ViewportShadingMode::SOLID) ? 0 : (editorUI.getUseIBL() ? 1 : 0));
                targetShader.setFloat("u_IBLIntensity", editorUI.getIBLIntensity());
                targetShader.setFloat("u_SkyboxRotation", glm::radians(editorUI.getSkyboxRotation()));
                targetShader.setInt("u_FlipX", editorUI.getFlipSkyboxX() ? 1 : 0);
                targetShader.setInt("u_FlipY", editorUI.getFlipSkyboxY() ? 1 : 0);

                shadowMap.bindTexture(1);
                targetShader.setInt("u_ShadowMap", 1);

                omniShadowMap.bindTexture(26);
                targetShader.setInt("u_PointShadowMap", 26);
                targetShader.setFloat("u_PointShadowFarPlane", omniShadowMap.getFarPlane());
                targetShader.setInt("u_PointShadowsEnabled", (pointShadowCasterIndex >= 0) ? 1 : 0);
                if (pointShadowCasterIndex >= 0 && pointShadowCasterIndex < static_cast<int>(curPointLights.size())) {
                    targetShader.setFloat("u_PointShadowLightSize", curPointLights[pointShadowCasterIndex].lightSize);
                    targetShader.setFloat("u_PointShadowBias", curPointLights[pointShadowCasterIndex].bias);
                } else {
                    targetShader.setFloat("u_PointShadowLightSize", 0.25f);
                    targetShader.setFloat("u_PointShadowBias", 0.025f);
                }

                const auto& pointLights = scene.getPointLights();
                int numLights = static_cast<int>(pointLights.size());
                if (numLights > 8) numLights = 8;
                targetShader.setInt("u_NumPointLights", numLights);
                for (int i = 0; i < numLights; ++i) {
                    std::string base = "u_PointLights[" + std::to_string(i) + "].";
                    targetShader.setVec3(base + "position", pointLights[i].position);
                    targetShader.setVec3(base + "color", pointLights[i].color);
                    targetShader.setFloat(base + "intensity", pointLights[i].intensity);
                    targetShader.setFloat(base + "constant", pointLights[i].constant);
                    targetShader.setFloat(base + "linear", pointLights[i].linear);
                    targetShader.setFloat(base + "quadratic", pointLights[i].quadratic);
                    targetShader.setInt(base + "enabled", pointLights[i].enabled ? 1 : 0);
                    targetShader.setInt(base + "castShadows", pointLights[i].castShadows ? 1 : 0);
                }
            };

            auto drawGroundPlane = [&]() {
                modelShader.use();
                glm::mat4 groundModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.002f, 0.0f));
                modelShader.setMat4("u_Model", groundModel);
                modelShader.setVec3("u_EntityColor", glm::vec3(0.14f, 0.16f, 0.20f));

                modelShader.setVec3("u_Material.albedo", glm::vec3(0.14f, 0.16f, 0.20f));
                modelShader.setFloat("u_Material.metallic", 0.05f);
                modelShader.setFloat("u_Material.roughness", 0.85f);
                modelShader.setFloat("u_Material.specular", 0.2f);
                modelShader.setVec3("u_Material.emission", glm::vec3(0.0f));
                modelShader.setFloat("u_Material.emissionStrength", 0.0f);
                modelShader.setFloat("u_Material.normalStrength", 1.0f);
                modelShader.setFloat("u_Material.ao", 1.0f);
                modelShader.setInt("u_Material.useAlbedoMap", 0);
                modelShader.setInt("u_Material.useNormalMap", 0);
                modelShader.setInt("u_Material.useMetallicMap", 0);
                modelShader.setInt("u_Material.useRoughnessMap", 0);
                modelShader.setInt("u_Material.useSpecularMap", 0);
                modelShader.setInt("u_Material.useEmissionMap", 0);
                modelShader.setInt("u_Material.useAOMap", 0);

                groundPlane->draw(&modelShader);
            };

            if (editorUI.isVisible()) {
                framebuffer.bind();
                glEnable(GL_DEPTH_TEST);
                renderer.clear();

                bool isWireframe = editorUI.isWireframeMode();
                if (isWireframe) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }

                // Aspect ratio exacto del Viewport FBO
                float aspectRatio = framebuffer.getAspectRatio();
                glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
                glm::mat4 view = camera.getViewMatrix();

                // Soporte para Modo OASIS_MESHLETS en Viewport
                auto shadingMode = editorUI.getShadingMode();
                bool prevMeshletDebug = editorUI.getMeshletConfig().debugColors;
                bool prevTerrainDebug = (terrain.getOASIS() != nullptr) ? terrain.getOASIS()->isDebugLODEnabled() : false;
                if (shadingMode == ASIS::ViewportShadingMode::OASIS_MESHLETS) {
                    editorUI.getMeshletConfig().debugColors = true;
                    if (terrain.getOASIS()) {
                        terrain.getOASIS()->setDebugLOD(true);
                    }
                }

                // 2.0. Dibujar Terreno 3D Procedural con Splatting PBR y Niebla (Fase 14)
                if (terrain.isVisible()) {
                    setupLightingUniforms(terrainShader, projection, view);
                    terrain.setReflectionIntensity(editorUI.getTerrainReflectionIntensity());
                    terrain.setAnisotropyLevel(editorUI.getAnisotropyLevel());
                    ASIS::Texture::setGlobalAnisotropy(editorUI.getAnisotropyLevel());
                    terrain.draw(terrainShader, &camera, aspectRatio);
                }

                // Configurar Shader Principal para Modelos con Iluminación PBR, Sombras y Multiluz
                setupLightingUniforms(modelShader, projection, view);

                // 2.1. Dibujar Plano de Suelo receptor de sombras si está activo
                if (editorUI.getShowGroundPlane()) {
                    drawGroundPlane();
                }

                // 2.2. Dibujar Cuadricula Infinita Anti-Aliased sobre el suelo si esta activa (Fase 16)
                if (editorUI.getShowGroundGrid() && editorUI.getShowOverlays()) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDepthFunc(GL_LEQUAL);
                    glDepthMask(GL_FALSE);
                    gridShader.use();
                    gridShader.setMat4("u_Projection", projection);
                    gridShader.setMat4("u_View", view);
                    gridShader.setVec3("u_CameraPos", camera.getPosition());
                    glBindVertexArray(infiniteGridVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glBindVertexArray(0);
                    glDepthMask(GL_TRUE);
                    glDepthFunc(GL_LESS);
                    glDisable(GL_BLEND);
                }

                // 2.3. Dibujar todas las Entidades de la escena
                modelShader.use();
                scene.draw(modelShader, &camera, &editorUI.getMeshletConfig(), &editorUI.getMeshletTelemetry());

                // Restaurar flags de debug tras el renderizado con OASIS_MESHLETS
                if (shadingMode == ASIS::ViewportShadingMode::OASIS_MESHLETS) {
                    editorUI.getMeshletConfig().debugColors = prevMeshletDebug;
                    if (terrain.getOASIS()) {
                        terrain.getOASIS()->setDebugLOD(prevTerrainDebug);
                    }
                }

                // 2.35. Dibujar Avatar 3D del Personaje en tercera persona (Fase 15)
                if (editorUI.isPlaying() && character.getCameraMode() == ASIS::CharacterCameraMode::THIRD_PERSON) {
                    character.drawAvatar(modelShader);
                }

                // 2.4. Dibujar Fondo de Cielo / HDRI 360° (Fase 12 y 17)
                float effectiveSkyExp = editorUI.getSkyboxExposure() * (editorUI.isAutoExposureEnabled() ? editorUI.getCurrentAdaptedExposure() : 1.0f);
                skybox.draw(camera, aspectRatio, editorUI.getShowSkybox(), effectiveSkyExp, editorUI.getSkyboxRotation(), editorUI.getSkyboxBlur(), editorUI.getFlipSkyboxX(), editorUI.getFlipSkyboxY(), &editorUI.getDayNightCycle());

                // 2.45. Renderizar Océano y Cuerpos de Agua 3D con Refracción, Espuma y SSR (Fase 19 y 20)
                if (water.isEnabled()) {
                    framebuffer.copyOpaqueBuffers();
                    waterShader.use();
                    const auto& dn = editorUI.getDayNightCycle();
                    waterShader.setInt("u_GlobalFogEnabled", dn.isFogEnabled() ? 1 : 0);
                    waterShader.setFloat("u_GlobalFogStart", dn.getFogStart());
                    waterShader.setFloat("u_GlobalFogEnd", dn.getFogEnd());
                    waterShader.setFloat("u_GlobalFogDensity", dn.getFogDensity());
                    waterShader.setVec3("u_GlobalFogColor", dn.getFogColor());

                    water.draw(waterShader, camera, aspectRatio,
                               framebuffer.getOpaqueDepthTexture(),
                               framebuffer.getOpaqueColorTexture(),
                               editorUI.getLightDir(),
                               editorUI.getLightColor(),
                               editorUI.getAmbientColor(),
                               skybox.getEnvironmentTexture(),
                               editorUI.isSSREnabled(),
                               editorUI.getSSRMaxSteps(),
                               editorUI.getSSRStepSize(),
                               editorUI.getSSRThickness(),
                               editorUI.getSSRIntensity());
                }

                // 2.5. Dibujar Sistema de Partículas 3D (VFX) (Fase 13)
                particleSystem.draw(camera, projection, view);

                if (isWireframe) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }

                framebuffer.unbind();

                // =========================================================
                // PASO 3: PIPELINE DE POST-PROCESADO Y AUTO-EXPOSICIÓN (Fase 12, 13 y 17)
                // =========================================================
                postProcess.resize(framebuffer.getWidth(), framebuffer.getHeight());
                GLuint displayTex = framebuffer.getColorTexture();

                // Adaptación Ocular y Auto-Exposición Cinemática (Fase 17)
                if (editorUI.isAutoExposureEnabled()) {
                    float avgLum = postProcess.measureAverageLuminance(displayTex);
                    editorUI.updateAutoExposure(avgLum, deltaTime);
                }

                // Screen-Space Contact Shadows (Fase 20)
                if (editorUI.isContactShadowsEnabled()) {
                    displayTex = postProcess.applyContactShadows(displayTex, framebuffer.getDepthTexture(),
                        projection, view, lightDir,
                        editorUI.getContactShadowsDistance(),
                        editorUI.getContactShadowsRaySteps(),
                        editorUI.getContactShadowsIntensity(),
                        camera.getNearPlane(), camera.getFarPlane());
                }

                // Oclusión Ambiental en Espacio de Pantalla (SSAO) (Fase 13)
                if (editorUI.isSSAOEnabled()) {
                    ssao.resize(framebuffer.getWidth(), framebuffer.getHeight());
                    GLuint ssaoBlur = ssao.compute(framebuffer.getDepthTexture(), projection, view,
                        editorUI.getSSAORadius(), editorUI.getSSAOBias(), editorUI.getSSAOPower());
                    displayTex = ssao.apply(displayTex, ssaoBlur, editorUI.getSSAOIntensity());
                }

                // Efectos Ópticos y Niebla Subacuática (Underwater Mode)
                if (water.isEnabled() && water.isUnderwater(camera.getPosition())) {
                    displayTex = postProcess.applyUnderwater(displayTex, framebuffer.getDepthTexture(),
                        static_cast<float>(glfwGetTime()), water.getWaterLevel(), camera.getPosition(),
                        projection, view, camera.getNearPlane(), camera.getFarPlane());
                }

                // Profundidad de Campo Fotográfica (Depth of Field / Bokeh) (Fase 13)
                if (editorUI.isDoFEnabled()) {
                    displayTex = postProcess.applyDepthOfField(displayTex, framebuffer.getDepthTexture(),
                        editorUI.getDoFFocusDistance(), editorUI.getDoFFocusRange(), editorUI.getDoFBokehRadius(),
                        camera.getNearPlane(), camera.getFarPlane());
                }

                if (editorUI.isBloomEnabled()) {
                    displayTex = postProcess.applyBloom(displayTex, editorUI.getBloomThreshold(), editorUI.getBloomIntensity());
                }
                if (editorUI.isFXAAEnabled()) {
                    displayTex = postProcess.applyFXAA(displayTex);
                }

                // =========================================================
                // CAPTURA DE PANTALLA / RENDER A IMAGEN PNG (F12) (Fase 12)
                // =========================================================
                if (editorUI.takeRenderImageRequest()) {
                    std::filesystem::create_directories("renders");
                    auto t = std::time(nullptr);
                    auto tm = *std::localtime(&t);
                    std::ostringstream oss;
                    oss << "renders/render_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".png";
                    std::string outPath = oss.str();
                    if (ASIS::PostProcess::renderToImage(displayTex, framebuffer.getWidth(), framebuffer.getHeight(), outPath)) {
                        std::string absPath = std::filesystem::absolute(outPath).string();
                        std::cout << "[Render F12] ✓ Captura guardada con exito en: " << absPath << "\n";
                        scriptEngine.addLog("[Render F12] ✓ Captura guardada con exito en: " + absPath);
                        editorUI.notifyRenderComplete(absPath);
                    } else {
                        scriptEngine.addLog("[Render F12 ERROR] No se pudo guardar la captura en: " + outPath);
                    }
                }

                // =========================================================
                // PASO 4: RENDERIZAR EDITOR UI (ImGui + Viewport FBO + Panel Blender)
                // =========================================================
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, window.getWidth(), window.getHeight());
                renderer.clear();

                editorUI.render(scene, scriptEngine, camera, framebuffer, physicsWorld, shadowMap, skybox, particleSystem, ssao, terrain, character, deltaTime, displayTex, &water);
            }
            else {
                // =========================================================
                // MODO PANTALLA COMPLETA INMERSIVA (Tecla F1 activada)
                // =========================================================
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, window.getWidth(), window.getHeight());
                glEnable(GL_DEPTH_TEST);
                renderer.clear();

                float aspectRatio = static_cast<float>(window.getWidth()) / static_cast<float>(window.getHeight() > 0 ? window.getHeight() : 1);
                glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
                glm::mat4 view = camera.getViewMatrix();

                // Soporte para Modo OASIS_MESHLETS en modo inmersivo
                auto shadingMode = editorUI.getShadingMode();
                bool prevMeshletDebug = editorUI.getMeshletConfig().debugColors;
                bool prevTerrainDebug = (terrain.getOASIS() != nullptr) ? terrain.getOASIS()->isDebugLODEnabled() : false;
                if (shadingMode == ASIS::ViewportShadingMode::OASIS_MESHLETS) {
                    editorUI.getMeshletConfig().debugColors = true;
                    if (terrain.getOASIS()) {
                        terrain.getOASIS()->setDebugLOD(true);
                    }
                }

                // Terreno 3D en modo inmersivo
                if (terrain.isVisible()) {
                    setupLightingUniforms(terrainShader, projection, view);
                    terrain.setReflectionIntensity(editorUI.getTerrainReflectionIntensity());
                    terrain.setAnisotropyLevel(editorUI.getAnisotropyLevel());
                    ASIS::Texture::setGlobalAnisotropy(editorUI.getAnisotropyLevel());
                    terrain.draw(terrainShader, &camera, aspectRatio);
                }

                setupLightingUniforms(modelShader, projection, view);

                // Suelo receptor de sombras si está activo
                if (editorUI.getShowGroundPlane()) {
                    drawGroundPlane();
                }

                // Rejilla de Suelo si esta activa (Fase 16)
                if (editorUI.getShowGroundGrid()) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDepthFunc(GL_LEQUAL);
                    glDepthMask(GL_FALSE);
                    gridShader.use();
                    gridShader.setMat4("u_Projection", projection);
                    gridShader.setMat4("u_View", view);
                    gridShader.setVec3("u_CameraPos", camera.getPosition());
                    glBindVertexArray(infiniteGridVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glBindVertexArray(0);
                    glDepthMask(GL_TRUE);
                    glDepthFunc(GL_LESS);
                    glDisable(GL_BLEND);
                }

                // Entidades
                modelShader.use();
                scene.draw(modelShader, &camera, &editorUI.getMeshletConfig(), &editorUI.getMeshletTelemetry());

                // Restaurar flags de debug tras el renderizado con OASIS_MESHLETS
                if (shadingMode == ASIS::ViewportShadingMode::OASIS_MESHLETS) {
                    editorUI.getMeshletConfig().debugColors = prevMeshletDebug;
                    if (terrain.getOASIS()) {
                        terrain.getOASIS()->setDebugLOD(prevTerrainDebug);
                    }
                }

                if (editorUI.isPlaying() && character.getCameraMode() == ASIS::CharacterCameraMode::THIRD_PERSON) {
                    character.drawAvatar(modelShader);
                }

                // Cielo / Skybox en modo inmersivo (Fase 12, 17 y 20)
                float immersiveSkyExp = editorUI.getSkyboxExposure() * (editorUI.isAutoExposureEnabled() ? editorUI.getCurrentAdaptedExposure() : 1.0f);
                skybox.draw(camera, aspectRatio, editorUI.getShowSkybox(), immersiveSkyExp, editorUI.getSkyboxRotation(), editorUI.getSkyboxBlur(), editorUI.getFlipSkyboxX(), editorUI.getFlipSkyboxY(), &editorUI.getDayNightCycle());

                // Océano 3D en modo inmersivo (Fase 19 y 20)
                if (water.isEnabled()) {
                    waterShader.use();
                    const auto& dn = editorUI.getDayNightCycle();
                    waterShader.setInt("u_GlobalFogEnabled", dn.isFogEnabled() ? 1 : 0);
                    waterShader.setFloat("u_GlobalFogStart", dn.getFogStart());
                    waterShader.setFloat("u_GlobalFogEnd", dn.getFogEnd());
                    waterShader.setFloat("u_GlobalFogDensity", dn.getFogDensity());
                    waterShader.setVec3("u_GlobalFogColor", dn.getFogColor());

                    water.draw(waterShader, camera, aspectRatio,
                               framebuffer.getOpaqueDepthTexture(),
                               framebuffer.getOpaqueColorTexture(),
                               editorUI.getLightDir(),
                               editorUI.getLightColor(),
                               editorUI.getAmbientColor(),
                               skybox.getEnvironmentTexture(),
                               editorUI.isSSREnabled(),
                               editorUI.getSSRMaxSteps(),
                               editorUI.getSSRStepSize(),
                               editorUI.getSSRThickness(),
                               editorUI.getSSRIntensity());
                }

                // Partículas 3D en modo inmersivo (Fase 13)
                particleSystem.draw(camera, projection, view);
            }

            window.swapBuffers();
            window.pollEvents();
        }

        character.shutdown();
        water.shutdown();
        ASIS::AudioEngine::get().shutdown();
        ASIS::MaterialManager::get().shutdown();
        editorUI.shutdown();
        terrain.shutdown();
        physicsWorld.shutdown();
        shadowMap.destroy();
        omniShadowMap.destroy();
        csm.shutdown();
        postProcess.destroy();
        ssao.destroy();
        particleSystem.destroy();
        skybox.destroy();
        framebuffer.destroy();
        networkServer.stop();
        std::cout << "[ASIS Engine] Sesion finalizada correctamente.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ASIS Engine ERROR FATAL]: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
