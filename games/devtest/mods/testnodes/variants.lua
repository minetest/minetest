-- This file is for variant properties.

local S = minetest.get_translator("testnodes")

local animated_tile = {
	name = "testnodes_anim.png",
	animation = {
		type = "vertical_frames",
		aspect_w = 16,
		aspect_h = 16,
		length = 4.0,
	},
}
minetest.register_node("testnodes:variant_animated", {
	description = S("Variant Animated Test Node").."\n"..
		S("Tiles animate from A to D in 4s cycle").."\n"..
		S("Has two variants"),
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 2,
	param2_variant = {width = 1, offset = 0},
	tiles = {
		"testnodes_node.png", animated_tile,
		"testnodes_node.png", animated_tile,
		"testnodes_node.png", animated_tile,
	},
	variants = {
		{
			tiles = {
				animated_tile, "testnodes_node.png",
				animated_tile, "testnodes_node.png",
				animated_tile, "testnodes_node.png",
			},
		},
	},
})

minetest.register_node("testnodes:variant_color", {
	description = S("Variant Color Test Node").."\n"..
		S("param2 = color + 3 bit variant").."\n"..
		S("Has six variants"),
	paramtype2 = "color",
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 8, -- Last two variants are the same as the first.
	param2_variant = {width = 3, offset = 0},
	tiles = {"testnodes_1g.png"},
	variants = {
		{tiles = {"testnodes_2g.png"}},
		{tiles = {"testnodes_3g.png"}},
		{tiles = {"testnodes_4g.png"}},
		{tiles = {"testnodes_5g.png"}},
		{tiles = {"testnodes_6g.png"}},
	},
	palette = "testnodes_palette_wallmounted.png",
})

minetest.register_node("testnodes:variant_drop", {
	description = S("Variant Drop Test Node").."\n"..
		S("Has four variants").."\n"..
		S("Drops one node with an inherited variant and one without"),
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 4,
	param2_variant = {width = 2, offset = 0},
	tiles = {"testnodes_1.png"},
	variants = {
		{tiles = {"testnodes_2.png"}},
		{tiles = {"testnodes_3.png"}},
		{tiles = {"testnodes_4.png"}},
	},
	drop = {
		max_items = 2,
		items = {
			{items = {"testnodes:variant_drop"}, inherit_variant = true},
			{items = {"testnodes:variant_drop"}, inherit_variant = false},
		},
	},
})

minetest.register_node("testnodes:variant_facedir", {
	description = S("Variant Facedir Test Node").."\n"..
		S("param2 = 3 bit variant + facedir").."\n"..
		S("Has six variants"),
	paramtype2 = "facedir",
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 6,
	param2_variant = {width = 3, offset = 5},
	tiles = {"testnodes_1f.png"},
	variants = {
		{tiles = {"testnodes_2f.png"}},
		{tiles = {"testnodes_3f.png"}},
		{tiles = {"testnodes_4f.png"}},
		{tiles = {"testnodes_5f.png"}},
		{tiles = {"testnodes_6f.png"}},
	},
})

minetest.register_node("testnodes:variant_falling", {
	description = S("Variant Falling Test Node").."\n"..
		S("Has six variants"),
	is_ground_content = false,
	groups = {dig_immediate = 3, falling_node = 1},
	variant_count = 6,
	param2_variant = {width = 3, offset = 0},
	tiles = {"testnodes_1.png"},
	variants = {
		{tiles = {"testnodes_2.png"}},
		{tiles = {"testnodes_3.png"}},
		{tiles = {"testnodes_4.png"}},
		{tiles = {"testnodes_5.png"}},
		{tiles = {"testnodes_6.png"}},
	},
})

minetest.register_node("testnodes:variant_falling_torchlike", {
	description = S("Variant Falling Torchlike Test Node").."\n"..
		S("Has six variants"),
	paramtype = "light",
	sunlight_propagates = true,
	is_ground_content = false,
	groups = {dig_immediate = 3, falling_node = 1},
	variant_count = 6,
	param2_variant = {width = 3, offset = 0},
	drawtype = "torchlike",
	tiles = {"testnodes_1.png"},
	variants = {
		{tiles = {"testnodes_2.png"}},
		{tiles = {"testnodes_3.png"}},
		{tiles = {"testnodes_4.png"}},
		{tiles = {"testnodes_5.png"}},
		{tiles = {"testnodes_6.png"}},
	},
})

