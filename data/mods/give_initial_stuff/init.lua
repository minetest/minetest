minetest.register_on_newplayer(function(player)
	print("on_newplayer")
	if minetest.setting_getbool("give_initial_stuff") then
		print("giving give_initial_stuff to player")
		player:add_to_inventory('tool "SteelPick" 0')
		player:add_to_inventory('node "torch" 99')
		player:add_to_inventory('tool "SteelAxe" 0')
		player:add_to_inventory('tool "SteelShovel" 0')
		player:add_to_inventory('node "cobble" 99')
	end
end)

