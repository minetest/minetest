-- Helper functions defined by builtin.lua:
-- dump2(obj, name="_", dumped={})
-- dump(obj, dumped={})
--
-- Textures:
-- Mods should prefix their textures with modname_, eg. given the mod
-- name "foomod", a texture could be called "foomod_superfurnace.png"
-- Due to historical reasons, the default mod does not follow this rule.
--
-- Global functions:
-- minetest.register_entity(name, prototype_table)
-- minetest.register_tool(name, {lots of stuff})
-- minetest.register_node(name, {lots of stuff})
-- minetest.register_craftitem(name, {lots of stuff})
-- minetest.register_craft({output=item, recipe={...})
-- minetest.register_globalstep(func)
-- minetest.register_on_placenode(func(pos, newnode, placer))
-- minetest.register_on_dignode(func(pos, oldnode, digger))
-- minetest.register_on_punchnode(func(pos, node, puncher))
-- minetest.register_on_generated(func(minp, maxp))
-- minetest.register_on_newplayer(func(ObjectRef))
-- minetest.register_on_respawnplayer(func(ObjectRef))
-- ^ return true in func to disable regular player placement
-- minetest.register_on_chat_message(func(name, message))
-- minetest.setting_get(name)
-- minetest.setting_getbool(name)
-- minetest.chat_send_all(text)
-- minetest.chat_send_player(name, text)
-- minetest.get_player_privs(name)
--
-- Global objects:
-- minetest.env - environment reference
--
-- Global tables:
-- minetest.registered_nodes
-- ^ List of registered node definitions, indexed by name
-- minetest.registered_craftitems
-- ^ List of registered craft item definitions, indexed by name
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
-- - add_item(pos, itemstring)
-- - add_rat(pos)
-- - add_firefly(pos)
-- - get_meta(pos) -- Get a NodeMetaRef at that position
-- - get_player_by_name(name) -- Get an ObjectRef to a player
--
-- NodeMetaRef
-- - get_type()
-- - allows_text_input()
-- - set_text(text) -- eg. set the text of a sign
-- - get_text()
-- - get_owner()
-- - set_infotext(infotext)
-- - inventory_set_list(name, {item1, item2, ...})
-- - inventory_get_list(name)
-- - set_inventory_draw_spec(string)
-- - set_allow_text_input(bool)
-- - set_allow_removal(bool)
-- - set_enforce_owner(bool)
-- - is_inventory_modified()
-- - reset_inventory_modified()
-- - is_text_modified()
-- - reset_text_modified()
-- - set_string(name, value)
-- - get_string(name)
--
-- ObjectRef is basically ServerActiveObject.
-- ObjectRef methods:
-- - remove(): remove object (after returning from Lua)
-- - getpos(): returns {x=num, y=num, z=num}
-- - setpos(pos); pos={x=num, y=num, z=num}
-- - moveto(pos, continuous=false): interpolated move
-- - add_to_inventory(itemstring): add an item to object inventory
-- - add_to_inventory_later(itemstring): like above, but after callback returns (only allowed for craftitem callbacks)
-- - get_hp(): returns number of hitpoints (2 * number of hearts)
-- - set_hp(hp): set number of hitpoints (2 * number of hearts)
-- - settexturemod(mod)
-- - setsprite(p={x=0,y=0}, num_frames=1, framelength=0.2,
-- -           select_horiz_by_yawpitch=false)
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

WATER_ALPHA = 160
WATER_VISC = 1
LAVA_VISC = 7
LIGHT_MAX = 14

--
-- Tool definition
--

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
-- The hand
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

--
-- Crafting definition
--

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

--
-- Node definitions
--

function digprop_constanttime(time)
	return {
		diggability = "constant",
		constant_time = time,
	}
end

function digprop_stonelike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 5,
		crackiness = 1,
		crumbliness = -0.1,
		cuttability = -0.2,
	}
end

