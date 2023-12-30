--
-- Aliases for map generator outputs
--

-- ESSENTIAL node aliases
-- Basic nodes
minetest.register_alias("mapgen_stone", "basenodes:stone")
minetest.register_alias("mapgen_water_source", "basenodes:water_source")
minetest.register_alias("mapgen_river_water_source", "basenodes:river_water_source")

-- Additional essential aliases for v6
minetest.register_alias("mapgen_lava_source", "basenodes:lava_source")
minetest.register_alias("mapgen_dirt", "basenodes:dirt")
minetest.register_alias("mapgen_dirt_with_grass", "basenodes:dirt_with_grass")
minetest.register_alias("mapgen_sand", "basenodes:sand")
minetest.register_alias("mapgen_tree", "basenodes:tree")
minetest.register_alias("mapgen_leaves", "basenodes:leaves")
minetest.register_alias("mapgen_apple", "basenodes:apple")

-- Essential alias for dungeons
minetest.register_alias("mapgen_cobble", "basenodes:cobble")

-- Optional aliases for v6 (they all have fallback values in the engine)
if minetest.settings:get_bool("devtest_v6_mapgen_aliases", false) then
	minetest.register_alias("mapgen_gravel", "basenodes:gravel")
	minetest.register_alias("mapgen_desert_stone", "basenodes:desert_stone")
	minetest.register_alias("mapgen_desert_sand", "basenodes:desert_sand")
	minetest.register_alias("mapgen_dirt_with_snow", "basenodes:dirt_with_snow")
	minetest.register_alias("mapgen_snowblock", "basenodes:snowblock")
	minetest.register_alias("mapgen_snow", "basenodes:snow")
	minetest.register_alias("mapgen_ice", "basenodes:ice")
	minetest.register_alias("mapgen_junglegrass", "basenodes:junglegrass")
	minetest.register_alias("mapgen_jungletree", "basenodes:jungletree")
	minetest.register_alias("mapgen_jungleleaves", "basenodes:jungleleaves")
	minetest.register_alias("mapgen_pine_tree", "basenodes:pine_tree")
	minetest.register_alias("mapgen_pine_needles", "basenodes:pine_needles")
end
-- Optional alias for mossycobble (should fall back to cobble)
if minetest.settings:get_bool("devtest_dungeon_mossycobble", false) then
	minetest.register_alias("mapgen_mossycobble", "basenodes:mossycobble")
end
-- Optional aliases for dungeon stairs (should fall back to full nodes)
if minetest.settings:get_bool("devtest_dungeon_stairs", false) then
	minetest.register_alias("mapgen_stair_cobble", "stairs:stair_cobble")
	if minetest.settings:get_bool("devtest_v6_mapgen_aliases", false) then
		minetest.register_alias("mapgen_stair_desert_stone", "stairs:stair_desert_stone")
	end
end

--
-- Register biomes for biome API
--

minetest.clear_registered_biomes()
minetest.clear_registered_decorations()
minetest.clear_registered_ores()

if minetest.settings:get_bool("devtest_register_biomes", true) then
	minetest.log("action", "Devtest: register biomes")
	minetest.register_biome({
		name = "mapgen:grassland",
		node_top = "basenodes:dirt_with_grass",
		depth_top = 1,
		node_filler = "basenodes:dirt",
		depth_filler = 1,
		node_riverbed = "basenodes:sand",
		depth_riverbed = 2,
		node_dungeon = "basenodes:cobble",
		node_dungeon_alt = "basenodes:mossycobble",
		node_dungeon_stair = "stairs:stair_cobble",
		y_max = 31000,
		y_min = 4,
		heat_point = 50,
		humidity_point = 50,
	})

	minetest.register_biome({
		name = "mapgen:grassland_ocean",
		node_top = "basenodes:sand",
		depth_top = 1,
		node_filler = "basenodes:sand",
		depth_filler = 3,
		node_riverbed = "basenodes:sand",
		depth_riverbed = 2,
		node_cave_liquid = "basenodes:water_source",
		node_dungeon = "basenodes:cobble",
		node_dungeon_alt = "basenodes:mossycobble",
		node_dungeon_stair = "stairs:stair_cobble",
		y_max = 3,
		y_min = -255,
		heat_point = 50,
		humidity_point = 50,
	})

	minetest.register_biome({
		name = "mapgen:grassland_under",
		node_cave_liquid = {"basenodes:water_source", "basenodes:lava_source"},
		node_dungeon = "basenodes:cobble",
		node_dungeon_alt = "basenodes:mossycobble",
		node_dungeon_stair = "stairs:stair_cobble",
		y_max = -256,
		y_min = -31000,
		heat_point = 50,
		humidity_point = 50,
	})

	minetest.register_biome({
		name = "mapgen:desert",
		node_top = "basenodes:desert_stone", -- error, to try override of it, to fix it
		depth_top = 1,
		node_filler = "basenodes:desert_sand",
		depth_filler = 1,
		node_riverbed = "basenodes:sand",
		depth_riverbed = 2,
		node_dungeon = "basenodes:cobble",
		node_dungeon_alt = "basenodes:mossycobble",
		node_dungeon_stair = "stairs:stair_cobble",
		y_max = 31000,
		y_min = 4,
		heat_point = 60,
		humidity_point = 40,
	})
end

if minetest.settings:get_bool("devtest_override_biomes", true) then
	minetest.log("action", "Devtest: override biomes")
	minetest.override_biome("mapgen:desert", {
		node_top = "basenodes:desert_sand"
	})
end

if minetest.settings:get_bool("devtest_register_decorations", true) then
	minetest.log("action", "Devtest: register decorations")
	minetest.register_decoration({
		name         = "mapgen:junglegrass",
		deco_type    = "simple",
		place_on     = "basenodes:dirt_with_grass",
		sidelen      = 16,
		noise_params = {
			offset  = 0,
			scale   = 0.007,
			spread  = {x = 100, y = 100, z = 100},
			seed    = 329,
			octaves = 3,
			persist = 0.6
		},
		y_max       = 30,
		y_min       = 1,
		biomes      = {"mapgen:grassland"},
		decoration  = "basenodes:leaves",
	})
end

if minetest.settings:get_bool("devtest_override_decorations", true) then
	minetest.log("action", "Devtest: override decorations")
	minetest.override_decoration("mapgen:junglegrass", {
		decoration = "basenodes:junglegrass"
	})

	minetest.reregister_decorations()
end

if minetest.settings:get_bool("devtest_register_ores", true) then
	minetest.log("action", "Devtest: register ores")
	minetest.register_ore({
		name           = "mapgen:test_ore",
		ore_type       = "scatter",
		ore            = "basenodes:desert_stone",
		wherein        = "basenodes:stone",
		clust_scarcity = 8 * 8 * 8,
		clust_num_ores = 9,
		clust_size     = 3,
		y_max          = 31000,
		y_min          = -31000,
		biomes         = {"mapgen:grassland_under"}
	})
end

if minetest.settings:get_bool("devtest_override_ores", true) then
	minetest.log("action", "Devtest: override ores")
	minetest.override_ore("mapgen:test_ore", {
		ore    = "basenodes:test_ore",
		biomes = {"mapgen:grassland_under", "mapgen:grassland"},
	})

	minetest.reregister_ores()
end
