-- Minetest: builtin/static_spawn.lua

local function warn_invalid_static_spawnpoint()
	if core.setting_get("static_spawnpoint") and
			not core.setting_get_pos("static_spawnpoint") then
		core.log('error', "The static_spawnpoint setting is invalid: \""..
				core.setting_get("static_spawnpoint").."\"")
	end
end

warn_invalid_static_spawnpoint()

local function put_player_in_spawn(obj)
	warn_invalid_static_spawnpoint()
	local static_spawnpoint = core.setting_get_pos("static_spawnpoint")
	if not static_spawnpoint then
		return false
	end
	core.log('action', "Moving "..obj:get_player_name()..
			" to static spawnpoint at "..
			core.pos_to_string(static_spawnpoint))
	obj:setpos(static_spawnpoint)
	return true
end

core.register_on_newplayer(function(obj)
	put_player_in_spawn(obj)
end)

core.register_on_respawnplayer(function(obj)
	return put_player_in_spawn(obj)
end)

