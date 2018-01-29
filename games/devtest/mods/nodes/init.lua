local cube_tiles = {
	{name="white_transparent.png^[colorize:#FF0000^arrow.png"},
	{name="white_transparent.png^[colorize:#FFFF00^arrow.png"},
	{name="white_transparent.png^[colorize:#00FF00^arrow.png"},
	{name="white_transparent.png^[colorize:#00FFFF^arrow.png"},
	{name="white_transparent.png^[colorize:#0000FF^arrow.png"},
	{name="white_transparent.png^[colorize:#FF00FF^arrow.png"},
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
	paramtype2 = "facedir",
	tiles = cube_tiles,
	groups = {cracky=3},
})

minetest.register_node("nodes:glasslike_framed", {
	description = "Framed Glass",
	drawtype = "glasslike_framed",
	paramtype2 = "facedir",
	tiles = cube_tiles,
	groups = {cracky=3},
})

--[[
- `allfaces`
- `allfaces_optional`
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