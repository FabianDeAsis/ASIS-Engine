#include "ScriptEngine.h"
#include "PhysicsWorld.h"
#include "SceneIO.h"
#include "PrefabManager.h"
#include "EditorUI.h"
#include "AudioEngine.h"
#include "CharacterController.h"
#include "Sequencer.h"
#include <iostream>

namespace ASIS {

// Hook de seguridad de Lua para prevenir bucles infinitos en scripts generados por IA
static void luaInstructionWatchdog(lua_State* L, lua_Debug* ar) {
    luaL_error(L, "Watchdog de Seguridad: Limite de instrucciones excedido (bucle infinito abortado).");
}

ScriptEngine::ScriptEngine()
    : m_scene(nullptr),
      m_physicsWorld(nullptr),
      m_editorUI(nullptr),
      m_hotReloadCheckTimer(0.0f),
      m_totalTime(0.0f),
      m_lastDeltaTime(0.0f),
      m_updateHasError(false)
{
}

void ScriptEngine::init(Scene* scene, PhysicsWorld* physicsWorld) {
    m_scene = scene;
    m_physicsWorld = physicsWorld;

    // 1. Abrir librerías estándar estrictamente seguras (Sandbox)
    // Excluidas: sol::lib::os, sol::lib::io, sol::lib::package, sol::lib::debug
    m_lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table
    );

    // Desactivar funciones globales de riesgo para seguridad
    m_lua["dofile"] = sol::nil;
    m_lua["loadfile"] = sol::nil;
    m_lua["collectgarbage"] = sol::nil;

    // 2. Registrar clases y APIs de C++ hacia Lua
    registerBindings();

    addLog("[Sistema] Sandbox Lua 5.4 inicializado y protegido con sol2.");
    std::cout << "[ASIS Engine] Modulo de scripting Lua 5.4 inicializado con Sandbox y sol2.\n";
}

