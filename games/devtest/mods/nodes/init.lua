local cube_tiles = {
	{name="white_transparent.png^[colorize:#FF0000^arrow2.png"},
	{name="white_transparent.png^[colorize:#FFFF00^arrow2.png"},
	{name="white_transparent.png^[colorize:#00FF00^arrow2.png"},
	{name="white_transparent.png^[colorize:#00FFFF^arrow2.png"},
	{name="white_transparent.png^[colorize:#0000FF^arrow2.png"},
	{name="white_transparent.png^[colorize:#FF00FF^arrow2.png"},
}

minetest.register_node("nodes:solid", {
	description = "Solid",
	drawtype = "normal",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	groups = {cracky=3},
})

minetest.register_node("nodes:glasslike", {
	description = "Glass",
	drawtype = "glasslike",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	groups = {cracky=3, oddly_breakable_by_hand=1},
})

minetest.register_node("nodes:glasslike_framed", {
	description = "Framed Glass",
	drawtype = "glasslike_framed",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	groups = {cracky=3, oddly_breakable_by_hand=1},
})

minetest.register_node("nodes:allfaces", {
	description = "Allfaces",
	drawtype = "allfaces",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
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