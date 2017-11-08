local players = {}

core.registered_on_key_press = {}
function core.register_on_key_press(func)
	core.registered_on_key_press[#core.registered_on_key_press+1] = func
end

core.registered_on_key_release = {}
function core.register_on_key_release(func)
	core.registered_on_key_release[#core.registered_on_key_release+1] = func
end

core.registered_on_key_hold = {}
function core.register_on_key_hold(func)
	core.registered_on_key_hold[#core.registered_on_key_hold+1]=func
end

core.register_on_joinplayer(function(player)
	local name = player:get_player_name()
	players[name] = {
		jump={false},
		right={false},
		left={false},
		LMB={false},
		RMB={false},
		sneak={false},
		aux1={false},
		down={false},
		up={false}
	}
end)

core.register_on_leaveplayer(function(player)
	local name = player:get_player_name()
	players[name] = nil
end)

core.register_globalstep(function(dtime)
	for _, player in pairs(core.get_connected_players()) do
		local player_name = player:get_player_name()
		local player_controls = player:get_player_control()
		for cname, cbool in pairs(player_controls) do
			--Press a key
			if cbool==true and players[player_name][cname][1]==false then
				for _, func in pairs(core.registered_on_key_press) do
					func(player, cname)
				end
				players[player_name][cname] = {true, os.clock()}
			--Hold a key
			elseif cbool==true and players[player_name][cname][1]==true then
				for _, func in pairs(core.registered_on_key_hold) do
					func(player, cname, os.clock()-players[player_name][cname][2])
				end
			--Release a key
			elseif cbool==false and players[player_name][cname][1]==true then
				for _, func in pairs(core.registered_on_key_release) do
					func(player, cname, os.clock()-players[player_name][cname][2])
				end
				players[player_name][cname] = {false}
			end
		end
	end
end)
