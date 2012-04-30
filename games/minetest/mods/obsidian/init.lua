minetest.register_node("obsidian:obsidian_block", {
	tile_images = {"obsidian_block.png"},
	inventory_image = minetest.inventorycube("obsidian_block.png"),
	is_ground_content = true,
	groups = {oddly_breakable_by_hand=1},
	drop = "obsidian:obsidian_block",
})

minetest.register_abm({nodenames = {"default:lava_source"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_obsidianject_count, active_obsidianject_count_wider)
		for i=-1,1 do
			for j=-1,1 do
				for k=-1,1 do
					p = {x=pos.x+i, y=pos.y+j, z=pos.z+k}
					n = minetest.env:get_node(p)
					if (n.name == "default:water_flowing") or (n.name == "default:water_source") then
						if not (((p.x > pos.x) and (p.z > pos.z)) or ((p.x < pos.x) and (p.z < pos.z)) or ((p.x < pos.x) and (p.z > pos.z)) or ((p.x > pos.x) and (p.z < pos.z))) then
							minetest.env:add_node(pos, {name="obsidian:obsidian_block"})
						end
					end
				end
			end
		end
	end
})

minetest.register_abm({nodenames = {"default:lava_flowing"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_obsidianject_count, active_obsidianject_count_wider)
		for i=-1,1 do
			for j=-1,1 do
				for k=-1,1 do
					p = {x=pos.x+i, y=pos.y+j, z=pos.z+k}
					n = minetest.env:get_node(p)
					if (n.name == "default:water_flowing") or (n.name == "default:water_source") then
						if not (((p.x > pos.x) and (p.z > pos.z)) or ((p.x < pos.x) and (p.z < pos.z)) or ((p.x < pos.x) and (p.z > pos.z)) or ((p.x > pos.x) and (p.z < pos.z))) then
							if (j == -1) then
								minetest.env:add_node({x=pos.x, y=pos.y-1, z=pos.z}, {name="obsidian:obsidian_block"})
							else
								minetest.env:add_node(pos, {name="cobble"})
							end
						end
					end
				end
			end
		end
	end
})

print( 'Obsidian Mod Loaded! ' )

