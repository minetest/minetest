minetest.register_node("nodes:glasslike", {
	description = "Glass",
	drawtype = "glasslike",
	paramtype2 = "facedir",
	tiles = {
		{name="white_transparent.png^[colorize:#FF0000^arrow.png"},
		{name="white_transparent.png^[colorize:#FFFF00^arrow.png"},
		{name="white_transparent.png^[colorize:#00FF00^arrow.png"},
		{name="white_transparent.png^[colorize:#00FFFF^arrow.png"},
		{name="white_transparent.png^[colorize:#0000FF^arrow.png"},
		{name="white_transparent.png^[colorize:#FF00FF^arrow.png"},
	},
	groups = {cracky=3},
})

minetest.register_node("nodes:glasslike_framed", {
	description = "Framed Glass",
	drawtype = "glasslike_framed",
	paramtype2 = "facedir",
	tiles = {
		{name="white_transparent.png^[colorize:#FF0000^arrow.png"},
		{name="white_transparent.png^[colorize:#FFFF00^arrow.png"},
		{name="white_transparent.png^[colorize:#00FF00^arrow.png"},
		{name="white_transparent.png^[colorize:#00FFFF^arrow.png"},
		{name="white_transparent.png^[colorize:#0000FF^arrow.png"},
		{name="white_transparent.png^[colorize:#FF00FF^arrow.png"},
	},
	groups = {cracky=3},
})
