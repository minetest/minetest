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
	groups = {cracky=3},
})

minetest.register_node("nodes:glasslike_framed", {
	description = "Framed Glass",
	drawtype = "glasslike_framed",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	groups = {cracky=3},
})

minetest.register_node("nodes:allfaces", {
	description = "Allfaces",
	drawtype = "allfaces",
	paramtype = "light",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	groups = {cracky=3},
})

--[[
- `torchlike`
- `signlike`
- `plantlike`
- `firelike`
- `fencelike`
- `raillike`
- `nodebox` -- See below
- `mesh` -- Use models for nodes, see below
- `plantlike_rooted` -- See below
]]