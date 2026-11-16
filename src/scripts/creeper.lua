-- ==============================================================
-- Minecraft Creeper Voxel Art generado por Antigravity
-- ==============================================================
Engine.clearScene()

-- 1. Cabeza del Creeper (Cubo Verde brillante en la parte superior)
local head = Engine.createCube("Creeper_Head", 0.0, 3.8, 0.0, 1.4)
if head then
    head:setColor(0.2, 0.75, 0.25)
end

-- Detalles de la cara (Ojos negros y boca característica del Creeper)
local eyeL = Engine.createCube("Creeper_EyeL", -0.32, 4.0, 0.72, 0.28)
if eyeL then eyeL:setColor(0.05, 0.05, 0.05) end

local eyeR = Engine.createCube("Creeper_EyeR",  0.32, 4.0, 0.72, 0.28)
if eyeR then eyeR:setColor(0.05, 0.05, 0.05) end

-- Boca / Ceño del Creeper
local mouthCenter = Engine.createCube("Creeper_MouthCenter", 0.0, 3.65, 0.72, 0.28)
if mouthCenter then mouthCenter:setColor(0.05, 0.05, 0.05) end

local mouthTop = Engine.createCube("Creeper_MouthTop", 0.0, 3.82, 0.72, 0.18)
if mouthTop then mouthTop:setColor(0.05, 0.05, 0.05) end

local mouthLL = Engine.createCube("Creeper_MouthLL", -0.22, 3.5, 0.72, 0.25)
if mouthLL then mouthLL:setColor(0.05, 0.05, 0.05) end

local mouthRR = Engine.createCube("Creeper_MouthRR",  0.22, 3.5, 0.72, 0.25)
if mouthRR then mouthRR:setColor(0.05, 0.05, 0.05) end

-- 2. Torso (Cuerpo alargado)
local torso = Engine.createCube("Creeper_Torso", 0.0, 2.2, 0.0, 1.0)
if torso then
    torso:setScale(1.1, 1.8, 0.75)
    torso:setColor(0.18, 0.68, 0.22)
end

-- 3. Cuatro Patas características
local legFL = Engine.createCube("Leg_FrontLeft",  -0.38, 0.65,  0.42, 0.65)
local legFR = Engine.createCube("Leg_FrontRight",  0.38, 0.65,  0.42, 0.65)
local legBL = Engine.createCube("Leg_BackLeft",   -0.38, 0.65, -0.42, 0.65)
local legBR = Engine.createCube("Leg_BackRight",   0.38, 0.65, -0.42, 0.65)

local legs = {legFL, legFR, legBL, legBR}
for _, leg in ipairs(legs) do
    if leg then
        leg:setScale(0.65, 1.1, 0.65)
        leg:setColor(0.15, 0.62, 0.2)
    end
end

-- 4. Animación de Caminata y Cabeceo del Creeper
function onUpdate(dt)
    local t = Engine.getTime()
    local swingAngle = math.sin(t * 5.0) * 18.0

    -- Patas oscilando en diagonal (como camina un Creeper)
    local fl = Engine.getEntity("Leg_FrontLeft")
    local fr = Engine.getEntity("Leg_FrontRight")
    local bl = Engine.getEntity("Leg_BackLeft")
    local br = Engine.getEntity("Leg_BackRight")

    if fl then fl:setRotation( swingAngle, 0, 0) end
    if fr then fr:setRotation(-swingAngle, 0, 0) end
    if bl then bl:setRotation(-swingAngle, 0, 0) end
    if br then br:setRotation( swingAngle, 0, 0) end

    -- Ligero cabeceo curioso
    local head = Engine.getEntity("Creeper_Head")
    if head then
        head:setRotation(math.sin(t * 2.0) * 5.0, math.sin(t * 1.2) * 10.0, math.cos(t * 1.5) * 4.0)
    end
end

Engine.log("¡Creeper de Minecraft generado con exito!")
