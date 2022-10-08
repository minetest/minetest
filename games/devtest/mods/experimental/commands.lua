minetest.register_chatcommand("test_inv", {
	params = "",
	description = "Test: Modify player's inventory formspec",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		player:set_inventory_formspec(
			"size[13,7.5]"..
			"image[6,0.6;1,2;player.png]"..
			"list[current_player;main;5,3.5;8,4;]"..
			"list[current_player;craft;8,0;3,3;]"..
			"list[current_player;craftpreview;12,1;1,1;]"..
			"list[detached:test_inventory;main;0,0;4,6;0]"..
			"button[0.5,7;2,1;button1;Button 1]"..
			"button_exit[2.5,7;2,1;button2;Exit Button]")
		return true, "Done."
	end,
})

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

