local hotbar_def = {
	type = "hotbar",
	position = {x = 0.5, y = 1},
	direction = 0,
	alignment = {x = -0.5, y = -1},
	offset = {x = 0, y = - 4}, -- Extra padding below.
}

local hotbar_hud_id = {}
local function update_builtin_hotbar(player)
	local name = player:get_player_name()
	local id = hotbar_hud_id[name]
	if player:hud_get_flags().hotbar then
		if not id then
			hotbar_hud_id[name] = player:hud_add(hotbar_def)
		end
	else
		if id then
			player:hud_remove(id)
		end
		hotbar_hud_id[name] = nil
	end
end

core.register_on_mods_loaded(function()
	core.register_on_joinplayer(function(player)
		update_builtin_hotbar(player)
	end)
end)

local function player_event_handler(player, eventname)
	assert(player:is_player())
	local name = player:get_player_name()
	if name == "" then
		return
	end

	if eventname == "hud_changed" then
		update_builtin_hotbar(player)
		return true
	end
	return false
end

core.register_playerevent(player_event_handler)

core.register_on_leaveplayer(function(player)
	hotbar_hud_id[player:get_player_name()] = nil
end)

-- Eventually builtin Lua HUD elements should be generalized
-- this is temporary to make hud_replace_builtin work with the builtin hotbar
local old_hud_replace_builtin = core.hud_replace_builtin
function core.hud_replace_builtin(hud_name, definition)
	if hud_name == "hotbar" then
		hotbar_def = table.copy(definition)
		for name, id in pairs(hotbar_hud_id) do
			local player = core.get_player_by_name(name)
			if player then
				player:hud_remove(id)
				hotbar_hud_id[name] = nil
				update_builtin_hotbar(player)
			end
		end
		return true
	end
	return old_hud_replace_builtin(hud_name, definition)
end
