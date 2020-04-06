minetest.register_on_joinplayer(function(player)
	local cb = function(player)
		minetest.chat_send_player(player:get_player_name(), "This is the \"Minimal development test\" [minimal], meant only for testing and development. Use Minetest Game for the real thing.")
	end
	minetest.after(2.0, cb, player)
end)