function digprop_dirtlike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 1.2,
		crackiness = 0,
		crumbliness = 1.2,
		cuttability = -0.4,
	}
end

function digprop_gravellike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 2,
		crackiness = 0.2,
		crumbliness = 1.5,
		cuttability = -1.0,
	}
end

function digprop_woodlike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 1.0,
		crackiness = 0.75,
		crumbliness = -1.0,
		cuttability = 1.5,
	}
end

function digprop_leaveslike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * (-0.5),
		crackiness = 0,
		crumbliness = 0,
		cuttability = 2.0,
	}
end

function digprop_glasslike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 0.1,
		crackiness = 2.0,
		crumbliness = -1.0,
		cuttability = -1.0,
	}
end

function inventorycube(img1, img2, img3)
	img2 = img2 or img1
	img3 = img3 or img1
	return "[inventorycube"
			.. "{" .. img1:gsub("%^", "&")
			.. "{" .. img2:gsub("%^", "&")
			.. "{" .. img3:gsub("%^", "&")
end

-- Legacy nodes

minetest.register_node("stone", {
	tile_images = {"stone.png"},
	inventory_image = inventorycube("stone.png"),
	paramtype = "mineral",
	is_ground_content = true,
	often_contains_mineral = true, -- Texture atlas hint
	material = digprop_stonelike(1.0),
	dug_item = 'NodeItem "cobble" 1',
})

minetest.register_node("dirt_with_grass", {
	tile_images = {"grass.png", "mud.png", "mud.png^grass_side.png"},
	inventory_image = inventorycube("mud.png^grass_side.png"),
	is_ground_content = true,
	material = digprop_dirtlike(1.0),
	dug_item = 'NodeItem "dirt" 1',
})

minetest.register_node("dirt_with_grass_footsteps", {
	tile_images = {"grass_footsteps.png", "mud.png", "mud.png^grass_side.png"},
	inventory_image = "grass_footsteps.png",
	is_ground_content = true,
	material = digprop_dirtlike(1.0),
	dug_item = 'NodeItem "dirt" 1',
})

minetest.register_node("dirt", {
	tile_images = {"mud.png"},
	inventory_image = inventorycube("mud.png"),
	is_ground_content = true,
	material = digprop_dirtlike(1.0),
})

minetest.register_node("sand", {
	tile_images = {"sand.png"},
	inventory_image = inventorycube("sand.png"),
	is_ground_content = true,
	material = digprop_dirtlike(1.0),
})

minetest.register_node("gravel", {
	tile_images = {"gravel.png"},
	inventory_image = inventorycube("gravel.png"),
	is_ground_content = true,
	material = digprop_gravellike(1.0),
})

minetest.register_node("sandstone", {
	tile_images = {"sandstone.png"},
	inventory_image = inventorycube("sandstone.png"),
	is_ground_content = true,
	material = digprop_dirtlike(1.0),  -- FIXME should this be stonelike?
	dug_item = 'NodeItem "sand" 1',  -- FIXME is this intentional?
})

minetest.register_node("clay", {
	tile_images = {"clay.png"},
	inventory_image = inventorycube("clay.png"),
	is_ground_content = true,
	material = digprop_dirtlike(1.0),
	dug_item = 'CraftItem "lump_of_clay" 4',
})

minetest.register_node("brick", {
	tile_images = {"brick.png"},
	inventory_image = inventorycube("brick.png"),
	is_ground_content = true,
	material = digprop_stonelike(1.0),
	dug_item = 'CraftItem "clay_brick" 4',
})

minetest.register_node("tree", {
	tile_images = {"tree_top.png", "tree_top.png", "tree.png"},
	inventory_image = inventorycube("tree_top.png", "tree.png", "tree.png"),
	is_ground_content = true,
	material = digprop_woodlike(1.0),
	cookresult_item = 'CraftItem "lump_of_coal" 1',
	furnace_burntime = 30,
})

