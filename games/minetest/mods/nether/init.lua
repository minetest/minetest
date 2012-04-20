-- Nether Mod (based on Nyanland by Jeija, Catapult by XYZ, and Livehouse by neko259)
-- lkjoel (main developer, code, ideas, textures)
-- == CONTRIBUTERS ==
-- jordan4ibanez (code, ideas, textures)
-- Gilli (code, ideas, textures, mainly for the Glowstone)
-- Death Dealer (code, ideas, textures)
-- LolManKuba (ideas, textures)
-- IPushButton2653 (ideas, textures)
-- Menche (textures)
-- sdzen (ideas)
-- godkiller447 (ideas)
-- If I didn't list you, please let me know!

--== EDITABLE OPTIONS ==--

-- Depth of the nether
NETHER_DEPTH = -20000
-- Height of the nether (bottom of the nether is NETHER_DEPTH - NETHER_HEIGHT)
NETHER_HEIGHT = 30
-- Maximum amount of randomness in the map generation
NETHER_RANDOM = 2
-- Frequency of Glowstone on the "roof" of the Nether (higher is less frequent)
GLOWSTONE_FREQ_ROOF = 500
-- Frequency of Glowstone on lava (higher is less frequent)
GLOWSTONE_FREQ_LAVA = 2
-- Frequency of lava (higher is less frequent)
LAVA_FREQ = 100
-- Maximum height of lava
LAVA_HEIGHT = 2
-- Frequency of nether trees (higher is less frequent)
NETHER_TREE_FREQ = 350
-- Height of nether trees
NETHER_TREESIZE = 2
-- Frequency of apples in a nether tree (higher is less frequent)
NETHER_APPLE_FREQ = 5
-- Frequency of healing apples in a nether tree (higher is less frequent)
NETHER_HEAL_APPLE_FREQ = 10
-- Start position for the Throne of Hades (y is relative to the bottom of the nether)
HADES_THRONE_STARTPOS = {x=0, y=1, z=0}
-- Spawn pos for when the nether hasn't been loaded yet (i.e. no portal in the nether) (y is relative to the bottom of the nether)
NETHER_SPAWNPOS = {x=0, y=5, z=0}
-- Throne of Hades
HADES_THRONE = {
	-- Lava Moat
	{pos={x=-1,y=-1,z=-1}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=0}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=1}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=2}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=3}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=4}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=5}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=6}, block="nether:lava_source"},
	{pos={x=-1,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=0,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=1,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=2,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=3,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=4,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=5,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=7}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=6}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=5}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=4}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=3}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=2}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=1}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=0}, block="nether:lava_source"},
	{pos={x=6,y=-1,z=-1}, block="nether:lava_source"},
	{pos={x=5,y=-1,z=-1}, block="nether:lava_source"},
	{pos={x=4,y=-1,z=-1}, block="nether:lava_source"},
	{pos={x=3,y=-1,z=-1}, block="nether:lava_source"},
	{pos={x=2,y=-1,z=-1}, block="nether:lava_source"},
	{pos={x=1,y=-1,z=-1}, block="nether:lava_source"},
	{pos={x=0,y=-1,z=-1}, block="nether:lava_source"},
	-- Floor 1
	{pos={x=0,y=0,z=0}, block="nether:netherrack"},
	{pos={x=0,y=0,z=1}, block="nether:netherrack"},
	{pos={x=0,y=0,z=2}, block="nether:netherrack"},
	{pos={x=0,y=0,z=3}, block="nether:netherrack"},
	{pos={x=0,y=0,z=4}, block="nether:netherrack"},
	{pos={x=0,y=0,z=5}, block="nether:netherrack"},
	{pos={x=1,y=0,z=5}, block="nether:netherrack"},
	{pos={x=2,y=0,z=5}, block="nether:netherrack"},
	{pos={x=3,y=0,z=5}, block="nether:netherrack"},
	{pos={x=4,y=0,z=5}, block="nether:netherrack"},
	{pos={x=5,y=0,z=5}, block="nether:netherrack"},
	{pos={x=0,y=0,z=6}, block="nether:netherrack"},
	{pos={x=1,y=0,z=6}, block="nether:netherrack"},
	{pos={x=2,y=0,z=6}, block="nether:netherrack"},
	{pos={x=3,y=0,z=6}, block="nether:netherrack"},
	{pos={x=4,y=0,z=6}, block="nether:netherrack"},
	{pos={x=5,y=0,z=6}, block="nether:netherrack"},
	{pos={x=5,y=0,z=4}, block="nether:netherrack"},
	{pos={x=5,y=0,z=3}, block="nether:netherrack"},
	{pos={x=5,y=0,z=2}, block="nether:netherrack"},
	{pos={x=5,y=0,z=1}, block="nether:netherrack"},
	{pos={x=5,y=0,z=0}, block="nether:netherrack"},
	{pos={x=4,y=0,z=0}, block="nether:netherrack"},
	{pos={x=3,y=0,z=0}, block="nether:netherrack"},
	{pos={x=2,y=0,z=0}, block="nether:netherrack"},
	{pos={x=1,y=0,z=0}, block="nether:netherrack"},
	-- Floor 2
	{pos={x=0,y=1,z=0}, block="nether:netherrack"},
	{pos={x=0,y=1,z=1}, block="nether:netherrack"},
	{pos={x=0,y=1,z=2}, block="nether:netherrack"},
	{pos={x=0,y=1,z=3}, block="nether:netherrack"},
	{pos={x=0,y=1,z=4}, block="nether:netherrack"},
	{pos={x=0,y=1,z=5}, block="nether:netherrack"},
	{pos={x=1,y=1,z=5}, block="nether:netherrack"},
	{pos={x=2,y=1,z=5}, block="nether:netherrack"},
	{pos={x=3,y=1,z=5}, block="nether:netherrack"},
	{pos={x=4,y=1,z=5}, block="nether:netherrack"},
	{pos={x=5,y=1,z=5}, block="nether:netherrack"},
	{pos={x=0,y=1,z=6}, block="nether:netherrack"},
	{pos={x=1,y=1,z=6}, block="nether:netherrack"},
	{pos={x=2,y=1,z=6}, block="nether:netherrack"},
	{pos={x=3,y=1,z=6}, block="nether:netherrack"},
	{pos={x=4,y=1,z=6}, block="nether:netherrack"},
	{pos={x=5,y=1,z=6}, block="nether:netherrack"},
	{pos={x=5,y=1,z=4}, block="nether:netherrack"},
	{pos={x=5,y=1,z=3}, block="nether:netherrack"},
	{pos={x=5,y=1,z=2}, block="nether:netherrack"},
	{pos={x=5,y=1,z=1}, block="nether:netherrack"},
	{pos={x=5,y=1,z=0}, block="nether:netherrack"},
	{pos={x=4,y=1,z=0}, block="nether:netherrack"},
	{pos={x=3,y=1,z=1}, block="nether:netherrack"},
	{pos={x=2,y=1,z=1}, block="nether:netherrack"},
	{pos={x=1,y=1,z=0}, block="nether:netherrack"},
	{pos={x=1,y=1,z=1}, block="nether:netherrack"},
	{pos={x=4,y=1,z=1}, block="nether:netherrack"},
	-- Floor 3
	{pos={x=0,y=2,z=0}, block="nether:netherrack"},
	{pos={x=0,y=2,z=1}, block="nether:netherrack"},
	{pos={x=0,y=2,z=2}, block="nether:netherrack"},
	{pos={x=0,y=2,z=3}, block="nether:netherrack"},
	{pos={x=0,y=2,z=4}, block="nether:netherrack"},
	{pos={x=0,y=2,z=5}, block="nether:netherrack"},
	{pos={x=1,y=2,z=5}, block="nether:netherrack"},
	{pos={x=2,y=2,z=5}, block="nether:netherrack"},
	{pos={x=3,y=2,z=5}, block="nether:netherrack"},
	{pos={x=4,y=2,z=5}, block="nether:netherrack"},
	{pos={x=5,y=2,z=5}, block="nether:netherrack"},
	{pos={x=0,y=2,z=6}, block="nether:netherrack"},
	{pos={x=1,y=2,z=6}, block="nether:netherrack"},
	{pos={x=2,y=2,z=6}, block="nether:netherrack"},
	{pos={x=3,y=2,z=6}, block="nether:netherrack"},
	{pos={x=4,y=2,z=6}, block="nether:netherrack"},
	{pos={x=5,y=2,z=6}, block="nether:netherrack"},
	{pos={x=5,y=2,z=4}, block="nether:netherrack"},
	{pos={x=5,y=2,z=3}, block="nether:netherrack"},
	{pos={x=5,y=2,z=2}, block="nether:netherrack"},
	{pos={x=5,y=2,z=1}, block="nether:netherrack"},
	{pos={x=5,y=2,z=0}, block="nether:netherrack"},
	{pos={x=4,y=2,z=0}, block="nether:netherrack"},
	{pos={x=3,y=2,z=2}, block="nether:netherrack"},
	{pos={x=2,y=2,z=2}, block="nether:netherrack"},
	{pos={x=1,y=2,z=0}, block="nether:netherrack"},
	{pos={x=1,y=2,z=1}, block="nether:netherrack"},
	{pos={x=4,y=2,z=1}, block="nether:netherrack"},
	{pos={x=1,y=2,z=2}, block="nether:netherrack"},
	{pos={x=4,y=2,z=2}, block="nether:netherrack"},
	-- Floor 4
	{pos={x=0,y=3,z=0}, block="nether:netherrack"},
	{pos={x=0,y=3,z=1}, block="nether:netherrack"},
	{pos={x=0,y=3,z=2}, block="nether:netherrack"},
	{pos={x=0,y=3,z=3}, block="nether:netherrack"},
	{pos={x=0,y=3,z=4}, block="nether:netherrack"},
	{pos={x=0,y=3,z=5}, block="nether:netherrack"},
	{pos={x=1,y=3,z=5}, block="nether:netherrack"},
	{pos={x=2,y=3,z=5}, block="nether:netherrack"},
	{pos={x=3,y=3,z=5}, block="nether:netherrack"},
	{pos={x=4,y=3,z=5}, block="nether:netherrack"},
	{pos={x=5,y=3,z=5}, block="nether:netherrack"},
	{pos={x=0,y=3,z=6}, block="nether:netherrack"},
	{pos={x=1,y=3,z=6}, block="nether:netherrack"},
	{pos={x=2,y=3,z=6}, block="nether:netherrack"},
	{pos={x=3,y=3,z=6}, block="nether:netherrack"},
	{pos={x=4,y=3,z=6}, block="nether:netherrack"},
	{pos={x=5,y=3,z=6}, block="nether:netherrack"},
	{pos={x=5,y=3,z=4}, block="nether:netherrack"},
	{pos={x=5,y=3,z=3}, block="nether:netherrack"},
	{pos={x=5,y=3,z=2}, block="nether:netherrack"},
	{pos={x=5,y=3,z=1}, block="nether:netherrack"},
	{pos={x=5,y=3,z=0}, block="nether:netherrack"},
	{pos={x=4,y=3,z=0}, block="nether:netherrack"},
	{pos={x=3,y=3,z=3}, block="nether:netherrack"},
	{pos={x=2,y=3,z=3}, block="nether:netherrack"},
	{pos={x=1,y=3,z=0}, block="nether:netherrack"},
	{pos={x=1,y=3,z=1}, block="nether:netherrack"},
	{pos={x=4,y=3,z=1}, block="nether:netherrack"},
	{pos={x=1,y=3,z=2}, block="nether:netherrack"},
	{pos={x=4,y=3,z=2}, block="nether:netherrack"},
	{pos={x=1,y=3,z=3}, block="nether:netherrack"},
	{pos={x=4,y=3,z=3}, block="nether:netherrack"},
	{pos={x=1,y=3,z=4}, block="nether:netherrack"},
	{pos={x=4,y=3,z=4}, block="nether:netherrack"},
	{pos={x=2,y=3,z=4}, block="nether:netherrack"},
	{pos={x=2,y=3,z=5}, block="nether:netherrack"},
	{pos={x=3,y=3,z=4}, block="nether:netherrack"},
	{pos={x=3,y=3,z=5}, block="nether:netherrack"},
	-- Floor 5
	{pos={x=2,y=4,z=4}, block="nether:netherrack"},
	{pos={x=2,y=4,z=5}, block="nether:netherrack"},
	{pos={x=3,y=4,z=4}, block="nether:netherrack"},
	{pos={x=3,y=4,z=5}, block="nether:netherrack"},
	{pos={x=2,y=4,z=6}, block="nether:netherrack"},
	{pos={x=3,y=4,z=6}, block="nether:netherrack"},
	-- Torches on floor 5
	{pos={x=0,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=5}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=5}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=5}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=5}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=2}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=2}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=3}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=3}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=2}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=2}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=3}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=3}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=6}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=6}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=6}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=6}, block="nether:nether_torch_bottom"},
	-- Nether Portal
	{pos={x=1,y=5,z=6}, portalblock=true},
}
-- Structure of the nether portal (all is relative to the nether portal creator block)
NETHER_PORTAL = {
	-- Floor 1
	{pos={x=0,y=0,z=0}, block="obsidian:obsidian_block"},
	{pos={x=1,y=0,z=0}, block="obsidian:obsidian_block"},
	{pos={x=2,y=0,z=0}, block="obsidian:obsidian_block"},
	{pos={x=3,y=0,z=0}, block="obsidian:obsidian_block"},
	{pos={x=0,y=0,z=1}, block="obsidian:obsidian_block"},
	{pos={x=1,y=0,z=1}, block="obsidian:obsidian_block"},
	{pos={x=2,y=0,z=1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=0,z=1}, block="obsidian:obsidian_block"},
	{pos={x=0,y=0,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=1,y=0,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=2,y=0,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=0,z=-1}, block="obsidian:obsidian_block"},
	-- Floor 2
	{pos={x=0,y=1,z=0}, block="obsidian:obsidian_block"},
	{pos={x=1,y=1,z=0}, block="nether:nether_portal"},
	{pos={x=2,y=1,z=0}, block="nether:nether_portal"},
	{pos={x=3,y=1,z=0}, block="obsidian:obsidian_block"},
	{pos={x=0,y=1,z=1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=1,z=1}, block="obsidian:obsidian_block"},
	{pos={x=0,y=1,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=1,z=-1}, block="obsidian:obsidian_block"},
	-- Floor 3
	{pos={x=0,y=2,z=0}, block="obsidian:obsidian_block"},
	{pos={x=1,y=2,z=0}, block="nether:nether_portal"},
	{pos={x=2,y=2,z=0}, block="nether:nether_portal"},
	{pos={x=3,y=2,z=0}, block="obsidian:obsidian_block"},
	{pos={x=0,y=2,z=1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=2,z=1}, block="obsidian:obsidian_block"},
	{pos={x=0,y=2,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=2,z=-1}, block="obsidian:obsidian_block"},
	-- Floor 4
	{pos={x=0,y=3,z=0}, block="obsidian:obsidian_block"},
	{pos={x=1,y=3,z=0}, block="nether:nether_portal"},
	{pos={x=2,y=3,z=0}, block="nether:nether_portal"},
	{pos={x=3,y=3,z=0}, block="obsidian:obsidian_block"},
	{pos={x=0,y=3,z=1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=3,z=1}, block="obsidian:obsidian_block"},
	{pos={x=0,y=3,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=3,z=-1}, block="obsidian:obsidian_block"},
	-- Floor 5
	{pos={x=0,y=4,z=0}, block="obsidian:obsidian_block"},
	{pos={x=1,y=4,z=0}, block="obsidian:obsidian_block"},
	{pos={x=2,y=4,z=0}, block="obsidian:obsidian_block"},
	{pos={x=3,y=4,z=0}, block="obsidian:obsidian_block"},
	{pos={x=0,y=4,z=1}, block="obsidian:obsidian_block"},
	{pos={x=1,y=4,z=1}, block="obsidian:obsidian_block"},
	{pos={x=2,y=4,z=1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=4,z=1}, block="obsidian:obsidian_block"},
	{pos={x=0,y=4,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=1,y=4,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=2,y=4,z=-1}, block="obsidian:obsidian_block"},
	{pos={x=3,y=4,z=-1}, block="obsidian:obsidian_block"},
}

