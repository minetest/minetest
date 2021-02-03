-- Node texture tests

local S = minetest.get_translator("testnodes")

minetest.register_node("testnodes:6sides", {
	description = S("Six Textures Test Node"),
	tiles = {
		"testnodes_normal1.png",
		"testnodes_normal2.png",
		"testnodes_normal3.png",
		"testnodes_normal4.png",
		"testnodes_normal5.png",
		"testnodes_normal6.png",
	},

	groups = { dig_immediate = 2 },
})

minetest.register_node("testnodes:anim", {
	description = S("Animated Test Node"),
	tiles = {
		{ name = "testnodes_anim.png",
		animation = {
			type = "vertical_frames",
			aspect_w = 16,
			aspect_h = 16,
			length = 4.0,
		}, },
	},

	groups = { dig_immediate = 2 },
})

-- Node texture transparency test

local alphas = { 64, 128, 191 }

for a=1,#alphas do
	local alpha = alphas[a]

	-- Transparency taken from texture
	minetest.register_node("testnodes:alpha_texture_"..alpha, {
		description = S("Texture Alpha Test Node (@1)", alpha),
		drawtype = "glasslike",
		paramtype = "light",
		tiles = {
			"testnodes_alpha"..alpha..".png",
		},
		use_texture_alpha = "blend",

		groups = { dig_immediate = 3 },
	})

	-- Transparency set via texture modifier
	minetest.register_node("testnodes:alpha_"..alpha, {
		description = S("Alpha Test Node (@1)", alpha),
		drawtype = "glasslike",
		paramtype = "light",
		tiles = {
			"testnodes_alpha.png^[opacity:" .. alpha,
		},
		use_texture_alpha = "blend",

		groups = { dig_immediate = 3 },
	})
end

