-- bucket (Minetest 0.4 mod)
-- A bucket, which can pick up water and lava

minetest.alias_craftitem("bucket", "bucket:bucket_empty")
minetest.alias_craftitem("bucket_water", "bucket:bucket_water")
minetest.alias_craftitem("bucket_lava", "bucket:bucket_lava")

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
			if n.name == "default:water_source" then
				minetest.env:add_node(pointed_thing.under, {name="air"})
				player:add_to_inventory_later('craft "bucket:bucket_water" 1')
				return true
			elseif n.name == "default:lava_source" then
				minetest.env:add_node(pointed_thing.under, {name="air"})
				player:add_to_inventory_later('craft "bucket:bucket_lava" 1')
				return true
			end
		end
		return false
	end,
})

minetest.register_craftitem("bucket:bucket_water", {
	image = "bucket_water.png",
	stack_max = 1,
	liquids_pointable = true,
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = function(item, player, pointed_thing)
		if pointed_thing.type == "node" then
			n = minetest.env:get_node(pointed_thing.under)
			if n.name == "default:water_source" then
				-- unchanged
			elseif n.name == "default:water_flowing" or n.name == "default:lava_source" or n.name == "default:lava_flowing" then
				minetest.env:add_node(pointed_thing.under, {name="default:water_source"})
			else
				minetest.env:add_node(pointed_thing.above, {name="default:water_source"})
			end
			player:add_to_inventory_later('craft "bucket:bucket_empty" 1')
			return true
		end
		return false
	end,
})

minetest.register_craftitem("bucket:bucket_lava", {
	image = "bucket_lava.png",
	stack_max = 1,
	liquids_pointable = true,
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = function(item, player, pointed_thing)
		if pointed_thing.type == "node" then
			n = minetest.env:get_node(pointed_thing.under)
			if n.name == "default:lava_source" then
				-- unchanged
			elseif n.name == "default:water_source" or n.name == "default:water_flowing" or n.name == "default:lava_flowing" then
				minetest.env:add_node(pointed_thing.under, {name="default:lava_source"})
			else
				minetest.env:add_node(pointed_thing.above, {name="default:lava_source"})
			end
			player:add_to_inventory_later('craft "bucket:bucket_empty" 1')
			return true
		end
		return false
	end,
})
