minetest.register_tool("tools:admin_pick", {
	description = "Admin pick",
	inventory_image = "handle.png^(pick_head.png^[colorize:#FF00FF)",
	tool_capabilities = {
		groupcaps = {
			oddly_breakable_by_hand = {uses=0, times={0.5, 0.5, 0.5}},
			crumbly = {uses=0, times={0.5, 0.5, 0.5}},
			cracky = {uses=0, times={0.5, 0.5, 0.5}},
			snappy = {uses=0, times={0.5, 0.5, 0.5}},
			choppy = {uses=0, times={0.5, 0.5, 0.5}},
		},
	},
})
