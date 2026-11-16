import os
import re
import time
from typing import Optional, Callable
import dotenv

# Cargar variables de entorno desde .env
env_file_path = os.path.join(os.path.dirname(__file__), ".env")
dotenv.load_dotenv(env_file_path, override=True)

SYSTEM_PROMPT = """Eres el generador de código de Inteligencia Artificial para ASIS Engine (un motor gráfico C++ / OpenGL 3D).
Tu función es convertir solicitudes en lenguaje natural en código Lua 5.4 ejecutable que construya escenas 3D creativas y detalladas.

Reglas obligatorias:
1. Devuelve ÚNICAMENTE el bloque de código ```lua ... ``` sin texto conversacional antes ni después.
2. NUNCA uses librerías restringidas del sistema operativo (io.*, os.execute, os.remove, os.rename, debug.*). Usa ÚNICAMENTE la API de ASIS Engine.
3. Para construir objetos complejos (como personajes, vehículos, monumentos, comida, edificios, robots, animales, barcos, árboles, etc.), aprovecha toda la variedad de primitivas 3D disponibles:
   - Cubos (Engine.createCube) para chasis, paredes, cajas, ladrillos.
   - Esferas (Engine.createSphere) para ruedas, ojos, orbes, cabezas, articulaciones, pelotas.
   - Cilindros (Engine.createCylinder) para columnas, torres, troncos de árbol, capas circulares, ruedas.
   - Conos (Engine.createCone) para tejados cónicos, pinos, sombreros, flechas, púas.
   - Toros (Engine.createTorus) para anillos, donas, aros, portales, volantes.
   - Planos (Engine.createPlane) para suelos y plataformas.
4. El plano del suelo está en Y = 0. Coloca las bases en Y >= 0.5 para que los objetos no queden enterrados.
5. Por defecto, NUNCA llames a `Engine.clearScene()` a menos que el usuario especifique explícitamente "borra", "limpia" o "nueva escena vacía". Añade las nuevas entidades a la escena existente conservando todo lo anterior.
6. Utiliza colores RGB vivos y variados (valores flotantes entre 0.0 y 1.0) para cada parte del objeto (ojos, ropa, detalles, etc.).
7. Al generar estructuras compuestas (castillos, autos, torres, barcos, robots), SIEMPRE empaqueta sus piezas dentro de un grupo usando `entity:setGroup('NombreEstructura')` para que el usuario pueda manipularlas en bloque en la jerarquía.

APIs disponibles del motor:
- Engine.createCube(name: string, x: float, y: float, z: float, size: float = 1.0) -> Entity
- Engine.createSphere(name: string, x: float, y: float, z: float, radius: float = 0.5) -> Entity
- Engine.createCylinder(name: string, x: float, y: float, z: float, radius: float = 0.5, height: float = 1.0) -> Entity
- Engine.createCone(name: string, x: float, y: float, z: float, radius: float = 0.5, height: float = 1.0) -> Entity
- Engine.createTorus(name: string, x: float, y: float, z: float, mainRadius: float = 0.6, tubeRadius: float = 0.2) -> Entity
- Engine.createPlane(name: string, x: float, y: float, z: float, size: float = 10.0) -> Entity
- Engine.createModel(name: string, modelPath: string, x: float, y: float, z: float) -> Entity
- Engine.getEntity(name: string) -> Entity or nil
- Engine.removeEntity(name: string) -> boolean
- Engine.clearScene() -- Usar SOLO si el usuario pide explícitamente limpiar o reiniciar la escena
- Engine.log(message: string)
- Engine.getTime() -> float
- Engine.getDeltaTime() -> float
- Engine.setGravity(gx: float, gy: float, gz: float) -- Configura gravedad (defecto: 0, -9.81, 0)

Métodos disponibles de Entity:
- entity:setPosition(x: float, y: float, z: float)
- entity:setRotation(pitch: float, yaw: float, roll: float) -- en grados
- entity:setScale(x: float, y: float, z: float)
- entity:setUniformScale(s: float)
- entity:setColor(r: float, g: float, b: float) -- valores entre 0.0 y 1.0
- entity:move(dx: float, dy: float, dz: float)
- entity:rotate(dpitch: float, dyaw: float, droll: float)
- entity:setVisible(bool: boolean)
- entity:setGroup(groupName: string) -- Asigna la entidad a un grupo en la jerarquía de escena
- entity:getGroup() -> string
- entity:enablePhysics(mass: float) -- Activa simulación física Bullet (mass > 0 cae y colisiona; mass == 0 es estático)
- entity:applyImpulse(fx: float, fy: float, fz: float) -- Aplica impulso de fuerza física

Opcional:
- Si construyes una estructura compuesta de varias piezas, asigna a todas las piezas `entity:setGroup("NombreEstructura")`.
- Si el usuario pide física, gravedad, torres que caen o colisiones, usa `entity:enablePhysics(1.0)` en las entidades que caen o colisionan.
- Si el usuario pide animación continua sin física, usa:
function onUpdate(dt)
    local t = Engine.getTime()
    -- rotar o mover entidades aquí usando entidad:rotate(...) o entidad:setPosition(...)
end
"""

