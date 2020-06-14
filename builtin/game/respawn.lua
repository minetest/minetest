-- Minetest: builtin/game/respawn.lua

function core.show_death_formspec(player)
	core.show_formspec(player:get_player_name(), "builtin:death",
		"size[11,5.5]"..
		"label[4.85,1.35;You died]"..
		"button_exit[4,3;3,0.5;respawn;Respawn]")
end

function core.respawn_player(player)
	if core.settings:get_bool("enable_damage") then
		local properties = player:get_properties()
		player:set_hp(properties.hp_max, {type="respawn"})
		player:set_breath(properties.breath_max)
	end
	-- Run on_respawnplayer callbacks
	for _, callback in pairs(core.registered_on_respawnplayers) do
		if callback(player) then
			return
		end
	end
	-- Respawn
	local pos = core.find_spawn_pos()
	player:set_pos(pos)
	core.log("action", player:get_player_name() .. " respawns at " ..
		core.pos_to_string(pos))
end

core.register_on_dieplayer(function(player, reason)
	core.chat_send_player(player:get_player_name(), "You died.")
	core.show_death_formspec(player, reason)
end)

core.register_on_joinplayer(function(player)
	if player:get_hp() == 0 then
		core.show_death_formspec(player, {type="set_hp", from="mod"})
	end
end)

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "builtin:death" and fields.quit and player:get_hp() == 0 then
		core.respawn_player(player)
	end
end)