--== END OF EDITABLE OPTIONS ==--

-- Generated variables
NETHER_BOTTOM = (NETHER_DEPTH - NETHER_HEIGHT)
NETHER_ROOF_ABS = (NETHER_DEPTH - NETHER_RANDOM)
HADES_THRONE_STARTPOS_ABS = {x=HADES_THRONE_STARTPOS.x, y=(NETHER_BOTTOM + HADES_THRONE_STARTPOS.y), z=HADES_THRONE_STARTPOS.z}
LAVA_Y = (NETHER_BOTTOM + LAVA_HEIGHT)
HADES_THRONE_ABS = {}
HADES_THRONE_ENDPOS_ABS = {}
HADES_THRONE_GENERATED = minetest.get_worldpath() .. "/netherhadesthrone.txt"
NETHER_SPAWNPOS_ABS = {x=NETHER_SPAWNPOS.x, y=(NETHER_BOTTOM + NETHER_SPAWNPOS.y), z=NETHER_SPAWNPOS.z}
for i,v in ipairs(HADES_THRONE) do
	v.pos.x = v.pos.x + HADES_THRONE_STARTPOS_ABS.x
	v.pos.y = v.pos.y + HADES_THRONE_STARTPOS_ABS.y
	v.pos.z = v.pos.z + HADES_THRONE_STARTPOS_ABS.z
	HADES_THRONE_ABS[i] = v
