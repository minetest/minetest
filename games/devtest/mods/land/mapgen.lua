minetest.register_alias("mapgen_stone", "land:stone")
minetest.register_alias("mapgen_dirt", "land:dirt")
minetest.register_alias("mapgen_dirt_with_grass", "land:grass")
minetest.register_alias("mapgen_sand", "land:sand")
minetest.register_alias("mapgen_water_source", "land:water_source")
minetest.register_alias("mapgen_river_water_source", "land:river_water_source")
minetest.register_alias("mapgen_lava_source", "land:lava_source")
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
