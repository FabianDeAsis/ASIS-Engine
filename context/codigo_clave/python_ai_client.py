# Snippet Clave: Cliente Python para Enviar Scripts Lua al Motor Gráfico
# Archivo de referencia: src/ai_module/engine_client.py

import socket

def send_lua_to_engine(host: str, port: int, lua_code: str) -> bool:
    delimiter = "###END###"
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(3.0)
            s.connect((host, port))
            payload = lua_code + delimiter
            s.sendall(payload.encode("utf-8"))
            response = s.recv(1024).decode("utf-8")
            return "received" in response
    except Exception as e:
        print(f"Error conectando con ASIS Engine: {e}")
        return False
