// Snippet Clave: Vinculación de Clases y APIs de C++ hacia Lua con sol2
// Archivo de referencia: src/core/ScriptEngine.cpp

#include <sol/sol.hpp>
#include "Entity.h"
#include "Scene.h"

void registerLuaAPIs(sol::state& lua, ASIS::Scene* scene) {
    // 1. Vincular clase Entity
    lua.new_usertype<ASIS::Entity>("Entity",
        "setPosition", &ASIS::Entity::setPosition,
        "setRotation", &ASIS::Entity::setRotation,
        "setScale", &ASIS::Entity::setScale,
        "setColor", &ASIS::Entity::setColor,
        "move", &ASIS::Entity::move,
        "rotate", &ASIS::Entity::rotate,
        "getName", &ASIS::Entity::getName
    );

    // 2. Vincular tabla global Engine
    sol::table engine = lua.create_named_table("Engine");

    engine["createCube"] = [scene](const std::string& name, float x, float y, float z, sol::optional<float> size) {
        return scene->createCube(name, x, y, z, size.value_or(1.0f));
    };

    engine["createModel"] = [scene](const std::string& name, const std::string& path, float x, float y, float z) {
        return scene->createModel(name, path, x, y, z);
    };

    engine["getEntity"] = [scene](const std::string& name) {
        return scene->getEntity(name);
    };

    engine["clearScene"] = [scene]() {
        scene->clear();
    };
}
