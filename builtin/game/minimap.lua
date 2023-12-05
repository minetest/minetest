local function get_size(player)
	local window = core.get_player_window_information(player:get_player_name())
	if window then -- will be nil if executed too early
		return window.size.y * 0.25
	else
		return 250 -- most common windows are about 1000 high
	end
end

local hud_ids = {}
local function update_builtin_minimap(player)
	local name = player:get_player_name()
	local id = hud_ids[name]
	-- mimic wired flag behavior of deprecated non HUD element minimap
	if player:hud_get_flags().minimap then
		local size = get_size(player)
		if id then
			player:hud_change(id, "size", {x = size , y = size})
		else
			hud_ids[name] = player:hud_add({
				hud_elem_type = "minimap",
				position = {x=1, y=0},
				alignment = {x=-1, y=1},
				offset = {x=-10, y=10},
				size = {x = size , y = size}
			})
		end
	else
		if id then
			player:hud_remove(id)
		end
		hud_ids[name] = nil
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
	hud_ids[player:get_player_name()] = nil
end)