minetest.register_node("jungletree", {
	tile_images = {"jungletree_top.png", "jungletree_top.png", "jungletree.png"},
	inventory_image = inventorycube("jungletree_top.png", "jungletree.png", "jungletree.png"),
	is_ground_content = true,
	material = digprop_woodlike(1.0),
	cookresult_item = 'CraftItem "lump_of_coal" 1',
	furnace_burntime = 30,
})

minetest.register_node("junglegrass", {
	drawtype = "plantlike",
	visual_scale = 1.3,
	tile_images = {"junglegrass.png"},
	inventory_image = "junglegrass.png",
	light_propagates = true,
	paramtype = "light",
	walkable = false,
	material = digprop_leaveslike(1.0),
	furnace_burntime = 2,
})

minetest.register_node("leaves", {
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	tile_images = {"leaves.png"},
	inventory_image = "leaves.png",
	light_propagates = true,
	paramtype = "light",
	material = digprop_leaveslike(1.0),
	extra_dug_item = 'NodeItem "sapling" 1',
	extra_dug_item_rarity = 20,
	furnace_burntime = 1,
})

minetest.register_node("cactus", {
	tile_images = {"cactus_top.png", "cactus_top.png", "cactus_side.png"},
	inventory_image = inventorycube("cactus_top.png", "cactus_side.png", "cactus_side.png"),
	is_ground_content = true,
	material = digprop_woodlike(0.75),
	furnace_burntime = 15,
})

minetest.register_node("papyrus", {
	drawtype = "plantlike",
	tile_images = {"papyrus.png"},
	inventory_image = "papyrus.png",
	light_propagates = true,
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	material = digprop_leaveslike(0.5),
	furnace_burntime = 1,
})

minetest.register_node("bookshelf", {
	tile_images = {"wood.png", "wood.png", "bookshelf.png"},
	-- FIXME: inventorycube only cares for the first texture
	--inventory_image = inventorycube("wood.png", "bookshelf.png", "bookshelf.png")
	inventory_image = inventorycube("bookshelf.png"),
	is_ground_content = true,
	material = digprop_woodlike(0.75),
	furnace_burntime = 30,
})

minetest.register_node("glass", {
	drawtype = "glasslike",
	tile_images = {"glass.png"},
	inventory_image = inventorycube("glass.png"),
	light_propagates = true,
	paramtype = "light",
	sunlight_propagates = true,
	is_ground_content = true,
	material = digprop_glasslike(1.0),
})

minetest.register_node("wooden_fence", {
	drawtype = "fencelike",
	tile_images = {"wood.png"},
	inventory_image = "fence.png",
	light_propagates = true,
	paramtype = "light",
	is_ground_content = true,
	selection_box = {
		type = "fixed",
		fixed = {-1/7, -1/2, -1/7, 1/7, 1/2, 1/7},
	},
	furnace_burntime = 15,
	material = digprop_woodlike(0.75),
})

minetest.register_node("rail", {
	drawtype = "raillike",
	tile_images = {"rail.png", "rail_curved.png", "rail_t_junction.png", "rail_crossing.png"},
	inventory_image = "rail.png",
	light_propagates = true,
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
		--fixed = <default>
	},
	material = digprop_dirtlike(0.75),
})

minetest.register_node("ladder", {
	drawtype = "signlike",
	tile_images = {"ladder.png"},
	inventory_image = "ladder.png",
	light_propagates = true,
	paramtype = "light",
	is_ground_content = true,
	wall_mounted = true,
	walkable = false,
	climbable = true,
	selection_box = {
		type = "wallmounted",
		--wall_top = = <default>
		--wall_bottom = = <default>
		--wall_side = = <default>
	},
	furnace_burntime = 5,
	material = digprop_woodlike(0.5),
})

