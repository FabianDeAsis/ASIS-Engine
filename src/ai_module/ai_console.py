import sys
import os
import time

# Permitir importaciones locales desde este directorio
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from engine_client import EngineClient
from ai_generator import AIGenerator

def main():
    print("==================================================================")
    print("    🤖 ASIS ENGINE - CONSOLA DE INTELIGENCIA ARTIFICIAL          ")
    print("==================================================================")
    print(" Controla y construye el motor gráfico en tiempo real con prompts.\n")

    client = EngineClient(host="127.0.0.1", port=5555)
    generator = AIGenerator()

    if generator.gemini_key:
        print("[IA Config] ✓ Google Gemini API conectada (Modelos: gemini-3.1-flash-lite, gemini-3.5-flash-lite)")
    elif generator.openai_key:
        print("[IA Config] ✓ OpenAI GPT API conectada")
    else:
        print("[IA Config] ⚠️ No se detectó GEMINI_API_KEY en .env. Se usarán plantillas offline.")

    # Intentar conexión inicial
    connected = client.connect(timeout=2.0)
    if not connected:
        print("[!] No se detectó el motor en ejecución.")
        print("    Abre primero el motor ejecutando 'run.bat' y luego escribe aquí.\n")

    print("Comandos especiales:")
    print(" - 'salir' o 'exit' : Cerrar esta consola")
    print(" - 'conectar'       : Reintentar conexión con el motor")
    print(" - 'limpiar'        : Borrar todos los objetos de la escena")
    print("------------------------------------------------------------------")

    # Ruta del script generado para que también quede guardado en disco
    scripts_dir = os.path.join(os.path.dirname(__file__), "..", "scripts")
    generated_file = os.path.join(scripts_dir, "ai_generated.lua")

    while True:
        try:
            prompt = input("\nASIS-IA > ").strip()
            if not prompt:
                continue

            if prompt.lower() in ["salir", "exit", "quit"]:
                print("Cerrando consola de IA.")
                client.disconnect()
                break

            if prompt.lower() == "conectar":
                client.connect(timeout=2.0)
                continue

            if prompt.lower() == "limpiar":
                prompt = "limpia la escena por favor"

            print(f"[IA] Procesando instrucción: '{prompt}'...")
            lua_code = generator.generate_lua(prompt)

            print("\n----- [CÓDIGO LUA GENERADO POR LA IA] -----")
            print(lua_code)
            print("-------------------------------------------\n")

            # Guardar el script generado en disco
            try:
                os.makedirs(scripts_dir, exist_ok=True)
                with open(generated_file, "w", encoding="utf-8") as f:
                    f.write(lua_code)
                print(f"[Disco] Guardado en: {generated_file}")
            except Exception as e:
                print(f"[Disco ADVERTENCIA] No se pudo escribir archivo: {e}")

            # Enviar directamente al motor gráfico mediante socket TCP
            if not client.is_connected():
                print("[!] Intentando reconectar con el motor...")
                client.connect(timeout=2.0)

            if client.is_connected():
                success, info = client.send_lua(lua_code)
                if success:
                    print(f">>> [✓] ¡Escena actualizada en tiempo real en la pantalla del motor! ({info}) <<<")
                else:
                    print(f"\n[⚠️ Feedback del Motor] Falló la ejecución en Lua:")
                    print(f"    Error: {info}")
                    print("[🤖 IA Auto-Corrección] Iniciando bucle de auto-corrección autónomo...")

                    corrected_code = generator.auto_correct_lua(lua_code, info)
                    print("\n----- [CÓDIGO LUA CORREGIDO POR LA IA] -----")
                    print(corrected_code)
                    print("--------------------------------------------\n")

                    try:
                        with open(generated_file, "w", encoding="utf-8") as f:
                            f.write(corrected_code)
                    except Exception:
                        pass

                    success2, info2 = client.send_lua(corrected_code)
                    if success2:
                        print(f">>> [✓] ¡Auto-corrección exitosa! Escena renderizada ({info2}) <<<")
                    else:
                        print(f"[X] Falló la auto-corrección secundaria: {info2}")
            else:
                print("[!] El motor no está abierto. El script fue guardado y se ejecutará al iniciar.")

        except (KeyboardInterrupt, EOFError):
            print("\nCerrando consola de IA...")
            client.disconnect()
            break

if __name__ == "__main__":
    main()
