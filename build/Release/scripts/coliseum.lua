-- ==============================================================
-- Coliseo Romano Tridimensional Voxel Art
-- ==============================================================
Engine.clearScene()

-- 1. Suelo de Arena Central
local arena = Engine.createCube("Arena_Floor", 0.0, 0.05, 0.0, 1.0)
if arena then
    arena:setScale(7.0, 0.1, 7.0)
    arena:setColor(0.85, 0.75, 0.52) -- Color arena dorada
end

-- 2. Anillo de Gradas Interiores (Cavea)
local innerRadius = 4.2
local numInner = 20
for i = 1, numInner do
    local angle = (i - 1) * (2 * math.pi / numInner)
    local x = math.cos(angle) * innerRadius
    local z = math.sin(angle) * innerRadius

    local seat = Engine.createCube("Seat_" .. i, x, 0.4, z, 0.8)
    if seat then
        seat:setRotation(0.0, -glm_degrees or math.deg(angle), 0.0)
        seat:setColor(0.72, 0.68, 0.62)
    end
end

-- 3. Muro Exterior y Columnas Arqueadas del Coliseo
local outerRadius = 5.8
local numPillars = 24
for i = 1, numPillars do
    local angle = (i - 1) * (2 * math.pi / numPillars)
    local x = math.cos(angle) * outerRadius
    local z = math.sin(angle) * outerRadius

    -- Dejar una entrada principal abierta (omitiendo 2 columnas)
    if i ~= 1 and i ~= 2 then
        -- Columna primer nivel
        local col1 = Engine.createCube("Col1_" .. i, x, 1.2, z, 0.8)
        if col1 then
            col1:setScale(0.7, 2.2, 0.7)
            col1:setColor(0.78, 0.74, 0.68) -- Mármol travertino romano
        end

        -- Columna segundo nivel (arcos superiores)
        local col2 = Engine.createCube("Col2_" .. i, x, 3.0, z, 0.8)
        if col2 then
            col2:setScale(0.65, 1.6, 0.65)
            col2:setColor(0.75, 0.70, 0.65)
        end

        -- Cornisa superior conectora
        local arch = Engine.createCube("Arch_" .. i, x, 3.9, z, 0.8)
        if arch then
            arch:setScale(1.1, 0.35, 0.7)
            arch:setColor(0.82, 0.78, 0.72)
        end
    end
end

-- 4. Banderas decorativas en la cúspide
for i = 1, 4 do
    local angle = (i - 1) * (math.pi / 2) + 0.3
    local x = math.cos(angle) * outerRadius
    local z = math.sin(angle) * outerRadius
    local flag = Engine.createCube("Flag_" .. i, x, 4.4, z, 0.4)
    if flag then
        flag:setScale(0.15, 0.7, 0.35)
        flag:setColor(0.8, 0.15, 0.15) -- Estandartes imperiales rojos
    end
end

Engine.log("¡Coliseo Romano monumental generado con exito!")
