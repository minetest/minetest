local S = minetest.get_translator("testnodes")

-- Nodebox examples and tests.

-- An simple example nodebox with one centered box
minetest.register_node("testnodes:nodebox_fixed", {
	description = S("Fixed Nodebox Test Node").."\n"..
		S("Nodebox is always the same"),
	tiles = {"testnodes_nodebox.png"},
	drawtype = "nodebox",
	paramtype = "light",
	node_box = {
		type = "fixed",
		fixed = {-0.25, -0.25, -0.25, 0.25, 0.25, 0.25},
	},

	groups = {dig_immediate=3},
})

-- 50% higher than a regular node
minetest.register_node("testnodes:nodebox_overhigh", {
	description = S("+50% high Nodebox Test Node"),
	tiles = {"testnodes_nodebox.png"},
	drawtype = "nodebox",
	paramtype = "light",
	node_box = {
		type = "fixed",
		fixed = {-0.5, -0.5, -0.5, 0.5, 1, 0.5},
	},

	groups = {dig_immediate=3},
})

-- 95% higher than a regular node
minetest.register_node("testnodes:nodebox_overhigh2", {
	description = S("+95% high Nodebox Test Node"),
	tiles = {"testnodes_nodebox.png"},
	drawtype = "nodebox",
	paramtype = "light",
	node_box = {
		type = "fixed",
		-- Y max: more is possible, but glitchy
		fixed = {-0.5, -0.5, -0.5, 0.5, 1.45, 0.5},
	},

	groups = {dig_immediate=3},
})

-- Height of nodebox changes with its param2 value
minetest.register_node("testnodes:nodebox_leveled", {
	description = S("Leveled Nodebox Test Node").."\n"..
		S("param2 = height (0..127)"),
	tiles = {"testnodes_nodebox.png^[colorize:#0F0:32"},
	drawtype = "nodebox",
	paramtype = "light",
	paramtype2 = "leveled",
	node_box = {
		type = "leveled",
		fixed = {-0.5, 0.0, -0.5, 0.5, -0.499, 0.5},
	},

	groups = {dig_immediate=3},
})


local nodebox_wall = {
	type = "connected",
	fixed = {-0.125, -0.500, -0.125, 0.125, 0.500, 0.125},
	connect_front = {-0.125, -0.500, -0.500, 0.125, 0.400, -0.125},
	connect_back = {-0.125, -0.500, 0.125, 0.125, 0.400, 0.500},
	connect_left = {-0.500, -0.500, -0.125, -0.125, 0.400, 0.125},
	connect_right = {0.125, -0.500, -0.125, 0.500, 0.400, 0.125},
}

local nodebox_wall_thick = {
	type = "connected",
	fixed = {-0.25, -0.500, -0.25, 0.25, 0.500, 0.25},
	connect_front = {-0.25, -0.500, -0.500, 0.25, 0.400, -0.25},
	connect_back = {-0.25, -0.500, 0.25, 0.25, 0.400, 0.500},
	connect_left = {-0.500, -0.500, -0.25, -0.25, 0.400, 0.25},
	connect_right = {0.25, -0.500, -0.25, 0.500, 0.400, 0.25},
}

-- Wall-like nodebox that connects to neighbors
minetest.register_node("testnodes:nodebox_connected", {
	description = S("Connected Nodebox Test Node").."\n"..
		S("Connects to neighbors"),
	tiles = {"testnodes_nodebox.png^[colorize:#F00:32"},
	groups = {connected_nodebox=1, dig_immediate=3},
	drawtype = "nodebox",
	paramtype = "light",
	connects_to = {"group:connected_nodebox"},
	connect_sides = {"front", "back", "left", "right"},
	node_box = nodebox_wall,
})

minetest.register_node("testnodes:nodebox_connected_facedir", {
	description = S("Facedir Connected Nodebox Test Node").."\n"..
		S("Connects to neighbors").."\n"..
		S("param2 = facedir rotation of textures (not of the nodebox!)"),
	tiles = {
		"testnodes_1.png",
		"testnodes_2.png",
		"testnodes_3.png",
		"testnodes_4.png",
		"testnodes_5.png",
		"testnodes_6.png",
	},
	groups = {connected_nodebox=1, dig_immediate=3},
	drawtype = "nodebox",
	paramtype = "light",
	paramtype2 = "facedir",
	connects_to = {"group:connected_nodebox"},
	connect_sides = {"front", "back", "left", "right"},
	node_box = nodebox_wall_thick,
})

minetest.register_node("testnodes:nodebox_connected_4dir", {
	description = S("4Dir Connected Nodebox Test Node").."\n"..
		S("Connects to neighbors").."\n"..
		S("param2 = 4dir rotation of textures (not of the nodebox!)"),
	tiles = {
		"testnodes_1f.png",
		"testnodes_2f.png",
		"testnodes_3f.png",
		"testnodes_4f.png",
		"testnodes_5f.png",
		"testnodes_6f.png",
	},
	groups = {connected_nodebox=1, dig_immediate=3},
	drawtype = "nodebox",
	paramtype = "light",
	paramtype2 = "4dir",
	connects_to = {"group:connected_nodebox"},
	connect_sides = {"front", "back", "left", "right"},
	node_box = nodebox_wall_thick,
})

