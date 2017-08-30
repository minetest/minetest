-- cache setting
local enable_damage = core.settings:get_bool("enable_damage")

local health_bar_definition =
{
	hud_elem_type = "statbar",
	position = { x=0.5, y=1 },
	text = "heart.png",
	number = core.PLAYER_MAX_HP_DEFAULT,
	direction = 0,
	size = { x=24, y=24 },
	offset = { x=(-10*24)-25, y=-(48+24+16)},
}

local breath_bar_definition =
{
	hud_elem_type = "statbar",
	position = { x=0.5, y=1 },
	text = "bubble.png",
	number = 20,
	direction = 0,
	size = { x=24, y=24 },
	offset = {x=25,y=-(48+24+16)},
}

local hud_ids = {}

local function initialize_builtin_statbars(player)

	if not player:is_player() then
		return
	end

	local name = player:get_player_name()

	if name == "" then
		return
	end

	if not hud_ids[name] then
		hud_ids[name] = {}
		-- flags are not transmitted to client on connect, we need to make sure
		-- our current flags are transmitted by sending them actively
		player:hud_set_flags(player:hud_get_flags())
	end
	local hud = hud_ids[name]

	if player:hud_get_flags().healthbar and enable_damage then
 		if hud.id_healthbar == nil then
 			local hp = player:get_hp()
 			local max_display_hp = math.max(core.PLAYER_MAX_HP_DEFAULT,
 				math.max(player:get_properties().hp_max, hp))
 			-- Limit width of health bar: Scale to the default maximal HP
			health_bar_definition.number =
				hp / max_display_hp * core.PLAYER_MAX_HP_DEFAULT
			hud.id_healthbar  = player:hud_add(health_bar_definition)
		end
	else
		if hud.id_healthbar ~= nil then
			player:hud_remove(hud.id_healthbar)
			hud.id_healthbar = nil
		end
	end

	if player:get_breath() < core.PLAYER_MAX_BREATH then
		if player:hud_get_flags().breathbar and enable_damage then
			if hud.id_breathbar == nil then
				hud.id_breathbar = player:hud_add(breath_bar_definition)
			end
		else
			if hud.id_breathbar ~= nil then
				player:hud_remove(hud.id_breathbar)
				hud.id_breathbar = nil
			end
		end
	elseif hud.id_breathbar ~= nil then
		player:hud_remove(hud.id_breathbar)
		hud.id_breathbar = nil
	end
end

local function cleanup_builtin_statbars(player)

	if not player:is_player() then
		return
	end

	local name = player:get_player_name()

	if name == "" then
		return
	end

	hud_ids[name] = nil
end

local function player_event_handler(player,eventname)
	assert(player:is_player())

	local name = player:get_player_name()

	if name == "" then
		return
	end

	if eventname == "health_changed" then
		initialize_builtin_statbars(player)

		if hud_ids[name].id_healthbar ~= nil then
 			local hp = player:get_hp()
 			local max_display_hp = math.max(core.PLAYER_MAX_HP_DEFAULT,
 				math.max(player:get_properties().hp_max, hp))
 			-- Limit width of health bar: Scale to the default maximal HP
			local hp_count = hp / max_display_hp * core.PLAYER_MAX_HP_DEFAULT
			player:hud_change(hud_ids[name].id_healthbar, "number", hp_count)
			return true
		end
	end

	if eventname == "breath_changed" then
		initialize_builtin_statbars(player)

		if hud_ids[name].id_breathbar ~= nil then
			player:hud_change(hud_ids[name].id_breathbar, "number", player:get_breath() * 2)
			return true
		end
	end

	if eventname == "hud_changed" then
		initialize_builtin_statbars(player)
		return true
	end

	return false
end

function core.hud_replace_builtin(name, definition)

	if definition == nil or
		type(definition) ~= "table" or
		definition.hud_elem_type ~= "statbar" then
		return false
	end

	if name == "health" then
		health_bar_definition = definition

		for name,ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if  player and hud_ids[name].id_healthbar then
				player:hud_remove(hud_ids[name].id_healthbar)
				initialize_builtin_statbars(player)
			end
		end
		return true
	end

	if name == "breath" then
		breath_bar_definition = definition

		for name,ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if  player and hud_ids[name].id_breathbar then
				player:hud_remove(hud_ids[name].id_breathbar)
				initialize_builtin_statbars(player)
			end
		end
		return true
	end

	return false
end

core.register_on_joinplayer(initialize_builtin_statbars)
core.register_on_leaveplayer(cleanup_builtin_statbars)
core.register_playerevent(player_event_handler)
