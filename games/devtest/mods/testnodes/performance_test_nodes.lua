-- Performance test mesh nodes

local S = minetest.get_translator("testnodes")

-- Complex mesh
minetest.register_node("testnodes:performance_mesh_clip", {
	description = S("Performance Test Node\nMarble with 'clip' transparency"),
	drawtype = "mesh",
	mesh = "testnodes_marble_glass.obj",
	tiles = {"testnodes_marble_glass.png"},
	paramtype = "light",
	use_texture_alpha = "clip",

	groups = {dig_immediate=3},
})

-- Complex mesh, alpha blending
minetest.register_node("testnodes:performance_mesh_blend", {
	description = S("Performance Test Node\nMarble with 'blend' transparency"),
	drawtype = "mesh",
	mesh = "testnodes_marble_glass.obj",
	tiles = {"testnodes_marble_glass.png"},
	paramtype = "light",
	use_texture_alpha = "blend",

	groups = {dig_immediate=3},
})

-- Overlay
minetest.register_node("testnodes:performance_overlay_clip", {
	description = S("Performance Test Node\nMarble with overlay with 'clip' transparency\nPalette for demonstration"),
	drawtype = "mesh",
	mesh = "testnodes_marble_metal.obj",
	tiles = {"testnodes_marble_metal.png"},
	overlay_tiles = {{name = "testnodes_marble_metal_overlay.png", color = "white"}},
	paramtype = "light",
	paramtype2 = "color",
	palette = "testnodes_palette_metal.png",
	color = "#705216";
	use_texture_alpha = "clip",

	groups = {dig_immediate=3},
})

-- Overlay
minetest.register_node("testnodes:performance_overlay_blend", {
	description = S("Performance Test Node\nMarble with overlay with 'blend' transparency\nPalette for demonstration"),
	drawtype = "mesh",
	mesh = "testnodes_marble_metal.obj",
	tiles = {"testnodes_marble_metal.png"},
	overlay_tiles = {{name = "testnodes_marble_metal_overlay.png", color = "white"}},
	paramtype = "light",
	paramtype2 = "color",
	palette = "testnodes_palette_metal.png",
	color = "#705216";
	use_texture_alpha = "blend",

	groups = {dig_immediate=3},
})
