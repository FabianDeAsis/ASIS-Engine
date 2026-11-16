-- ==============================================================
-- Pastel de Cumpleaños Tridimensional Voxel Art
-- ==============================================================
Engine.clearScene()

-- 1. Base del Plato
local plate = Engine.createCube("Cake_Plate", 0.0, 0.1, 0.0, 5.0)
if plate then
    plate:setScale(5.0, 0.2, 5.0)
    plate:setColor(0.9, 0.92, 0.95)
end

-- 2. Capa Inferior del Pastel (Bizcocho de Chocolate)
local layersBase = 6
for x = -2, 2 do
    for z = -2, 2 do
        if math.abs(x) + math.abs(z) <= 3 then -- Forma octogonal redondeada
            local c = Engine.createCube(string.format("Cake_B1_%d_%d", x, z), x * 0.95, 0.7, z * 0.95, 0.95)
            if c then c:setColor(0.45, 0.25, 0.12) end -- Marrón chocolate
        end
    end
end

-- 3. Capa de Relleno de Fresa
for x = -2, 2 do
    for z = -2, 2 do
        if math.abs(x) + math.abs(z) <= 3 then
            local c = Engine.createCube(string.format("Cake_Cream_%d_%d", x, z), x * 0.95, 1.3, z * 0.95, 0.95)
            if c then c:setColor(0.95, 0.4, 0.55) end -- Rosa fresa
        end
    end
end

-- 4. Capa Superior (Cobertura de Vainilla y Glaseado)
for x = -1, 1 do
    for z = -1, 1 do
        local c = Engine.createCube(string.format("Cake_Top_%d_%d", x, z), x * 0.95, 1.9, z * 0.95, 0.95)
        if c then c:setColor(0.98, 0.95, 0.88) end -- Crema blanco vainilla
    end
end

-- 5. Velas de Colores en la parte superior
local candle1 = Engine.createCube("Candle_1", -0.5, 2.7, -0.5, 0.25)
if candle1 then candle1:setScale(0.25, 0.8, 0.25); candle1:setColor(0.2, 0.6, 0.9) end

local candle2 = Engine.createCube("Candle_2",  0.5, 2.7, -0.5, 0.25)
if candle2 then candle2:setScale(0.25, 0.8, 0.25); candle2:setColor(0.9, 0.2, 0.3) end

local candle3 = Engine.createCube("Candle_3",  0.0, 2.7,  0.5, 0.25)
if candle3 then candle3:setScale(0.25, 0.8, 0.25); candle3:setColor(0.3, 0.8, 0.3) end

-- Llamas de las velas (cubitos amarillos/naranjas con animación de parpadeo)
local flame1 = Engine.createCube("Flame_1", -0.5, 3.25, -0.5, 0.18)
local flame2 = Engine.createCube("Flame_2",  0.5, 3.25, -0.5, 0.18)
local flame3 = Engine.createCube("Flame_3",  0.0, 3.25,  0.5, 0.18)

local flames = {flame1, flame2, flame3}
for _, f in ipairs(flames) do
    if f then f:setColor(1.0, 0.85, 0.1) end
end

-- Animación de parpadeo de las llamas
function onUpdate(dt)
    local t = Engine.getTime()
    for i, name in ipairs({"Flame_1", "Flame_2", "Flame_3"}) do
        local f = Engine.getEntity(name)
        if f then
            local s = 0.16 + math.sin(t * 12.0 + i * 2.0) * 0.05
            f:setScale(s, s * 1.5, s)
            f:setColor(1.0, 0.6 + math.sin(t * 8.0 + i) * 0.3, 0.1)
        end
    end
end

Engine.log("¡Pastel de cumpleaños con velas parpadeantes generado!")
