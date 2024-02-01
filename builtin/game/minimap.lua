local minimap_def = {
	hud_elem_type = "minimap",
	position = {x=1, y=0},
	alignment = {x=-1, y=1},
	offset = {x=-10, y=10},
	size = {x = 256 , y = 256}
}

local minimap_hud_id = {}
local function update_builtin_minimap(player)
	local name = player:get_player_name()

	-- Don't add a minimap for clients which already have it hardcoded in C++
	if minetest.get_player_information(name).protocol_version < 44 then
		return
	end

	local id = minimap_hud_id[name]
	if player:hud_get_flags().minimap then
		if not id then
			minimap_hud_id[name] = player:hud_add(minimap_def)
		end
	else
		if id then
			player:hud_remove(id)
		end
		minimap_hud_id[name] = nil
	end
end

core.register_on_mods_loaded(function()
	core.register_on_joinplayer(function(player)
		update_builtin_minimap(player)
	end)
end)

local function player_event_handler(player, eventname)
	assert(player:is_player())
	local name = player:get_player_name()
	if name == "" then
		return
	end

	if eventname == "hud_changed" then
		update_builtin_minimap(player)
		return true
	end
	return false
end

core.register_playerevent(player_event_handler)

core.register_on_leaveplayer(function(player)
	minimap_hud_id[player:get_player_name()] = nil
end)

-- Eventually builtin Lua HUD elements should be generalized
-- this is temporary to make hud_replace_builtin work with the builtin minimap
local old_hud_replace_builtin = core.hud_replace_builtin
function core.hud_replace_builtin(hud_name, definition)
	if hud_name == "minimap" then
		minimap_def = table.copy(definition)
		for name, id in pairs(minimap_hud_id) do
			local player = core.get_player_by_name(name)
			if player then
				player:hud_remove(id)
				minimap_hud_id[name] = nil
				update_builtin_minimap(player)
			end
		end
		return true
	end
	return old_hud_replace_builtin(hud_name, definition)
end
