minetest.register_craftitem("gun_powder:gun_powder", {
	description = "Gun Powder",
	inventory_image = "gun_powder.png",
})
	
minetest.register_node("gun_powder:gun_stone", {
	description = "Stone with gun powder",
	tile_images = {"default_stone.png^gun_powder_mineral.png"},
	is_ground_content = true,
	groups = {cracky=3},
	drop = 'gun_powder:gun_powder',
})

