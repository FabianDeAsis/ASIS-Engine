-- ==============================================================
-- ASIS Engine - Script Principal en Lua (Hot-Reload Habilitado)
-- Fase 17: Prefabs, Spawning Dinámico y Gameplay Triggers
-- ==============================================================

function onInit()
    Engine.log("Inicializando escena desde Lua...")

    -- 1. Limpiar cualquier entidad previa
    Engine.clearScene()

    -- 2. Crear un Cubo Principal en el centro de la escena
    local center = Engine.createCube("CenterCube", 0.0, 1.5, 0.0, 1.3)
    if center then
        center:setColor(0.9, 0.25, 0.3)
    end

    -- 3. Crear Satélites que orbitarán alrededor del centro
    local sat1 = Engine.createCube("Sat1", 3.0, 1.0, 0.0, 0.6)
    if sat1 then
        sat1:setColor(0.2, 0.85, 0.3)
    end

    local sat2 = Engine.createCube("Sat2", -3.0, 1.0, 0.0, 0.6)
    if sat2 then
        sat2:setColor(0.25, 0.55, 0.95)
    end

    local sat3 = Engine.createCube("Sat3", 0.0, 3.2, 0.0, 0.5)
    if sat3 then
        sat3:setColor(0.95, 0.85, 0.15)
    end

    -- 4. Instanciar Prefabs Dinámicos en la escena (Fase 17)
    Engine.log("Instanciando Prefabs de juego desde Lua...")
    Engine.spawnPrefab("Moneda_Dorada", 2.5, 0.8, 2.0)
    Engine.spawnPrefab("Moneda_Dorada", -2.5, 0.8, 2.0)
    Engine.spawnPrefab("Caja_Madera_Fisica", -2.0, 3.0, -2.0)
    Engine.spawnPrefab("Portal_Teletransporte", 0.0, 0.0, -4.5)

    -- 5. Crear modelo 3D cargado con Assimp a un costado
    local objModel = Engine.createModel("MainModel", "models/cube.obj", 4.0, 1.0, -2.0)
    if objModel then
        objModel:setScale(0.8, 0.8, 0.8)
    end

    Engine.log("¡Escena generada con éxito desde Lua con Prefabs y Triggers activos!")
end

function onUpdate(dt)
    local t = Engine.getTime()

    -- 1. Rotación continua del cubo central en varios ejes
    local center = Engine.getEntity("CenterCube")
    if center then
        center:rotate(25.0 * dt, 40.0 * dt, 15.0 * dt)
    end

    -- 2. Órbita circular del Satélite 1 (verde)
    local sat1 = Engine.getEntity("Sat1")
    if sat1 then
        local x = math.cos(t * 1.8) * 3.2
        local z = math.sin(t * 1.8) * 3.2
        local y = 1.2 + math.sin(t * 3.5) * 0.4
        sat1:setPosition(x, y, z)
        sat1:rotate(60.0 * dt, 60.0 * dt, 0.0)
    end

    -- 3. Órbita opuesta del Satélite 2 (azul)
    local sat2 = Engine.getEntity("Sat2")
    if sat2 then
        local x = math.cos(t * 1.8 + math.pi) * 3.2
        local z = math.sin(t * 1.8 + math.pi) * 3.2
        local y = 1.2 + math.cos(t * 3.5) * 0.4
        sat2:setPosition(x, y, z)
        sat2:rotate(0.0, 80.0 * dt, 40.0 * dt)
    end

    -- 4. Movimiento vertical ondulante del Satélite 3 (amarillo)
    local sat3 = Engine.getEntity("Sat3")
    if sat3 then
        local y = 2.8 + math.sin(t * 2.5) * 0.7
        sat3:setPosition(0.0, y, 0.0)
        sat3:rotate(100.0 * dt, 0.0, 100.0 * dt)
    end

    -- 5. Animación del modelo Assimp
    local objModel = Engine.getEntity("MainModel")
    if objModel then
        objModel:rotate(0.0, 30.0 * dt, 0.0)
    end

    -- 6. Rotación elegante de las monedas coleccionables
    local m1 = Engine.getEntity("Moneda_1")
    if m1 then m1:rotate(0.0, 130.0 * dt, 0.0) end
    local m2 = Engine.getEntity("Moneda_2")
    if m2 then m2:rotate(0.0, 130.0 * dt, 0.0) end
end

-- ==============================================================
-- Callbacks de Gameplay Triggers y Colisiones (Fase 17)
-- ==============================================================

function onTriggerEnter(triggerName, otherName)
    Engine.log("⚡ [TRIGGER ACTIVADO] '" .. triggerName .. "' detectó ingreso de '" .. otherName .. "'")

    if string.find(triggerName, "Moneda") or string.find(triggerName, "Coin") then
        Engine.log("💰 ¡Moneda Dorada Recogida por " .. otherName .. "! (+100 Puntos)")
        Engine.playJump(0.8)
        Engine.destroyEntity(triggerName)
    elseif string.find(triggerName, "Portal") then
        Engine.log("🌀 ¡Entraste en el Portal de Teletransporte!")
        Engine.playImpact(0.0, 1.2, -4.5, 7.5)
        -- Teletransportar al jugador a una ubicación épica elevada en el mapa
        Engine.teleportPlayer(0.0, 10.0, 12.0)
    elseif string.find(triggerName, "Barril") or string.find(triggerName, "Explosivo") then
        Engine.log("💥 ¡BUM! ¡El barril explosivo detonó por impacto!")
        local pos = Engine.getEntityPosition(triggerName)
        Engine.explode(triggerName, 12.0, 45.0)
        Engine.destroyEntity(triggerName)
    end
end

function onTriggerExit(triggerName, otherName)
    Engine.log("🚪 [TRIGGER SALIDA] '" .. otherName .. "' salió de la zona '" .. triggerName .. "'")
end
