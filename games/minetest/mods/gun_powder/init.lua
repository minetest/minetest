function gunpowder(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="default_hard_footstep", gain=0.25}
	table.dig = table.dig or
			{name="default_hard_footstep", gain=0.4}
	table.dug = table.dug or
			{name="default_hard_footstep", gain=1.0}
	--node_sound_defaults(table)
	return table
end

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
	sounds = gunpowder(),
})