end
local htx = 0
local hty = 0
local htz = 0
for i,v in ipairs(HADES_THRONE_ABS) do
	if v.pos.x > htx then
		htx = v.pos.x
	end
	if v.pos.y > hty then
		hty = v.pos.y
	end
	if v.pos.z > htz then
		htz = v.pos.z
	end
end
HADES_THRONE_ENDPOS_ABS = {x=htx, y=hty, z=htz}
local nether = {}

-- == General Utility Functions ==

-- Check if file exists
function nether:fileexists(file)
	file = io.open(file, "r")
	if file ~= nil then
		file:close()
		return true
	else
		return false
	end
end

-- Simple "touch" function
function nether:touch(file)
	if nether:fileexists(file) ~= true then
		file = io.open(file, "w")
		if file ~= nil then
			file:write("")
			file:close()
		end
	end
end

-- Print a message
function nether:printm(message)
	print("[Nether] " .. message)
end

-- Print an error message
function nether:printerror(message)
	nether:printm("Error! " .. message)
end

-- == Nether related stuff ==

-- Find if a position is inside the Nether
function nether:inside_nether(pos)
	if pos.y >= NETHER_BOTTOM and pos.y <= NETHER_DEPTH then
		return true
	end
	return false
end

-- Nether Lava
minetest.register_node("nether:lava_flowing", {
	description = "Nether Lava (flowing)",
	inventory_image = minetest.inventorycube("default_lava.png"),
	drawtype = "flowingliquid",
	tile_images = {"default_lava.png"},
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "flowing",
	liquid_alternative_flowing = "nether:lava_flowing",
	liquid_alternative_source = "nether:lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		{image="default_lava.png", backface_culling=false},
		{image="default_lava.png", backface_culling=true},
	},
	groups = {lava=3, liquid=2, hot=3},
})

