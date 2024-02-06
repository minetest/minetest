-- cache setting
local enable_damage = core.settings:get_bool("enable_damage")

local bar_definitions = {
	hp = {
		type = "statbar",
		position = {x = 0.5, y = 1},
		text = "heart.png",
		text2 = "heart_gone.png",
		number = core.PLAYER_MAX_HP_DEFAULT,
		item = core.PLAYER_MAX_HP_DEFAULT,
		direction = 0,
		size = {x = 24, y = 24},
		offset = {x = (-10 * 24) - 25, y = -(48 + 24 + 16)},
	},
	breath = {
		type = "statbar",
		position = {x = 0.5, y = 1},
		text = "bubble.png",
		text2 = "bubble_gone.png",
		number = core.PLAYER_MAX_BREATH_DEFAULT * 2,
		item = core.PLAYER_MAX_BREATH_DEFAULT * 2,
		direction = 0,
		size = {x = 24, y = 24},
		offset = {x = 25, y= -(48 + 24 + 16)},
	},
	minimap = {
		hud_elem_type = "minimap",
		position = {x = 1, y = 0},
		alignment = {x = -1, y = 1},
		offset = {x = -10, y = 10},
		size = {x = 256 , y = 256},
	},
}

local hud_ids = {}

local function scaleToHudMax(player, field)
	-- Scale "hp" or "breath" to the hud maximum dimensions
	local current = player["get_" .. field](player)
	local nominal = bar_definitions[field].item
	local max_display = math.max(player:get_properties()[field .. "_max"], current)
	return math.ceil(current / max_display * nominal)
end

local function update_builtin_statbars(player)
	local name = player:get_player_name()

	if name == "" then
		return
	end

	local flags = player:hud_get_flags()
	if not hud_ids[name] then
		hud_ids[name] = {}
		-- flags are not transmitted to client on connect, we need to make sure
		-- our current flags are transmitted by sending them actively
		player:hud_set_flags(flags)
	end
	local hud = hud_ids[name]

	local immortal = player:get_armor_groups().immortal == 1

	if flags.healthbar and enable_damage and not immortal then
		local number = scaleToHudMax(player, "hp")
		if hud.id_healthbar == nil then
			local hud_def = table.copy(bar_definitions.hp)
			hud_def.number = number
			hud.id_healthbar = player:hud_add(hud_def)
		else
			player:hud_change(hud.id_healthbar, "number", number)
		end
	elseif hud.id_healthbar then
		player:hud_remove(hud.id_healthbar)
		hud.id_healthbar = nil
	end

	local show_breathbar = flags.breathbar and enable_damage and not immortal

	local breath     = player:get_breath()
	local breath_max = player:get_properties().breath_max
	if show_breathbar and breath <= breath_max then
		local number = scaleToHudMax(player, "breath")
		if not hud.id_breathbar and breath < breath_max then
			local hud_def = table.copy(bar_definitions.breath)
			hud_def.number = number
			hud.id_breathbar = player:hud_add(hud_def)
		elseif hud.id_breathbar then
			player:hud_change(hud.id_breathbar, "number", number)
		end
	end

	if hud.id_breathbar and (not show_breathbar or breath == breath_max) then
		core.after(1, function(player_name, breath_bar)
			local player = core.get_player_by_name(player_name)
			if player then
				player:hud_remove(breath_bar)
			end
		end, name, hud.id_breathbar)
		hud.id_breathbar = nil
	end

	-- Don't add a minimap for clients which already have it hardcoded in C++.
	local show_minimap = flags.minimap and
			minetest.get_player_information(name).protocol_version >= 44
	if show_minimap and not hud.id_minimap then
		hud.id_minimap = player:hud_add(bar_definitions.minimap)
	elseif not show_minimap and hud.id_minimap then
		player:hud_remove(hud.id_minimap)
		hud.id_minimap = nil
	end
end

local function cleanup_builtin_statbars(player)
	local name = player:get_player_name()

	if name == "" then
		return
	end

	hud_ids[name] = nil
end

local function player_event_handler(player,eventname)
	assert(player:is_player())

	local name = player:get_player_name()

	if name == "" or not hud_ids[name] then
		return
	end

	if eventname == "health_changed" then
		update_builtin_statbars(player)

		if hud_ids[name].id_healthbar then
			return true
		end
	end

	if eventname == "breath_changed" then
		update_builtin_statbars(player)

		if hud_ids[name].id_breathbar then
			return true
		end
	end

	if eventname == "hud_changed" or eventname == "properties_changed" then
		update_builtin_statbars(player)
		return true
	end

	return false
end

function core.hud_replace_builtin(hud_name, definition)
	if type(definition) ~= "table" then
		return false
	end

	definition = table.copy(definition)

	if hud_name == "health" then
		definition.item = definition.item or definition.number or core.PLAYER_MAX_HP_DEFAULT
		bar_definitions.hp = definition

		for name, ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if player and ids.id_healthbar then
				player:hud_remove(ids.id_healthbar)
				ids.id_healthbar = nil
				update_builtin_statbars(player)
			end
		end
		return true
	end

	if hud_name == "breath" then
		definition.item = definition.item or definition.number or core.PLAYER_MAX_BREATH_DEFAULT
		bar_definitions.breath = definition

		for name, ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if player and ids.id_breathbar then
				player:hud_remove(ids.id_breathbar)
				ids.id_breathbar = nil
				update_builtin_statbars(player)
			end
		end
		return true
	end

	if hud_name == "minimap" then
		bar_definitions.minimap = definition

		for name, ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if player and ids.id_minimap then
				player:hud_remove(ids.id_minimap)
				ids.id_minimap = nil
				update_builtin_statbars(player)
			end
		end
		return true
	end

	return false
end

-- Append "update_builtin_statbars" as late as possible
-- This ensures that the HUD is hidden when the flags are updated in this callback
core.register_on_mods_loaded(function()
	core.register_on_joinplayer(update_builtin_statbars)
end)
core.register_on_leaveplayer(cleanup_builtin_statbars)
core.register_playerevent(player_event_handler)