void ScriptEngine::registerBindings() {
    // -------------------------------------------------------------
    // Registrar Tipo de Objeto: Entity
    // -------------------------------------------------------------
    m_lua.new_usertype<Entity>("Entity",
        "setPosition", &Entity::setPosition,
        "getPosition", &Entity::getPosition,
        "setRotation", &Entity::setRotation,
        "getRotation", &Entity::getRotation,
        "setScale", &Entity::setScale,
        "getScale", &Entity::getScale,
        "setUniformScale", &Entity::setUniformScale,
        "setColor", &Entity::setColor,
        "move", &Entity::move,
        "rotate", &Entity::rotate,
        "getName", &Entity::getName,
        "setVisible", &Entity::setVisible,
        "setGroup", &Entity::setGroup,
        "getGroup", &Entity::getGroup,
        "isTrigger", &Entity::isTrigger,
        "setTrigger", [this](Entity& self, bool trig) {
            self.setTrigger(trig);
            if (m_physicsWorld) {
                m_physicsWorld->setBodyTrigger(self.getName(), trig);
            }
        },
        "getTriggerTag", &Entity::getTriggerTag,
        "setTriggerTag", &Entity::setTriggerTag,
        "destroy", [this](Entity& self) {
            std::string n = self.getName();
            if (m_physicsWorld) m_physicsWorld->removeRigidBody(n);
            if (m_scene) m_scene->removeEntity(n);
        },
        "enablePhysics", [this](Entity& self, sol::optional<float> mass) {
            if (m_scene && m_physicsWorld) {
                auto ent = m_scene->getEntity(self.getName());
                if (ent) m_physicsWorld->addRigidBody(ent, mass.value_or(1.0f));
            }
        },
        "applyImpulse", [this](Entity& self, float fx, float fy, float fz) {
            if (m_physicsWorld) {
                m_physicsWorld->applyImpulse(self.getName(), glm::vec3(fx, fy, fz));
            }
        }
    );

    // -------------------------------------------------------------
    // Registrar Espacio de Nombres Global: Engine
    // -------------------------------------------------------------
    sol::table engine = m_lua.create_named_table("Engine");

    engine["createCube"] = [this](const std::string& name, float x, float y, float z, sol::optional<float> size) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->createCube(name, x, y, z, size.value_or(1.0f));
    };

    engine["createSphere"] = [this](const std::string& name, float x, float y, float z, sol::optional<float> radius) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->createSphere(name, x, y, z, radius.value_or(0.5f));
    };

    engine["createCylinder"] = [this](const std::string& name, float x, float y, float z, sol::optional<float> radius, sol::optional<float> height) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->createCylinder(name, x, y, z, radius.value_or(0.5f), height.value_or(1.0f));
    };

    engine["createCone"] = [this](const std::string& name, float x, float y, float z, sol::optional<float> radius, sol::optional<float> height) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->createCone(name, x, y, z, radius.value_or(0.5f), height.value_or(1.0f));
    };

    engine["createTorus"] = [this](const std::string& name, float x, float y, float z, sol::optional<float> mainRadius, sol::optional<float> tubeRadius) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->createTorus(name, x, y, z, mainRadius.value_or(0.6f), tubeRadius.value_or(0.2f));
    };

    engine["createPlane"] = [this](const std::string& name, float x, float y, float z, sol::optional<float> size) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->createPlane(name, x, y, z, size.value_or(10.0f));
    };

    engine["createModel"] = [this](const std::string& name, const std::string& path, float x, float y, float z) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->createModel(name, path, x, y, z);
    };

    engine["getEntity"] = [this](const std::string& name) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return m_scene->getEntity(name);
    };

    engine["removeEntity"] = [this](const std::string& name) -> bool {
        if (!m_scene) return false;
        return m_scene->removeEntity(name);
    };

    engine["setGroup"] = [this](const std::string& entName, const std::string& groupName) {
        if (m_scene) m_scene->setEntityGroup(entName, groupName);
    };

    engine["clearScene"] = [this]() {
        if (m_scene) m_scene->clear();
    };

    engine["importModel"] = [this](const std::string& path, sol::optional<float> x, sol::optional<float> y, sol::optional<float> z) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        return SceneIO::importModel(*m_scene, path, glm::vec3(x.value_or(0.0f), y.value_or(0.0f), z.value_or(0.0f)));
    };

    engine["exportScene"] = [this](const std::string& path) -> bool {
        if (!m_scene) return false;
        return SceneIO::exportSceneToOBJ(*m_scene, path);
    };

    // Spawning Dinámico de Prefabs y Entidades (Fase 17)
    engine["spawnPrefab"] = [this](const std::string& pathOrName, sol::optional<float> x, sol::optional<float> y, sol::optional<float> z) -> sol::object {
        if (!m_scene) return sol::nil;
        glm::vec3 spawnPos(x.value_or(0.0f), y.value_or(0.0f), z.value_or(0.0f));
        auto spawned = PrefabManager::instantiatePrefab(pathOrName, *m_scene, m_physicsWorld, spawnPos);
        if (spawned.empty()) return sol::nil;
        if (spawned.size() == 1) {
            return sol::make_object(m_lua, spawned[0]);
        }
        sol::table t = m_lua.create_table();
        for (size_t i = 0; i < spawned.size(); ++i) {
            t[i + 1] = spawned[i];
        }
        return t;
    };

    engine["spawnEntity"] = [this](const std::string& name, const std::string& type, sol::optional<float> x, sol::optional<float> y, sol::optional<float> z, sol::optional<float> size) -> std::shared_ptr<Entity> {
        if (!m_scene) return nullptr;
        float px = x.value_or(0.0f), py = y.value_or(0.0f), pz = z.value_or(0.0f);
        float s = size.value_or(1.0f);
        if (type == "Sphere") return m_scene->createSphere(name, px, py, pz, s * 0.5f);
        if (type == "Cylinder") return m_scene->createCylinder(name, px, py, pz, s * 0.5f, s);
        if (type == "Cone") return m_scene->createCone(name, px, py, pz, s * 0.5f, s);
        if (type == "Torus") return m_scene->createTorus(name, px, py, pz, s * 0.6f, s * 0.2f);
        if (type == "Plane") return m_scene->createPlane(name, px, py, pz, s);
        return m_scene->createCube(name, px, py, pz, s);
    };

    engine["destroyEntity"] = [this](const std::string& name) -> bool {
        if (m_physicsWorld) m_physicsWorld->removeRigidBody(name);
        if (m_scene) return m_scene->removeEntity(name);
        return false;
    };

    engine["setTrigger"] = [this](const std::string& name, bool trig) {
        if (m_scene) {
            auto ent = m_scene->getEntity(name);
            if (ent) ent->setTrigger(trig);
        }
        if (m_physicsWorld) {
            m_physicsWorld->setBodyTrigger(name, trig);
        }
    };

    engine["isTrigger"] = [this](const std::string& name) -> bool {
        if (m_scene) {
            auto ent = m_scene->getEntity(name);
            if (ent) return ent->isTrigger();
        }
        return false;
    };

    engine["enablePhysics"] = [this](const std::string& name, sol::optional<float> mass) {
        if (m_scene && m_physicsWorld) {
            auto ent = m_scene->getEntity(name);
            if (ent) m_physicsWorld->addRigidBody(ent, mass.value_or(1.0f));
        }
    };

    engine["applyImpulse"] = [this](const std::string& name, float fx, float fy, float fz) {
        if (m_physicsWorld) {
            m_physicsWorld->applyImpulse(name, glm::vec3(fx, fy, fz));
        }
    };

    engine["setGravity"] = [this](float gx, float gy, float gz) {
        if (m_physicsWorld) {
            m_physicsWorld->setGravity(glm::vec3(gx, gy, gz));
        }
    };

    engine["log"] = [this](const std::string& message) {
        std::cout << "[Lua Script] " << message << std::endl;
        addLog("[Script] " + message);
    };

    engine["getTime"] = [this]() -> float {
        return m_totalTime;
    };

    engine["getDeltaTime"] = [this]() -> float {
        return m_lastDeltaTime;
    };

    // Control de Auto-Exposición y Cámara (Fase 17)
    engine["setAutoExposure"] = [this](bool enabled) {
        if (m_editorUI) m_editorUI->setAutoExposureEnabled(enabled);
    };

    engine["isAutoExposure"] = [this]() -> bool {
        return m_editorUI ? m_editorUI->isAutoExposureEnabled() : false;
    };

    engine["setExposure"] = [this](float exp) {
        if (m_editorUI) m_editorUI->setExposure(exp);
    };

    engine["getExposure"] = [this]() -> float {
        return m_editorUI ? m_editorUI->getExposure() : 1.0f;
    };

    engine["getSceneLuminance"] = [this]() -> float {
        return m_editorUI ? m_editorUI->getMeasuredSceneLuminance() : 0.18f;
    };

    // Audio Espacial y SFX en Triggers (Fase 16 y 17)
    engine["playJump"] = [](sol::optional<float> vol) {
        AudioEngine::get().playJump(vol.value_or(1.0f));
    };

    engine["playImpact"] = [](float x, float y, float z, sol::optional<float> impulse) {
        AudioEngine::get().playImpact(glm::vec3(x, y, z), impulse.value_or(5.0f));
    };

    engine["playSound3D"] = [](const std::string& name, float x, float y, float z, sol::optional<float> vol, sol::optional<float> pitch) {
        AudioEngine::get().playSound3D(name, glm::vec3(x, y, z), vol.value_or(1.0f), pitch.value_or(1.0f));
    };

    // Teletransporte del Jugador y Gameplay Dinámico (Fase 17 / 20)
    engine["teleportPlayer"] = [this](float x, float y, float z) {
        if (m_character) {
            m_character->setPosition(glm::vec3(x, y, z));
            addLog("[Gameplay] Teletransportando jugador a (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
        }
    };

    engine["getPlayerPosition"] = [this]() -> sol::table {
        sol::table t = m_lua.create_table();
        glm::vec3 pos(0.0f);
        if (m_character) {
            pos = m_character->getPosition();
        }
        t[1] = pos.x;
        t[2] = pos.y;
        t[3] = pos.z;
        return t;
    };

    engine["getEntityPosition"] = [this](const std::string& name) -> sol::table {
        sol::table t = m_lua.create_table();
        glm::vec3 pos(0.0f);
        if (m_scene) {
            auto ent = m_scene->getEntity(name);
            if (ent) pos = ent->getPosition();
        }
        t[1] = pos.x;
        t[2] = pos.y;
        t[3] = pos.z;
        return t;
    };

    engine["explode"] = [this](sol::object targetOrName, sol::optional<float> radiusOpt, sol::optional<float> forceOpt) {
        glm::vec3 center(0.0f);
        float radius = radiusOpt.value_or(12.0f);
        float force = forceOpt.value_or(35.0f);

        if (targetOrName.is<std::string>()) {
            std::string name = targetOrName.as<std::string>();
            if (m_scene) {
                auto ent = m_scene->getEntity(name);
                if (ent) center = ent->getPosition();
            }
        } else if (targetOrName.is<sol::table>()) {
            sol::table tbl = targetOrName.as<sol::table>();
            center.x = tbl.get_or(1, 0.0f);
            center.y = tbl.get_or(2, 0.0f);
            center.z = tbl.get_or(3, 0.0f);
        }

        if (m_physicsWorld) {
            m_physicsWorld->applyExplosionImpulse(center, radius, force);
        }

        AudioEngine::get().playImpact(center, 25.0f);

        if (m_character) {
            glm::vec3 charPos = m_character->getPosition();
            glm::vec3 diff = charPos - center;
            float dist = glm::length(diff);
            if (dist < radius && dist > 0.001f) {
                glm::vec3 knockback = glm::normalize(diff) * (force * 0.35f * (1.0f - dist / radius)) + glm::vec3(0.0f, 6.0f, 0.0f);
                m_character->setVelocity(m_character->getVelocity() + knockback);
            }
        }

        addLog("[Gameplay] Explosion en (" + std::to_string(center.x) + ", " + std::to_string(center.y) + ", " + std::to_string(center.z) + ")!");
    };

    // -------------------------------------------------------------
    // Control de Animación Cinemática / Sequencer (Fase 22)
    // -------------------------------------------------------------
    engine["playSequence"] = [this](sol::optional<std::string> seqPath) {
        if (seqPath.has_value() && !seqPath.value().empty()) {
            std::string path = seqPath.value();
            if (!std::filesystem::exists(path) && std::filesystem::exists("assets/sequences/" + path)) {
                path = "assets/sequences/" + path;
            }
            Sequencer::get().loadFromFile(path);
        }
        Sequencer::get().play();
        addLog("[Sequencer] Reproduciendo secuencia cinematica...");
    };

    engine["pauseSequence"] = [this]() {
        Sequencer::get().pause();
        addLog("[Sequencer] Secuencia en pausa.");
    };

    engine["stopSequence"] = [this]() {
        Sequencer::get().stop();
        addLog("[Sequencer] Secuencia detenida y reiniciada a t=0s.");
    };

    engine["setSequenceTime"] = [](float timeSec) {
        Sequencer::get().setTime(timeSec);
    };

    engine["isSequencePlaying"] = []() -> bool {
        return Sequencer::get().isPlaying();
    };

    engine["triggerCameraShake"] = [this](float intensity, sol::optional<float> duration) {
        float dur = duration.value_or(0.5f);
        Sequencer::get().triggerImpulseShake(intensity, dur);
        addLog("[Sequencer] Camara Shake disparado: intensidad=" + std::to_string(intensity) + ", duracion=" + std::to_string(dur) + "s");
    };

    // Control y Creación Cinemática Dinámica (Fase 22.5)
    engine["seq_clear"] = []() {
        Sequencer::get().clearAllTracks();
    };

    engine["seq_addEntityTrack"] = [](const std::string& entityName) {
        Sequencer::get().addEntityTrack(entityName);
    };

    engine["seq_addKeyframe"] = [](const std::string& entityName, float time,
                                   float px, float py, float pz,
                                   float rx, float ry, float rz,
                                   float sx, float sy, float sz,
                                   sol::optional<int> interpMode) {
        auto& tracks = Sequencer::get().getEntityTracks();
        EntityTransformTrack* targetTrack = nullptr;
        for (auto& tr : tracks) {
            if (tr.entityName == entityName) {
                targetTrack = &tr;
                break;
            }
        }
        if (!targetTrack) {
            Sequencer::get().addEntityTrack(entityName);
            for (auto& tr : Sequencer::get().getEntityTracks()) {
                if (tr.entityName == entityName) {
                    targetTrack = &tr;
                    break;
                }
            }
        }
        if (targetTrack) {
            glm::vec3 pos(px, py, pz);
            glm::quat rot = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
            glm::vec3 scl(sx, sy, sz);
            InterpolationMode mode = (InterpolationMode)interpMode.value_or(2); // CUBIC_BEZIER
            targetTrack->addKeyframe(time, pos, rot, scl, mode);
        }
    };

    engine["seq_bake"] = [this](sol::optional<float> duration, sol::optional<float> interval) {
        if (m_scene) {
            Sequencer::get().bakeScene(*m_scene, duration.value_or(10.0f), interval.value_or(0.5f));
        }
    };

    // Funciones globales en Lua para acceso directo
    m_lua["playSequence"] = engine["playSequence"];
    m_lua["pauseSequence"] = engine["pauseSequence"];
    m_lua["stopSequence"] = engine["stopSequence"];
    m_lua["setSequenceTime"] = engine["setSequenceTime"];
    m_lua["isSequencePlaying"] = engine["isSequencePlaying"];
    m_lua["triggerCameraShake"] = engine["triggerCameraShake"];
    m_lua["seq_clear"] = engine["seq_clear"];
    m_lua["seq_addEntityTrack"] = engine["seq_addEntityTrack"];
    m_lua["seq_addKeyframe"] = engine["seq_addKeyframe"];
    m_lua["seq_bake"] = engine["seq_bake"];
}

