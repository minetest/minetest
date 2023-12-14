local S = minetest.get_translator("testnodes")

-- Sunken node example.
minetest.register_node("testnodes:sunken_torchlike", {
	description = "Sunken Torchlike Test Node in Liquid Source Range 5",
	drawtype = "sunken",
	tiles = {"testnodes_liquidsource_r5.png"},
	special_tiles = {
		{name = "testnodes_liquidsource_r5.png", backface_culling = false},
		{name = "testnodes_liquidsource_r5.png", backface_culling = true},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	walkable = false,
	buildable_to = true,
	is_ground_content = false,
	liquidtype = "source",
	--liquid_alternative_flowing = "testnodes:rliquid_flowing_5",
	--liquid_alternative_source = "testnodes:rliquid_5",
	liquid_alternative_flowing = "testnodes:sunken_torchlike",
	liquid_alternative_source = "testnodes:sunken_torchlike",
	liquid_range = 0,

	inner_node = "testnodes:torchlike",
})

minetest.register_node("testnodes:sunken_nodebox", {
	description = "Sunken Nodebox Test Node in Liquid Source Range 5",
	drawtype = "sunken",
	tiles = {"testnodes_liquidsource_r5.png"},
	special_tiles = {
		{name = "testnodes_liquidsource_r5.png", backface_culling = false},
		{name = "testnodes_liquidsource_r5.png", backface_culling = true},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	walkable = false,
	buildable_to = true,
	is_ground_content = false,
	liquidtype = "source",
	--liquid_alternative_flowing = "testnodes:rliquid_flowing_5",
	--liquid_alternative_source = "testnodes:rliquid_5",
	liquid_alternative_flowing = "testnodes:sunken_nodebox",
	liquid_alternative_source = "testnodes:sunken_nodebox",
	liquid_range = 0,

	inner_node = "testnodes:nodebox_fixed",
})

minetest.register_node("testnodes:sunken_mesh", {
	description = "Sunken Mesh Test Node in Liquid Source Range 5",
	drawtype = "sunken",
	tiles = {"testnodes_liquidsource_r5.png"},
	special_tiles = {
		{name = "testnodes_liquidsource_r5.png", backface_culling = false},
		{name = "testnodes_liquidsource_r5.png", backface_culling = true},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	walkable = false,
	buildable_to = true,
	is_ground_content = false,
	liquidtype = "source",
	--liquid_alternative_flowing = "testnodes:rliquid_flowing_5",
	--liquid_alternative_source = "testnodes:rliquid_5",
	liquid_alternative_flowing = "testnodes:sunken_nodebox",
	liquid_alternative_source = "testnodes:sunken_nodebox",
	liquid_range = 0,

	inner_node = "testnodes:mesh",
})


-- Covered node example.
-- An simple example nodebox with one centered box
minetest.register_node("testnodes:covered_torchlike_node", {
	description = S("Covered Tochlike Test Node").."\n"..
		S("Torchlike node inside"),
	tiles = { "testnodes_glasslike.png" },
	drawtype = "covered",
	paramtype = "light",

	inner_node = "testnodes:torchlike",

	groups = {dig_immediate=3},
})

minetest.register_node("testnodes:covered_nodebox_node", {
	description = S("Covered Fixed Nodebox Test Node").."\n"..
		S("Torchlike node inside"),
	tiles = { "testnodes_glasslike.png" },
	drawtype = "covered",
	paramtype = "light",

	inner_node = "testnodes:nodebox_fixed",

	groups = {dig_immediate=3},
})

minetest.register_node("testnodes:covered_mesh_node", {
	description = S("Covered Fixed Mesh Test Node").."\n"..
		S("Torchlike node inside"),
	tiles = { "testnodes_nodebox.png" },
	drawtype = "covered",
	paramtype = "light",

	inner_node = "testnodes:mesh",

	groups = {dig_immediate=3},
})

