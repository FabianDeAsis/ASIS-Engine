import socket
import time
from typing import Tuple

class EngineClient:
    """
    Cliente socket TCP bidireccional para comunicar el módulo de IA con el núcleo C++ de ASIS Engine.
    Permite enviar código Lua y recibir retroalimentación instantánea del motor (errores o confirmación).
    """
    def __init__(self, host: str = "127.0.0.1", port: int = 5555):
        self.host = host
        self.port = port
        self.sock = None
        self.delimiter = "###END###"

    def connect(self, timeout: float = 3.0) -> bool:
        """Intenta conectarse al motor gráfico."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(timeout)
            self.sock.connect((self.host, self.port))
            print(f"[Cliente IA] Conectado exitosamente con ASIS Engine en {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"[Cliente IA] No se pudo conectar al motor en {self.host}:{self.port} ({e})")
            self.sock = None
            return False

    def is_connected(self) -> bool:
        return self.sock is not None

    def send_lua(self, lua_code: str) -> Tuple[bool, str]:
        """
        Envía código Lua al motor y espera la retroalimentación de ejecución del bucle principal.
        Retorna: (éxito: bool, mensaje_o_error: str)
        """
        if not self.sock:
            if not self.connect():
                return False, "Motor no conectado"

        try:
            # Enviar código delimitado
            payload = lua_code + self.delimiter
            self.sock.sendall(payload.encode("utf-8"))

            # 1. Recibir acuse de recibo inmediato (ACK)
            self.sock.settimeout(2.0)
            initial_ack = self.sock.recv(512).decode("utf-8", errors="replace")

            # 2. Recibir resultado de la ejecución desde el render loop
            self.sock.settimeout(3.0)
            execution_feedback = self.sock.recv(4096).decode("utf-8", errors="replace")

            if "RESULT:ERROR:" in execution_feedback:
                error_msg = execution_feedback.split("RESULT:ERROR:", 1)[1].strip()
                return False, error_msg
            elif "RESULT:OK:" in execution_feedback:
                ok_info = execution_feedback.split("RESULT:OK:", 1)[1].strip()
                return True, ok_info

            return True, "Ejecutado (sin reporte de error)"
        except socket.timeout:
            return True, "Ejecutado (ACK recibido sin errores reportados)"
        except Exception as e:
            print(f"[Cliente IA ERROR] Error al comunicar con el motor: {e}")
            self.disconnect()
            return False, str(e)

    def disconnect(self):
        """Cierra la conexión con el motor."""
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None
            print("[Cliente IA] Conexión cerrada.")
