minetest.register_on_newplayer(function(player)
	print("on_newplayer")
	if minetest.setting_getbool("give_initial_stuff") then
		print("giving give_initial_stuff to player")
		player:get_inventory():add_item('main', 'default:pick_steel')
		player:get_inventory():add_item('main', 'default:torch 99')
		player:get_inventory():add_item('main', 'default:axe_steel')
		player:get_inventory():add_item('main', 'default:shovel_steel')
		player:get_inventory():add_item('main', 'default:cobble 99')
	end
end)

