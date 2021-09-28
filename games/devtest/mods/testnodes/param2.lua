-- This file is for misc. param2 tests that aren't covered in drawtypes.lua already.

local S = minetest.get_translator("testnodes")

minetest.register_node("testnodes:facedir", {
	description = S("Facedir Test Node"),
	paramtype2 = "facedir",
	tiles = {
		"testnodes_1.png",
		"testnodes_2.png",
		"testnodes_3.png",
		"testnodes_4.png",
		"testnodes_5.png",
		"testnodes_6.png",
	},

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:facedir_nodebox", {
	description = S("Facedir Nodebox Test Node"),
	tiles = {
		"testnodes_1.png",
		"testnodes_2.png",
		"testnodes_3.png",
		"testnodes_4.png",
		"testnodes_5.png",
		"testnodes_6.png",
	},
	drawtype = "nodebox",
	paramtype = "light",
	paramtype2 = "facedir",
	node_box = {
		type = "fixed",
		fixed = {-0.5, -0.5, -0.5, 0.2, 0.2, 0.2},
	},

	groups = {dig_immediate=3},
})

minetest.register_node("testnodes:wallmounted", {
	description = S("Wallmounted Test Node"),
	paramtype2 = "wallmounted",
	tiles = {
		"testnodes_1w.png",
		"testnodes_2w.png",
		"testnodes_3w.png",
		"testnodes_4w.png",
		"testnodes_5w.png",
		"testnodes_6w.png",
	},

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:wallmounted_nodebox", {
	description = S("Wallmounted Nodebox Test Node"),
	paramtype2 = "wallmounted",
	paramtype = "light",
	tiles = {
		"testnodes_1w.png",
		"testnodes_2w.png",
		"testnodes_3w.png",
		"testnodes_4w.png",
		"testnodes_5w.png",
		"testnodes_6w.png",
	},
	drawtype = "nodebox",
	node_box = {
		type = "wallmounted",
		wall_top = { -0.5, 0, -0.5, 0.5, 0.5, 0.5 },
		wall_bottom = { -0.5, -0.5, -0.5, 0.5, 0, 0.5 },
		wall_side = { -0.5, -0.5, -0.5, 0, 0.5, 0.5 },
	},

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:color", {
	description = S("Color Test Node"),
	paramtype2 = "color",
	palette = "testnodes_palette_full.png",
	tiles = {
		"testnodes_node.png",
	},

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:colorfacedir", {
	description = S("Color Facedir Test Node"),
	paramtype2 = "colorfacedir",
	palette = "testnodes_palette_facedir.png",
	tiles = {
		"testnodes_1g.png",
		"testnodes_2g.png",
		"testnodes_3g.png",
		"testnodes_4g.png",
		"testnodes_5g.png",
		"testnodes_6g.png",
	},

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:colorfacedir_nodebox", {
	description = S("Color Facedir Nodebox Test Node"),
	tiles = {
		"testnodes_1g.png",
		"testnodes_2g.png",
		"testnodes_3g.png",
		"testnodes_4g.png",
		"testnodes_5g.png",
		"testnodes_6g.png",
	},
	drawtype = "nodebox",
	paramtype = "light",
	paramtype2 = "colorfacedir",
	palette = "testnodes_palette_facedir.png",
	node_box = {
		type = "fixed",
		fixed = {-0.5, -0.5, -0.5, 0.2, 0.2, 0.2},
	},

	groups = {dig_immediate=3},
})

minetest.register_node("testnodes:colorwallmounted", {
	description = S("Color Wallmounted Test Node"),
	paramtype2 = "colorwallmounted",
	paramtype = "light",
	palette = "testnodes_palette_wallmounted.png",
	tiles = {
		"testnodes_1wg.png",
		"testnodes_2wg.png",
		"testnodes_3wg.png",
		"testnodes_4wg.png",
		"testnodes_5wg.png",
		"testnodes_6wg.png",
	},

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:colorwallmounted_nodebox", {
	description = S("Color Wallmounted Nodebox Test Node"),
	paramtype2 = "colorwallmounted",
	paramtype = "light",
	palette = "testnodes_palette_wallmounted.png",
	tiles = {
		"testnodes_1wg.png",
		"testnodes_2wg.png",
		"testnodes_3wg.png",
		"testnodes_4wg.png",
		"testnodes_5wg.png",
		"testnodes_6wg.png",
	},
	drawtype = "nodebox",
	node_box = {
		type = "wallmounted",
		wall_top = { -0.5, 0, -0.5, 0.5, 0.5, 0.5 },
		wall_bottom = { -0.5, -0.5, -0.5, 0.5, 0, 0.5 },
		wall_side = { -0.5, -0.5, -0.5, 0, 0.5, 0.5 },
	},

	groups = { dig_immediate = 3 },
})

