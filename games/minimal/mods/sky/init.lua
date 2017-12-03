minetest.register_node("sky:cloud", {
	description = "Cloud",
	tiles = {{name="cloud.png"}},
	groups = {oddly_breakable_by_hand=3},
	drawtype = "plantlike",
	use_texture_alpha = true,
	visual_scale = 2.0,
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
})

minetest.register_ore({
	ore_type = "puff",
	ore = "sky:cloud",
	wherein = "air",
	y_min = 48,
	y_max = 256,
	noise_threshold = 0.5,
	noise_params = {
		offset = 0,
		scale = 1,
		spread = {x = 100, y = 100, z = 100},
		seed = 23,
		octaves = 3,
		persist = 0.7
	},
	np_puff_top = {
		offset = 0,
		scale = 12,
		spread = {x = 100, y = 100, z = 100},
		seed = 47,
		octaves = 3,
		persist = 0.7
	},
	np_puff_bottom = {
		offset = 0,
		scale = 12,
		spread = {x = 100, y = 100, z = 100},
		seed = 11,
		octaves = 3,
		persist = 0.7
	},
})
