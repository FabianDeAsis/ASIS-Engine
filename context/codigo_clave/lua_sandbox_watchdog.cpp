// ==============================================================================
// Snippet: Sandbox de Lua 5.4 y Watchdog de Instrucciones (Prevención de Congelamientos)
// ==============================================================================
#include <sol/sol.hpp>

// Hook ejecutado por el intérprete Lua tras alcanzar N instrucciones
static void luaInstructionWatchdog(lua_State* L, lua_Debug* ar) {
    luaL_error(L, "Watchdog de Seguridad: Limite de instrucciones excedido (bucle infinito abortado).");
}

void setupSandboxAndExecute(sol::state& lua, const std::string& unsafeAICode) {
    // 1. Sandbox: solo habilitar librerías puras de computación y cadenas
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    lua["dofile"] = sol::nil;
    lua["loadfile"] = sol::nil;
    lua["collectgarbage"] = sol::nil;

    // 2. Activar Watchdog con cuota de 2.5 millones de instrucciones de CPU
    lua_State* L = lua.lua_state();
    lua_sethook(L, luaInstructionWatchdog, LUA_MASKCOUNT, 2500000);

    // 3. Ejecución protegida
    sol::protected_function_result res = lua.safe_script(unsafeAICode, sol::script_pass_on_error);

    // 4. Desactivar hook al finalizar
    lua_sethook(L, nullptr, 0, 0);

    if (!res.valid()) {
        sol::error err = res;
        std::cerr << "Error capturado sin crashear el motor: " << err.what() << "\n";
    }
}
