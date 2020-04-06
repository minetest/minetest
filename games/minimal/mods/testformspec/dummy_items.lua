minetest.register_node("testformspec:node", {
	description = "Formspec Test Node",
	tiles = { "testformspec_node.png" },
	groups = { dig_immediate = 3, dummy = 1 },
})

minetest.register_craftitem("testformspec:item", {
	description = "Formspec Test Item",
	inventory_image = "testformspec_item.png",
	groups = { dummy = 1 },
})
