minetest.register_on_newplayer(function(player)
	print("on_newplayer")
	if minetest.setting_getbool("give_initial_stuff") then
		print("giving give_initial_stuff to player")
		player:add_to_inventory('ToolItem "SteelPick" 0')
		player:add_to_inventory('NodeItem "torch" 99')
		player:add_to_inventory('ToolItem "SteelAxe" 0')
		player:add_to_inventory('ToolItem "SteelShovel" 0')
		player:add_to_inventory('NodeItem "cobble" 99')
	end
end)

