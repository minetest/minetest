minetest.register_craftitem("gun_powder:gun_powder", {
	inventory_image = "gun_powder.png",
	stack_max = 64,
})
	
minetest.register_node("gun_powder:gun_stone", {
	description = "Stone",
	tile_images = {"gun_stone.png"},
	is_ground_content = true,
	groups = {cracky=3},
	drop = 'gun_powder:gun_powder',
	legacy_mineral = true,
})