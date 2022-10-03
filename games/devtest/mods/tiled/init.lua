minetest.register_node("tiled:tiled", {
        description = "Tiled Node (world-aligned)",
        tiles = {{
                name = "tiled_tiled.png",
                align_style = "world",
                scale = 8,
        }},
        groups = {cracky=3},
})

minetest.register_node("tiled:tiled_n", {
        description = "Tiled Node (node-aligned)",
        tiles = {{
                name = "tiled_tiled_node.png",
                align_style = "node",
        }},
        groups = {cracky=3},
})

stairs.register_stair_and_slab("tiled_n", "tiled:tiled_n",
		{cracky=3},
		{{name="tiled_tiled_node.png", align_style="node"}},
		"Tiled Stair (node-aligned)",
		"Tiled Slab (node-aligned)")

stairs.register_stair_and_slab("tiled", "tiled:tiled",
		{cracky=3},
		{{name="tiled_tiled.png", align_style="world", scale=8}},
		"Tiled Stair (world-aligned)",
		"Tiled Slab (world-aligned)")