bool ScriptEngine::loadScript(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        std::string err = "[ASIS Engine ERROR] Script Lua no encontrado: " + filepath;
        std::cerr << err << std::endl;
        addLog(err);
        return false;
    }

    try {
        m_scriptPath = std::filesystem::absolute(filepath).string();
        m_lastWriteTime = std::filesystem::last_write_time(m_scriptPath);

        sol::protected_function_result result = m_lua.safe_script_file(m_scriptPath, sol::script_pass_on_error);

        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "\n[ASIS Engine ERROR LUA SINTAXIS/CARGA]:\n" << err.what() << "\n\n";
            addLog("[ERROR LUA] " + std::string(err.what()));
            return false;
        }

        std::cout << "[ASIS Engine] Script Lua cargado exitosamente: " << filepath << std::endl;
        addLog("[Sistema] Script cargado: " + filepath);

        // Ejecutar onInit() si existe
        sol::protected_function onInit = m_lua["onInit"];
        if (onInit.valid()) {
            sol::protected_function_result initResult = onInit();
            if (!initResult.valid()) {
                sol::error err = initResult;
                std::cerr << "[ASIS Engine ERROR LUA en onInit()]:\n" << err.what() << std::endl;
                addLog("[ERROR onInit] " + std::string(err.what()));
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[ASIS Engine ERROR LUA]: Excepción atrapada: " << e.what() << std::endl;
        addLog("[EXCEPTION] " + std::string(e.what()));
        return false;
    }
}

