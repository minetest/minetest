function basic_dump2(o)
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "boolean" then
		return tostring(o)
	elseif type(o) == "function" then
		return "<function>"
	elseif type(o) == "userdata" then
		return "<userdata>"
	elseif type(o) == "nil" then
		return "nil"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

function dump2(o, name, dumped)
	name = name or "_"
	dumped = dumped or {}
	io.write(name, " = ")
	if type(o) == "number" or type(o) == "string" or type(o) == "boolean"
			or type(o) == "function" or type(o) == "nil"
			or type(o) == "userdata" then
		io.write(basic_dump2(o), "\n")
	elseif type(o) == "table" then
		if dumped[o] then
			io.write(dumped[o], "\n")
		else
			dumped[o] = name
			io.write("{}\n") -- new table
			for k,v in pairs(o) do
				local fieldname = string.format("%s[%s]", name, basic_dump2(k))
				dump2(v, fieldname, dumped)
			end
		end
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

function dump(o, dumped)
	dumped = dumped or {}
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "table" then
		if dumped[o] then
			return "<circular reference>"
		end
		dumped[o] = true
		local t = {}
		for k,v in pairs(o) do
			t[#t+1] = "" .. k .. " = " .. dump(v, dumped)
		end
		return "{" .. table.concat(t, ", ") .. "}"
	elseif type(o) == "boolean" then
		return tostring(o)
	elseif type(o) == "function" then
		return "<function>"
	elseif type(o) == "userdata" then
		return "<userdata>"
	elseif type(o) == "nil" then
		return "nil"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

-- Textures:
-- Mods should prefix their textures with modname_, eg. given the mod
-- name "foomod", a texture could be called "foomod_superfurnace.png"
--
-- Global functions:
-- minetest.register_entity(name, prototype_table)
-- minetest.register_globalstep(func)
--
-- Global objects:
-- minetest.env - environment reference
--
-- Global tables:
-- minetest.registered_entities
-- ^ List of registered entity prototypes, indexed by name
-- minetest.object_refs
-- ^ List of object references, indexed by active object id
-- minetest.luaentities
-- ^ List of lua entities, indexed by active object id
--
-- EnvRef methods:
-- - add_node(pos, content); pos={x=num, y=num, z=num}
--
-- ObjectRef methods:
-- - remove(): remove object (after returning from Lua)
-- - getpos(): returns {x=num, y=num, z=num}
-- - setpos(pos); pos={x=num, y=num, z=num}
-- - moveto(pos, continuous=false): interpolated move
-- - add_to_inventory(itemstring): add an item to object inventory
--
-- Registered entities:
-- - Functions receive a "luaentity" as self:
--   - It has the member .object, which is an ObjectRef pointing to the object
--   - The original prototype stuff is visible directly via a metatable
--

print("omg lol")
print("minetest dump: "..dump(minetest))

-- Global environment step function
function on_step(dtime)
	-- print("on_step")
end

minetest.register_globalstep(on_step)

minetest.register_tool("WPick", {
	image = "tool_woodpick.png",
	basetime = 2.0,
	dt_weight = 0,
	dt_crackiness = -0.5,
	dt_crumbliness = 2,
	dt_cuttability = 0,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("STPick", {
	image = "tool_stonepick.png",
	basetime = 1.5,
	dt_weight = 0,
	dt_crackiness = -0.5,
	dt_crumbliness = 2,
	dt_cuttability = 0,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("SteelPick", {
	image = "tool_steelpick.png",
	basetime = 1.0,
	dt_weight = 0,
	dt_crackiness = -0.5,
	dt_crumbliness = 2,
	dt_cuttability = 0,
	basedurability = 333,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("MesePick", {
	image = "tool_mesepick.png",
	basetime = 0,
	dt_weight = 0,
	dt_crackiness = 0,
	dt_crumbliness = 0,
	dt_cuttability = 0,
	basedurability = 1337,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("WShovel", {
	image = "tool_woodshovel.png",
	basetime = 2.0,
	dt_weight = 0.5,
	dt_crackiness = 2,
	dt_crumbliness = -1.5,
	dt_cuttability = 0.3,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("STShovel", {
	image = "tool_stoneshovel.png",
	basetime = 1.5,
	dt_weight = 0.5,
	dt_crackiness = 2,
	dt_crumbliness = -1.5,
	dt_cuttability = 0.1,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("SteelShovel", {
	image = "tool_steelshovel.png",
	basetime = 1.0,
	dt_weight = 0.5,
	dt_crackiness = 2,
	dt_crumbliness = -1.5,
	dt_cuttability = 0.0,
	basedurability = 330,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("WAxe", {
	image = "tool_woodaxe.png",
	basetime = 2.0,
	dt_weight = 0.5,
	dt_crackiness = -0.2,
	dt_crumbliness = 1,
	dt_cuttability = -0.5,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("STAxe", {
	image = "tool_stoneaxe.png",
	basetime = 1.5,
	dt_weight = 0.5,
	dt_crackiness = -0.2,
	dt_crumbliness = 1,
	dt_cuttability = -0.5,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("SteelAxe", {
	image = "tool_steelaxe.png",
	basetime = 1.0,
	dt_weight = 0.5,
	dt_crackiness = -0.2,
	dt_crumbliness = 1,
	dt_cuttability = -0.5,
	basedurability = 330,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("WSword", {
	image = "tool_woodsword.png",
	basetime = 3.0,
	dt_weight = 3,
	dt_crackiness = 0,
	dt_crumbliness = 1,
	dt_cuttability = -1,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("STSword", {
	image = "tool_stonesword.png",
	basetime = 2.5,
	dt_weight = 3,
	dt_crackiness = 0,
	dt_crumbliness = 1,
	dt_cuttability = -1,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("SteelSword", {
	image = "tool_steelsword.png",
	basetime = 2.0,
	dt_weight = 3,
	dt_crackiness = 0,
	dt_crumbliness = 1,
	dt_cuttability = -1,
	basedurability = 330,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("", {
	image = "",
	basetime = 0.5,
	dt_weight = 1,
	dt_crackiness = 0,
	dt_crumbliness = -1,
	dt_cuttability = 0,
	basedurability = 50,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})

--[[
minetest.register_tool("horribletool", {
	image = "lava.png",
	basetime = 2.0
	dt_weight = 0.2
	dt_crackiness = 0.2
	dt_crumbliness = 0.2
	dt_cuttability = 0.2
	basedurability = 50
	dd_weight = -5
	dd_crackiness = -5
	dd_crumbliness = -5
	dd_cuttability = -5
})
--]]

minetest.register_node("somenode", {
	tile_images = {"lava.png", "mese.png", "stone.png", "grass.png", "cobble.png", "tree_top.png"},
	inventory_image = "treeprop.png"
})

minetest.register_node("TNT", {
	tile_images = {"tnt_top.png", "tnt_bottom.png", "tnt_side.png", "tnt_side.png", "tnt_side.png", "tnt_side.png"},
	inventory_image = "tnt_side.png"
})

minetest.register_craft({
	output = 'ToolItem "STPick" 4',
	recipe = {
		{'NodeItem "cobble" 1', 'NodeItem "cobble" 1', 'NodeItem "cobble" 1'},
		{'', 'CraftItem "Stick"', ''},
		{'', 'CraftItem "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'NodeItem "TNT" 4',
	recipe = {
		{'NodeItem "wood" 1'},
		{'CraftItem "lump_of_coal" 1'},
		{'NodeItem "wood" 1'}
	}
})

local TNT = {
	-- Maybe handle gravity and collision this way? dunno
	physical = true,
	weight = 5,
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	textures = {"tnt_top.png","tnt_bottom.png","tnt_side.png","tnt_side.png","tnt_side.png","tnt_side.png"},
	--visual = "single_sprite",
	--textures = {"mese.png^[forcesingle"},
	-- Initial value for our timer
	timer = 0,
	-- List names of state variables, for serializing object state
	state_variables = {"timer"},
}

-- Called periodically
function TNT:on_step(dtime)
	--print("TNT:on_step()")
end

-- Called when object is punched
function TNT:on_punch(hitter)
	print("TNT:on_punch()")
	self.object:remove()
	hitter:add_to_inventory("CraftItem testobject1 1")
end

-- Called when object is right-clicked
function TNT:on_rightclick(clicker)
	pos = self.object:getpos()
	pos = {x=pos.x, y=pos.y+0.1, z=pos.z}
	self.object:moveto(pos, false)
end

print("TNT dump: "..dump(TNT))

print("Registering TNT");
minetest.register_entity("TNT", TNT)

print("minetest.registered_entities:")
dump2(minetest.registered_entities)

--[[
function TNT:on_rightclick(clicker)
	print("TNT:on_rightclick()")
	print("self: "..dump(self))
	print("getmetatable(self): "..dump(getmetatable(self)))
	print("getmetatable(getmetatable(self)): "..dump(getmetatable(getmetatable(self))))
	pos = self.object:getpos()
	print("TNT:on_rightclick(): object position: "..dump(pos))
	pos = {x=pos.x+0.5+1, y=pos.y+0.5, z=pos.z+0.5}
	--minetest.env:add_node(pos, 0)
end
--]]

--[=[

register_block(0, {
	textures = "stone.png",
	makefacetype = 0,
	get_dig_duration = function(env, pos, digger)
		-- Check stuff like digger.current_tool
		return 1.5
	end,
	on_dig = function(env, pos, digger)
		env:remove_node(pos)
		digger.inventory.put("MaterialItem2 0");
	end,
})

register_block(1, {
	textures = {"grass.png","mud.png","mud_grass_side.png","mud_grass_side.png","mud_grass_side.png","mud_grass_side.png"},
	makefacetype = 0,
	get_dig_duration = function(env, pos, digger)
		-- Check stuff like digger.current_tool
		return 0.5
	end,
	on_dig = function(env, pos, digger)
		env:remove_node(pos)
		digger.inventory.put("MaterialItem2 1");
	end,
})

-- Consider the "miscellaneous block namespace" to be 0xc00...0xfff = 3072...4095
register_block(3072, {
	textures = {"tnt_top.png","tnt_bottom.png","tnt_side.png","tnt_side.png","tnt_side.png","tnt_side.png"},
	makefacetype = 0,
	get_dig_duration = function(env, pos, digger)
		-- Cannot be dug
		return nil
	end,
	-- on_dig = function(env, pos, digger) end, -- Not implemented
	on_hit = function(env, pos, hitter)
		-- Replace with TNT object, which will explode after timer, follow gravity, blink and stuff
		env:add_object("tnt", pos)
		env:remove_node(pos)
	end,
})
--]=]

