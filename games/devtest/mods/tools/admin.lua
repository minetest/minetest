minetest.register_tool("tools:admin_pick", {
	description = "Admin pick",
	inventory_image = "handle.png^(pick_head.png^[colorize:#FF00FF)",
	tool_capabilities = {
		groupcaps = {
			crumbly = {uses=65535, times={0, 0, 0}},
			cracky = {uses=65535, times={0, 0, 0}},
			snappy = {uses=65535, times={0, 0, 0}},
			choppy = {uses=65535, times={0, 0, 0}},
		},
	},
})