minetest.register_node("nether:lava_source", {
	description = "Nether Lava",
	inventory_image = minetest.inventorycube("default_lava.png"),
	drawtype = "liquid",
	tile_images = {"default_lava.png"},
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "source",
	liquid_alternative_flowing = "nether:lava_flowing",
	liquid_alternative_source = "nether:lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		-- New-style lava source material (mostly unused)
		{image="default_lava.png", backface_culling=false},
	},
	groups = {lava=3, liquid=2, hot=3},
})

-- Netherrack
minetest.register_node("nether:netherrack", {
	description = "Netherrack",
	tile_images = {"nether_netherrack.png"},
	is_ground_content = true,
	groups = {cracky=3, oddly_breakable_by_hand=3},
	drop = "nether:netherrack",
	sounds = default.node_sound_stone_defaults(),
})

-- Nether tree
minetest.register_node("nether:nether_tree", {
	description = "Nether Tree",
	tile_images = {"nether_tree_top.png", "nether_tree_top.png", "nether_tree.png"},
	is_ground_content = true,
	groups = {tree=1, snappy=2, choppy=2, oddly_breakable_by_hand=1},
	sounds = default.node_sound_wood_defaults(),
})

-- Nether leaves
minetest.register_node("nether:nether_leaves", {
	description = "Nether Leaves",
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	tile_images = {"nether_leaves.png"},
	paramtype = "light",
	groups = {snappy=3, leafdecay=2},
	drop = "nether:nether_leaves",
	sounds = default.node_sound_leaves_defaults(),
})

