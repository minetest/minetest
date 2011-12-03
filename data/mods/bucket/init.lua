minetest.register_craft({
	output = 'craft "bucket:bucket_empty" 1',
	recipe = {
		{'craft "steel_ingot"', '', 'craft "steel_ingot"'},
		{'', 'craft "steel_ingot"', ''},
	}
})

minetest.register_craftitem("bucket:bucket_empty", {
	image = "bucket.png",
	stack_max = 1,
	liquids_pointable = true,
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = function(item, player, pointed_thing)
		if pointed_thing.type == "node" then
			n = minetest.env:get_node(pointed_thing.under)
			if n.name == "water_source" then
				minetest.env:add_node(pointed_thing.under, {name="air"})
				player:add_to_inventory_later('craft "bucket:bucket_water" 1')
				return true
			elseif n.name == "lava_source" then
				minetest.env:add_node(pointed_thing.under, {name="air"})
				player:add_to_inventory_later('craft "bucket:bucket_lava" 1')
				return true
			end
		end
		return false
	end,
})

minetest.register_craftitem("bucket:bucket_water", {
	image = "bucket:bucket_water.png",
	stack_max = 1,
	liquids_pointable = true,
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = function(item, player, pointed_thing)
		if pointed_thing.type == "node" then
			n = minetest.env:get_node(pointed_thing.under)
			if n.name == "water_source" then
				-- unchanged
			elseif n.name == "water_flowing" or n.name == "lava_source" or n.name == "lava_flowing" then
				minetest.env:add_node(pointed_thing.under, {name="water_source"})
			else
				minetest.env:add_node(pointed_thing.above, {name="water_source"})
			end
			player:add_to_inventory_later('craft "bucket:bucket_empty" 1')
			return true
		end
		return false
	end,
})

minetest.register_craftitem("bucket:bucket_lava", {
	image = "bucket:bucket_lava.png",
	stack_max = 1,
	liquids_pointable = true,
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = function(item, player, pointed_thing)
		if pointed_thing.type == "node" then
			n = minetest.env:get_node(pointed_thing.under)
			if n.name == "lava_source" then
				-- unchanged
			elseif n.name == "water_source" or n.name == "water_flowing" or n.name == "lava_flowing" then
				minetest.env:add_node(pointed_thing.under, {name="lava_source"})
			else
				minetest.env:add_node(pointed_thing.above, {name="lava_source"})
			end
			player:add_to_inventory_later('craft "bucket:bucket_empty" 1')
			return true
		end
		return false
	end,
})
