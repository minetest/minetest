-- legacy (Minetest 0.4 mod)
-- Provides as much backwards-compatibility as feasible

WATER_ALPHA = 160
WATER_VISC = 1
LAVA_VISC = 7
LIGHT_MAX = 14

--
-- Tool definition
-- Compatibility to 0.3 and old 0.4
--

minetest.register_tool(":WPick", {
	image = "default_tool_woodpick.png",
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
minetest.register_tool(":STPick", {
	image = "default_tool_stonepick.png",
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
minetest.register_tool(":SteelPick", {
	image = "default_tool_steelpick.png",
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
minetest.register_tool(":MesePick", {
	image = "default_tool_mesepick.png",
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
minetest.register_tool(":WShovel", {
	image = "default_tool_woodshovel.png",
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
minetest.register_tool(":STShovel", {
	image = "default_tool_stoneshovel.png",
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
minetest.register_tool(":SteelShovel", {
	image = "default_tool_steelshovel.png",
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
minetest.register_tool(":WAxe", {
	image = "default_tool_woodaxe.png",
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
minetest.register_tool(":STAxe", {
	image = "default_tool_stoneaxe.png",
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
minetest.register_tool(":SteelAxe", {
	image = "default_tool_steelaxe.png",
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
minetest.register_tool(":WSword", {
	image = "default_tool_woodsword.png",
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
minetest.register_tool(":STSword", {
	image = "default_tool_stonesword.png",
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
minetest.register_tool(":SteelSword", {
	image = "default_tool_steelsword.png",
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

--
-- Crafting definition (recipe = legacy, output = default)
-- Makes current compatible with 0.3 and old 0.4
--

minetest.register_craft({
	output = 'node "default:wood" 4',
	recipe = {
		{'node "tree"'},
	}
})

minetest.register_craft({
	output = 'craft "default:stick" 4',
	recipe = {
		{'node "wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:fence_wooden" 2',
	recipe = {
		{'craft "Stick"', 'craft "Stick"', 'craft "Stick"'},
		{'craft "Stick"', 'craft "Stick"', 'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'node "default:sign_wall" 1',
	recipe = {
		{'node "wood"', 'node "wood"', 'node "wood"'},
		{'node "wood"', 'node "wood"', 'node "wood"'},
		{'', 'craft "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'node "default:torch" 4',
	recipe = {
		{'craft "lump_of_coal"'},
		{'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_wooden"',
	recipe = {
		{'node "wood"', 'node "wood"', 'node "wood"'},
		{'', 'craft "Stick"', ''},
		{'', 'craft "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_stone"',
	recipe = {
		{'node "cobble"', 'node "cobble"', 'node "cobble"'},
		{'', 'craft "Stick"', ''},
		{'', 'craft "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_steel"',
	recipe = {
		{'craft "steel_ingot"', 'craft "steel_ingot"', 'craft "steel_ingot"'},
		{'', 'craft "Stick"', ''},
		{'', 'craft "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_mese"',
	recipe = {
		{'node "mese"', 'node "mese"', 'node "mese"'},
		{'', 'craft "Stick"', ''},
		{'', 'craft "Stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:shovel_wood"',
	recipe = {
		{'node "wood"'},
		{'craft "Stick"'},
		{'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:shovel_stone"',
	recipe = {
		{'node "cobble"'},
		{'craft "Stick"'},
		{'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:shovel_steel"',
	recipe = {
		{'craft "steel_ingot"'},
		{'craft "Stick"'},
		{'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:axe_wooden"',
	recipe = {
		{'node "wood"', 'node "wood"'},
		{'node "wood"', 'craft "Stick"'},
		{'', 'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:axe_stone"',
	recipe = {
		{'node "cobble"', 'node "cobble"'},
		{'node "cobble"', 'craft "Stick"'},
		{'', 'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:axe_steel"',
	recipe = {
		{'craft "steel_ingot"', 'craft "steel_ingot"'},
		{'craft "steel_ingot"', 'craft "Stick"'},
		{'', 'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:sword_wood"',
	recipe = {
		{'node "wood"'},
		{'node "wood"'},
		{'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:sword_stone"',
	recipe = {
		{'node "cobble"'},
		{'node "cobble"'},
		{'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:sword_steel"',
	recipe = {
		{'craft "steel_ingot"'},
		{'craft "steel_ingot"'},
		{'craft "Stick"'},
	}
})

minetest.register_craft({
	output = 'node "default:rail" 15',
	recipe = {
		{'craft "steel_ingot"', '', 'craft "steel_ingot"'},
		{'craft "steel_ingot"', 'craft "Stick"', 'craft "steel_ingot"'},
		{'craft "steel_ingot"', '', 'craft "steel_ingot"'},
	}
})

minetest.register_craft({
	output = 'node "default:chest" 1',
	recipe = {
		{'node "wood"', 'node "wood"', 'node "wood"'},
		{'node "wood"', '', 'node "wood"'},
		{'node "wood"', 'node "wood"', 'node "wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:chest_locked" 1',
	recipe = {
		{'node "wood"', 'node "wood"', 'node "wood"'},
		{'node "wood"', 'craft "steel_ingot"', 'node "wood"'},
		{'node "wood"', 'node "wood"', 'node "wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:furnace" 1',
	recipe = {
		{'node "cobble"', 'node "cobble"', 'node "cobble"'},
		{'node "cobble"', '', 'node "cobble"'},
		{'node "cobble"', 'node "cobble"', 'node "cobble"'},
	}
})

minetest.register_craft({
	output = 'node "default:steelblock" 1',
	recipe = {
		{'craft "steel_ingot"', 'craft "steel_ingot"', 'craft "steel_ingot"'},
		{'craft "steel_ingot"', 'craft "steel_ingot"', 'craft "steel_ingot"'},
		{'craft "steel_ingot"', 'craft "steel_ingot"', 'craft "steel_ingot"'},
	}
})

minetest.register_craft({
	output = 'node "default:sandstone" 1',
	recipe = {
		{'node "sand"', 'node "sand"'},
		{'node "sand"', 'node "sand"'},
	}
})

minetest.register_craft({
	output = 'node "default:clay" 1',
	recipe = {
		{'craft "lump_of_clay"', 'craft "lump_of_clay"'},
		{'craft "lump_of_clay"', 'craft "lump_of_clay"'},
	}
})

minetest.register_craft({
	output = 'node "default:brick" 1',
	recipe = {
		{'craft "clay_brick"', 'craft "clay_brick"'},
		{'craft "clay_brick"', 'craft "clay_brick"'},
	}
})

minetest.register_craft({
	output = 'craft "default:paper" 1',
	recipe = {
		{'node "papyrus"', 'node "papyrus"', 'node "papyrus"'},
	}
})

minetest.register_craft({
	output = 'craft "default:book" 1',
	recipe = {
		{'craft "paper"'},
		{'craft "paper"'},
		{'craft "paper"'},
	}
})

minetest.register_craft({
	output = 'node "default:bookshelf" 1',
	recipe = {
		{'node "wood"', 'node "wood"', 'node "wood"'},
		{'craft "book"', 'craft "book"', 'craft "book"'},
		{'node "wood"', 'node "wood"', 'node "wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:ladder" 1',
	recipe = {
		{'craft "Stick"', '', 'craft "Stick"'},
		{'craft "Stick"', 'craft "Stick"', 'craft "Stick"'},
		{'craft "Stick"', '', 'craft "Stick"'},
	}
})

--
-- Node compatibility with old 0.4
--

minetest.register_node(":stone", {
	tile_images = {"default_stone.png"},
	inventory_image = minetest.inventorycube("default_stone.png"),
	paramtype = "mineral",
	is_ground_content = true,
	often_contains_mineral = true, -- Texture atlas hint
	material = minetest.digprop_stonelike(1.0),
	dug_item = 'node "cobble" 1',
})

minetest.register_node(":dirt_with_grass", {
	tile_images = {"default_grass.png", "default_dirt.png", "default_dirt.png^default_grass_side.png"},
	inventory_image = minetest.inventorycube("default_dirt.png^default_grass_side.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	dug_item = 'node "dirt" 1',
})

minetest.register_node(":dirt_with_grass_footsteps", {
	tile_images = {"default_grass_footsteps.png", "default_dirt.png", "default_dirt.png^default_grass_side.png"},
	inventory_image = "default_grass_footsteps.png",
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	dug_item = 'node "dirt" 1',
})

minetest.register_node(":dirt", {
	tile_images = {"default_dirt.png"},
	inventory_image = minetest.inventorycube("default_dirt.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
})

minetest.register_node(":sand", {
	tile_images = {"default_sand.png"},
	inventory_image = minetest.inventorycube("default_sand.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	cookresult_itemstring = 'node "default:glass" 1',
})

minetest.register_node(":gravel", {
	tile_images = {"default_gravel.png"},
	inventory_image = minetest.inventorycube("default_gravel.png"),
	is_ground_content = true,
	material = minetest.digprop_gravellike(1.0),
})

minetest.register_node(":sandstone", {
	tile_images = {"default_sandstone.png"},
	inventory_image = minetest.inventorycube("default_sandstone.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	dug_item = 'node "sand" 1',
})

minetest.register_node(":clay", {
	tile_images = {"default_clay.png"},
	inventory_image = minetest.inventorycube("default_clay.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	dug_item = 'craft "lump_of_clay" 4',
})

minetest.register_node(":brick", {
	tile_images = {"default_brick.png"},
	inventory_image = minetest.inventorycube("default_brick.png"),
	is_ground_content = true,
	material = minetest.digprop_stonelike(1.0),
	dug_item = 'craft "clay_brick" 4',
})

minetest.register_node(":tree", {
	tile_images = {"default_tree_top.png", "default_tree_top.png", "default_tree.png"},
	inventory_image = minetest.inventorycube("default_tree_top.png", "default_tree.png", "default_tree.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(1.0),
	cookresult_itemstring = 'craft "default:lump_of_coal" 1',
	furnace_burntime = 30,
})

minetest.register_node(":jungletree", {
	tile_images = {"default_jungletree_top.png", "default_jungletree_top.png", "default_jungletree.png"},
	inventory_image = minetest.inventorycube("default_jungletree_top.png", "default_jungletree.png", "default_jungletree.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(1.0),
	cookresult_itemstring = 'craft "default:lump_of_coal" 1',
	furnace_burntime = 30,
})

minetest.register_node(":junglegrass", {
	drawtype = "plantlike",
	visual_scale = 1.3,
	tile_images = {"default_junglegrass.png"},
	inventory_image = "default_junglegrass.png",
	light_propagates = true,
	paramtype = "light",
	walkable = false,
	material = minetest.digprop_leaveslike(1.0),
	furnace_burntime = 2,
})

minetest.register_node(":leaves", {
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	tile_images = {"default_leaves.png"},
	inventory_image = minetest.inventorycube("default_leaves.png"),
	light_propagates = true,
	paramtype = "light",
	material = minetest.digprop_leaveslike(1.0),
	extra_dug_item = 'node "default:sapling" 1',
	extra_dug_item_rarity = 20,
	furnace_burntime = 1,
})

minetest.register_node(":cactus", {
	tile_images = {"default_cactus_top.png", "default_cactus_top.png", "default_cactus_side.png"},
	inventory_image = minetest.inventorycube("default_cactus_top.png", "default_cactus_side.png", "default_cactus_side.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(0.75),
	furnace_burntime = 15,
})

minetest.register_node(":papyrus", {
	drawtype = "plantlike",
	tile_images = {"default_papyrus.png"},
	inventory_image = "default_papyrus.png",
	light_propagates = true,
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	material = minetest.digprop_leaveslike(0.5),
	furnace_burntime = 1,
})

minetest.register_node(":bookshelf", {
	tile_images = {"default_wood.png", "default_wood.png", "default_bookshelf.png"},
	inventory_image = minetest.inventorycube("default_wood.png", "default_bookshelf.png", "default_bookshelf.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(0.75),
	furnace_burntime = 30,
})

minetest.register_node(":glass", {
	drawtype = "glasslike",
	tile_images = {"default_glass.png"},
	inventory_image = minetest.inventorycube("default_glass.png"),
	light_propagates = true,
	paramtype = "light",
	sunlight_propagates = true,
	is_ground_content = true,
	material = minetest.digprop_glasslike(1.0),
})

minetest.register_node(":wooden_fence", {
	drawtype = "fencelike",
	tile_images = {"default_wood.png"},
	inventory_image = "default_fence.png",
	light_propagates = true,
	paramtype = "light",
	is_ground_content = true,
	selection_box = {
		type = "fixed",
		fixed = {-1/7, -1/2, -1/7, 1/7, 1/2, 1/7},
	},
	furnace_burntime = 15,
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node(":rail", {
	drawtype = "raillike",
	tile_images = {"default_rail.png", "default_rail_curved.png", "default_rail_t_junction.png", "default_rail_crossing.png"},
	inventory_image = "default_rail.png",
	light_propagates = true,
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
		--fixed = <default>
	},
	material = minetest.digprop_dirtlike(0.75),
})

minetest.register_node(":ladder", {
	drawtype = "signlike",
	tile_images = {"default_ladder.png"},
	inventory_image = "default_ladder.png",
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
	material = minetest.digprop_woodlike(0.5),
})

minetest.register_node(":coalstone", {
	tile_images = {"default_stone.png^mineral_coal.png"},
	inventory_image = "default_stone.png^mineral_coal.png",
	is_ground_content = true,
	material = minetest.digprop_stonelike(1.5),
})

minetest.register_node(":wood", {
	tile_images = {"default_wood.png"},
	inventory_image = minetest.inventorycube("default_wood.png"),
	is_ground_content = true,
	furnace_burntime = 7,
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node(":mese", {
	tile_images = {"default_mese.png"},
	inventory_image = minetest.inventorycube("default_mese.png"),
	is_ground_content = true,
	furnace_burntime = 30,
	material = minetest.digprop_stonelike(0.5),
})

minetest.register_node(":cloud", {
	tile_images = {"default_cloud.png"},
	inventory_image = minetest.inventorycube("default_cloud.png"),
	is_ground_content = true,
})

minetest.register_node(":water_flowing", {
	drawtype = "flowingliquid",
	tile_images = {"default_water.png"},
	alpha = WATER_ALPHA,
	inventory_image = minetest.inventorycube("default_water.png"),
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
		{image="default_water.png", backface_culling=false},
		{image="default_water.png", backface_culling=true},
	},
})

minetest.register_node(":water_source", {
	drawtype = "liquid",
	tile_images = {"default_water.png"},
	alpha = WATER_ALPHA,
	inventory_image = minetest.inventorycube("default_water.png"),
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
		{image="default_water.png", backface_culling=false},
	},
})

minetest.register_node(":lava_flowing", {
	drawtype = "flowingliquid",
	tile_images = {"default_lava.png"},
	inventory_image = minetest.inventorycube("default_lava.png"),
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
		{image="default_lava.png", backface_culling=false},
		{image="default_lava.png", backface_culling=true},
	},
})

minetest.register_node(":lava_source", {
	drawtype = "liquid",
	tile_images = {"default_lava.png"},
	inventory_image = minetest.inventorycube("default_lava.png"),
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
		{image="default_lava.png", backface_culling=false},
	},
	furnace_burntime = 60,
})

minetest.register_node(":torch", {
	drawtype = "torchlike",
	tile_images = {"default_torch_on_floor.png", "default_torch_on_ceiling.png", "default_torch.png"},
	inventory_image = "default_torch_on_floor.png",
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
	material = minetest.digprop_constanttime(0.0),
	furnace_burntime = 4,
})

minetest.register_node(":sign_wall", {
	drawtype = "signlike",
	tile_images = {"default_sign_wall.png"},
	inventory_image = "default_sign_wall.png",
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
	material = minetest.digprop_constanttime(0.5),
	furnace_burntime = 10,
})

minetest.register_node(":chest", {
	tile_images = {"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_front.png"},
	inventory_image = minetest.inventorycube("default_chest_top.png", "default_chest_front.png", "default_chest_side.png"),
	paramtype = "facedir_simple",
	metadata_name = "chest",
	material = minetest.digprop_woodlike(1.0),
	furnace_burntime = 30,
})

minetest.register_node(":locked_chest", {
	tile_images = {"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_lock.png"},
	inventory_image = minetest.inventorycube("default_chest_top.png", "default_chest_lock.png", "default_chest_side.png"),
	paramtype = "facedir_simple",
	metadata_name = "locked_chest",
	material = minetest.digprop_woodlike(1.0),
	furnace_burntime = 30,
})

minetest.register_node(":furnace", {
	tile_images = {"default_furnace_side.png", "default_furnace_side.png", "default_furnace_side.png",
		"default_furnace_side.png", "default_furnace_side.png", "default_furnace_front.png"},
	inventory_image = minetest.inventorycube("default_furnace_side.png", "default_furnace_front.png", "default_furnace_side.png"),
	paramtype = "facedir_simple",
	metadata_name = "furnace",
	material = minetest.digprop_stonelike(3.0),
})

minetest.register_node(":cobble", {
	tile_images = {"default_cobble.png"},
	inventory_image = minetest.inventorycube("default_cobble.png"),
	is_ground_content = true,
	cookresult_itemstring = 'node "default:stone" 1',
	material = minetest.digprop_stonelike(0.9),
})

minetest.register_node(":mossycobble", {
	tile_images = {"default_mossycobble.png"},
	inventory_image = minetest.inventorycube("default_mossycobble.png"),
	is_ground_content = true,
	material = minetest.digprop_stonelike(0.8),
})

minetest.register_node(":steelblock", {
	tile_images = {"default_steel_block.png"},
	inventory_image = minetest.inventorycube("default_steel_block.png"),
	is_ground_content = true,
	material = minetest.digprop_stonelike(5.0),
})

minetest.register_node(":nyancat", {
	tile_images = {"default_nc_side.png", "default_nc_side.png", "default_nc_side.png",
		"default_nc_side.png", "default_nc_back.png", "default_nc_front.png"},
	inventory_image = "default_nc_front.png",
	paramtype = "facedir_simple",
	material = minetest.digprop_stonelike(3.0),
	furnace_burntime = 1,
})

minetest.register_node(":nyancat_rainbow", {
	tile_images = {"default_nc_rb.png"},
	inventory_image = "default_nc_rb.png",
	material = minetest.digprop_stonelike(3.0),
	furnace_burntime = 1,
})

minetest.register_node(":sapling", {
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"default_sapling.png"},
	inventory_image = "default_sapling.png",
	paramtype = "light",
	light_propagates = true,
	walkable = false,
	material = minetest.digprop_constanttime(0.0),
	furnace_burntime = 10,
})

minetest.register_node(":apple", {
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"default_apple.png"},
	inventory_image = "default_apple.png",
	paramtype = "light",
	light_propagates = true,
	sunlight_propagates = true,
	walkable = false,
	dug_item = 'craft "apple" 1',
	material = minetest.digprop_constanttime(0.0),
	furnace_burntime = 3,
})

--
-- Crafting items
-- Compatibility to 0.3 and old 0.4
--

minetest.register_craftitem(":Stick", {
	image = "default_stick.png",
	--furnace_burntime = ...,
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":paper", {
	image = "default_paper.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":book", {
	image = "default_book.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":lump_of_coal", {
	image = "default_lump_of_coal.png",
	furnace_burntime = 40;
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":lump_of_iron", {
	image = "default_lump_of_iron.png",
	cookresult_itemstring = 'craft "default:steel_ingot" 1',
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":lump_of_clay", {
	image = "default_lump_of_clay.png",
	cookresult_itemstring = 'craft "default:clay_brick" 1',
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":steel_ingot", {
	image = "default_steel_ingot.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":clay_brick", {
	image = "default_clay_brick.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":rat", {
	image = "rat.png",
	cookresult_itemstring = 'craft "cooked_rat" 1',
	on_drop = function(item, dropper, pos)
		minetest.env:add_rat(pos)
		return true
	end,
})

minetest.register_craftitem(":cooked_rat", {
	image = "cooked_rat.png",
	cookresult_itemstring = 'craft "scorched_stuff" 1',
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = minetest.craftitem_eat(6),
})

minetest.register_craftitem(":scorched_stuff", {
	image = "default_scorched_stuff.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem(":firefly", {
	image = "firefly.png",
	on_drop = function(item, dropper, pos)
		minetest.env:add_firefly(pos)
		return true
	end,
})

minetest.register_craftitem(":apple", {
	image = "default_apple.png",
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = minetest.craftitem_eat(4),
})

minetest.register_craftitem(":apple_iron", {
	image = "default_apple_iron.png",
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = minetest.craftitem_eat(8),
})

--
-- Falling stuff
--

default.register_falling_node("sand", "default_sand.png")
default.register_falling_node("gravel", "default_gravel.png")

--
-- Global callbacks
--

function on_placenode(p, node)
	nodeupdate(p)
end
minetest.register_on_placenode(on_placenode)

function on_dignode(p, node)
	nodeupdate(p)
end
minetest.register_on_dignode(on_dignode)

-- END