-- Nether apple
minetest.register_node("nether:nether_apple", {
	description = "Nether Apple",
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"nether_apple.png"},
	inventory_image = "nether_apple.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	groups = {fleshy=3, dig_immediate=3},
	on_use = minetest.item_eat(-4),
	sounds = default.node_sound_defaults(),
})

-- Nether torch
minetest.register_node("nether:nether_torch", {
	description = "Nether Torch",
	drawtype = "torchlike",
	tile_images = {"nether_torch_on_floor.png", "nether_torch_on_ceiling.png", "nether_torch.png"},
	inventory_image = "nether_torch_on_floor.png",
	wield_image = "nether_torch_on_floor.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	sunlight_propagates = true,
	walkable = false,
	light_source = LIGHT_MAX - 1,
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, 0.5-0.6, -0.1, 0.1, 0.5, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.5, -0.3, -0.1, -0.5+0.3, 0.3, 0.1},
	},
	groups = {choppy=2, dig_immediate=3},
	legacy_wallmounted = true,
	sounds = default.node_sound_defaults(),
})

-- Nether torch (only shows the bottom torch. This is a hack)
minetest.register_node("nether:nether_torch_bottom", {
	description = "Nether Torch Bottom Side (you hacker!)",
	drawtype = "torchlike",
	tile_images = {"nether_torch_on_floor.png", "nether_torch_on_floor.png", "nether_torch_on_floor.png"},
	inventory_image = "nether_torch_on_floor.png",
	wield_image = "nether_torch_on_floor.png",
	paramtype = "light",
	paramtype2 = "facedir",
	sunlight_propagates = true,
	walkable = false,
	light_source = LIGHT_MAX - 1,
	drop = "nether:nether_torch",
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
	},
	groups = {choppy=2, dig_immediate=3},
	legacy_wallmounted = true,
	sounds = default.node_sound_defaults(),
})

-- Nether Pearl
minetest.register_craftitem("nether:nether_pearl", {
	description = "Nether Pearl",
	wield_image = "nether_pearl.png",
	inventory_image = "nether_pearl.png",
	visual = "sprite",
	physical = true,
	textures = {"nether_pearl.png"},
})

-- Nether Glowstone (Thanks to Gilli)
minetest.register_node( "nether:glowstone", { 
	description = "Nether Glowstone",
	tile_images = {"nether_glowstone.png"},
	light_source = 15, -- Like in Minecraft
	inventory_inventory_image = minetest.inventorycube( "nether_glowstone.png" ),
	is_ground_content = true,
	groups = {snappy=2, choppy=2, oddly_breakable_by_hand = 1.5},
})

