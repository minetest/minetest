-- Create a detached inventory
local inv = minetest.create_detached_inventory("test_inventory", {
	allow_move = function(inv, from_list, from_index, to_list, to_index, count, player)
		experimental.print_to_everything("allow move asked")
		return count -- Allow all
	end,
	allow_put = function(inv, listname, index, stack, player)
		experimental.print_to_everything("allow put asked")
		return 1 -- Allow only 1
	end,
	allow_take = function(inv, listname, index, stack, player)
		experimental.print_to_everything("allow take asked")
		return 4 -- Allow 4 at max
	end,
	on_move = function(inv, from_list, from_index, to_list, to_index, count, player)
		experimental.print_to_everything(player:get_player_name().." moved items")
	end,
	on_put = function(inv, listname, index, stack, player)
		experimental.print_to_everything(player:get_player_name().." put items")
	end,
	on_take = function(inv, listname, index, stack, player)
		experimental.print_to_everything(player:get_player_name().." took items")
	end,
})
inv:set_size("main", 4*6)
inv:add_item("main", "experimental:callback_node")
inv:add_item("main", "experimental:particle_spawner")


