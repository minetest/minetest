minetest.register_on_joinplayer(function(player)
	local cb = function(player)
		if not player or not player:is_player() then
			return
		end
		minetest.chat_send_player(player:get_player_name(), "This is the \"Development Test\" [devtest], meant only for testing and development. Use Minetest Game for the real thing.")
	end
	minetest.after(2.0, cb, player)
end)
