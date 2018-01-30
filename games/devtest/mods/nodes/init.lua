local cube_tiles = {
	{name="white_transparent.png^arrow2.png", color="#FF0000"},
	{name="white_transparent.png^arrow2.png", color="#FFFF00"},
	{name="white_transparent.png^arrow2.png", color="#00FF00"},
	{name="white_transparent.png^arrow2.png", color="#00FFFF"},
	{name="white_transparent.png^arrow2.png", color="#0000FF"},
	{name="white_transparent.png^arrow2.png", color="#FF00FF"},
}

local cube_overlays = {
	{name="arrow.png", color="#FFFFFF"},
}

minetest.register_node("nodes:solid", {
	description = "Solid",
	drawtype = "normal",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	overlay_tiles = cube_overlays,
	groups = {cracky=3},
})

minetest.register_node("nodes:glasslike", {
	description = "Glass",
	drawtype = "glasslike",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	overlay_tiles = cube_overlays,
	groups = {cracky=3, oddly_breakable_by_hand=1},
})

minetest.register_node("nodes:glasslike_framed", {
	description = "Framed Glass",
	drawtype = "glasslike_framed",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	overlay_tiles = cube_overlays,
	groups = {cracky=3, oddly_breakable_by_hand=1},
})

minetest.register_node("nodes:allfaces", {
	description = "Allfaces",
	drawtype = "allfaces",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	overlay_tiles = cube_overlays,
	groups = {cracky=3, snappy=3},
})

minetest.register_node("nodes:torch", {
	description = "Torch",
	drawtype = "torchlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	light_source = 10,
	tiles = {
		{name="torch_floor.png"},
		{name="torch_ceil.png"},
		{name="torch_wall.png"},
	},
	groups = {attached_node=1, dig_immediate=3},
	inventory_image = "torch_floor.png",
	wield_image = "torch_floor.png",
})

minetest.register_node("nodes:torch_off", {
	description = "Torch (off)",
	drawtype = "torchlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	tiles = {
		{name="torch_floor_off.png"},
		{name="torch_ceil_off.png"},
		{name="torch_wall_off.png"},
	},
	groups = {attached_node=1, dig_immediate=3},
	inventory_image = "torch_floor_off.png",
	wield_image = "torch_floor_off.png",
})

--[[
- `signlike`
- `plantlike`
- `firelike`
- `fencelike`
- `raillike`
- `nodebox` -- See below
- `mesh` -- Use models for nodes, see below
- `plantlike_rooted` -- See below
]]