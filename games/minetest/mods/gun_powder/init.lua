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
	sounds = default.node_sound_stone_defaults(),
})

function default.node_sound_stone_defaults(table)
	table = table or {}
	table.footstep = table.footstep or
			{name="default_hard_footstep", gain=0.2}
	default.node_sound_defaults(table)
	return table
end

