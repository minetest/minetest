local PATH = minetest.get_modpath("land")
-- dofile(PATH.."/mapgen.lua")

minetest.register_node("land:stone", {
	description = "Stone",
	tiles = {{name="stone.png", align_style="world"}},
	groups = {cracky=3},
})

minetest.register_node("land:dirt", {
	description = "Dirt",
	tiles = {{name="dirt.png", align_style="world"}},
	groups = {crumbly=3, soil=1},
})

minetest.register_node("land:gravel", {
	description = "Gravel",
	tiles = {{name="gravel.png", align_style="world"}},
	groups = {falling_node=1, crumbly=2},
})

minetest.register_node("land:sand", {
	description = "Sand",
	tiles = {{name="sand.png", align_style="world"}},
	groups = {falling_node=1, crumbly=3},
})

minetest.register_node("land:grass", {
	description = "Dirt with grass",
	use_texture_alpha = true,
	tiles = {
		{name="grass.png", align_style="world"},
		{name="dirt.png", align_style="world"},
	},
	overlay_tiles = {
		{name="grid.png^compass.png", align_style="world", scale=4},
		{},
		{name="grass_overlay.png", align_style="world"},
	},
	groups = {crumbly=3, soil=1},
})

minetest.register_alias("mapgen_stone", "land:stone")
minetest.register_alias("mapgen_dirt", "land:dirt")
minetest.register_alias("mapgen_dirt_with_grass", "land:grass")
minetest.register_alias("mapgen_sand", "land:sand")
-- minetest.register_alias("mapgen_water_source", "default:water_source")
-- minetest.register_alias("mapgen_river_water_source", "default:river_water_source")
-- minetest.register_alias("mapgen_lava_source", "default:lava_source")
minetest.register_alias("mapgen_gravel", "land:gravel")

minetest.clear_registered_biomes()
minetest.clear_registered_decorations()

minetest.register_biome({
        name = "land:grassland",
        node_top = "land:grass",
        depth_top = 1,
        node_filler = "land:dirt",
        depth_filler = 2,
        y_min = 5,
        y_max = 31000,
        heat_point = 50,
        humidity_point = 50,
})

minetest.register_biome({
        name = "land:ocean",
        node_top = "land:sand",
        depth_top = 1,
        node_filler = "land:gravel",
        depth_filler = 2,
        y_min = -31000,
        y_max = 4,
        heat_point = 50,
        humidity_point = 50,
})
