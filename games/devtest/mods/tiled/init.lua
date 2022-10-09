local align_help = "Texture spans over a space of 8×8 nodes"
local align_help_n = "Tiles looks the same for every node"

minetest.register_node("tiled:tiled", {
        description = "Tiled Node (world-aligned)".."\n"..align_help,
        tiles = {{
                name = "tiled_tiled.png",
                align_style = "world",
                scale = 8,
        }},
        groups = {cracky=3},
})

minetest.register_node("tiled:tiled_n", {
        description = "Tiled Node (node-aligned)".."\n"..align_help_n,
        tiles = {{
                name = "tiled_tiled_node.png",
                align_style = "node",
        }},
        groups = {cracky=3},
})

stairs.register_stair_and_slab("tiled_n", "tiled:tiled_n",
		{cracky=3},
		{{name="tiled_tiled_node.png", align_style="node"}},
		"Tiled Stair (node-aligned)".."\n"..align_help_n,
		"Tiled Slab (node-aligned)".."\n"..align_help_n)

stairs.register_stair_and_slab("tiled", "tiled:tiled",
		{cracky=3},
		{{name="tiled_tiled.png", align_style="world", scale=8}},
		"Tiled Stair (world-aligned)".."\n"..align_help,
		"Tiled Slab (world-aligned)".."\n"..align_help)


