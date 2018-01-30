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
