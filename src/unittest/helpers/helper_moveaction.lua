minetest.register_allow_player_inventory_action(function(player, action, inventory, info)
	if info.stack:get_name() == "default:water" then
		return 0
	end

	if info.stack:get_name() == "default:lava" then
		return 5
	end

	return info.stack:get_count()
end)
