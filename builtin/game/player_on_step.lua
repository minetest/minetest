
core.register_globalstep(function(...)
	local players = core.get_connected_players()
	for i = 1, #players do
		-- callback mode 5: RUN_CALLBACKS_MODE_OR_SC
		-- if any callback returns true, the player is kicked or similar
		core.run_callbacks(core.registered_player_on_steps, 5, players[i], ...)
	end
end)
