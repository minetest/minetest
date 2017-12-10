minetest.register_node("build:solid", {
	description = "Solid",
	tiles = {{name="steel.png^arrow.png", align_style="world"}},
	groups = {cracky=3, build=1},
	connects_to = {"group:build"},
})

minetest.register_node("build:wall", {
	description = "Wall",
	tiles = {{name="steel.png^arrow.png", align_style="world"}},
	groups = {cracky=3, build=1},
	drawtype = "nodebox",
	paramtype = "light",
	connects_to = {"group:build"},
	connect_sides = {"front", "back", "left", "right"},
	node_box = {
		type = "connected",
		fixed = {-0.125, -0.500, -0.125, 0.125, 0.500, 0.125},
		connect_top = {-0.250, 0.375, -0.250, 0.250, 0.625, 0.250},
		connect_bottom = {-0.250, -0.625, -0.250, 0.250, -0.375, 0.250},
		connect_front = {-0.125, -0.500, -0.500, 0.125, 0.500, -0.125},
		connect_back = {-0.125, -0.500, 0.125, 0.125, 0.500, 0.500},
		connect_left = {-0.500, -0.500, -0.125, -0.125, 0.500, 0.125},
		connect_right = {0.125, -0.500, -0.125, 0.500, 0.500, 0.125},
	},
})

minetest.register_node("build:fence", {
	description = "Fence",
	tiles = {{name="steel.png^arrow.png", align_style="world"}},
	groups = {cracky=3, build=1},
	drawtype = "nodebox",
	paramtype = "light",
	connects_to = {"group:build"},
	node_box = {
		type = "connected",
		fixed = {-0.125, -0.500, -0.125, 0.125, 0.425, 0.125},
		connect_top = {-0.125, 0.425, -0.125, 0.125, 0.500, 0.125},
		connect_bottom = {-0.250, -0.500, -0.250, 0.250, -0.250, 0.250},
		connect_front = {
			{-0.0625, 0.125, -0.500, 0.0625, 0.375, -0.125},
			{-0.0625, -0.375, -0.500, 0.0625, -0.125, -0.125},
		},
		connect_back = {
			{-0.0625, 0.125, 0.500, 0.0625, 0.375, 0.125},
			{-0.0625, -0.375, 0.500, 0.0625, -0.125, 0.125},
		},
		connect_left = {
			{-0.500, 0.125, -0.0625, -0.125, 0.375, 0.0625},
			{-0.500, -0.375, -0.0625, -0.125, -0.125, 0.0625},
		},
		connect_right = {
			{0.500, 0.125, -0.0625, 0.125, 0.375, 0.0625},
			{0.500, -0.375, -0.0625, 0.125, -0.125, 0.0625},
		},
	},
})

minetest.register_node("build:spire", {
	description = "Spire",
	tiles = {{name="gold.png^arrow.png", align_style="world"}},
	groups = {cracky=3, build=1},
	drawtype = "nodebox",
	paramtype = "light",
	connects_to = {"group:build"},
	connect_sides = {"top", "bottom"},
	node_box = {
		type = "fixed",
		fixed = {-0.0625, -0.500, -0.0625, 0.0625, 0.500, 0.0625},
	},
})

minetest.register_node("build:top", {
	description = "Spire Top",
	tiles = {{name="gold.png^arrow.png", align_style="world"}},
	groups = {cracky=3, build=1},
	drawtype = "nodebox",
	paramtype = "light",
	connects_to = {"group:build"},
	connect_sides = {"bottom"},
	node_box = {
		type = "connected",
		fixed = {-0.125, -0.125, -0.125, 0.125, 0.125, 0.125},
		connect_bottom = {-0.0625, -0.500, -0.0625, 0.0625, -0.125, 0.0625},
	},
})
