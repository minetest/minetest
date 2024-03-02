
local interval = 10

core.register_playerstep(function(player, dtime)
		interval = interval - dtime
		if interval < 0 then
			core.chat_send_player(player:get_player_name(), "Message from playerstep call.")
			interval = 600
		end
	end)
