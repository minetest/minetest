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
		use_texture_alpha = true,

		groups = { dig_immediate = 3 },
	})

	-- Transparency set via "alpha" parameter
	minetest.register_node("testnodes:alpha_"..alpha, {
		description = S("Alpha Test Node (@1)", alpha),
		-- It seems that only the liquid drawtype supports the alpha parameter
		drawtype = "liquid",
		paramtype = "light",
		tiles = {
			"testnodes_alpha.png",
		},
		alpha = alpha,


		liquidtype = "source",
		liquid_range = 0,
		liquid_viscosity = 0,
		liquid_alternative_source = "testnodes:alpha_"..alpha,
		liquid_alternative_flowing = "testnodes:alpha_"..alpha,
		groups = { dig_immediate = 3 },
	})
end


-- Bumpmapping and Parallax Occlusion

-- This node has a normal map which corresponds to a pyramid with sides tilted
-- by an angle of 45°, i.e. the normal map contains four vectors which point
-- diagonally away from the surface (e.g. (0.7, 0.7, 0)),
-- and the heights in the height map linearly increase towards the centre,
-- so that the surface corresponds to a simple pyramid.
-- The node can help to determine if e.g. tangent space transformations work
-- correctly.
-- If, for example, the light comes from above, then the (tilted) pyramids
-- should look like they're lit from this light direction on all node faces.
-- The white albedo texture has small black indicators which can be used to see
-- how it is transformed ingame (and thus see if there's rotation around the
-- normal vector).
minetest.register_node("testnodes:height_pyramid", {
	description = "Bumpmapping and Parallax Occlusion Tester (height pyramid)",
	tiles = {"testnodes_height_pyramid.png"},
	groups = {dig_immediate = 3},
})

-- The stairs nodes should help to validate if shading works correctly for
-- rotated nodes (which have rotated textures).
stairs.register_stair_and_slab("height_pyramid", "experimantal:height_pyramid",
	{dig_immediate = 3},
	{"testnodes_height_pyramid.png"},
	"Bumpmapping and Parallax Occlusion Tester Stair (height pyramid)",
	"Bumpmapping and Parallax Occlusion Tester Slab (height pyramid)")

-- This node has a simple heightmap for parallax occlusion testing and flat
-- normalmap.
-- When parallax occlusion is enabled, the yellow scrawl should stick out of
-- the texture when viewed at an angle.
minetest.register_node("testnodes:parallax_extruded", {
	description = "Parallax Occlusion Tester",
	tiles = {"testnodes_parallax_extruded.png"},
	groups = {dig_immediate = 3},
})

-- Analogously to the height pyramid stairs nodes,
-- these nodes should help to validate if parallax occlusion works correctly for
-- rotated nodes (which have rotated textures).
stairs.register_stair_and_slab("parallax_extruded",
	"experimantal:parallax_extruded",
	{dig_immediate = 3},
	{"testnodes_parallax_extruded.png"},
	"Parallax Occlusion Tester Stair",
	"Parallax Occlusion Tester Slab")
