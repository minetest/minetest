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

local torch_box = {
	type = "wallmounted",
	wall_top = {-0.2, -0.4, -0.2, 0.2, 0.5, 0.2},
	wall_bottom = {-0.2, -0.5, -0.2, 0.2, 0.4, 0.2},
	wall_side = {-0.5, -0.4, -0.2, -0.1, 0.4, 0.2},
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
	inventory_image = "torch_floor_flame.png",
	inventory_overlay = "torch_floor.png",
	wield_image = "torch_floor_flame.png",
	wield_overlay = "torch_floor_flame.png",
	selection_box = torch_box,
	walkable = false,
	groups = {attached_node=1, dig_immediate=3},
})

minetest.register_node("nodes:torch_off", {
	description = "Torch (off)",
	drawtype = "torchlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	tiles = torch_tiles,
	overlay_tiles = torch_overlays,
	color = "#000000",
	inventory_image = "torch_floor_flame.png",
	inventory_overlay = "torch_floor.png",
	wield_image = "torch_floor_flame.png",
	wield_overlay = "torch_floor_flame.png",
	selection_box = torch_box,
	walkable = false,
	groups = {attached_node=1, dig_immediate=3},
})

local sign_box = {
	type = "fixed",
	fixed = {{-0.5, -0.5, -0.4, 0.5, -0.4, 0.4}},
}

minetest.register_node("nodes:signlike", {
	description = "Signlike",
	drawtype = "signlike",
	paramtype = "light",
	paramtype2 = "colorwallmounted",
	tiles = {{name="sign.png", color="#FFDD99"}},
	overlay_tiles = {{name="sign_frame.png"}},
	palette = "vgaplus.png",
	color = "#000000", -- default item color
	inventory_image = "sign_frame.png",
	inventory_overlay = "sign.png",
	wield_image = "sign_frame.png",
	wield_overlay = "sign.png",
	selection_box = sign_box,
	collision_box = sign_box,
	groups = {attached_node=1, dig_immediate=3},
})

minetest.register_node("nodes:signlike_fancy", {
	description = "Fancy Signlike",
	drawtype = "signlike",
	paramtype = "light",
	paramtype2 = "colorwallmounted",
	tiles = {{name="sign_base.png"}},
	overlay_tiles = {{name="sign_area.png", color="#FFDD99"}},
	palette = "vgaplus.png",
	color = "#000000", -- default item color
	inventory_image = "sign_base.png",
	inventory_overlay = "sign_area.png",
	wield_image = "sign_base.png",
	wield_overlay = "sign_area.png",
	selection_box = sign_box,
	collision_box = sign_box,
	groups = {attached_node=1, dig_immediate=3},
})

minetest.register_node("nodes:plantlike", {
	description = "Plantlike",
	drawtype = "plantlike",
	paramtype = "light",
	paramtype2 = "meshoptions",
	tiles = {{name="plant.png"}},
	walkable = false,
	groups = {snappy=3},
})

minetest.register_node("nodes:plantlike_rotary", {
	description = "Rotary Plantlike",
	drawtype = "plantlike",
	paramtype = "light",
	paramtype2 = "degrotate",
	tiles = {{name="flower.png"}},
	walkable = false,
	groups = {snappy=3},
	place_param2 = 15,
})

minetest.register_node("nodes:plantlike_leveled", {
	description = "Leveled Plantlike",
	drawtype = "plantlike",
	paramtype = "light",
	paramtype2 = "leveled",
	tiles = {{name="vplant.png"}},
	collision_box = {type="leveled", fixed={-0.4, -0.5, -0.4, 0.4, -0.5, 0.4}},
	selection_box = {type="leveled", fixed={-0.4, -0.5, -0.4, 0.4, -0.5, 0.4}},
	groups = {snappy=2},
	place_param2 = 21,
})

--[[
- `firelike`
- `fencelike`
- `raillike`
- `nodebox` -- See below
- `mesh` -- Use models for nodes, see below
- `plantlike_rooted` -- See below
]]