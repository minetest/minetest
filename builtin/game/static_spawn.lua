-- Minetest: builtin/static_spawn.lua

local static_spawnpoint_string = minetest.settings:get("static_spawnpoint")
if static_spawnpoint_string and
		static_spawnpoint_string ~= "" and
		not minetest.setting_get_pos("static_spawnpoint") then
	error('The static_spawnpoint setting is invalid: "' ..
			static_spawnpoint_string .. '"')
end

local function put_player_in_spawn(player_obj)
	local static_spawnpoint = minetest.setting_get_pos("static_spawnpoint")
	if not static_spawnpoint then
		return false
	end
	minetest.log("action", "Moving " .. player_obj:get_player_name() ..
		" to static spawnpoint at " .. minetest.pos_to_string(static_spawnpoint))
	player_obj:set_pos(static_spawnpoint)
	return true
end

minetest.register_on_newplayer(put_player_in_spawn)
minetest.register_on_respawnplayer(put_player_in_spawn)
