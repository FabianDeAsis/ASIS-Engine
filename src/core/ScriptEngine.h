#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <mutex>
#include <sol/sol.hpp>

#include "Scene.h"

namespace ASIS {

class PhysicsWorld;
class EditorUI;
class CharacterController;

struct ScriptExecutionResult {
    bool success;
    std::string errorMessage;
    int entityCount;
};

class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine() = default;

    // Inicializar el entorno Lua seguro y vincular las APIs de C++
    void init(Scene* scene, PhysicsWorld* physicsWorld = nullptr);
    void setEditorUI(EditorUI* ui) { m_editorUI = ui; }
    void setCharacterController(CharacterController* character) { m_character = character; }

    // Cargar y ejecutar un script Lua desde disco
    bool loadScript(const std::string& filepath);

    // Ejecutar una cadena de código Lua con sandbox y watchdog de instrucciones
    ScriptExecutionResult executeString(const std::string& code);

    // Bucle de actualización (invoca onUpdate(dt) en Lua)
    void update(float deltaTime);

    // Despacho de Eventos de Disparadores y Colisiones (Fase 17)
    void dispatchTriggerEnter(const std::string& triggerName, const std::string& otherName);
    void dispatchTriggerExit(const std::string& triggerName, const std::string& otherName);

    // Comprobación y recarga automática si el archivo fue modificado en disco
    void checkHotReload(float deltaTime);

    float getTotalTime() const { return m_totalTime; }

    // Logs para el panel de consola de ImGui
    std::vector<std::string> getLogs();
    void addLog(const std::string& log);
    void clearLogs();

private:
    sol::state m_lua;
    Scene* m_scene;
    PhysicsWorld* m_physicsWorld;
    EditorUI* m_editorUI;
    CharacterController* m_character = nullptr;

    std::string m_scriptPath;
    std::filesystem::file_time_type m_lastWriteTime;
    float m_hotReloadCheckTimer;
    float m_totalTime;
    float m_lastDeltaTime;
    bool m_updateHasError;

    std::mutex m_logMutex;
    std::vector<std::string> m_logs;

    void registerBindings();
};

} // namespace ASIS
