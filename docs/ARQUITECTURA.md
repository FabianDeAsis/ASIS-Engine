# 🏛️ Arquitectura del Motor Gráfico (ASIS Engine)

## 1. Visión General del Sistema
El motor adopta una arquitectura híbrida desacoplada en tres capas principales:

1. **Capa de Rendimiento (C++20)**: Núcleo del motor, renderizado gráfico (OpenGL/Vulkan), ventana e inputs (GLFW), matemáticas (GLM) y servidor de comunicación interna.
2. **Capa de Lógica y Hot-Reload (Lua 5.4 + sol2)**: Scripts de comportamiento de entidades, transformaciones, escenas y llamadas a la API expuesta por el núcleo en tiempo real.
3. **Capa de Inteligencia Artificial (Python 3.12)**: Servidor puente que procesa las intenciones del usuario (prompts), interactúa con Antigravity / LLMs y genera/modifica código Lua dinámicamente sin reiniciar el motor.

---

## 2. Diagrama de Módulos y Dependencias

```
+--------------------------------------------------------------------------+
|                       USUARIO / DESARROLLADOR                            |
|                 (Instrucciones en Lenguaje Natural)                      |
+------------------------------------+-------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
|                     CAPA DE IA (Python 3.12)                             |
|  - Conector Antigravity / API LLM                                        |
|  - Generador & Validador de Scripts Lua                                  |
|  - Cliente IPC / Socket de Comunicación con C++                          |
+------------------------------------+-------------------------------------+
                                     | (Socket TCP / JSON-RPC / Lua Files)
                                     v
+--------------------------------------------------------------------------+
|                 NÚCLEO DEL MOTOR GRÁFICO (C++20)                         |
|                                                                          |
|   +-----------------------+              +---------------------------+   |
|   |   Servidor IPC /      |              |   Módulo de Scripting     |   |
|   |   Listener Comandos   | -----------> |   (Lua 5.4 + sol2 Binding)|   |
|   +-----------------------+              +-------------+-------------+   |
|                                                        |                 |
|                                                        v                 |
|   +----------------------------------------------------+-------------+   |
|   |                        GESTOR DE ESCENA                          |   |
|   |                (Objetos, Cámaras, Luces, Transform)              |   |
|   +------------------------------------+-----------------------------+   |
|                                        |                                 |
|          +-----------------------------+-----------------------------+   |
|          |                                                           |   |
|          v                                                           v   |
|   +---------------+                                           +--------------+
|   | Sub-sistema   |                                           | Sub-sistema  |
|   | de Entrada    |                                           | Renderizado  |
|   | (GLFW Input)  |                                           | (OpenGL/Glad)|
|   +---------------+                                           +-------+------+
|                                                                       |   |
+-----------------------------------------------------------------------|---|
                                                                        v
                                                              +-------------------+
                                                              |  PANTALLA / GPU   |
                                                              +-------------------+
```

---

## 3. Flujo de Datos

### A. Flujo de Ejecución Normal (Ciclo Principal del Motor)
```
[Entrada Usuario (Teclado/Ratón)] 
       │
       ▼
 [GLFW Poll Events] ──► [Actualizar Inputs en C++]
                                │
                                ▼
                   [Ejecutar Scripts Lua (Update)]
                                │
                                ▼
                   [Actualizar Matriz & Escena]
                                │
                                ▼
                   [Renderer: Dibujar Shaders & Meshes]
                                │
                                ▼
                     [GLFW Swap Buffers ──► Pantalla]
```

### B. Flujo de Datos con IA (Generación en Tiempo Real)
```
1. Usuario escribe prompt: "Crea una pirámide rotando con color dorado".
2. Capa Python (ai_module):
   - Recibe el prompt y el esquema de la API Lua disponible.
   - Genera el script Lua seguro con la nueva entidad.
   - Envía el comando o guarda el archivo de script.
3. Núcleo C++:
   - Detecta el cambio o recibe el script vía socket.
   - Ejecuta el script dentro de la máquina virtual de Lua (sol2).
   - Registra el nuevo objeto en la escena sin detener el render.
4. GPU:
   - En el siguiente frame, el nuevo objeto ya se renderiza inmediatamente.
```
