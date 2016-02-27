-- default (Minetest 0.4 mod)
-- Most default stuff

-- The API documentation in here was moved into doc/lua_api.txt

WATER_ALPHA = 160
WATER_VISC = 1
LAVA_VISC = 7
LIGHT_MAX = 14

-- Definitions made by this mod that other mods can use too
default = {}

-- Load other files
dofile(minetest.get_modpath("default").."/mapgen.lua")

-- Set a noticeable inventory formspec for players
minetest.register_on_joinplayer(function(player)
	local cb = function(player)
		minetest.chat_send_player(player:get_player_name(), "This is the [minimal] \"Minimal Development Test\" game. Use [minetest_game] for the real thing.")
	end
	minetest.after(2.0, cb, player)
end)

--
-- Tool definition
--

-- The hand
minetest.register_item(":", {
	type = "none",
	wield_image = "wieldhand.png",
	wield_scale = {x=1,y=1,z=2.5},
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level = 0,
		groupcaps = {
			fleshy = {times={[2]=2.00, [3]=1.00}, uses=0, maxlevel=1},
			crumbly = {times={[2]=3.00, [3]=0.70}, uses=0, maxlevel=1},
			snappy = {times={[3]=0.40}, uses=0, maxlevel=1},
			oddly_breakable_by_hand = {times={[1]=7.00,[2]=4.00,[3]=1.40}, uses=0, maxlevel=3},
		}
	}
})

minetest.register_tool("default:pick_wood", {
	description = "Wooden Pickaxe",
	inventory_image = "default_tool_woodpick.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			cracky={times={[2]=2.00, [3]=1.20}, uses=10, maxlevel=1}
		}
	},
})
minetest.register_tool("default:pick_stone", {
	description = "Stone Pickaxe",
	inventory_image = "default_tool_stonepick.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			cracky={times={[1]=2.00, [2]=1.20, [3]=0.80}, uses=20, maxlevel=1}
		}
	},
})
minetest.register_tool("default:pick_steel", {
	description = "Steel Pickaxe",
	inventory_image = "default_tool_steelpick.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			cracky={times={[1]=4.00, [2]=1.60, [3]=1.00}, uses=10, maxlevel=2}
		}
	},
})
minetest.register_tool("default:pick_mese", {
	description = "Mese Pickaxe",
	inventory_image = "default_tool_mesepick.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=3,
		groupcaps={
			cracky={times={[1]=2.0, [2]=1.0, [3]=0.5}, uses=20, maxlevel=3},
			crumbly={times={[1]=2.0, [2]=1.0, [3]=0.5}, uses=20, maxlevel=3},
			snappy={times={[1]=2.0, [2]=1.0, [3]=0.5}, uses=20, maxlevel=3}
		}
	},
})
minetest.register_tool("default:shovel_wood", {
	description = "Wooden Shovel",
	inventory_image = "default_tool_woodshovel.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			crumbly={times={[1]=2.00, [2]=0.80, [3]=0.50}, uses=10, maxlevel=1}
		}
	},
})
minetest.register_tool("default:shovel_stone", {
	description = "Stone Shovel",
	inventory_image = "default_tool_stoneshovel.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			crumbly={times={[1]=1.20, [2]=0.50, [3]=0.30}, uses=20, maxlevel=1}
		}
	},
})
minetest.register_tool("default:shovel_steel", {
	description = "Steel Shovel",
	inventory_image = "default_tool_steelshovel.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			crumbly={times={[1]=1.00, [2]=0.70, [3]=0.60}, uses=10, maxlevel=2}
		}
	},
})
minetest.register_tool("default:axe_wood", {
	description = "Wooden Axe",
	inventory_image = "default_tool_woodaxe.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			choppy={times={[2]=1.40, [3]=0.80}, uses=10, maxlevel=1},
			fleshy={times={[2]=1.50, [3]=0.80}, uses=10, maxlevel=1}
		}
	},
})
minetest.register_tool("default:axe_stone", {
	description = "Stone Axe",
	inventory_image = "default_tool_stoneaxe.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			choppy={times={[1]=1.50, [2]=1.00, [3]=0.60}, uses=20, maxlevel=1},
			fleshy={times={[2]=1.30, [3]=0.70}, uses=20, maxlevel=1}
		}
	},
})
minetest.register_tool("default:axe_steel", {
	description = "Steel Axe",
	inventory_image = "default_tool_steelaxe.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			choppy={times={[1]=2.00, [2]=1.60, [3]=1.00}, uses=10, maxlevel=2},
			fleshy={times={[2]=1.10, [3]=0.60}, uses=40, maxlevel=1}
		}
	},
})
minetest.register_tool("default:sword_wood", {
	description = "Wooden Sword",
	inventory_image = "default_tool_woodsword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=0,
		groupcaps={
			fleshy={times={[2]=1.10, [3]=0.60}, uses=10, maxlevel=1},
			snappy={times={[2]=1.00, [3]=0.50}, uses=10, maxlevel=1},
			choppy={times={[3]=1.00}, uses=20, maxlevel=0}
		}
	}
})
minetest.register_tool("default:sword_stone", {
	description = "Stone Sword",
	inventory_image = "default_tool_stonesword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=0,
		groupcaps={
			fleshy={times={[2]=0.80, [3]=0.40}, uses=20, maxlevel=1},
			snappy={times={[2]=0.80, [3]=0.40}, uses=20, maxlevel=1},
			choppy={times={[3]=0.90}, uses=20, maxlevel=0}
		}
	}
})
minetest.register_tool("default:sword_steel", {
	description = "Steel Sword",
	inventory_image = "default_tool_steelsword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=1,
		groupcaps={
			fleshy={times={[1]=2.00, [2]=0.80, [3]=0.40}, uses=10, maxlevel=2},
			snappy={times={[2]=0.70, [3]=0.30}, uses=40, maxlevel=1},
			choppy={times={[3]=0.70}, uses=40, maxlevel=0}
		}
	}
})

