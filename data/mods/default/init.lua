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
-- minetest.register_tool(name, {lots of stuff})
-- minetest.register_node(name, {lots of stuff})
-- minetest.register_craft({output=item, recipe={...})
-- minetest.register_globalstep(func)
-- minetest.register_on_placenode(func)
-- minetest.register_on_dignode(func)
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
-- EnvRef is basically ServerEnvironment and ServerMap combined.
-- EnvRef methods:
-- - add_node(pos, node)
-- - remove_node(pos)
-- - get_node(pos)
-- - add_luaentity(pos, name)
--
-- ObjectRef is basically ServerActiveObject.
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
-- - Callbacks:
--   - on_activate(self, staticdata)
--   - on_step(self, dtime)
--   - on_punch(self, hitter)
--   - on_rightclick(self, clicker)
--   - get_staticdata(self): return string
--
-- MapNode representation:
-- {name="name", param1=num, param2=num}
--
-- Position representation:
-- {x=num, y=num, z=num}
--

-- print("minetest dump: "..dump(minetest))

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
	output = 'NodeItem "wood" 4',
	recipe = {
		{'NodeItem "tree"'},
	}
})

minetest.register_craft({
	output = 'CraftItem "Stick" 4',
	recipe = {
		{'NodeItem "wood"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "wooden_fence" 2',
	recipe = {
		{'CraftItem "Stick"', 'CraftItem "Stick"', 'CraftItem "Stick"'},
		{'CraftItem "Stick"', 'CraftItem "Stick"', 'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "sign_wall" 1',
	recipe = {
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
		{'', 'CraftItem "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'NodeItem "torch" 4',
	recipe = {
		{'CraftItem "lump_of_coal"'},
		{'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "WPick"',
	recipe = {
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
		{'', 'CraftItem "Stick"', ''},
		{'', 'CraftItem "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'ToolItem "STPick"',
	recipe = {
		{'NodeItem "cobble"', 'NodeItem "cobble"', 'NodeItem "cobble"'},
		{'', 'CraftItem "Stick"', ''},
		{'', 'CraftItem "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'ToolItem "SteelPick"',
	recipe = {
		{'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"'},
		{'', 'CraftItem "Stick"', ''},
		{'', 'CraftItem "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'ToolItem "MesePick"',
	recipe = {
		{'NodeItem "mese"', 'NodeItem "mese"', 'NodeItem "mese"'},
		{'', 'CraftItem "Stick"', ''},
		{'', 'CraftItem "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'ToolItem "WShovel"',
	recipe = {
		{'NodeItem "wood"'},
		{'CraftItem "Stick"'},
		{'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "STShovel"',
	recipe = {
		{'NodeItem "cobble"'},
		{'CraftItem "Stick"'},
		{'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "SteelShovel"',
	recipe = {
		{'CraftItem "steel_ingot"'},
		{'CraftItem "Stick"'},
		{'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "WAxe"',
	recipe = {
		{'NodeItem "wood"', 'NodeItem "wood"'},
		{'NodeItem "wood"', 'CraftItem "Stick"'},
		{'', 'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "STAxe"',
	recipe = {
		{'NodeItem "cobble"', 'NodeItem "cobble"'},
		{'NodeItem "cobble"', 'CraftItem "Stick"'},
		{'', 'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "SteelAxe"',
	recipe = {
		{'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"'},
		{'CraftItem "steel_ingot"', 'CraftItem "Stick"'},
		{'', 'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "WSword"',
	recipe = {
		{'NodeItem "wood"'},
		{'NodeItem "wood"'},
		{'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "STSword"',
	recipe = {
		{'NodeItem "cobble"'},
		{'NodeItem "cobble"'},
		{'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'ToolItem "SteelSword"',
	recipe = {
		{'CraftItem "steel_ingot"'},
		{'CraftItem "steel_ingot"'},
		{'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "rail" 15',
	recipe = {
		{'CraftItem "steel_ingot"', '', 'CraftItem "steel_ingot"'},
		{'CraftItem "steel_ingot"', 'CraftItem "Stick"', 'CraftItem "steel_ingot"'},
		{'CraftItem "steel_ingot"', '', 'CraftItem "steel_ingot"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "chest" 1',
	recipe = {
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
		{'NodeItem "wood"', '', 'NodeItem "wood"'},
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "locked_chest" 1',
	recipe = {
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
		{'NodeItem "wood"', 'CraftItem "steel_ingot"', 'NodeItem "wood"'},
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "furnace" 1',
	recipe = {
		{'NodeItem "cobble"', 'NodeItem "cobble"', 'NodeItem "cobble"'},
		{'NodeItem "cobble"', '', 'NodeItem "cobble"'},
		{'NodeItem "cobble"', 'NodeItem "cobble"', 'NodeItem "cobble"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "steelblock" 1',
	recipe = {
		{'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"'},
		{'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"'},
		{'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"', 'CraftItem "steel_ingot"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "sandstone" 1',
	recipe = {
		{'NodeItem "sand"', 'NodeItem "sand"'},
		{'NodeItem "sand"', 'NodeItem "sand"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "clay" 1',
	recipe = {
		{'CraftItem "lump_of_clay"', 'CraftItem "lump_of_clay"'},
		{'CraftItem "lump_of_clay"', 'CraftItem "lump_of_clay"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "brick" 1',
	recipe = {
		{'CraftItem "clay_brick"', 'CraftItem "clay_brick"'},
		{'CraftItem "clay_brick"', 'CraftItem "clay_brick"'},
	}
})

minetest.register_craft({
	output = 'CraftItem "paper" 1',
	recipe = {
		{'NodeItem "papyrus"', 'NodeItem "papyrus"', 'NodeItem "papyrus"'},
	}
})

minetest.register_craft({
	output = 'CraftItem "book" 1',
	recipe = {
		{'CraftItem "paper"'},
		{'CraftItem "paper"'},
		{'CraftItem "paper"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "bookshelf" 1',
	recipe = {
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
		{'CraftItem "book"', 'CraftItem "book"', 'CraftItem "book"'},
		{'NodeItem "wood"', 'NodeItem "wood"', 'NodeItem "wood"'},
	}
})

minetest.register_craft({
	output = 'NodeItem "ladder" 1',
	recipe = {
		{'CraftItem "Stick"', '', 'CraftItem "Stick"'},
		{'CraftItem "Stick"', 'CraftItem "Stick"', 'CraftItem "Stick"'},
		{'CraftItem "Stick"', '', 'CraftItem "Stick"'},
	}
})

minetest.register_craft({
	output = 'CraftItem "apple_iron" 1',
	recipe = {
		{'', 'CraftItem "steel_ingot"', ''},
		{'CraftItem "steel_ingot"', 'CraftItem "apple"', 'CraftItem "steel_ingot"'},
		{'', 'CraftItem "steel_ingot"', ''},
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

--
-- Some common functions
--

function nodeupdate_single(p)
	n = minetest.env:get_node(p)
	if n.name == "sand" or n.name == "gravel" then
		p_bottom = {x=p.x, y=p.y-1, z=p.z}
		n_bottom = minetest.env:get_node(p_bottom)
		if n_bottom.name == "air" then
			minetest.env:remove_node(p)
			minetest.env:add_luaentity(p, "falling_"..n.name)
			nodeupdate(p)
		end
	end
end

function nodeupdate(p)
	for x = -1,1 do
	for y = -1,1 do
	for z = -1,1 do
		p2 = {x=p.x+x, y=p.y+y, z=p.z+z}
		nodeupdate_single(p2)
	end
	end
	end
end

--
-- TNT (not functional)
--

local TNT = {
	-- Static definition
	physical = true, -- Collides with things
	-- weight = 5,
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	textures = {"tnt_top.png","tnt_bottom.png","tnt_side.png","tnt_side.png","tnt_side.png","tnt_side.png"},
	--visual = "single_sprite",
	--textures = {"mese.png^[forcesingle"},
	-- Initial value for our timer
	timer = 0,
	-- Number of punches required to defuse
	health = 1,
}

-- Called when a TNT object is created
function TNT:on_activate(staticdata)
	print("TNT:on_activate()")
	self.object:setvelocity({x=0, y=2, z=0})
	self.object:setacceleration({x=0, y=-10, z=0})
end

-- Called periodically
function TNT:on_step(dtime)
	--print("TNT:on_step()")
end

-- Called when object is punched
function TNT:on_punch(hitter)
	print("TNT:on_punch()")
	self.health = self.health - 1
	if self.health <= 0 then
		self.object:remove()
		hitter:add_to_inventory("NodeItem TNT 1")
	end
end

-- Called when object is right-clicked
function TNT:on_rightclick(clicker)
	--pos = self.object:getpos()
	--pos = {x=pos.x, y=pos.y+0.1, z=pos.z}
	--self.object:moveto(pos, false)
end

print("TNT dump: "..dump(TNT))

print("Registering TNT");
minetest.register_entity("TNT", TNT)

--
-- Falling stuff
--

function register_falling_node(nodename, texture)
	minetest.register_entity("falling_"..nodename, {
		-- Static definition
		physical = true,
		collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
		visual = "cube",
		textures = {texture,texture,texture,texture,texture,texture},
		-- State
		-- Methods
		on_step = function(self, dtime)
			-- Set gravity
			self.object:setacceleration({x=0, y=-10, z=0})
			-- Turn to actual sand when collides to ground or just move
			local pos = self.object:getpos()
			local bcp = {x=pos.x, y=pos.y-0.7, z=pos.z} -- Position of bottom center point
			local bcn = minetest.env:get_node(bcp)
			if bcn.name ~= "air" then
				-- Turn to a sand node
				local np = {x=bcp.x, y=bcp.y+1, z=bcp.z}
				minetest.env:add_node(np, {name=nodename})
				self.object:remove()
			else
				-- Do nothing
			end
		end
	})
end

register_falling_node("sand", "sand.png")
register_falling_node("gravel", "gravel.png")

--[[
minetest.register_entity("falling_sand", {
	-- Definition
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	textures = {"sand.png","sand.png","sand.png","sand.png","sand.png","sand.png"},
	-- State
	fallspeed = 0,
	-- Methods
	on_step = function(self, dtime)
		-- Apply gravity
		self.fallspeed = self.fallspeed + dtime * 5
		fp = self.object:getpos()
		fp.y = fp.y - self.fallspeed * dtime
		self.object:moveto(fp)
		-- Turn to actual sand when collides to ground or just move
		bcp = {x=fp.x, y=fp.y-0.5, z=fp.z} -- Position of bottom center point
		bcn = minetest.env:get_node(bcp)
		if bcn.name ~= "air" then
			-- Turn to a sand node
			np = {x=bcp.x, y=bcp.y+1, z=bcp.z}
			minetest.env:add_node(np, {name="sand"})
			self.object:remove()
		else
			-- Do nothing
		end
	end
})
--]]

--
-- Global callbacks
--

-- Global environment step function
function on_step(dtime)
	-- print("on_step")
end
minetest.register_globalstep(on_step)

function on_placenode(p, node)
	print("on_placenode")
	nodeupdate(p)
end
minetest.register_on_placenode(on_placenode)

function on_dignode(p, node)
	print("on_dignode")
	nodeupdate(p)
end
minetest.register_on_dignode(on_dignode)

function on_punchnode(p, node)
	print("on_punchnode")
	if node.name == "TNT" then
		minetest.env:remove_node(p)
		minetest.env:add_luaentity(p, "TNT")
		nodeupdate(p)
	end
end
minetest.register_on_punchnode(on_punchnode)

--
-- Done, print some random stuff
--

print("minetest.registered_entities:")
dump2(minetest.registered_entities)

--
-- Some random pre-implementation planning and drafting
--

--[[
function TNT:on_rightclick(clicker)
	print("TNT:on_rightclick()")
	print("self: "..dump(self))
	print("getmetatable(self): "..dump(getmetatable(self)))
	print("getmetatable(getmetatable(self)): "..dump(getmetatable(getmetatable(self))))
	pos = self.object:getpos()
	print("TNT:on_rightclick(): object position: "..dump(pos))
	pos = {x=pos.x+0.5+1, y=pos.y+0.5, z=pos.z+0.5}
	--minetest.env:add_node(pos, {name="stone")
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