minetest.register_node("coalstone", {
	tile_images = {"stone.png^mineral_coal.png"},
	inventory_image = "stone.png^mineral_coal.png",
	is_ground_content = true,
	material = digprop_stonelike(1.5),
})

minetest.register_node("wood", {
	tile_images = {"wood.png"},
	inventory_image = inventorycube("wood.png"),
	is_ground_content = true,
	furnace_burntime = 7,
	material = digprop_woodlike(0.75),
})

minetest.register_node("mese", {
	tile_images = {"mese.png"},
	inventory_image = inventorycube("mese.png"),
	is_ground_content = true,
	furnace_burntime = 30,
	material = digprop_stonelike(0.5),
})

minetest.register_node("cloud", {
	tile_images = {"cloud.png"},
	inventory_image = inventorycube("cloud.png"),
	is_ground_content = true,
})

minetest.register_node("water_flowing", {
	drawtype = "flowingliquid",
	tile_images = {"water.png"},
	alpha = WATER_ALPHA,
	inventory_image = inventorycube("water.png"),
	paramtype = "light",
	light_propagates = true,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "flowing",
	liquid_alternative_flowing = "water_flowing",
	liquid_alternative_source = "water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	special_materials = {
		{image="water.png", backface_culling=false},
		{image="water.png", backface_culling=true},
	},
})

minetest.register_node("water_source", {
	drawtype = "liquid",
	tile_images = {"water.png"},
	alpha = WATER_ALPHA,
	inventory_image = inventorycube("water.png"),
	paramtype = "light",
	light_propagates = true,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "source",
	liquid_alternative_flowing = "water_flowing",
	liquid_alternative_source = "water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	special_materials = {
		-- New-style water source material (mostly unused)
		{image="water.png", backface_culling=false},
	},
})

minetest.register_node("lava_flowing", {
	drawtype = "flowingliquid",
	tile_images = {"lava.png"},
	inventory_image = inventorycube("lava.png"),
	paramtype = "light",
	light_propagates = false,
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "flowing",
	liquid_alternative_flowing = "lava_flowing",
	liquid_alternative_source = "lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		{image="lava.png", backface_culling=false},
		{image="lava.png", backface_culling=true},
	},
})

minetest.register_node("lava_source", {
	drawtype = "liquid",
	tile_images = {"lava.png"},
	inventory_image = inventorycube("lava.png"),
	paramtype = "light",
	light_propagates = false,
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "source",
	liquid_alternative_flowing = "lava_flowing",
	liquid_alternative_source = "lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		-- New-style lava source material (mostly unused)
		{image="lava.png", backface_culling=false},
	},
	furnace_burntime = 60,
})

minetest.register_node("torch", {
	drawtype = "torchlike",
	tile_images = {"torch_on_floor.png", "torch_on_ceiling.png", "torch.png"},
	inventory_image = "torch_on_floor.png",
	paramtype = "light",
	light_propagates = true,
	sunlight_propagates = true,
	walkable = false,
	wall_mounted = true,
	light_source = LIGHT_MAX-1,
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, 0.5-0.6, -0.1, 0.1, 0.5, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.5, -0.3, -0.1, -0.5+0.3, 0.3, 0.1},
	},
	material = digprop_constanttime(0.0),
	furnace_burntime = 4,
})

minetest.register_node("sign_wall", {
	drawtype = "signlike",
	tile_images = {"sign_wall.png"},
	inventory_image = "sign_wall.png",
	paramtype = "light",
	light_propagates = true,
	sunlight_propagates = true,
	walkable = false,
	wall_mounted = true,
	metadata_name = "sign",
	selection_box = {
		type = "wallmounted",
		--wall_top = <default>
		--wall_bottom = <default>
		--wall_side = <default>
	},
	material = digprop_constanttime(0.5),
	furnace_burntime = 10,
})