--
-- Crafting definition
--

minetest.register_craft({
	output = 'default:wood 4',
	recipe = {
		{'default:tree'},
	}
})

minetest.register_craft({
	output = 'default:stick 4',
	recipe = {
		{'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:fence_wood 2',
	recipe = {
		{'default:stick', 'default:stick', 'default:stick'},
		{'default:stick', 'default:stick', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sign_wall',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:wood', 'default:wood', 'default:wood'},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:torch 4',
	recipe = {
		{'default:coal_lump'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:pick_wood',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:pick_stone',
	recipe = {
		{'default:cobble', 'default:cobble', 'default:cobble'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:pick_steel',
	recipe = {
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:pick_mese',
	recipe = {
		{'default:mese', 'default:mese', 'default:mese'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:shovel_wood',
	recipe = {
		{'default:wood'},
		{'default:stick'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:shovel_stone',
	recipe = {
		{'default:cobble'},
		{'default:stick'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:shovel_steel',
	recipe = {
		{'default:steel_ingot'},
		{'default:stick'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:axe_wood',
	recipe = {
		{'default:wood', 'default:wood'},
		{'default:wood', 'default:stick'},
		{'', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:axe_stone',
	recipe = {
		{'default:cobble', 'default:cobble'},
		{'default:cobble', 'default:stick'},
		{'', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:axe_steel',
	recipe = {
		{'default:steel_ingot', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:stick'},
		{'', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sword_wood',
	recipe = {
		{'default:wood'},
		{'default:wood'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sword_stone',
	recipe = {
		{'default:cobble'},
		{'default:cobble'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sword_steel',
	recipe = {
		{'default:steel_ingot'},
		{'default:steel_ingot'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:rail 15',
	recipe = {
		{'default:steel_ingot', '', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:stick', 'default:steel_ingot'},
		{'default:steel_ingot', '', 'default:steel_ingot'},
	}
})

minetest.register_craft({
	output = 'default:chest',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:wood', '', 'default:wood'},
		{'default:wood', 'default:wood', 'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:chest_locked',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:wood', 'default:steel_ingot', 'default:wood'},
		{'default:wood', 'default:wood', 'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:furnace',
	recipe = {
		{'default:cobble', 'default:cobble', 'default:cobble'},
		{'default:cobble', '', 'default:cobble'},
		{'default:cobble', 'default:cobble', 'default:cobble'},
	}
})

minetest.register_craft({
	output = 'default:steelblock',
	recipe = {
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
	}
})

minetest.register_craft({
	output = 'default:sandstone',
	recipe = {
		{'default:sand', 'default:sand'},
		{'default:sand', 'default:sand'},
	}
})

minetest.register_craft({
	output = 'default:clay',
	recipe = {
		{'default:clay_lump', 'default:clay_lump'},
		{'default:clay_lump', 'default:clay_lump'},
	}
})

minetest.register_craft({
	output = 'default:brick',
	recipe = {
		{'default:clay_brick', 'default:clay_brick'},
		{'default:clay_brick', 'default:clay_brick'},
	}
})

minetest.register_craft({
	output = 'default:paper',
	recipe = {
		{'default:papyrus', 'default:papyrus', 'default:papyrus'},
	}
})

minetest.register_craft({
	output = 'default:book',
	recipe = {
		{'default:paper'},
		{'default:paper'},
		{'default:paper'},
	}
})

minetest.register_craft({
	output = 'default:bookshelf',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:book', 'default:book', 'default:book'},
		{'default:wood', 'default:wood', 'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:ladder',
	recipe = {
		{'default:stick', '', 'default:stick'},
		{'default:stick', 'default:stick', 'default:stick'},
		{'default:stick', '', 'default:stick'},
	}
})

-- Tool repair
minetest.register_craft({
	type = "toolrepair",
	additional_wear = -0.02,
})

--
-- Cooking recipes
--

minetest.register_craft({
	type = "cooking",
	output = "default:glass",
	recipe = "default:sand",
})

minetest.register_craft({
	type = "cooking",
	output = "default:coal_lump",
	recipe = "default:tree",
})

minetest.register_craft({
	type = "cooking",
	output = "default:stone",
	recipe = "default:cobble",
})

minetest.register_craft({
	type = "cooking",
	output = "default:steel_ingot",
	recipe = "default:iron_lump",
})

minetest.register_craft({
	type = "cooking",
	output = "default:clay_brick",
	recipe = "default:clay_lump",
})

--
-- Fuels
--

minetest.register_craft({
	type = "fuel",
	recipe = "default:tree",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:junglegrass",
	burntime = 2,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:leaves",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:cactus",
	burntime = 15,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:papyrus",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:bookshelf",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:fence_wood",
	burntime = 15,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:ladder",
	burntime = 5,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:wood",
	burntime = 7,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:mese",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:lava_source",
	burntime = 60,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:torch",
	burntime = 4,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:sign_wall",
	burntime = 10,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:chest",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:chest_locked",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:nyancat",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:nyancat_rainbow",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:sapling",
	burntime = 10,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:apple",
	burntime = 3,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:coal_lump",
	burntime = 40,
})

--
-- Node definitions
--

-- Default node sounds

function default.node_sound_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="", gain=1.0}
	table.dug = table.dug or
			{name="default_dug_node", gain=1.0}
	return table
end

function default.node_sound_stone_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="default_hard_footstep", gain=0.2}
	default.node_sound_defaults(table)
	return table
end

function default.node_sound_dirt_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="", gain=0.5}
	--table.dug = table.dug or
	--		{name="default_dirt_break", gain=0.5}
	table.place = table.place or
			{name="default_grass_footstep", gain=0.5}
	default.node_sound_defaults(table)
	return table
end

function default.node_sound_sand_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="default_grass_footstep", gain=0.25}
	--table.dug = table.dug or
	--		{name="default_dirt_break", gain=0.25}
	table.dug = table.dug or
			{name="", gain=0.25}
	default.node_sound_defaults(table)
	return table
end

function default.node_sound_wood_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="default_hard_footstep", gain=0.3}
	default.node_sound_defaults(table)
	return table
end

function default.node_sound_leaves_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="default_grass_footstep", gain=0.25}
	table.dig = table.dig or
			{name="default_dig_crumbly", gain=0.4}
	table.dug = table.dug or
			{name="", gain=1.0}
	default.node_sound_defaults(table)
	return table
end

function default.node_sound_glass_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="default_stone_footstep", gain=0.25}
	table.dug = table.dug or
			{name="default_break_glass", gain=1.0}
	default.node_sound_defaults(table)
	return table
end

-- Register nodes

minetest.register_node("default:stone", {
	description = "Stone",
	tiles ={"default_stone.png"},
	groups = {cracky=3},
	drop = 'default:cobble',
	legacy_mineral = true,
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:stone_with_coal", {
	description = "Stone with coal",
	tiles ={"default_stone.png^default_mineral_coal.png"},
	groups = {cracky=3},
	drop = 'default:coal_lump',
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:stone_with_iron", {
	description = "Stone with iron",
	tiles ={"default_stone.png^default_mineral_iron.png"},
	groups = {cracky=3},
	drop = 'default:iron_lump',
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:dirt_with_grass", {
	description = "Dirt with grass",
	tiles ={"default_grass.png", "default_dirt.png",
		{name = "default_dirt.png^default_grass_side.png",
		tileable_vertical = false}},
	groups = {crumbly=3, soil=1},
	drop = 'default:dirt',
	sounds = default.node_sound_dirt_defaults({
		footstep = {name="default_grass_footstep", gain=0.4},
	}),
})

minetest.register_node("default:dirt_with_grass_footsteps", {
	description = "Dirt with grass and footsteps",
	tiles ={"default_grass_footsteps.png", "default_dirt.png",
		{name = "default_dirt.png^default_grass_side.png",
		tileable_vertical = false}},
	groups = {crumbly=3, soil=1},
	drop = 'default:dirt',
	sounds = default.node_sound_dirt_defaults({
		footstep = {name="default_grass_footstep", gain=0.4},
	}),
})

minetest.register_node("default:dirt", {
	description = "Dirt",
	tiles ={"default_dirt.png"},
	groups = {crumbly=3, soil=1},
	sounds = default.node_sound_dirt_defaults(),
})

minetest.register_node("default:sand", {
	description = "Sand",
	tiles ={"default_sand.png"},
	groups = {crumbly=3, falling_node=1},
	sounds = default.node_sound_sand_defaults(),
})

minetest.register_node("default:gravel", {
	description = "Gravel",
	tiles ={"default_gravel.png"},
	groups = {crumbly=2, falling_node=1},
	sounds = default.node_sound_dirt_defaults({
		footstep = {name="default_gravel_footstep", gain=0.45},
	}),
})

minetest.register_node("default:sandstone", {
	description = "Sandstone",
	tiles ={"default_sandstone.png"},
	groups = {crumbly=2,cracky=2},
	drop = 'default:sand',
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:clay", {
	description = "Clay",
	tiles ={"default_clay.png"},
	groups = {crumbly=3},
	drop = 'default:clay_lump 4',
	sounds = default.node_sound_dirt_defaults({
		footstep = "",
	}),
})

minetest.register_node("default:brick", {
	description = "Brick",
	tiles ={"default_brick.png"},
	is_ground_content = false,
	groups = {cracky=3},
	drop = 'default:clay_brick 4',
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:tree", {
	description = "Tree",
	tiles ={"default_tree_top.png", "default_tree_top.png", "default_tree.png"},
	is_ground_content = false,
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=1},
	sounds = default.node_sound_wood_defaults(),
})

minetest.register_node("default:junglegrass", {
	description = "Jungle Grass",
	drawtype = "plantlike",
	visual_scale = 1.3,
	tiles ={"default_junglegrass.png"},
	inventory_image = "default_junglegrass.png",
	wield_image = "default_junglegrass.png",
	paramtype = "light",
	walkable = false,
	groups = {snappy=3,attached_node=1},
	sounds = default.node_sound_leaves_defaults(),
})

minetest.register_node("default:leaves", {
	description = "Leaves",
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	tiles ={"default_leaves.png"},
	paramtype = "light",
	is_ground_content = false,
	groups = {snappy=3},
	drop = {
		max_items = 1,
		items = {
			{
				-- player will get sapling with 1/20 chance
				items = {'default:sapling'},
				rarity = 20,
			},
			{
				-- player will get leaves only if he get no saplings,
				-- this is because max_items is 1
				items = {'default:leaves'},
			}
		}
	},
	sounds = default.node_sound_leaves_defaults(),
})

minetest.register_node("default:cactus", {
	description = "Cactus",
	tiles ={"default_cactus_top.png", "default_cactus_top.png", "default_cactus_side.png"},
	groups = {snappy=2,choppy=3},
	sounds = default.node_sound_wood_defaults(),
})

minetest.register_node("default:papyrus", {
	description = "Papyrus",
	drawtype = "plantlike",
	tiles ={"default_papyrus.png"},
	inventory_image = "default_papyrus.png",
	wield_image = "default_papyrus.png",
	paramtype = "light",
	walkable = false,
	groups = {snappy=3},
	sounds = default.node_sound_leaves_defaults(),
})

minetest.register_node("default:bookshelf", {
	description = "Bookshelf",
	tiles ={"default_wood.png", "default_wood.png", "default_bookshelf.png"},
	is_ground_content = false,
	groups = {snappy=2,choppy=3,oddly_breakable_by_hand=2},
	sounds = default.node_sound_wood_defaults(),
})

minetest.register_node("default:glass", {
	description = "Glass",
	drawtype = "glasslike",
	tiles ={"default_glass.png"},
	paramtype = "light",
	is_ground_content = false,
	sunlight_propagates = true,
	groups = {snappy=2,cracky=3,oddly_breakable_by_hand=3},
	sounds = default.node_sound_glass_defaults(),
})

minetest.register_node("default:fence_wood", {
	description = "Wooden Fence",
	drawtype = "fencelike",
	tiles ={"default_wood.png"},
	inventory_image = "default_fence.png",
	wield_image = "default_fence.png",
	paramtype = "light",
	is_ground_content = false,
	selection_box = {
		type = "fixed",
		fixed = {-1/7, -1/2, -1/7, 1/7, 1/2, 1/7},
	},
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=2},
	sounds = default.node_sound_wood_defaults(),
})

minetest.register_node("default:rail", {
	description = "Rail",
	drawtype = "raillike",
	tiles ={"default_rail.png", "default_rail_curved.png", "default_rail_t_junction.png", "default_rail_crossing.png"},
	inventory_image = "default_rail.png",
	wield_image = "default_rail.png",
	paramtype = "light",
	is_ground_content = false,
	walkable = false,
	selection_box = {
		type = "fixed",
		fixed = {-1/2, -1/2, -1/2, 1/2, -1/2+1/16, 1/2},
	},
	groups = {bendy=2,snappy=1,dig_immediate=2},
})

minetest.register_node("default:ladder", {
	description = "Ladder",
	drawtype = "signlike",
	tiles ={"default_ladder.png"},
	inventory_image = "default_ladder.png",
	wield_image = "default_ladder.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	is_ground_content = false,
	walkable = false,
	climbable = true,
	selection_box = {
		type = "wallmounted",
		--wall_top = = <default>
		--wall_bottom = = <default>
		--wall_side = = <default>
	},
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=3},
	legacy_wallmounted = true,
	sounds = default.node_sound_wood_defaults(),
})

minetest.register_node("default:wood", {
	description = "Wood",
	tiles ={"default_wood.png"},
	is_ground_content = false,
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=2},
	sounds = default.node_sound_wood_defaults(),
})

minetest.register_node("default:mese", {
	description = "Mese",
	tiles ={"default_mese.png"},
	groups = {cracky=1,level=2},
	sounds = default.node_sound_defaults(),
})

minetest.register_node("default:cloud", {
	description = "Cloud",
	tiles ={"default_cloud.png"},
	is_ground_content = false,
	sounds = default.node_sound_defaults(),
})

minetest.register_node("default:water_flowing", {
	description = "Water (flowing)",
	inventory_image = minetest.inventorycube("default_water.png"),
	drawtype = "flowingliquid",
	tiles ={"default_water.png"},
	special_tiles = {
		{name="default_water.png", backface_culling=false},
		{name="default_water.png", backface_culling=true},
	},
	alpha = WATER_ALPHA,
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	is_ground_content = false,
	drowning = 1,
	liquidtype = "flowing",
	liquid_alternative_flowing = "default:water_flowing",
	liquid_alternative_source = "default:water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	groups = {water=3, liquid=3},
})

minetest.register_node("default:water_source", {
	description = "Water",
	inventory_image = minetest.inventorycube("default_water.png"),
	drawtype = "liquid",
	tiles ={"default_water.png"},
	special_tiles = {
		-- New-style water source material (mostly unused)
		{name="default_water.png", backface_culling=false},
	},
	alpha = WATER_ALPHA,
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	is_ground_content = false,
	drowning = 1,
	liquidtype = "source",
	liquid_alternative_flowing = "default:water_flowing",
	liquid_alternative_source = "default:water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	groups = {water=3, liquid=3},
})

minetest.register_node("default:lava_flowing", {
	description = "Lava (flowing)",
	inventory_image = minetest.inventorycube("default_lava.png"),
	drawtype = "flowingliquid",
	tiles ={"default_lava.png"},
	special_tiles = {
		{
			image="default_lava_flowing_animated.png",
			backface_culling=false,
			animation={type="vertical_frames", aspect_w=16, aspect_h=16, length=3.3}
		},
		{
			image="default_lava_flowing_animated.png",
			backface_culling=true,
			animation={type="vertical_frames", aspect_w=16, aspect_h=16, length=3.3}
		},
	},
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	is_ground_content = false,
	drowning = 1,
	liquidtype = "flowing",
	liquid_alternative_flowing = "default:lava_flowing",
	liquid_alternative_source = "default:lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	groups = {lava=3, liquid=2, hot=3},
})

minetest.register_node("default:lava_source", {
	description = "Lava",
	inventory_image = minetest.inventorycube("default_lava.png"),
	drawtype = "liquid",
	--tiles ={"default_lava.png"},
	tiles ={
		{name="default_lava_source_animated.png", animation={type="vertical_frames", aspect_w=16, aspect_h=16, length=3.0}}
	},
	special_tiles = {
		-- New-style lava source material (mostly unused)
		{name="default_lava.png", backface_culling=false},
	},
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	is_ground_content = false,
	drowning = 1,
	liquidtype = "source",
	liquid_alternative_flowing = "default:lava_flowing",
	liquid_alternative_source = "default:lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	groups = {lava=3, liquid=2, hot=3},
})

minetest.register_node("default:torch", {
	description = "Torch",
	drawtype = "torchlike",
	tiles ={"default_torch_on_floor.png", "default_torch_on_ceiling.png", "default_torch.png"},
	inventory_image = "default_torch_on_floor.png",
	wield_image = "default_torch_on_floor.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	sunlight_propagates = true,
	is_ground_content = false,
	walkable = false,
	light_source = LIGHT_MAX-1,
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, 0.5-0.6, -0.1, 0.1, 0.5, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.5, -0.3, -0.1, -0.5+0.3, 0.3, 0.1},
	},
	groups = {choppy=2,dig_immediate=3,attached_node=1},
	legacy_wallmounted = true,
	sounds = default.node_sound_defaults(),
})

minetest.register_node("default:sign_wall", {
	description = "Sign",
	drawtype = "signlike",
	tiles ={"default_sign_wall.png"},
	inventory_image = "default_sign_wall.png",
	wield_image = "default_sign_wall.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	sunlight_propagates = true,
	is_ground_content = false,
	walkable = false,
	selection_box = {
		type = "wallmounted",
		--wall_top = <default>
		--wall_bottom = <default>
		--wall_side = <default>
	},
	groups = {choppy=2,dig_immediate=2,attached_node=1},
	legacy_wallmounted = true,
	sounds = default.node_sound_defaults(),
	on_construct = function(pos)
		--local n = minetest.get_node(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("formspec", "field[text;;${text}]")
		meta:set_string("infotext", "\"\"")
	end,
	on_receive_fields = function(pos, formname, fields, sender)
		--print("Sign at "..minetest.pos_to_string(pos).." got "..dump(fields))
		local meta = minetest.get_meta(pos)
		fields.text = fields.text or ""
		print((sender:get_player_name() or "").." wrote \""..fields.text..
				"\" to sign at "..minetest.pos_to_string(pos))
		meta:set_string("text", fields.text)
		meta:set_string("infotext", '"'..fields.text..'"')
	end,
})

minetest.register_node("default:chest", {
	description = "Chest",
	tiles ={"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_front.png"},
	paramtype2 = "facedir",
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=2},
	legacy_facedir_simple = true,
	is_ground_content = false,
	sounds = default.node_sound_wood_defaults(),
	on_construct = function(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("formspec",
				"size[8,9]"..
				"list[current_name;main;0,0;8,4;]"..
				"list[current_player;main;0,5;8,4;]" ..
				"listring[]")
		meta:set_string("infotext", "Chest")
		local inv = meta:get_inventory()
		inv:set_size("main", 8*4)
	end,
	can_dig = function(pos,player)
		local meta = minetest.get_meta(pos);
		local inv = meta:get_inventory()
		return inv:is_empty("main")
	end,
})

local function has_locked_chest_privilege(meta, player)
	if player:get_player_name() ~= meta:get_string("owner") then
		return false
	end
	return true
end

minetest.register_node("default:chest_locked", {
	description = "Locked Chest",
	tiles ={"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_lock.png"},
	paramtype2 = "facedir",
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=2},
	legacy_facedir_simple = true,
	is_ground_content = false,
	sounds = default.node_sound_wood_defaults(),
	after_place_node = function(pos, placer)
		local meta = minetest.get_meta(pos)
		meta:set_string("owner", placer:get_player_name() or "")
		meta:set_string("infotext", "Locked Chest (owned by "..
				meta:get_string("owner")..")")
	end,
	on_construct = function(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("formspec",
				"size[8,9]"..
				"list[current_name;main;0,0;8,4;]"..
				"list[current_player;main;0,5;8,4;]" ..
				"listring[]")
		meta:set_string("infotext", "Locked Chest")
		meta:set_string("owner", "")
		local inv = meta:get_inventory()
		inv:set_size("main", 8*4)
	end,
	can_dig = function(pos,player)
		local meta = minetest.get_meta(pos);
		local inv = meta:get_inventory()
		return inv:is_empty("main")
	end,
	allow_metadata_inventory_move = function(pos, from_list, from_index, to_list, to_index, count, player)
		local meta = minetest.get_meta(pos)
		if not has_locked_chest_privilege(meta, player) then
			minetest.log("action", player:get_player_name()..
					" tried to access a locked chest belonging to "..
					meta:get_string("owner").." at "..
					minetest.pos_to_string(pos))
			return 0
		end
		return count
	end,
    allow_metadata_inventory_put = function(pos, listname, index, stack, player)
		local meta = minetest.get_meta(pos)
		if not has_locked_chest_privilege(meta, player) then
			minetest.log("action", player:get_player_name()..
					" tried to access a locked chest belonging to "..
					meta:get_string("owner").." at "..
					minetest.pos_to_string(pos))
			return 0
		end
		return stack:get_count()
	end,
    allow_metadata_inventory_take = function(pos, listname, index, stack, player)
		local meta = minetest.get_meta(pos)
		if not has_locked_chest_privilege(meta, player) then
			minetest.log("action", player:get_player_name()..
					" tried to access a locked chest belonging to "..
					meta:get_string("owner").." at "..
					minetest.pos_to_string(pos))
			return 0
		end
		return stack:get_count()
	end,
	on_metadata_inventory_move = function(pos, from_list, from_index, to_list, to_index, count, player)
		minetest.log("action", player:get_player_name()..
				" moves stuff in locked chest at "..minetest.pos_to_string(pos))
	end,
    on_metadata_inventory_put = function(pos, listname, index, stack, player)
		minetest.log("action", player:get_player_name()..
				" moves stuff to locked chest at "..minetest.pos_to_string(pos))
	end,
    on_metadata_inventory_take = function(pos, listname, index, stack, player)
		minetest.log("action", player:get_player_name()..
				" takes stuff from locked chest at "..minetest.pos_to_string(pos))
	end,
})

default.furnace_inactive_formspec =
	"size[8,9]"..
	"image[2,2;1,1;default_furnace_fire_bg.png]"..
	"list[current_name;fuel;2,3;1,1;]"..
	"list[current_name;src;2,1;1,1;]"..
	"list[current_name;dst;5,1;2,2;]"..
	"list[current_player;main;0,5;8,4;]" ..
	"listring[current_name;dst]" ..
	"listring[current_player;main]" ..
	"listring[current_name;src]" ..
	"listring[current_player;main]"

minetest.register_node("default:furnace", {
	description = "Furnace",
	tiles ={"default_furnace_side.png", "default_furnace_side.png", "default_furnace_side.png",
		"default_furnace_side.png", "default_furnace_side.png", "default_furnace_front.png"},
	paramtype2 = "facedir",
	groups = {cracky=2},
	legacy_facedir_simple = true,
	is_ground_content = false,
	sounds = default.node_sound_stone_defaults(),
	on_construct = function(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("formspec", default.furnace_inactive_formspec)
		meta:set_string("infotext", "Furnace")
		local inv = meta:get_inventory()
		inv:set_size("fuel", 1)
		inv:set_size("src", 1)
		inv:set_size("dst", 4)
	end,
	can_dig = function(pos,player)
		local meta = minetest.get_meta(pos);
		local inv = meta:get_inventory()
		if not inv:is_empty("fuel") then
			return false
		elseif not inv:is_empty("dst") then
			return false
		elseif not inv:is_empty("src") then
			return false
		end
		return true
	end,
})

minetest.register_node("default:furnace_active", {
	description = "Furnace",
	tiles ={"default_furnace_side.png", "default_furnace_side.png", "default_furnace_side.png",
		"default_furnace_side.png", "default_furnace_side.png", "default_furnace_front_active.png"},
	paramtype2 = "facedir",
	light_source = 8,
	drop = "default:furnace",
	groups = {cracky=2},
	legacy_facedir_simple = true,
	is_ground_content = false,
	sounds = default.node_sound_stone_defaults(),
	on_construct = function(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("formspec", default.furnace_inactive_formspec)
		meta:set_string("infotext", "Furnace");
		local inv = meta:get_inventory()
		inv:set_size("fuel", 1)
		inv:set_size("src", 1)
		inv:set_size("dst", 4)
	end,
	can_dig = function(pos,player)
		local meta = minetest.get_meta(pos);
		local inv = meta:get_inventory()
		if not inv:is_empty("fuel") then
			return false
		elseif not inv:is_empty("dst") then
			return false
		elseif not inv:is_empty("src") then
			return false
		end
		return true
	end,
})

function swap_node(pos,name)
	local node = minetest.get_node(pos)
	if node.name == name then
		return
	end
	node.name = name
	minetest.swap_node(pos, node)
end

minetest.register_abm({
	nodenames = {"default:furnace","default:furnace_active"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local meta = minetest.get_meta(pos)
		for i, name in ipairs({
				"fuel_totaltime",
				"fuel_time",
				"src_totaltime",
				"src_time"
		}) do
			if meta:get_string(name) == "" then
				meta:set_float(name, 0.0)
			end
		end

		local inv = meta:get_inventory()

		local srclist = inv:get_list("src")
		local cooked = nil
		
		if srclist then
			cooked = minetest.get_craft_result({method = "cooking", width = 1, items = srclist})
		end
		
		local was_active = false
		
		if meta:get_float("fuel_time") < meta:get_float("fuel_totaltime") then
			was_active = true
			meta:set_float("fuel_time", meta:get_float("fuel_time") + 1)
			meta:set_float("src_time", meta:get_float("src_time") + 1)
			if cooked and cooked.item and meta:get_float("src_time") >= cooked.time then
				-- check if there's room for output in "dst" list
				if inv:room_for_item("dst",cooked.item) then
					-- Put result in "dst" list
					inv:add_item("dst", cooked.item)
					-- take stuff from "src" list
					srcstack = inv:get_stack("src", 1)
					srcstack:take_item()
					inv:set_stack("src", 1, srcstack)
				else
					print("Could not insert '"..cooked.item:to_string().."'")
				end
				meta:set_string("src_time", 0)
			end
		end
		
		if meta:get_float("fuel_time") < meta:get_float("fuel_totaltime") then
			local percent = math.floor(meta:get_float("fuel_time") /
					meta:get_float("fuel_totaltime") * 100)
			meta:set_string("infotext","Furnace active: "..percent.."%")
			swap_node(pos,"default:furnace_active")
			meta:set_string("formspec",
				"size[8,9]"..
				"image[2,2;1,1;default_furnace_fire_bg.png^[lowpart:"..
						(100-percent)..":default_furnace_fire_fg.png]"..
				"list[current_name;fuel;2,3;1,1;]"..
				"list[current_name;src;2,1;1,1;]"..
				"list[current_name;dst;5,1;2,2;]"..
				"list[current_player;main;0,5;8,4;]" ..
				"listring[current_name;dst]" ..
				"listring[current_player;main]" ..
				"listring[current_name;src]" ..
				"listring[current_player;main]")
			return
		end

		local fuel = nil
		local cooked = nil
		local fuellist = inv:get_list("fuel")
		local srclist = inv:get_list("src")
		
		if srclist then
			cooked = minetest.get_craft_result({method = "cooking", width = 1, items = srclist})
		end
		if fuellist then
			fuel = minetest.get_craft_result({method = "fuel", width = 1, items = fuellist})
		end

		if fuel.time <= 0 then
			meta:set_string("infotext","Furnace out of fuel")
			swap_node(pos,"default:furnace")
			meta:set_string("formspec", default.furnace_inactive_formspec)
			return
		end

		if cooked.item:is_empty() then
			if was_active then
				meta:set_string("infotext","Furnace is empty")
				swap_node(pos,"default:furnace")
				meta:set_string("formspec", default.furnace_inactive_formspec)
			end
			return
		end

		meta:set_string("fuel_totaltime", fuel.time)
		meta:set_string("fuel_time", 0)
		
		local stack = inv:get_stack("fuel", 1)
		stack:take_item()
		inv:set_stack("fuel", 1, stack)
	end,
})

minetest.register_node("default:cobble", {
	description = "Cobble",
	tiles ={"default_cobble.png"},
	is_ground_content = false,
	groups = {cracky=3},
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:mossycobble", {
	description = "Mossy Cobble",
	tiles ={"default_mossycobble.png"},
	is_ground_content = false,
	groups = {cracky=3},
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:steelblock", {
	description = "Steel Block",
	tiles ={"default_steel_block.png"},
	is_ground_content = false,
	groups = {snappy=1,bendy=2},
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("default:nyancat", {
	description = "Nyancat",
	tiles ={"default_nc_side.png", "default_nc_side.png", "default_nc_side.png",
		"default_nc_side.png", "default_nc_back.png", "default_nc_front.png"},
	inventory_image = "default_nc_front.png",
	paramtype2 = "facedir",
	groups = {cracky=2},
	legacy_facedir_simple = true,
	is_ground_content = false,
	sounds = default.node_sound_defaults(),
})

minetest.register_node("default:nyancat_rainbow", {
	description = "Nyancat Rainbow",
	tiles ={"default_nc_rb.png"},
	inventory_image = "default_nc_rb.png",
	is_ground_content = false,
	groups = {cracky=2},
	sounds = default.node_sound_defaults(),
})

minetest.register_node("default:sapling", {
	description = "Sapling",
	drawtype = "plantlike",
	visual_scale = 1.0,
	tiles ={"default_sapling.png"},
	inventory_image = "default_sapling.png",
	wield_image = "default_sapling.png",
	paramtype = "light",
	walkable = false,
	groups = {snappy=2,dig_immediate=3,attached_node=1},
	sounds = default.node_sound_defaults(),
})

minetest.register_node("default:apple", {
	description = "Apple",
	drawtype = "plantlike",
	visual_scale = 1.0,
	tiles ={"default_apple.png"},
	inventory_image = "default_apple.png",
	paramtype = "light",
	is_ground_content = false,
	sunlight_propagates = true,
	walkable = false,
	groups = {fleshy=3,dig_immediate=3},
	on_use = minetest.item_eat(4),
	sounds = default.node_sound_defaults(),
})

--
-- Grow tree function
--

local c_air = minetest.get_content_id("air")
local c_ignore = minetest.get_content_id("ignore")
local c_tree = minetest.get_content_id("default:tree")
local c_leaves = minetest.get_content_id("default:leaves")
local c_apple = minetest.get_content_id("default:apple")
function default.grow_tree(data, a, pos, is_apple_tree, seed)
	--[[
		NOTE: Tree-placing code is currently duplicated in the engine
		and in games that have saplings; both are deprecated but not
		replaced yet
	]]--
	local pr = PseudoRandom(seed)
	local th = pr:next(4, 5)
	local x, y, z = pos.x, pos.y, pos.z
	for yy = y, y+th-1 do
		local vi = a:index(x, yy, z)
		if a:contains(x, yy, z) and (data[vi] == c_air or yy == y) then
			data[vi] = c_tree
		end
	end
	y = y+th-1 -- (x, y, z) is now last piece of trunk
	local leaves_a = VoxelArea:new{MinEdge={x=-2, y=-1, z=-2}, MaxEdge={x=2, y=2, z=2}}
	local leaves_buffer = {}
	
	-- Force leaves near the trunk
	local d = 1
	for xi = -d, d do
	for yi = -d, d do
	for zi = -d, d do
		leaves_buffer[leaves_a:index(xi, yi, zi)] = true
	end
	end
	end
	
	-- Add leaves randomly
	for iii = 1, 8 do
		local d = 1
		local xx = pr:next(leaves_a.MinEdge.x, leaves_a.MaxEdge.x - d)
		local yy = pr:next(leaves_a.MinEdge.y, leaves_a.MaxEdge.y - d)
		local zz = pr:next(leaves_a.MinEdge.z, leaves_a.MaxEdge.z - d)
		
		for xi = 0, d do
		for yi = 0, d do
		for zi = 0, d do
			leaves_buffer[leaves_a:index(xx+xi, yy+yi, zz+zi)] = true
		end
		end
		end
	end
	
	-- Add the leaves
	for xi = leaves_a.MinEdge.x, leaves_a.MaxEdge.x do
	for yi = leaves_a.MinEdge.y, leaves_a.MaxEdge.y do
	for zi = leaves_a.MinEdge.z, leaves_a.MaxEdge.z do
		if a:contains(x+xi, y+yi, z+zi) then
			local vi = a:index(x+xi, y+yi, z+zi)
			if data[vi] == c_air or data[vi] == c_ignore then
				if leaves_buffer[leaves_a:index(xi, yi, zi)] then
					if is_apple_tree and pr:next(1, 100) <=  10 then
						data[vi] = c_apple
					else
						data[vi] = c_leaves
					end
				end
			end
		end
	end
	end
	end
end

--
-- ABMs
--

minetest.register_abm({
	nodenames = {"default:sapling"},
	interval = 10,
	chance = 50,
	action = function(pos, node)
		if minetest.get_item_group(minetest.get_node(
			{x = pos.x, y = pos.y - 1, z = pos.z}).name, "soil") == 0 then
			return
		end
		print("A sapling grows into a tree at "..minetest.pos_to_string(pos))
		local vm = minetest.get_voxel_manip()
		local minp, maxp = vm:read_from_map({x=pos.x-16, y=pos.y, z=pos.z-16}, {x=pos.x+16, y=pos.y+16, z=pos.z+16})
		local a = VoxelArea:new{MinEdge=minp, MaxEdge=maxp}
		local data = vm:get_data()
		default.grow_tree(data, a, pos, math.random(1, 4) == 1, math.random(1,100000))
		vm:set_data(data)
		vm:write_to_map(data)
		vm:update_map()
	end
})

minetest.register_abm({
	nodenames = {"default:dirt"},
	interval = 2,
	chance = 200,
	action = function(pos, node)
		local above = {x=pos.x, y=pos.y+1, z=pos.z}
		local name = minetest.get_node(above).name
		local nodedef = minetest.registered_nodes[name]
		if nodedef and (nodedef.sunlight_propagates or nodedef.paramtype == "light")
				and nodedef.liquidtype == "none"
				and (minetest.get_node_light(above) or 0) >= 13 then
			if name == "default:snow" or name == "default:snowblock" then
				minetest.set_node(pos, {name = "default:dirt_with_snow"})
			else
				minetest.set_node(pos, {name = "default:dirt_with_grass"})
			end
		end
	end
})

minetest.register_abm({
	nodenames = {"default:dirt_with_grass"},
	interval = 2,
	chance = 20,
	action = function(pos, node)
		local above = {x=pos.x, y=pos.y+1, z=pos.z}
		local name = minetest.get_node(above).name
		local nodedef = minetest.registered_nodes[name]
		if name ~= "ignore" and nodedef
				and not ((nodedef.sunlight_propagates or nodedef.paramtype == "light")
				and nodedef.liquidtype == "none") then
			minetest.set_node(pos, {name = "default:dirt"})
		end
	end
})

--
-- Crafting items
--

minetest.register_craftitem("default:stick", {
	description = "Stick",
	inventory_image = "default_stick.png",
})

minetest.register_craftitem("default:paper", {
	description = "Paper",
	inventory_image = "default_paper.png",
})

minetest.register_craftitem("default:book", {
	description = "Book",
	inventory_image = "default_book.png",
})

minetest.register_craftitem("default:coal_lump", {
	description = "Lump of coal",
	inventory_image = "default_coal_lump.png",
})

minetest.register_craftitem("default:iron_lump", {
	description = "Lump of iron",
	inventory_image = "default_iron_lump.png",
})

minetest.register_craftitem("default:clay_lump", {
	description = "Lump of clay",
	inventory_image = "default_clay_lump.png",
})

minetest.register_craftitem("default:steel_ingot", {
	description = "Steel ingot",
	inventory_image = "default_steel_ingot.png",
})

minetest.register_craftitem("default:clay_brick", {
	description = "Clay brick",
	inventory_image = "default_steel_ingot.png",
	inventory_image = "default_clay_brick.png",
})

minetest.register_craftitem("default:scorched_stuff", {
	description = "Scorched stuff",
	inventory_image = "default_scorched_stuff.png",
})

--
-- Support old code
--

function default.spawn_falling_node(p, nodename)
	spawn_falling_node(p, nodename)
end

-- Horrible crap to support old code
-- Don't use this and never do what this does, it's completely wrong!
-- (More specifically, the client and the C++ code doesn't get the group)
function default.register_falling_node(nodename, texture)
	minetest.log("error", debug.traceback())
	minetest.log('error', "WARNING: default.register_falling_node is deprecated")
	if minetest.registered_nodes[nodename] then
		minetest.registered_nodes[nodename].groups.falling_node = 1
	end
end

--
-- Global callbacks
--

-- Global environment step function
function on_step(dtime)
	-- print("on_step")
end
minetest.register_globalstep(on_step)

function on_placenode(p, node)
	--print("on_placenode")
end
minetest.register_on_placenode(on_placenode)

function on_dignode(p, node)
	--print("on_dignode")
end
minetest.register_on_dignode(on_dignode)

function on_punchnode(p, node)
end
minetest.register_on_punchnode(on_punchnode)

--
-- Test some things
--

local function test_get_craft_result()
	minetest.log("info", "test_get_craft_result()")
	-- normal
	local input = {
		method = "normal",
		width = 2,
		items = {"", "default:coal_lump", "", "default:stick"}
	}
	minetest.log("info", "torch crafting input: "..dump(input))
	local output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "torch crafting output: "..dump(output))
	minetest.log("info", "torch crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	minetest.log("info", "torch crafting output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "default:torch")
	assert(output.item:get_count() == 4)
	-- fuel
	local input = {
		method = "fuel",
		width = 1,
		items = {"default:coal_lump"}
	}
	minetest.log("info", "coal fuel input: "..dump(input))
	local output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "coal fuel output: "..dump(output))
	minetest.log("info", "coal fuel decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)
	-- cook
	local input = {
		method = "cooking",
		width = 1,
		items = {"default:cobble"}
	}
	minetest.log("info", "cobble cooking input: "..dump(output))
	local output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "cobble cooking output: "..dump(output))
	minetest.log("info", "cobble cooking decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)
	assert(output.item)
	minetest.log("info", "cobble cooking output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "default:stone")
	assert(output.item:get_count() == 1)
end
test_get_craft_result()

--
-- Done, print some random stuff
--

--print("minetest.registered_entities:")
--dump2(minetest.registered_entities)

-- END

