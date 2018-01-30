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

local torch_tiles = {
	{name="torch_floor.png", color="#FFFFFF"},
	{name="torch_ceil.png", color="#FFFFFF"},
	{name="torch_wall.png", color="#FFFFFF"},
}

local torch_overlays = {
	{name="torch_floor_flame.png"},
	{name="torch_ceil_flame.png"},
	{name="torch_wall_flame.png"},
}

minetest.register_node("nodes:torch", {
	description = "Torch",
	drawtype = "torchlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	light_source = 10,
	tiles = torch_tiles,
	overlay_tiles = torch_overlays,
	color = "#FFFF99",
	groups = {attached_node=1, dig_immediate=3},
	inventory_image = "torch_floor.png^torch_floor_flame.png",
	wield_image = "torch_floor.png^torch_floor_flame.png",
})

minetest.register_node("nodes:torch_off", {
	description = "Torch (off)",
	drawtype = "torchlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	tiles = torch_tiles,
	overlay_tiles = torch_overlays,
	color = "#000000",
	groups = {attached_node=1, dig_immediate=3},
	inventory_image = "torch_floor.png^(torch_floor_flame.png^[colorize:#000000)",
	wield_image = "torch_floor.png^(torch_floor_flame.png^[colorize:#000000)",
})

minetest.register_node("nodes:signlike", {
	description = "Signlike",
	drawtype = "signlike",
	paramtype = "light",
	paramtype2 = "colorwallmounted",
	tiles = {{name="sign.png", color="#FFDD99"}},
	overlay_tiles = {{name="sign_frame.png"}},
	palette = "vgaplus.png",
	groups = {attached_node=1, dig_immediate=3},
	inventory_image = "sign.png^(sign_frame.png^[colorize:#000000)",
	wield_image = "sign.png^(sign_frame.png^[colorize:#000000)",
})

--[[
- `plantlike`
- `firelike`
- `fencelike`
- `raillike`
- `nodebox` -- See below
- `mesh` -- Use models for nodes, see below
- `plantlike_rooted` -- See below
]]