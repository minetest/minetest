minetest.register_chatcommand("test_bulk_set_node", {
	params = "",
	description = "Test: Bulk-set 9×9×9 stone nodes",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local pos_list = {}
		local ppos = player:get_pos()
		local i = 1
		for x=2,10 do
			for y=2,10 do
				for z=2,10 do
					pos_list[i] = {x=ppos.x + x,y = ppos.y + y,z = ppos.z + z}
					i = i + 1
				end
			end
		end
		minetest.bulk_set_node(pos_list, {name = "mapgen_stone"})
		return true, "Done."
	end,
})