class AIGenerator:
    def __init__(self):
        dotenv.load_dotenv(os.path.join(os.path.dirname(__file__), ".env"), override=True)
        raw_key = os.getenv("GEMINI_API_KEY") or os.getenv("GOOGLE_API_KEY")
        self.gemini_key = raw_key.strip().strip("'\"") if raw_key else None

        raw_openai = os.getenv("OPENAI_API_KEY")
        self.openai_key = raw_openai.strip().strip("'\"") if raw_openai else None

    def generate_lua(self, user_prompt: str, log_cb: Optional[Callable[[str], None]] = None) -> str:
        """
        Genera código Lua a partir de una instrucción en lenguaje natural.
        Prioridad:
        1. Google Gemini API (modelos ultra-rápidos: gemini-3.1-flash-lite, gemini-3.5-flash-lite, gemini-flash-lite-latest)
        2. OpenAI API (si OPENAI_API_KEY está configurada)
        3. Generador semántico procedural inteligente offline
        """
        user_prompt_clean = user_prompt.strip()

        def log(msg: str):
            print(msg)
            if log_cb:
                try:
                    log_cb(msg)
                except Exception:
                    pass

        # 1. Consulta a Google Gemini API
        if self.gemini_key:
            import requests

            # Modelos ultra-rápidos y estables (tiempo de respuesta < 1.5s)
            candidate_models = [
                "gemini-3.1-flash-lite",
                "gemini-3.5-flash-lite",
                "gemini-flash-lite-latest"
            ]

            for model in candidate_models:
                t0 = time.time()
                try:
                    url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={self.gemini_key}"
                    headers = {"Content-Type": "application/json"}
                    data = {
                        "contents": [
                            {"parts": [{"text": SYSTEM_PROMPT}, {"text": f"Instrucción del usuario: {user_prompt_clean}"}]}
                        ],
                        "generationConfig": {
                            "temperature": 0.4
                        }
                    }

                    # Timeout corto de 10s: un modelo rápido debe responder en < 3s
                    res = requests.post(url, headers=headers, json=data, timeout=10)
                    dt = round(time.time() - t0, 2)

                    if res.status_code == 200:
                        text = res.json()["candidates"][0]["content"]["parts"][0]["text"]
                        log(f"[IA Gemini] ✓ Generado con éxito por '{model}' en {dt}s.")
                        return self._extract_code(text)
                    elif res.status_code == 429:
                        log(f"[IA Gemini] Cuota por minuto alcanzada en '{model}' (HTTP 429). Probando siguiente modelo...")
                    else:
                        try:
                            err_msg = res.json().get("error", {}).get("message", res.text[:80])
                        except Exception:
                            err_msg = res.text[:80]
                        log(f"[IA Gemini] Modelo '{model}' respondió con error {res.status_code} ({dt}s): {err_msg}")
                except requests.exceptions.Timeout:
                    log(f"[IA Advertencia] Timeout de 10s en '{model}'. Pasando al siguiente modelo...")
                except Exception as e:
                    log(f"[IA Advertencia] Error con '{model}': {e}")
        else:
            log("[IA Advertencia] No se encontró GEMINI_API_KEY configurada en .env.")

        # 2. Consulta a OpenAI API (si está configurada)
        if self.openai_key:
            try:
                import requests
                url = "https://api.openai.com/v1/chat/completions"
                headers = {
                    "Content-Type": "application/json",
                    "Authorization": f"Bearer {self.openai_key}"
                }
                data = {
                    "model": "gpt-4o-mini",
                    "messages": [
                        {"role": "system", "content": SYSTEM_PROMPT},
                        {"role": "user", "content": user_prompt_clean}
                    ],
                    "temperature": 0.4
                }
                res = requests.post(url, headers=headers, json=data, timeout=12)
                if res.status_code == 200:
                    text = res.json()["choices"][0]["message"]["content"]
                    log("[IA OpenAI] ✓ Generación exitosa con GPT.")
                    return self._extract_code(text)
            except Exception as e:
                log(f"[IA Advertencia] Error con OpenAI API: {e}")

        # 3. Modo de respaldo semántico offline (genera modelos Voxel 3D ricos según palabras clave)
        log(f"[IA Offline] Sintetizando modelo Voxel inteligente para: '{user_prompt_clean}'")
        return self._offline_smart_generator(user_prompt_clean)

    def auto_correct_lua(self, failed_code: str, error_message: str) -> str:
        """
        Feedback Loop: Utiliza Gemini para auto-corregir un script Lua fallido.
        """
        correction_prompt = f"""El motor gráfico ASIS Engine falló al ejecutar este código:
ERROR DE LUA: {error_message}

CÓDIGO:
```lua
{failed_code}
```
Corrige el código Lua para solucionar exactamente ese error. Devuelve ÚNICAMENTE el bloque ```lua ... ``` corregido."""

        print(f"[IA Auto-Corrección] Diagnosticando y reparando código con Gemini...")
        return self.generate_lua(correction_prompt)

    def _extract_code(self, text: str) -> str:
        match = re.search(r"```(?:lua)?\s*(.*?)\s*```", text, re.DOTALL)
        if match:
            return match.group(1).strip()
        return text.strip()

    def _offline_smart_generator(self, prompt: str) -> str:
        p = prompt.lower()

        if "limpia" in p or "borra" in p or "clear" in p:
            return "-- Limpieza de escena\nEngine.clearScene()\nEngine.log('Escena limpiada.')"

        # Simulación Física: Torre de bloques con proyectil esférico dinámico y Bullet Physics
        if any(w in p for w in ["fisica", "gravedad", "caer", "torre", "jenga", "derribar", "colision", "impacto"]):
            return """-- Simulacion Fisica: Torre de Bloques con Bullet Physics
-- Base estatica (mass = 0)
local base = Engine.createCube("BasePedestal", 0, 0.4, 0, 1.0)
base:setScale(3.5, 0.8, 3.5)
base:setColor(0.2, 0.25, 0.3)
base:setGroup("TorreFisica")
base:enablePhysics(0.0) -- masa 0 = estatico

-- Bloques con gravedad dinamica (mass = 1.0)
local colors = {
    {0.9, 0.2, 0.2}, -- Rojo
    {0.2, 0.8, 0.3}, -- Verde
    {0.2, 0.5, 0.9}, -- Azul
    {0.9, 0.8, 0.1}, -- Amarillo
    {0.8, 0.3, 0.9}, -- Purpura
    {0.2, 0.9, 0.9}  -- Cyan
}

local idx = 1
for floor = 1, 6 do
    local y = 1.1 + (floor - 1) * 1.05
    for col = 1, 2 do
        local x = (col - 1.5) * 1.1
        local c = colors[((idx - 1) % #colors) + 1]
        local block = Engine.createCube("Bloque_" .. floor .. "_" .. col, x, y, 0, 0.95)
        block:setColor(c[1], c[2], c[3])
        block:setGroup("TorreFisica")
        block:enablePhysics(1.0)
        idx = idx + 1
    end
end

-- Esfera proyectil física en la cima
local top = Engine.createSphere("ProyectilEsfera", 0.2, 8.0, 0.2, 0.5)
top:setColor(1.0, 0.95, 0.3)
top:setGroup("TorreFisica")
top:enablePhysics(2.0)
top:applyImpulse(0.5, -3.0, 0.5)

Engine.log("Torre con Bullet Physics generada: bloques y esfera proyectil dinamicos.")"""

        # 1. Auto / Coche / Deportivo (con ruedas cilindricas y faros esfericos)
        if any(w in p for w in ["auto", "coche", "carro", "deportivo", "vehiculo"]):
            return """-- Auto Deportivo Multi-Primitivas
-- Chasis principal
local body = Engine.createCube("Chassis", 0, 0.8, 0, 1.0)
body:setScale(3.2, 0.7, 1.6)
body:setColor(0.9, 0.1, 0.1) -- Rojo deportivo
body:setGroup("Auto")

-- Cabina (techo)
local cabin = Engine.createCube("Cabin", 0, 1.6, 0, 1.0)
cabin:setScale(1.6, 0.7, 1.3)
cabin:setColor(0.15, 0.15, 0.18) -- Vidrio oscuro
cabin:setGroup("Auto")

-- Capó delantero
local hood = Engine.createCube("Hood", 1.2, 0.95, 0, 1.0)
hood:setScale(0.9, 0.35, 1.4)
hood:setColor(0.85, 0.08, 0.08)
hood:setGroup("Auto")

-- Alerón trasero
local spoilerL = Engine.createCube("SpoilerL", -1.4, 1.4, 0.6, 0.2)
spoilerL:setColor(0.1, 0.1, 0.1)
spoilerL:setGroup("Auto")
local spoilerR = Engine.createCube("SpoilerR", -1.4, 1.4, -0.6, 0.2)
spoilerR:setColor(0.1, 0.1, 0.1)
spoilerR:setGroup("Auto")
local wing = Engine.createCube("SpoilerWing", -1.4, 1.6, 0, 1.0)
wing:setScale(0.4, 0.1, 1.8)
wing:setColor(0.1, 0.1, 0.1)
wing:setGroup("Auto")

-- 4 Ruedas usando CILINDROS 3D reales
local function makeWheel(name, x, z)
    local w = Engine.createCylinder(name, x, 0.5, z, 0.45, 0.35)
    w:setRotation(90, 0, 0)
    w:setColor(0.12, 0.12, 0.14)
    w:setGroup("Auto")
    return w
end
local w1 = makeWheel("W_FL", 1.0, 0.9)
local w2 = makeWheel("W_FR", 1.0, -0.9)
local w3 = makeWheel("W_BL", -1.0, 0.9)
local w4 = makeWheel("W_BR", -1.0, -0.9)

-- Faros con ESFERAS 3D luminosas
local hl1 = Engine.createSphere("HeadlightL", 1.7, 0.9, 0.5, 0.2)
hl1:setColor(1.0, 1.0, 0.8)
hl1:setGroup("Auto")
local hl2 = Engine.createSphere("HeadlightR", 1.7, 0.9, -0.5, 0.2)
hl2:setColor(1.0, 1.0, 0.8)
hl2:setGroup("Auto")

function onUpdate(dt)
    w1:rotate(0, 0, 150 * dt)
    w2:rotate(0, 0, 150 * dt)
    w3:rotate(0, 0, 150 * dt)
    w4:rotate(0, 0, 150 * dt)
end
Engine.log("Auto deportivo multi-primitivas con ruedas cilindricas y faros esfericos.")"""

        # 2. Creeper de Minecraft
        if "creeper" in p:
            return """-- Creeper de Minecraft Voxel Art
-- Torso
local body = Engine.createCube("CreeperBody", 0, 2.8, 0, 1.0)
body:setScale(1.2, 2.0, 0.8)
body:setColor(0.2, 0.75, 0.2) -- Verde creeper
body:setGroup("Creeper")

-- Cabeza
local head = Engine.createCube("CreeperHead", 0, 4.4, 0, 1.4)
head:setColor(0.25, 0.82, 0.25)
head:setGroup("Creeper")

-- Ojos negros
local eyeL = Engine.createCube("EyeL", 0.4, 4.6, 0.75, 0.3)
eyeL:setColor(0.05, 0.05, 0.05)
eyeL:setGroup("Creeper")
local eyeR = Engine.createCube("EyeR", -0.4, 4.6, 0.75, 0.3)
eyeR:setColor(0.05, 0.05, 0.05)
eyeR:setGroup("Creeper")

-- Boca / ceño
local mouth = Engine.createCube("Mouth", 0, 4.1, 0.75, 1.0)
mouth:setScale(0.5, 0.5, 0.2)
mouth:setColor(0.05, 0.05, 0.05)
mouth:setGroup("Creeper")

-- 4 Patas
local legFL = Engine.createCube("LegFL", 0.4, 0.9, 0.5, 1.0)
legFL:setScale(0.6, 1.8, 0.6)
legFL:setColor(0.18, 0.65, 0.18)
legFL:setGroup("Creeper")

local legFR = Engine.createCube("LegFR", -0.4, 0.9, 0.5, 1.0)
legFR:setScale(0.6, 1.8, 0.6)
legFR:setColor(0.18, 0.65, 0.18)
legFR:setGroup("Creeper")

local legBL = Engine.createCube("LegBL", 0.4, 0.9, -0.5, 1.0)
legBL:setScale(0.6, 1.8, 0.6)
legBL:setColor(0.18, 0.65, 0.18)
legBL:setGroup("Creeper")

local legBR = Engine.createCube("LegBR", -0.4, 0.9, -0.5, 1.0)
legBR:setScale(0.6, 1.8, 0.6)
legBR:setColor(0.18, 0.65, 0.18)
legBR:setGroup("Creeper")

function onUpdate(dt)
    local t = Engine.getTime()
    local swing = math.sin(t * 3.0) * 15.0
    legFL:setRotation(swing, 0, 0)
    legFR:setRotation(-swing, 0, 0)
    legBL:setRotation(-swing, 0, 0)
    legBR:setRotation(swing, 0, 0)
end
Engine.log("Creeper de Minecraft renderizado con exito en grupo 'Creeper'.")"""

        # 3. Castillo Medieval (con torres cilindricas y tejados conicos)
        if "castillo" in p or "torre" in p or "fortaleza" in p:
            return """-- Castillo Medieval con Torres Cilindricas y Tejados Conicos
-- Murallas
local wallF = Engine.createCube("WallF", 0, 1.0, 3.0, 1.0)
wallF:setScale(5.0, 2.0, 0.6)
wallF:setColor(0.6, 0.6, 0.65)
wallF:setGroup("Castillo")

local wallB = Engine.createCube("WallB", 0, 1.0, -3.0, 1.0)
wallB:setScale(5.0, 2.0, 0.6)
wallB:setColor(0.6, 0.6, 0.65)
wallB:setGroup("Castillo")

local wallL = Engine.createCube("WallL", -3.0, 1.0, 0, 1.0)
wallL:setScale(0.6, 2.0, 5.0)
wallL:setColor(0.6, 0.6, 0.65)
wallL:setGroup("Castillo")

local wallR = Engine.createCube("WallR", 3.0, 1.0, 0, 1.0)
wallR:setScale(0.6, 2.0, 5.0)
wallR:setColor(0.6, 0.6, 0.65)
wallR:setGroup("Castillo")

-- 4 Torres de esquina usando CILINDROS y tejados usando CONOS
local function makeTower(name, x, z)
    local t = Engine.createCylinder(name, x, 2.0, z, 0.8, 4.0)
    t:setColor(0.52, 0.52, 0.58)
    t:setGroup("Castillo")
    local roof = Engine.createCone(name .. "_Roof", x, 4.8, z, 1.0, 1.6)
    roof:setColor(0.85, 0.2, 0.2) -- Techo conico rojo
    roof:setGroup("Castillo")
end
makeTower("T_FL", 3.0, 3.0)
makeTower("T_FR", -3.0, 3.0)
makeTower("T_BL", 3.0, -3.0)
makeTower("T_BR", -3.0, -3.0)

-- Torre central (Keep)
local keep = Engine.createCube("Keep", 0, 3.0, 0, 1.0)
keep:setScale(2.5, 6.0, 2.5)
keep:setColor(0.55, 0.55, 0.6)
keep:setGroup("Castillo")

local keepRoof = Engine.createCone("KeepRoof", 0, 7.0, 0, 1.8, 2.2)
keepRoof:setColor(0.9, 0.18, 0.18)
keepRoof:setGroup("Castillo")

-- Puerta
local gate = Engine.createCube("Gate", 0, 0.8, 3.1, 1.0)
gate:setScale(1.2, 1.6, 0.2)
gate:setColor(0.25, 0.2, 0.15)
gate:setGroup("Castillo")
Engine.log("Castillo medieval generado con torres cilindricas y tejados conicos.")"""

        # 4. Barco Pirata
        if "barco" in p or "navio" in p or "pirata" in p:
            return """-- Barco Pirata con Velas y Mástiles
-- Casco
local hull = Engine.createCube("Hull", 0, 0.8, 0, 1.0)
hull:setScale(7.0, 1.5, 2.6)
hull:setColor(0.4, 0.25, 0.15) -- Madera oscura
hull:setGroup("BarcoPirata")

local proa = Engine.createCube("Proa", 4.0, 1.2, 0, 1.0)
proa:setScale(1.5, 1.8, 1.8)
proa:setColor(0.45, 0.28, 0.16)
proa:setGroup("BarcoPirata")

-- Cubierta
local deck = Engine.createCube("Deck", 0, 1.6, 0, 1.0)
deck:setScale(5.8, 0.3, 2.4)
deck:setColor(0.65, 0.5, 0.35)
deck:setGroup("BarcoPirata")

-- Mástil principal cilindrico
local mast = Engine.createCylinder("Mast", 0, 4.0, 0, 0.18, 5.0)
mast:setColor(0.35, 0.2, 0.1)
mast:setGroup("BarcoPirata")

-- Velas blancas
local sail1 = Engine.createCube("Sail1", 0, 4.5, 0, 1.0)
sail1:setScale(0.1, 2.5, 3.5)
sail1:setColor(0.95, 0.95, 0.92)
sail1:setGroup("BarcoPirata")

-- Bandera pirata negra en la punta
local flag = Engine.createCube("Flag", 0, 6.4, 0.4, 1.0)
flag:setScale(0.1, 0.5, 0.8)
flag:setColor(0.1, 0.1, 0.1)
flag:setGroup("BarcoPirata")

function onUpdate(dt)
    local t = Engine.getTime()
    local roll = math.sin(t * 1.5) * 4.0
    local pitch = math.cos(t * 1.2) * 2.0
    hull:setRotation(pitch, 0, roll)
    deck:setRotation(pitch, 0, roll)
    mast:setRotation(pitch, 0, roll)
    sail1:setRotation(pitch, 0, roll)
    flag:setRotation(pitch, 0, roll)
end
Engine.log("Barco pirata renderizado con balanceo marino en grupo 'BarcoPirata'.")"""

        # 5. Pastel de Cumpleaños (con capas cilindricas y cerezas esfericas)
        if "pastel" in p or "torta" in p or "cake" in p:
            return """-- Pastel de Cumpleaños con Capas Cilindricas
-- Plato
local plate = Engine.createCylinder("Plate", 0, 0.15, 0, 2.6, 0.3)
plate:setColor(0.92, 0.92, 0.95)
plate:setGroup("Pastel")

-- Capa 1: Chocolate (Cilindro)
local layer1 = Engine.createCylinder("Layer1", 0, 0.8, 0, 2.1, 1.2)
layer1:setColor(0.35, 0.2, 0.12) -- Chocolate
layer1:setGroup("Pastel")

-- Capa 2: Fresa (Cilindro)
local layer2 = Engine.createCylinder("Layer2", 0, 1.8, 0, 1.5, 1.0)
layer2:setColor(0.95, 0.4, 0.6) -- Rosa fresa
layer2:setGroup("Pastel")

-- Capa 3: Vainilla (Cilindro)
local layer3 = Engine.createCylinder("Layer3", 0, 2.6, 0, 0.9, 0.7)
layer3:setColor(1.0, 0.95, 0.8) -- Crema
layer3:setGroup("Pastel")

-- Cerezas esfericas
local c1 = Engine.createSphere("Cherry1", 0.5, 3.1, 0, 0.18)
c1:setColor(0.9, 0.05, 0.1)
c1:setGroup("Pastel")
local c2 = Engine.createSphere("Cherry2", -0.5, 3.1, 0, 0.18)
c2:setColor(0.9, 0.05, 0.1)
c2:setGroup("Pastel")

-- Vela cilindrica y llama conica
local candle = Engine.createCylinder("Candle", 0, 3.3, 0, 0.08, 0.7)
candle:setColor(0.3, 0.7, 0.9)
candle:setGroup("Pastel")
local flame = Engine.createCone("Flame", 0, 3.8, 0, 0.12, 0.3)
flame:setColor(1.0, 0.7, 0.1)
flame:setGroup("Pastel")

function onUpdate(dt)
    local t = Engine.getTime()
    local pulse = 0.12 + math.sin(t * 8.0) * 0.03
    flame:setScale(pulse, pulse * 1.5, pulse)
end
Engine.log("Pastel de cumpleanos con capas cilindricas y cerezas esfericas.")"""

        # 6. Dinosaurio / T-Rex
        if "dino" in p or "trex" in p or "t-rex" in p:
            return """-- T-Rex Dinosaurio con Ojos Esfericos
-- Torso
local body = Engine.createCube("Body", 0, 2.5, 0, 1.0)
body:setScale(1.6, 2.0, 3.0)
body:setColor(0.2, 0.6, 0.25)
body:setGroup("Dinosaurio")

-- Cuello y Cabeza
local neck = Engine.createCube("Neck", 0, 3.8, 1.2, 1.0)
neck:setScale(1.2, 1.5, 1.2)
neck:setColor(0.22, 0.62, 0.26)
neck:setGroup("Dinosaurio")

local head = Engine.createCube("Head", 0, 4.4, 2.0, 1.0)
head:setScale(1.4, 1.3, 2.2)
head:setColor(0.25, 0.68, 0.3)
head:setGroup("Dinosaurio")

local jaw = Engine.createCube("Jaw", 0, 3.8, 2.1, 1.0)
jaw:setScale(1.2, 0.4, 1.8)
jaw:setColor(0.85, 0.3, 0.3) -- Boca roja
jaw:setGroup("Dinosaurio")

-- Ojos esféricos amarillos
local eyeL = Engine.createSphere("EyeL", 0.75, 4.7, 1.8, 0.25)
eyeL:setColor(0.95, 0.85, 0.1)
eyeL:setGroup("Dinosaurio")
local eyeR = Engine.createSphere("EyeR", -0.75, 4.7, 1.8, 0.25)
eyeR:setColor(0.95, 0.85, 0.1)
eyeR:setGroup("Dinosaurio")

-- Cola
local tail1 = Engine.createCube("Tail1", 0, 2.2, -1.8, 1.0)
tail1:setScale(1.1, 1.1, 1.8)
tail1:setColor(0.2, 0.58, 0.24)
tail1:setGroup("Dinosaurio")

local tail2 = Engine.createCube("Tail2", 0, 2.5, -3.0, 1.0)
tail2:setScale(0.7, 0.7, 1.6)
tail2:setColor(0.18, 0.55, 0.22)
tail2:setGroup("Dinosaurio")

-- Patas
local legL = Engine.createCube("LegL", 1.0, 1.2, 0, 1.0)
legL:setScale(0.6, 2.4, 0.9)
legL:setColor(0.18, 0.55, 0.22)
legL:setGroup("Dinosaurio")

local legR = Engine.createCube("LegR", -1.0, 1.2, 0, 1.0)
legR:setScale(0.6, 2.4, 0.9)
legR:setColor(0.18, 0.55, 0.22)
legR:setGroup("Dinosaurio")

function onUpdate(dt)
    local t = Engine.getTime()
    local sway = math.sin(t * 3.0) * 8.0
    tail2:setRotation(0, sway * 2.0, 0)
    jaw:setRotation(math.abs(math.sin(t * 2.0)) * 12.0, 0, 0)
end
Engine.log("Dinosaurio T-Rex renderizado con exito en grupo 'Dinosaurio'.")"""

        # Respaldo genérico procedural artístico (Escultura moderna con Toros, Esferas y Cilindros)
        return f"""-- Escultura Multi-Primitivas para: '{prompt}'
local base = Engine.createCylinder("BasePedestal", 0, 0.3, 0, 1.8, 0.6)
base:setColor(0.2, 0.22, 0.28)
base:setGroup("Escultura")

local ring = Engine.createTorus("Sculpt_Ring", 0, 2.0, 0, 1.2, 0.25)
ring:setColor(0.9, 0.6, 0.15)
ring:setGroup("Escultura")

local orb = Engine.createSphere("Sculpt_Orb", 0, 2.0, 0, 0.65)
orb:setColor(0.2, 0.75, 0.95)
orb:setGroup("Escultura")

local crown = Engine.createCone("Sculpt_Crown", 0, 3.4, 0, 0.6, 1.2)
crown:setColor(0.85, 0.2, 0.3)
crown:setGroup("Escultura")

function onUpdate(dt)
    local t = Engine.getTime()
    ring:rotate(30.0 * dt, 45.0 * dt, 0)
    orb:setPosition(0, 2.0 + math.sin(t * 2.5) * 0.3, 0)
end
Engine.log("Escultura moderna generada con Toro, Esfera, Cono y Cilindro.")"""