minetest.register_node("testnodes:variant_mesh", {
	description = S("Variant Mesh Test Node").."\n"..
		S("Has ten variants"),
	paramtype = "light",
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 10,
	param2_variant = {width = 8, offset = 0},
	drawtype = "mesh",
	mesh = "testnodes_ocorner.obj",
	tiles = {"testnodes_mesh_stripes.png"},
	variants = {
		{tiles = {"testnodes_mesh_stripes2.png"}},
		{tiles = {"testnodes_mesh_stripes3.png"}},
		{tiles = {"testnodes_mesh_stripes4.png"}},
		{tiles = {"testnodes_mesh_stripes5.png"}},
		{tiles = {"testnodes_mesh_stripes6.png"}},
		{tiles = {"testnodes_mesh_stripes7.png"}},
		{tiles = {"testnodes_mesh_stripes8.png"}},
		{tiles = {"testnodes_mesh_stripes9.png"}},
		{tiles = {"testnodes_mesh_stripes10.png"}},
	},
})

minetest.register_node("testnodes:variant_overlay", {
	description = S("Variant Overlay Test Node").."\n"..
		S("Has two variants"),
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 4,
	param2_variant = {width = 1, offset = 0},
	tiles = {"testnodes_node.png"},
	overlay_tiles = {{name = "testnodes_overlay.png", color = "#555"}},
	variants = {
		{
			tiles = {"testnodes_node.png"},
		},
		{
			tiles = {"testnodes_node.png"},
			overlay_tiles = {{name = "testnodes_overlay.png", color = "#FFF"}},
		},
		{
			tiles = {"testnodes_node.png"},
			overlay_tiles = {},
		},
	},
	color = "#F00",
})

minetest.register_node("testnodes:variant_plantlike_rooted", {
	description = S("Variant Plantlike Rooted Test Node").."\n"..
		S("Has six variants"),
	paramtype = "light",
	is_ground_content = false,
	groups = {dig_immediate = 3},
	drawtype = "plantlike_rooted",
	variant_count = 8,
	param2_variant = {width = 8, offset = 0},
	tiles = {"testnodes_1.png"},
	special_tiles = {"testnodes_6.png"},
	variants = {
		{tiles = {"testnodes_2.png"}, special_tiles = {"testnodes_5.png"}},
		{tiles = {"testnodes_3.png"}, special_tiles = {"testnodes_4.png"}},
		{tiles = {"testnodes_4.png"}, special_tiles = {"testnodes_3.png"}},
		{tiles = {"testnodes_5.png"}, special_tiles = {"testnodes_2.png"}},
		{tiles = {"testnodes_6.png"}, special_tiles = {"testnodes_1.png"}},
		{tiles = {"testnodes_6.png"}},
		{special_tiles = {"testnodes_1.png"}},
	},
})

minetest.register_node("testnodes:variant_raillike", {
	description = S("Variant Raillike Test Node").."\n"..
		S("Has four variants"),
	paramtype = "light",
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 4,
	param2_variant = {width = 2, offset = 0},
	drawtype = "raillike",
	variants = {
		[0] = {tiles = {"testnodes_1.png"}},
		{tiles = {"testnodes_2.png"}},
		{tiles = {"testnodes_3.png"}},
		{tiles = {"testnodes_4.png"}},
	},
})

minetest.register_node("testnodes:variant_wallmounted", {
	description = S("Variant Wallmounted Test Node").."\n"..
		S("Has six variants corresponding to direction"),
	paramtype = "light",
	paramtype2 = "wallmounted",
	is_ground_content = false,
	groups = {dig_immediate = 3},
	variant_count = 6,
	param2_variant = {width = 3, offset = 0},
	tiles = {"testnodes_1w.png"},
	variants = {
		{tiles = {"testnodes_2w.png"}},
		{tiles = {"testnodes_3w.png"}},
		{tiles = {"testnodes_4w.png"}},
		{tiles = {"testnodes_5w.png"}},
		{tiles = {"testnodes_6w.png"}},
	},
})
