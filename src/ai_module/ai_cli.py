import sys
import os

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from engine_client import EngineClient
from ai_generator import AIGenerator

def main():
    if len(sys.argv) < 2:
        print("[AI CLI Error] No se proporcionó ningún prompt.")
        sys.exit(1)

    prompt = sys.argv[1].strip()
    if not prompt:
        print("[AI CLI Error] Prompt vacío.")
        sys.exit(1)

    client = EngineClient(host="127.0.0.1", port=5555)
    connected = client.connect(timeout=1.5)

    def log_to_engine(msg: str):
        print(msg)
        if connected and client.is_connected():
            try:
                # Sanitizar comillas para que Lua no falle
                clean = msg.replace('"', "'").replace('\\', '/')
                client.send_lua(f'Engine.log("{clean}")')
            except Exception:
                pass

    log_to_engine(f"[IA] Solicitud recibida: '{prompt}'...")

    generator = AIGenerator()

    # 1. Generar código Lua con Gemini (o fallback inteligente)
    lua_code = generator.generate_lua(prompt, log_cb=log_to_engine)

    # 2. Guardar en disco para registro
    scripts_dir = os.path.join(os.path.dirname(__file__), "..", "scripts")
    generated_file = os.path.join(scripts_dir, "ai_generated.lua")
    try:
        os.makedirs(scripts_dir, exist_ok=True)
        with open(generated_file, "w", encoding="utf-8") as f:
            f.write(lua_code)
    except Exception as e:
        print(f"[AI CLI Advertencia] No se pudo guardar en disco: {e}")

    # 3. Enviar al motor para renderizado
    if not connected or not client.is_connected():
        client.connect(timeout=1.5)

    if client.is_connected():
        success, info = client.send_lua(lua_code)
        if success:
            log_to_engine(f"[IA] ✓ ¡Escena renderizada en pantalla! ({info})")
        else:
            log_to_engine(f"[IA Advertencia] Falló la ejecución en Lua: {info}")
            log_to_engine("[IA Auto-Corrección] Activando auto-corrección autónoma con Gemini...")
            corrected_code = generator.auto_correct_lua(lua_code, info)

            try:
                with open(generated_file, "w", encoding="utf-8") as f:
                    f.write(corrected_code)
            except Exception:
                pass

            success2, info2 = client.send_lua(corrected_code)
            if success2:
                log_to_engine(f"[IA] ✓ ¡Auto-corrección exitosa! ({info2})")
            else:
                log_to_engine(f"[IA Error] Auto-corrección falló: {info2}")
        client.disconnect()
    else:
        print("[AI CLI] Motor no conectado.")

if __name__ == "__main__":
    main()
