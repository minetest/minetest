local S = minetest.get_translator("testnodes")

-- Sunken node example.
minetest.register_node("testnodes:sunken_torchlike", {
	description = "Sunken Torchluje Test Node in Liquid Source Range 5",
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
	liquid_alternative_flowing = "testnodes:sunken_torchlike",
	liquid_alternative_source = "testnodes:sunken_torchlike",
	liquid_range = 0,

	inner_node = "testnodes:torchlike",
})

-- Covered node example.
-- An simple example nodebox with one centered box
minetest.register_node("testnodes:covered_torchlike_node", {
	description = S("Covered Tochlike Test Node").."\n"..
		S("Torchlike node inside"),
	--tiles = {"testnodes_nodebox.png"},
	tiles = { "testnodes_glasslike.png" },
	drawtype = "covered",
	paramtype = "light",

	inner_node = "testnodes:torchlike",

	groups = {dig_immediate=3},
})

