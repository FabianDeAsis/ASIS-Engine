import sys
import os

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from engine_client import EngineClient

def send_file(filepath: str):
    if not os.path.exists(filepath):
        print(f"Error: No existe {filepath}")
        return False

    with open(filepath, "r", encoding="utf-8") as f:
        code = f.read()

    client = EngineClient(host="127.0.0.1", port=5555)
    if client.connect():
        ok = client.send_lua(code)
        client.disconnect()
        return ok
    return False

if __name__ == "__main__":
    if len(sys.argv) > 1:
        success = send_file(sys.argv[1])
        print("Enviado con éxito:", success)