ScriptExecutionResult ScriptEngine::executeString(const std::string& code) {
    ScriptExecutionResult result;
    result.success = false;
    result.entityCount = m_scene ? static_cast<int>(m_scene->getEntityCount()) : 0;

    lua_State* L = m_lua.lua_state();

    // Activar Watchdog de límite de instrucciones (máx 2.5 millones por ejecución)
    lua_sethook(L, luaInstructionWatchdog, LUA_MASKCOUNT, 2500000);

    try {
        sol::protected_function_result evalResult = m_lua.safe_script(code, sol::script_pass_on_error);

        // Desactivar hook al finalizar la ejecución
        lua_sethook(L, nullptr, 0, 0);

        if (!evalResult.valid()) {
            sol::error err = evalResult;
            result.errorMessage = err.what();
            std::cerr << "[ASIS Engine ERROR LUA eval]: " << result.errorMessage << std::endl;
            addLog("[ERROR EVAL] " + result.errorMessage);
            return result;
        }

        result.success = true;
        result.entityCount = m_scene ? static_cast<int>(m_scene->getEntityCount()) : 0;
        addLog("[IA Lua] Código ejecutado con éxito (" + std::to_string(result.entityCount) + " entidades)");
        return result;
    }
    catch (const std::exception& e) {
        lua_sethook(L, nullptr, 0, 0);
        result.errorMessage = e.what();
        std::cerr << "[ASIS Engine ERROR LUA eval]: " << result.errorMessage << std::endl;
        addLog("[EXCEPTION EVAL] " + result.errorMessage);
        return result;
    }
}

