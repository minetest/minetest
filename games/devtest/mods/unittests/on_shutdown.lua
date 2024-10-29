-- Test whether players still exist on shutdown
local players = {}

core.register_on_joinplayer(function(player)
	players[player:get_player_name()] = true
end)

core.register_on_leaveplayer(function(player)
	local name = player:get_player_name();
	assert(players[name], "Unrecorded player join.")
	players[name] = nil
end)

core.register_on_shutdown(function()
	for _, player in pairs(core.get_connected_players()) do
		local name = player:get_player_name()
		assert(players[name], "Unrecorded player join or left too early.")
		players[name] = nil
	end

	assert(not next(players), "Invalid connected players on shutdown.")
end)
