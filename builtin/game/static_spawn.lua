-- Minetest: builtin/static_spawn.lua

local function check_static_spawnpoint()
	local static_spawnpoint = core.setting_get("static_spawnpoint")
	if not static_spawnpoint then
		return
	end
	local pos = static_spawnpoint and core.string_to_pos(static_spawnpoint)
	if not pos then
		core.log('error', "The static_spawnpoint setting is invalid: \""
				..static_spawnpoint.."\"")
	end
	return pos
end

check_static_spawnpoint()

local function put_player_in_spawn(obj)
	local pos = check_static_spawnpoint()
	if not pos then
		return false
	end
	core.log('action', "Moving "..obj:get_player_name()
			.." to static spawnpoint at "..core.pos_to_string(pos))
	obj:setpos(pos)
	return true
end

core.register_on_newplayer(function(obj)
	put_player_in_spawn(obj)
end)

core.register_on_respawnplayer(function(obj)
	return put_player_in_spawn(obj)
end)

