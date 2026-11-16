# ==============================================================================
# Snippet: Bucle Bidireccional de Auto-Corrección Autónomo (Feedback Loop)
# ==============================================================================
def execute_with_self_healing(client, generator, lua_code: str):
    # 1. Enviar código al motor y esperar feedback de ejecución
    success, info = client.send_lua(lua_code)
    
    if success:
        print(f"✓ Ejecución exitosa en el motor gráfico ({info})")
        return lua_code
    
    # 2. Si el motor C++ reporta un fallo de Lua, activar auto-reparación
    print(f"[!] Error reportado por el motor: {info}")
    print("[🤖 IA] Auto-corrigiendo código con Gemini...")
    
    correction_prompt = f"""El motor gráfico ASIS Engine falló con este error:
ERROR: {info}

CÓDIGO:
```lua
{lua_code}
```
Corrige el código Lua para solucionar el error. Devuelve ÚNICAMENTE el bloque ```lua ... ``` corregido."""

    corrected_code = generator.generate_lua(correction_prompt)
    
    # 3. Reenviar script reparado al motor
    success2, info2 = client.send_lua(corrected_code)
    if success2:
        print(f"✓ ¡Auto-corrección exitosa! ({info2})")
    return corrected_code
