stairs = {}

-- Node will be called stairs:stair_<subname>
function stairs.register_stair(subname, recipeitem, groups, images, description)
	minetest.register_node("stairs:stair_" .. subname, {
		description = description,
		drawtype = "nodebox",
		tile_images = images,
		paramtype = "light",
		paramtype2 = "facedir",
		is_ground_content = true,
		groups = groups,
		node_box = {
			type = "fixed",
			fixed = {
				{-0.5, -0.5, -0.5, 0.5, 0, 0.5},
				{-0.5, 0, 0, 0.5, 0.5, 0.5},
			},
		},
		on_place = function(itemstack, placer, pointed_thing)
			if pointed_thing.type ~= "node" then
				return itemstack
			end
			
			local p0 = pointed_thing.under
			local p1 = pointed_thing.above
			if p0.y-1 == p1.y then
				local placer_pos = placer:getpos()
				local dir = {
					x = pointed_thing.above.x - placer_pos.x,
					y = pointed_thing.above.y - placer_pos.y,
					z = pointed_thing.above.z - placer_pos.z
				}
				local param2 = minetest.dir_to_facedir(dir)
				if param2 == 0 then
					param2 = 20
				elseif param2 == 1 then
					param2 = 23
				elseif param2 == 2 then
					param2 = 22
				elseif param2 == 3 then
					param2 = 21
				end
				return minetest.item_place(itemstack, placer, pointed_thing, param2)
			end
			
			-- Otherwise place regularly
			return minetest.item_place(itemstack, placer, pointed_thing)
		end,
	})

	minetest.register_craft({
		output = 'stairs:stair_' .. subname .. ' 4',
		recipe = {
			{recipeitem, "", ""},
			{recipeitem, recipeitem, ""},
			{recipeitem, recipeitem, recipeitem},
		},
	})
end

-- Node will be called stairs:slab_<subname>
function stairs.register_slab(subname, recipeitem, groups, images, description)
	minetest.register_node("stairs:slab_" .. subname, {
		description = description,
		drawtype = "nodebox",
		tile_images = images,
		paramtype = "light",
		is_ground_content = true,
		groups = groups,
		node_box = {
			type = "fixed",
			fixed = {-0.5, -0.5, -0.5, 0.5, 0, 0.5},
		},
		selection_box = {
			type = "fixed",
			fixed = {-0.5, -0.5, -0.5, 0.5, 0, 0.5},
		},
	})

	minetest.register_craft({
		output = 'stairs:slab_' .. subname .. ' 3',
		recipe = {
			{recipeitem, recipeitem, recipeitem},
		},
	})
end

-- Nodes will be called stairs:{stair,slab}_<subname>
function stairs.register_stair_and_slab(subname, recipeitem, groups, images, desc_stair, desc_slab)
	stairs.register_stair(subname, recipeitem, groups, images, desc_stair)
	stairs.register_slab(subname, recipeitem, groups, images, desc_slab)
end

stairs.register_stair_and_slab("wood", "default:wood",
		{snappy=2,choppy=2,oddly_breakable_by_hand=2},
		{"default_wood.png"},
		"Wooden stair",
		"Wooden slab")

stairs.register_stair_and_slab("stone", "default:stone",
		{cracky=3},
		{"default_stone.png"},
		"Stone stair",
		"Stone slab")

stairs.register_stair_and_slab("cobble", "default:cobble",
		{cracky=3},
		{"default_cobble.png"},
		"Cobble stair",
		"Cobble slab")

stairs.register_stair_and_slab("brick", "default:brick",
		{cracky=3},
		{"default_brick.png"},
		"Brick stair",
		"Brick slab")

stairs.register_stair_and_slab("sandstone", "default:sandstone",
		{crumbly=2,cracky=2},
		{"default_sandstone.png"},
		"Sandstone stair",
		"Sandstone slab")