-- Create the Nether
minetest.register_on_generated(function(minp, maxp)
	local addpos = {}
	hadesthronecounter = 1
	if ((maxp.y >= NETHER_BOTTOM) and (minp.y <= NETHER_DEPTH)) then
		-- Pass 1: Terrain generation
		for x=minp.x, maxp.x, 1 do
			for y=minp.y, maxp.y, 1 do
				for z=minp.z, maxp.z, 1 do
					addpos = {x=x, y=y, z=z}
					if y == NETHER_DEPTH then
						minetest.env:add_node(addpos, {name="nether:netherrack"})
					elseif y == NETHER_BOTTOM then
						minetest.env:add_node(addpos, {name="nether:netherrack"})
					elseif (math.floor(math.random(0, GLOWSTONE_FREQ_ROOF)) == 1) and (y >= NETHER_ROOF_ABS-1) and (nether:can_add_sticky_node(addpos) == true) then
						minetest.env:add_node(addpos, {name="nether:glowstone"})
					--[[elseif (math.floor(math.random(0, GLOWSTONE_FREQ_LAVA)) == 1) and ((nether:nodebelow(addpos) == "nether:lava_source") or (nether:nodebelow(addpos) == "nether:lava_flowing")) then
						minetest.env:add_node(addpos, {name="nether:glowstone"})
						print("GLOWSTONE" .. "X:" .. addpos.x .. "Y:" .. addpos.y .. "Z:" .. addpos.z)]]
					elseif (y == math.floor(math.random((NETHER_DEPTH-NETHER_RANDOM), NETHER_DEPTH))) and (nether:can_add_sticky_node(addpos) == true) then
						minetest.env:add_node(addpos, {name="nether:netherrack"})
					elseif (y == math.floor(math.random(NETHER_BOTTOM, (NETHER_BOTTOM+NETHER_RANDOM)))) and (nether:can_add_sticky_node(addpos) == true) then
						minetest.env:add_node(addpos, {name="nether:netherrack"})
					elseif y <= NETHER_DEPTH and y >= NETHER_BOTTOM then
						minetest.env:add_node(addpos, {name="air"})
					end
				end
			end
		end
		-- Pass 2: Details
		for x=minp.x, maxp.x, 1 do
			for y=minp.y, maxp.y, 1 do
				for z=minp.z, maxp.z, 1 do
					addpos = {x=x, y=y, z=z}
					if y < NETHER_DEPTH and y > NETHER_BOTTOM then
						if math.random(NETHER_TREE_FREQ) == 1 and y == (NETHER_BOTTOM + 1) then
							nether:grow_nethertree(addpos)
						elseif math.random(LAVA_FREQ) == 1 and y <= LAVA_Y then
							minetest.env:add_node(addpos, {name="nether:lava_source"})
						end
					end
				end
			end
		end
		-- We don't want the Throne of Hades to get regenerated (especially since it will screw up portals)
		if (minp.x <= HADES_THRONE_STARTPOS_ABS.x) and (maxp.x >= HADES_THRONE_STARTPOS_ABS.x) and (minp.y <= HADES_THRONE_STARTPOS_ABS.y) and (maxp.y >= HADES_THRONE_STARTPOS_ABS.y) and (minp.z <= HADES_THRONE_STARTPOS_ABS.z) and (maxp.z >= HADES_THRONE_STARTPOS_ABS.z) and (nether:fileexists(HADES_THRONE_GENERATED) == false)
		then
			-- Pass 3: Make way for the Throne of Hades!
			for x=(HADES_THRONE_STARTPOS_ABS.x - 1), (HADES_THRONE_ENDPOS_ABS.x + 1), 1 do
				for z=(HADES_THRONE_STARTPOS_ABS.z - 1), (HADES_THRONE_ENDPOS_ABS.z + 1), 1 do
					-- Notice I did not put a -1 for the beginning. This is because we don't want the throne to float
					for y=HADES_THRONE_STARTPOS_ABS.y, (HADES_THRONE_ENDPOS_ABS.y + 1), 1 do
						addpos = {x=x, y=y, z=z}
						minetest.env:add_node(addpos, {name="air"})
					end
				end
			end
			-- Pass 4: Throne of Hades
			for i,v in ipairs(HADES_THRONE_ABS) do
				if v.portalblock == true then
					NETHER_PORTALS_FROM_NETHER[table.getn(NETHER_PORTALS_FROM_NETHER)+1] = v.pos
					nether:save_portal_from_nether(v.pos)
					nether:createportal(v.pos)
				else
					minetest.env:add_node(v.pos, {name=v.block})
				end
			end
			nether:touch(HADES_THRONE_GENERATED)
		end
	end
end)

-- Return the name of the node below a position
function nether:nodebelow(pos)
	return minetest.env:get_node({x=pos.x, y=(pos.y-1), z=pos.z}).name
end