minetest.register_node("chest", {
	tile_images = {"chest_top.png", "chest_top.png", "chest_side.png",
		"chest_side.png", "chest_side.png", "chest_front.png"},
	inventory_image = "chest_top.png",
	--inventory_image = inventorycube("chest_top.png", "chest_side.png", "chest_front.png"),
	paramtype = "facedir_simple",
	metadata_name = "chest",
	material = digprop_woodlike(1.0),
	furnace_burntime = 30,
})

minetest.register_node("locked_chest", {
	tile_images = {"chest_top.png", "chest_top.png", "chest_side.png",
		"chest_side.png", "chest_side.png", "chest_lock.png"},
	inventory_image = "chest_lock.png",
	paramtype = "facedir_simple",
	metadata_name = "locked_chest",
	material = digprop_woodlike(1.0),
	furnace_burntime = 30,
})

minetest.register_node("furnace", {
	tile_images = {"furnace_side.png", "furnace_side.png", "furnace_side.png",
		"furnace_side.png", "furnace_side.png", "furnace_front.png"},
	inventory_image = "furnace_front.png",
	paramtype = "facedir_simple",
	metadata_name = "furnace",
	material = digprop_stonelike(3.0),
})

minetest.register_node("cobble", {
	tile_images = {"cobble.png"},
	inventory_image = inventorycube("cobble.png"),
	is_ground_content = true,
	cookresult_item = 'NodeItem "stone" 1',
	material = digprop_stonelike(0.9),
})

minetest.register_node("mossycobble", {
	tile_images = {"mossycobble.png"},
	inventory_image = inventorycube("mossycobble.png"),
	is_ground_content = true,
	material = digprop_stonelike(0.8),
})

minetest.register_node("steelblock", {
	tile_images = {"steel_block.png"},
	inventory_image = inventorycube("steel_block.png"),
	is_ground_content = true,
	material = digprop_stonelike(5.0),
})

minetest.register_node("nyancat", {
	tile_images = {"nc_side.png", "nc_side.png", "nc_side.png",
		"nc_side.png", "nc_back.png", "nc_front.png"},
	inventory_image = "nc_front.png",
	paramtype = "facedir_simple",
	material = digprop_stonelike(3.0),
	furnace_burntime = 1,
})

minetest.register_node("nyancat_rainbow", {
	tile_images = {"nc_rb.png"},
	inventory_image = "nc_rb.png",
	material = digprop_stonelike(3.0),
	furnace_burntime = 1,
})

minetest.register_node("sapling", {
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"sapling.png"},
	inventory_image = "sapling.png",
	paramtype = "light",
	light_propagates = true,
	walkable = false,
	material = digprop_constanttime(0.0),
	furnace_burntime = 10,
})

minetest.register_node("apple", {
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"apple.png"},
	inventory_image = "apple.png",
	paramtype = "light",
	light_propagates = true,
	sunlight_propagates = true,
	walkable = false,
	dug_item = 'CraftItem "apple" 1',
	material = digprop_constanttime(0.0),
	furnace_burntime = 3,
})

--
-- Crafting items
--

local craftitem_place_item = function(item, placer, pos)
	--print("craftitem_place_item")
	--print("item: " .. dump(item))
	--print("placer: " .. dump(placer))
	--print("pos: " .. dump(pos))
	minetest.env:add_item(pos, 'CraftItem "' .. item .. '" 1')
	return true
end

local craftitem_eat = function(hp_change)
	return function(item, user, pointed_thing)  -- closure
		--print("craftitem_eat(" .. hp_change .. ")")
		--print("item: " .. dump(item))
		--print("user: " .. dump(user))
		--print("pointed_thing: " .. dump(pointed_thing))
		user:set_hp(user:get_hp() + hp_change)
		return true
	end
end

