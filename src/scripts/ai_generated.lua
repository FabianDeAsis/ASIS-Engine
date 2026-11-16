-- Creación de un Creeper de Minecraft
local grupo = "Creeper"

-- Cuerpo (Torso)
local torso = Engine.createCube("CreeperTorso", 0, 2.5, 0, 1.2)
torso:setColor(0.3, 0.7, 0.2)
torso:setGroup(grupo)

-- Cabeza
local cabeza = Engine.createCube("CreeperCabeza", 0, 4.0, 0, 1.0)
cabeza:setColor(0.4, 0.8, 0.3)
cabeza:setGroup(grupo)

-- Ojos y boca (Detalles negros)
local ojo1 = Engine.createCube("OjoIzq", -0.25, 4.2, 0.5, 0.2)
ojo1:setColor(0, 0, 0)
ojo1:setGroup(grupo)

local ojo2 = Engine.createCube("OjoDer", 0.25, 4.2, 0.5, 0.2)
ojo2:setColor(0, 0, 0)
ojo2:setGroup(grupo)

local boca = Engine.createCube("Boca", 0, 3.8, 0.5, 0.3)
boca:setColor(0, 0, 0)
boca:setGroup(grupo)

-- Patas
local pata1 = Engine.createCube("Pata1", -0.4, 1.0, 0.4, 0.6)
pata1:setColor(0.2, 0.5, 0.1)
pata1:setGroup(grupo)

local pata2 = Engine.createCube("Pata2", 0.4, 1.0, 0.4, 0.6)
pata2:setColor(0.2, 0.5, 0.1)
pata2:setGroup(grupo)

local pata3 = Engine.createCube("Pata3", -0.4, 1.0, -0.4, 0.6)
pata3:setColor(0.2, 0.5, 0.1)
pata3:setGroup(grupo)

local pata4 = Engine.createCube("Pata4", 0.4, 1.0, -0.4, 0.6)
pata4:setColor(0.2, 0.5, 0.1)
pata4:setGroup(grupo)

Engine.log("Creeper generado con éxito.")