void ScriptEngine::update(float deltaTime) {
    m_lastDeltaTime = deltaTime;
    m_totalTime += deltaTime;

    checkHotReload(deltaTime);

    sol::protected_function onUpdate = m_lua["onUpdate"];
    if (onUpdate.valid()) {
        sol::protected_function_result result = onUpdate(deltaTime);
        if (!result.valid()) {
            sol::error err = result;
            if (!m_updateHasError) {
                std::cerr << "[ASIS Engine ERROR LUA en onUpdate()]: " << err.what() << std::endl;
                addLog("[ERROR onUpdate] " + std::string(err.what()));
                m_updateHasError = true;
            }
        } else {
            m_updateHasError = false;
        }
    }
}

void ScriptEngine::checkHotReload(float deltaTime) {
    m_hotReloadCheckTimer += deltaTime;

    if (m_hotReloadCheckTimer >= 0.25f) {
        m_hotReloadCheckTimer = 0.0f;

        if (!m_scriptPath.empty() && std::filesystem::exists(m_scriptPath)) {
            try {
                auto currentWriteTime = std::filesystem::last_write_time(m_scriptPath);
                if (currentWriteTime != m_lastWriteTime) {
                    std::cout << "\n=======================================================\n";
                    std::cout << " [HOT-RELOAD LUA] Detectado cambio en: " << m_scriptPath << "\n";
                    std::cout << " Recargando script en vivo sin reiniciar el motor...\n";
                    std::cout << "=======================================================\n";
                    m_lastWriteTime = currentWriteTime;
                    loadScript(m_scriptPath);
                }
            } catch (const std::filesystem::filesystem_error&) {
            }
        }
    }
}