-- Check if we can add a "sticky" node (i.e. it has to stick to something else, or else it won't be added)
-- This is largely based on Gilli's code
function nether:can_add_sticky_node(pos)
	local nodehere = false
	local objname
	for x = -1, 1 do
		for y = -1, 1 do
			for z = -1, 1 do
				local p = {x=pos.x+x, y=pos.y+y, z=pos.z+z}
				local n = minetest.env:get_node(p)
				objname = n.name
				if objname ~= "air" and minetest.registered_nodes[objname].walkable == true then
					nodehere = true
				end
			end
		end
	end
	return nodehere
end

-- Add a "sticky" node
function nether:add_sticky_node(pos, opts)
	if nether:can_add_sticky_node(pos) == true then
		minetest.env:add_node(pos, opts)
		return true
	else
		return false
	end
end

-- Create a nether tree
function nether:grow_nethertree(pos)
	--TRUNK
	pos.y=pos.y+1
	local trunkpos={x=pos.x, z=pos.z}
	for y=pos.y, pos.y+4+math.random(2) do
		trunkpos.y=y
		minetest.env:add_node(trunkpos, {name="nether:nether_tree"})
	end
	--LEAVES
	local leafpos={}
	for x=(trunkpos.x-NETHER_TREESIZE), (trunkpos.x+NETHER_TREESIZE), 1 do
		for y=(trunkpos.y-NETHER_TREESIZE), (trunkpos.y+NETHER_TREESIZE), 1 do
			for z=(trunkpos.z-NETHER_TREESIZE), (trunkpos.z+NETHER_TREESIZE), 1 do
				if (x-trunkpos.x)*(x-trunkpos.x)
				+(y-trunkpos.y)*(y-trunkpos.y)
				+(z-trunkpos.z)*(z-trunkpos.z)
				<= NETHER_TREESIZE*NETHER_TREESIZE + NETHER_TREESIZE then
					leafpos={x=x, y=y, z=z}
					if minetest.env:get_node(leafpos).name=="air" then
						if math.random(NETHER_APPLE_FREQ) == 1 then
							if math.random(NETHER_HEAL_APPLE_FREQ) == 1 then
								minetest.env:add_node(leafpos, {name="default:apple"})
							else
								minetest.env:add_node(leafpos, {name="nether:nether_apple"})
							end
						else
							minetest.env:add_node(leafpos, {name="nether:nether_leaves"})
						end				
					end				
				end
			end
		end
	end
end

-- == PORTAL RELATED STUFF ==
NETHER_PORTALS_TO_NETHER = {}
NETHER_PORTALS_FROM_NETHER = {}
NETHER_PORTALS_TO_NETHER_FILE = minetest.get_worldpath() .. "/portalstonether.txt"
NETHER_PORTALS_FROM_NETHER_FILE = minetest.get_worldpath() .. "/portalsfromnether.txt"

-- Count the number of times a position appears in a table
function table_count(tt, item)
	local count
	count = 0
	for ii,xx in pairs(tt) do
		if (item.x == xx.x) and (item.y == xx.y) and (item.z == xx.z) then
			count = count + 1
		end
	end
	return count
end


-- Remove duplicate positions from table
function table_unique(tt)
	local newtable
	newtable = {}
	for ii,xx in ipairs(tt) do
		if(table_count(newtable, xx) == 0) then
			newtable[#newtable+1] = xx
		end
	end
	return newtable
end

-- Copied from neko259 with a few minor edits from lkjoel
function split(pString, pPattern)
	local Table = {}
	local fpat = "(.-)" .. pPattern
	local last_end = 1
	local s, e, cap = pString:find(fpat, 1)
	while s do
		if s ~= 1 or cap ~= "" then
			table.insert(Table,cap)
		end
		last_end = e+1
		s, e, cap = pString:find(fpat, last_end)
	end
	if last_end <= #pString then
		cap = pString:sub(last_end)
		table.insert(Table, cap)
	end
	return Table
end

-- Save a portal to nether
function nether:save_portal_to_nether(pos)
	local file = io.open(NETHER_PORTALS_TO_NETHER_FILE, "a")
	if file ~= nil then
		file:write("x" .. pos.x .. "\ny" .. pos.y .. "\nz" .. pos.z .. "\np", "\n")
		file:close()
	else
		nether:printerror("Cannot write portal to file!")
	end
end

-- Save all nether portals
function nether:save_portals_to_nether()
	local array2 = NETHER_PORTALS_TO_NETHER
	NETHER_PORTALS_TO_NETHER = table_unique(array2)
	file = io.open(NETHER_PORTALS_TO_NETHER_FILE, "w")
	if file ~= nil then
		file:write("")
		file:close()
		for i,v in ipairs(NETHER_PORTALS_TO_NETHER) do
			nether:save_portal_to_nether(v)
		end
	else
		nether:printerror("Cannot create portal file!")
	end
end

-- Save a portal from nether
function nether:save_portal_from_nether(pos)
	local file = io.open(NETHER_PORTALS_FROM_NETHER_FILE, "a")
	if file ~= nil then
		file:write("x" .. pos.x .. "\ny" .. pos.y .. "\nz" .. pos.z .. "\np", "\n")
		file:close()
	else
		nether:printerror("Cannot write portal to file!")
	end
end

-- Save all portals from nether
function nether:save_portals_from_nether()
	local array2 = NETHER_PORTALS_FROM_NETHER
	NETHER_PORTALS_FROM_NETHER = table_unique(array2)
	file = io.open(NETHER_PORTALS_FROM_NETHER_FILE, "w")
	if file ~= nil then
		file:write("")
		file:close()
		for i,v in ipairs(NETHER_PORTALS_FROM_NETHER) do
			nether:save_portal_from_nether(v)
		end
	else
		nether:printerror("Cannot create portal file!")
	end
end

-- Read portals to nether
function nether:read_portals_to_nether()
	local array = {}
	local array2 = {}
	local file = io.open(NETHER_PORTALS_TO_NETHER_FILE, "r")
	if file ~= nil then
		for line in io.lines(NETHER_PORTALS_TO_NETHER_FILE) do
			if not (line == "" or line == nil) then
				if line:sub(1, 1) == "p" then
					array2[table.getn(array2)+1] = array
				elseif line:sub(1, 1) == "x" then
					array.x = tonumber(split(line, "x")[1])
				elseif line:sub(1, 1) == "y" then
					array.y = tonumber(split(line, "y")[1])
				elseif line:sub(1, 1) == "z" then
					array.z = tonumber(split(line, "z")[1])
				end
			end
		end
	else
		file = io.open(NETHER_PORTALS_TO_NETHER_FILE, "w")
		if file ~= nil then
			file:write("")
			file:close()
		else
			nether:printerror("Cannot create portal file!")
		end
	end
	NETHER_PORTALS_TO_NETHER = table_unique(array2)
end

-- Read portals from nether
function nether:read_portals_from_nether()
	local array = {}
	local array2 = {}
	local file = io.open(NETHER_PORTALS_FROM_NETHER_FILE, "r")
	if file ~= nil then
		for line in io.lines(NETHER_PORTALS_FROM_NETHER_FILE) do
			if not (line == "" or line == nil) then
				if line:sub(1, 1) == "p" then
					array2[table.getn(array2)+1] = array
				elseif line:sub(1, 1) == "x" then
					array.x = tonumber(split(line, "x")[1])
				elseif line:sub(1, 1) == "y" then
					array.y = tonumber(split(line, "y")[1])
				elseif line:sub(1, 1) == "z" then
					array.z = tonumber(split(line, "z")[1])
				end
			end
		end
	else
		file = io.open(NETHER_PORTALS_FROM_NETHER_FILE, "w")
		if file ~= nil then
			file:write("")
			file:close()
		else
			nether:printerror("Cannot create portal file!")
		end
	end
	NETHER_PORTALS_FROM_NETHER = table_unique(array2)
end

nether:read_portals_to_nether()
nether:read_portals_from_nether()

-- Teleport the player
function nether:teleport_player(from_nether, player)
	local randomportal = 1
	local coin = math.floor(math.random(0, 1))
	if coin == 0 then
		coin = -1
	else
		coin = 1
	end
	local coin2 = math.floor(math.random(1, 2))
	local num = 1
	local forgetit = false
	if from_nether == true then
		num = table.getn(NETHER_PORTALS_TO_NETHER)
		if num == 1 then
			randomportal = 1
		elseif num < 1 then
			forgetit = true
			teleportpos = NETHER_SPAWNPOS_ABS
		else
			randomportal = math.floor(math.random(1, num))
		end
		if forgetit == false then
			portalpos = NETHER_PORTALS_TO_NETHER[randomportal]
		end
	else
		num = table.getn(NETHER_PORTALS_FROM_NETHER)
		if num == 1 then
			randomportal = 1
		elseif num < 1 then
			forgetit = true
			teleportpos = NETHER_SPAWNPOS_ABS
		else
			randomportal = math.floor(math.random(1, num))
		end
		if forgetit == false then
			portalpos = NETHER_PORTALS_FROM_NETHER[randomportal]
		end
	end
	if forgetit == false then
		teleportpos = {x=portalpos.x + coin2, y=portalpos.y + 1, z=portalpos.z + coin}
	end
	player:setpos(teleportpos)
end

-- Creates a portal
function nether:createportal(pos)
	local currx
	local curry
	local currz
	local currpos = {}
	for i,v in ipairs(NETHER_PORTAL) do
		currx = v.pos.x + pos.x
		curry = v.pos.y + pos.y
		currz = v.pos.z + pos.z
		currpos = {x=currx, y=curry, z=currz}
		minetest.env:add_node(currpos, {name=v.block})
	end
end

-- Portal Creator
minetest.register_node("nether:nether_portal_creator", {
	tile_images = {"nether_portal_creator.png"},
	description = "Nether Portal Creator",
})

minetest.register_on_placenode(function(pos, node)
	if node.name == "nether:nether_portal_creator" then
		if nether:inside_nether(pos) then
			NETHER_PORTALS_FROM_NETHER[table.getn(NETHER_PORTALS_FROM_NETHER)+1] = pos
			nether:save_portal_from_nether(pos)
		else
			NETHER_PORTALS_TO_NETHER[table.getn(NETHER_PORTALS_TO_NETHER)+1] = pos
			nether:save_portal_to_nether(pos)
		end
		nether:createportal(pos)
	end
	nodeupdate(pos)
end)

minetest.register_abm({
	nodenames = "nether:nether_portal_creator",
	interval = 1.0,
	chance = 1,
	action = function(pos)
		nether:createportal(pos)
	end
})

-- Portal Stuff
minetest.register_node("nether:nether_portal", {
	description = "Nether Portal",
	drawtype = "glasslike",
	tile_images = {"nether_portal_stuff.png"},
	inventory_image = "nether_portal_stuff.png",
	wield_image = "nether_portal_stuff.png",
	light_source = LIGHT_MAX - 2,
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	groups = {choppy=2,dig_immediate=3},
	legacy_wallmounted = false,
	buildable_to = true,
	post_effect_color = {a=64, r=150, g=100, b=200},
	metadata_name = "generic"
})

minetest.register_abm({
	nodenames = {"nether:nether_portal"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node)
		local nodemeta = minetest.env:get_meta(pos)
		local objs = minetest.env:get_objects_inside_radius(pos, 1)
		if objs[1] ~= nil then
			for k, obj in pairs(objs) do
				local objpos = obj:getpos()
				local objmeta = minetest.env:get_meta(objpos)
				if objpos.y>pos.y-1 and objpos.y<pos.y and obj:get_player_name() ~= nil and obj:get_player_name() ~= "" then
					local innether = nether:inside_nether(obj:getpos())
					if innether == true and (objmeta:get_string("teleportingfromnether") == "" or objmeta:get_string("teleportingfromnether") == nil) then
						objmeta:set_string("teleportingfromnether", "true")
						objmeta:set_string("teleportingtonether", "")
						nether:teleport_player(innether, obj)
					elseif innether == false and (objmeta:get_string("teleportingtonether") == "" or objmeta:get_string("teleportingtonether") == nil) then
						objmeta:set_string("teleportingtonether", "true")
						objmeta:set_string("teleportingfromnether", "")
						nether:teleport_player(innether, obj)
					end
				end
			end
		end
	end,
})

-- CRAFTING DEFINITIONS
minetest.register_craft({
	output = "nether:nether_portal_creator",
	recipe = {
		{"obsidian:obsidian_block", "obsidian:obsidian_block", "obsidian:obsidian_block"},
		{"obsidian:obsidian_block", "nether:nether_pearl", "obsidian:obsidian_block"},
		{"obsidian:obsidian_block", "obsidian:obsidian_block", "obsidian:obsidian_block"},
	}
})

print("Nether mod loaded!")
