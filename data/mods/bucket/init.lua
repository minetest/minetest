-- bucket (Minetest 0.4 mod)
-- A bucket, which can pick up water and lava

minetest.register_alias("bucket", "bucket:bucket_empty")
minetest.register_alias("bucket_water", "bucket:bucket_water")
minetest.register_alias("bucket_lava", "bucket:bucket_lava")

minetest.register_craft({
	output = 'bucket:bucket_empty 1',
	recipe = {
		{'default:steel_ingot', '', 'default:steel_ingot'},
		{'', 'default:steel_ingot', ''},
	}
})

bucket = {}
bucket.liquids = {}

-- Register a new liquid
--   source = name of the source node
--   flowing = name of the flowing node
--   itemname = name of the new bucket item (or nil if liquid is not takeable)
--   inventory_image = texture of the new bucket item (ignored if itemname == nil)
-- This function can be called from any mod (that depends on bucket).
function bucket.register_liquid(source, flowing, itemname, inventory_image)
	bucket.liquids[source] = {
		source = source,
		flowing = flowing,
		itemname = itemname,
	}
	bucket.liquids[flowing] = bucket.liquids[source]

	if itemname ~= nil then
		minetest.register_craftitem(itemname, {
			inventory_image = inventory_image,
			stack_max = 1,
			liquids_pointable = true,
			on_use = function(itemstack, user, pointed_thing)
				-- Must be pointing to node
				if pointed_thing.type ~= "node" then
					return
				end
				-- Check if pointing to a liquid
				n = minetest.env:get_node(pointed_thing.under)
				if bucket.liquids[n.name] == nil then
					-- Not a liquid
					minetest.env:add_node(pointed_thing.above, {name=source})
				elseif n.name ~= source then
					-- It's a liquid
					minetest.env:add_node(pointed_thing.under, {name=source})
				end
				return {name="bucket:bucket_empty"}
			end
		})
	end
end

minetest.register_craftitem("bucket:bucket_empty", {
	inventory_image = "bucket.png",
	stack_max = 1,
	liquids_pointable = true,
	on_use = function(itemstack, user, pointed_thing)
		-- Must be pointing to node
		if pointed_thing.type ~= "node" then
			return
		end
		-- Check if pointing to a liquid source
		n = minetest.env:get_node(pointed_thing.under)
		liquiddef = bucket.liquids[n.name]
		if liquiddef ~= nil and liquiddef.source == n.name and liquiddef.itemname ~= nil then
			minetest.env:add_node(pointed_thing.under, {name="air"})
			return {name=liquiddef.itemname}
		end
	end,
})

bucket.register_liquid(
	"default:water_source",
	"default:water_flowing",
	"bucket:bucket_water",
	"bucket_water.png"
)

bucket.register_liquid(
	"default:lava_source",
	"default:lava_flowing",
	"bucket:bucket_lava",
	"bucket_lava.png"
)

minetest.register_craft({
	type = "fuel",
	recipe = "default:bucket_lava",
	burntime = 60,
})