void ScriptEngine::addLog(const std::string& log) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_logs.push_back(log);
    // Limitar histórico a los últimos 300 mensajes
    if (m_logs.size() > 300) {
        m_logs.erase(m_logs.begin(), m_logs.begin() + 50);
    }
}

std::vector<std::string> ScriptEngine::getLogs() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    return m_logs;
}

void ScriptEngine::clearLogs() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_logs.clear();
}

void ScriptEngine::dispatchTriggerEnter(const std::string& triggerName, const std::string& otherName) {
    sol::protected_function onTriggerEnter = m_lua["onTriggerEnter"];
    if (onTriggerEnter.valid()) {
        sol::protected_function_result result = onTriggerEnter(triggerName, otherName);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[ASIS Lua ERROR onTriggerEnter]: " << err.what() << std::endl;
            addLog("[ERROR onTriggerEnter] " + std::string(err.what()));
        }
    }
}

void ScriptEngine::dispatchTriggerExit(const std::string& triggerName, const std::string& otherName) {
    sol::protected_function onTriggerExit = m_lua["onTriggerExit"];
    if (onTriggerExit.valid()) {
        sol::protected_function_result result = onTriggerExit(triggerName, otherName);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[ASIS Lua ERROR onTriggerExit]: " << err.what() << std::endl;
            addLog("[ERROR onTriggerExit] " + std::string(err.what()));
        }
    }
}

} // namespace ASIS
