-- Moving node

minetest.register_node("testnodes:bulk_abm", {
	description = "Bulk ABM Test Node".."\n"..
		"Moves in a pattern using a bulk ABM",
	tiles = {"testnodes_bulk_abm.png"},
	groups = { dig_immediate = 3 },
})

minetest.register_abm({
	label = "Bulk test node movement",
	nodenames = "testnodes:bulk_abm",
	interval = 1,
	chance = 1,
	catch_up = false,
	bulk_action = function(pos_list)
		for _, pos in ipairs(pos_list) do
			local node = minetest.get_node(pos)
			if node.name == "testnodes:bulk_abm" then
				local dir = minetest.facedir_to_dir(node.param2)
				node.param2 = (node.param2 + 1) % 24
				if dir and minetest.get_node(pos + dir).name == "air" then
					minetest.remove_node(pos)
					minetest.set_node(pos + dir, node)
				else
					minetest.swap_node(pos, node)
				end
			end
		end
	end,
})
