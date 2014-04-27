-- Minetest: builtin/static_spawn.lua

local function warn_invalid_static_spawnpoint()
	if minetest.setting_get("static_spawnpoint") and
			not minetest.setting_get_pos("static_spawnpoint") then
		minetest.log('error', "The static_spawnpoint setting is invalid: \""..
				minetest.setting_get("static_spawnpoint").."\"")
	end
end

warn_invalid_static_spawnpoint()

local function put_player_in_spawn(obj)
	warn_invalid_static_spawnpoint()
	local static_spawnpoint = minetest.setting_get_pos("static_spawnpoint")
	if not static_spawnpoint then
		return false
	end
	minetest.log('action', "Moving "..obj:get_player_name()..
			" to static spawnpoint at "..
			minetest.pos_to_string(static_spawnpoint))
	obj:setpos(static_spawnpoint)
	return true
end

minetest.register_on_newplayer(function(obj)
	put_player_in_spawn(obj)
end)

minetest.register_on_respawnplayer(function(obj)
	return put_player_in_spawn(obj)
end)

