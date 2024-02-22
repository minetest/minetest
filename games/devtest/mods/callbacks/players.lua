
local message = function(msg)
	minetest.log("action", "[callbacks] "..msg)
	minetest.chat_send_all(msg)
end

core.register_on_punchplayer(function(player, hitter, time_from_last_punch, tool_capabilities, dir, damage)
	if not hitter then
		message("Player "..player:get_player_name().." punched without hitter.")
	end
end)
