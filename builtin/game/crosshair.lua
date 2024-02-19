local crosshair_def = {
	type = "crosshair",
	position = {x = 0.5, y = 0.5},
	scale = {x = 1, y = 1},
}

local crosshair_hud_id = {}
local function update_builtin_crosshair(player)
	local name = player:get_player_name()
	local id = crosshair_hud_id[name]
	if player:hud_get_flags().crosshair then
		if not id then
			crosshair_hud_id[name] = player:hud_add(crosshair_def)
		end
	else
		if id then
			player:hud_remove(id)
		end
		crosshair_hud_id[name] = nil
	end
end

core.register_on_mods_loaded(function()
	core.register_on_joinplayer(function(player)
		update_builtin_crosshair(player)
	end)
end)

local function player_event_handler(player, eventname)
	assert(player:is_player())
	local name = player:get_player_name()
	if name == "" then
		return
	end

	if eventname == "hud_changed" then
		update_builtin_crosshair(player)
		return true
	end
	return false
end

core.register_playerevent(player_event_handler)

core.register_on_leaveplayer(function(player)
	crosshair_hud_id[player:get_player_name()] = nil
end)

-- Eventually builtin Lua HUD elements should be generalized
-- this is temporary to make hud_replace_builtin work with the builtin crosshair
local old_hud_replace_builtin = core.hud_replace_builtin
function core.hud_replace_builtin(hud_name, definition)
	if hud_name == "crosshair" then
		crosshair_def = table.copy(definition)
		for name, id in pairs(crosshair_hud_id) do
			local player = core.get_player_by_name(name)
			if player then
				player:hud_remove(id)
				crosshair_hud_id[name] = nil
				update_builtin_crosshair(player)
			end
		end
		return true
	end
	return old_hud_replace_builtin(hud_name, definition)
end
