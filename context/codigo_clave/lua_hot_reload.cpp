// Snippet Clave: Sistema de Recarga en Caliente (Hot-Reload) de Scripts Lua
// Archivo de referencia: src/core/ScriptEngine.cpp

#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>

void checkAndReloadScript(sol::state& lua, const std::string& path, std::filesystem::file_time_type& lastWriteTime) {
    if (std::filesystem::exists(path)) {
        auto currentWrite = std::filesystem::last_write_time(path);
        if (currentWrite != lastWriteTime) {
            lastWriteTime = currentWrite;
            std::cout << "[HOT-RELOAD] Recargando " << path << "...\n";

            // safe_script_file previene crashes si el script tiene errores de sintaxis
            sol::protected_function_result result = lua.safe_script_file(path, sol::script_pass_on_error);
            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "[ERROR LUA]: " << err.what() << "\n";
                return;
            }

            // Ejecutar onInit() de reinicialización
            sol::protected_function onInit = lua["onInit"];
            if (onInit.valid()) onInit();
        }
    }
}