minetest.register_craftitem("Stick", {
	image = "stick.png",
	--furnace_burntime = ...,
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("paper", {
	image = "paper.png",
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("book", {
	image = "book.png",
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("lump_of_coal", {
	image = "lump_of_coal.png",
	furnace_burntime = 40;
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("lump_of_iron", {
	image = "lump_of_iron.png",
	cookresult_item = 'CraftItem "steel_ingot" 1',
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("lump_of_clay", {
	image = "lump_of_clay.png",
	cookresult_item = 'CraftItem "clay_brick" 1',
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("steel_ingot", {
	image = "steel_ingot.png",
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("clay_brick", {
	image = "clay_brick.png",
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("rat", {
	image = "rat.png",
	cookresult_item = 'CraftItem "cooked_rat" 1',
	on_drop = function(item, dropper, pos)
		minetest.env:add_rat(pos)
		return true
	end,
})

minetest.register_craftitem("cooked_rat", {
	image = "cooked_rat.png",
	cookresult_item = 'CraftItem "scorched_stuff" 1',
	on_place_on_ground = craftitem_place_item,
	on_use = craftitem_eat(6),
})

minetest.register_craftitem("scorched_stuff", {
	image = "scorched_stuff.png",
	on_place_on_ground = craftitem_place_item,
})

minetest.register_craftitem("firefly", {
	image = "firefly.png",
	on_drop = function(item, dropper, pos)
		minetest.env:add_firefly(pos)
		return true
	end,
})

minetest.register_craftitem("apple", {
	image = "apple.png",
	on_place_on_ground = craftitem_place_item,
	on_use = craftitem_eat(4),
})

minetest.register_craftitem("apple_iron", {
	image = "apple_iron.png",
	on_place_on_ground = craftitem_place_item,
	on_use = craftitem_eat(8),
})

print(dump(minetest.registered_craftitems))


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
		--minetest.env:add_luaentity(p, "testentity")
		--minetest.env:add_firefly(p)
		nodeupdate(p)
	end
end
minetest.register_on_punchnode(on_punchnode)

minetest.register_on_chat_message(function(name, message)
	print("default on_chat_message: name="..dump(name).." message="..dump(message))
	local cmd = "/giveme"
	if message:sub(0, #cmd) == cmd then
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to give")
			return true -- Handled chat message
		end
		stackstring = string.match(message, cmd.." (.*)")
		if stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' stackstring')
			return true -- Handled chat message
		end
		print(cmd..' invoked, stackstring="'..stackstring..'"')
		player = minetest.env:get_player_by_name(name)
		added, error_msg = player:add_to_inventory(stackstring)
		if added then
			minetest.chat_send_player(name, '"'..stackstring
					..'" added to inventory.');
		else
			minetest.chat_send_player(name, 'Could not give "'..stackstring
					..'": '..error_msg);
		end
		return true -- Handled chat message
	end
	local cmd = "/give"
	if message:sub(0, #cmd) == cmd then
		print("minetest.get_player_privs(name)="
				..dump(minetest.get_player_privs(name)))
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to give")
			return true -- Handled chat message
		end
		name2, stackstring = string.match(message, cmd.." ([%a%d_-]+) (.*)")
		if name == nil or stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' name stackstring')
			return true -- Handled chat message
		end
		print(cmd..' invoked, name2="'..name2
				..'" stackstring="'..stackstring..'"')
		player = minetest.env:get_player_by_name(name2)
		if player == nil then
			minetest.chat_send_player(name, name2..' is not a known player')
			return true -- Handled chat message
		end
		added, error_msg = player:add_to_inventory(stackstring)
		if added then
			minetest.chat_send_player(name, '"'..stackstring
					..'" added to '..name2..'\'s inventory.');
			minetest.chat_send_player(name2, '"'..stackstring
					..'" added to inventory.');
		else
			minetest.chat_send_player(name, 'Could not give "'..stackstring
					..'": '..error_msg);
		end
		return true -- Handled chat message
	end
end)

--
-- Done, print some random stuff
--

--print("minetest.registered_entities:")
--dump2(minetest.registered_entities)

-- END
