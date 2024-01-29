--[[
Register function to easily register new builtin hud elements
`def` is a table and contains the following fields:
  elem_def  the HUD element definition which can be changed with hud_replace_builtin
  events    (optional) additional event names on which the element will be updated
            ("hud_changed" will always be used.)
  show_elem(player, flags, id)
            (optional) a function to decide if the element should be shown to a player
            It is called before the element gets updated.
  update_def(player, elem_def)
            (optional) a function to change the elem_def before it will be used.
            (elem_def can be changed, since the table which got set by using
            hud_replace_builtin isn't exposed to the API.)
  update_elem(player, id)
            (optional) a function to change the element after it has been updated
            (Is not called when the element is first set or recreated.)
]]--

local registered_elements = {}
local update_events = {}
local function register_builtin_hud_element(name, def)
	registered_elements[name] = def
	for _, event in ipairs(def.events or {}) do
		update_events[event] = update_events[event] or {}
		table.insert(update_events[event], name)
	end
end

-- Stores HUD ids for all players
local hud_ids = {}

-- Updates one element
-- In case the element is already added, it only calls the update_elem function from
-- registered_elements. (To recreate the element remove it first.)
local function update_element(player, player_hud_ids, elem_name, flags)
	local def = registered_elements[elem_name]
	local id = player_hud_ids[elem_name]

	if def.show_elem and not def.show_elem(player, flags, id) then
		if id then
			player:hud_remove(id)
			player_hud_ids[elem_name] = nil
		end
		return
	end

	if not id then
		if def.update_def then
			def.update_def(player, def.elem_def)
		end

		id = player:hud_add(def.elem_def)
		player_hud_ids[elem_name] = id
		return
	end

	if def.update_elem then
		def.update_elem(player, id)
	end
end

-- Updates all elements
-- If to_update is specified it will only update those elements.
local function update_hud(player, to_update)
	local flags = player:hud_get_flags()
	local playername = player:get_player_name()
	hud_ids[playername] = hud_ids[playername] or {}
	local player_hud_ids = hud_ids[playername]

	if to_update then
		for _, elem_name in ipairs(to_update) do
			update_element(player, player_hud_ids, elem_name, flags)
		end
	else
		for elem_name, _ in pairs(registered_elements) do
			update_element(player, player_hud_ids, elem_name, flags)
		end
	end
end

local function player_event_handler(player, eventname)
	assert(player:is_player())

	if eventname == "hud_changed" then
		update_hud(player)
		return
	end

	-- Custom events
	local to_update = update_events[eventname]
	if to_update then
		update_hud(player, to_update)
	end
end

-- Returns true if successful, otherwise false,
-- but currently the return value is not documented in the Lua API.
function core.hud_replace_builtin(elem_name, elem_def)
	assert(type(elem_def) == "table")

	local registered = registered_elements[elem_name]
	if not registered then
		return false
	end

	registered.elem_def = table.copy(elem_def)

	for playername, player_hud_ids in pairs(hud_ids) do
		local player = core.get_player_by_name(playername)
		local id = player_hud_ids[elem_name]
		if player and id then
			player:hud_remove(id)
			player_hud_ids[elem_name] = nil
			update_element(player, player_hud_ids, elem_name, player:hud_get_flags())
		end
	end
	return true
end

local function cleanup_builtin_hud(player)
	hud_ids[player:get_player_name()] = nil
end


-- Append "update_hud" as late as possible
-- This ensures that the HUD is hidden when the flags are updated in this callback
core.register_on_mods_loaded(function()
	core.register_on_joinplayer(function(player)
		update_hud(player)
	end)
end)
core.register_on_leaveplayer(cleanup_builtin_hud)
core.register_playerevent(player_event_handler)


---- Builtin HUD Elements

--- Healthbar

-- Cache setting
local enable_damage = core.settings:get_bool("enable_damage")

local function scale_to_hud_max(player, field)
	-- Scale "hp" or "breath" to the hud maximum dimensions
	local current = player["get_" .. field](player)
	local nominal
	if field == "hp" then -- HUD is called health but field is hp
		nominal = registered_elements.health.elem_def.item
	else
		nominal = registered_elements[field].elem_def.item
	end
	local max_display = math.max(player:get_properties()[field .. "_max"], current)
	return math.ceil(current / max_display * nominal)
end

register_builtin_hud_element("health", {
	elem_def = {
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
	events = {"properties_changed", "health_changed"},
	show_elem = function(player, flags)
		return flags.healthbar and enable_damage and
				player:get_armor_groups().immortal ~= 1
	end,
	update_def = function(player, elem_def)
		elem_def.item = elem_def.item or elem_def.number or core.PLAYER_MAX_HP_DEFAULT
		elem_def.number = scale_to_hud_max(player, "hp")
	end,
	update_elem = function(player, id)
		player:hud_change(id, "number", scale_to_hud_max(player, "hp"))
	end,
})

--- Breathbar

-- Stores core.after calls for every player
local breathbar_removal_jobs = {}

register_builtin_hud_element("breath", {
	elem_def = {
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
	events = {"properties_changed", "breath_changed"},
	show_elem = function(player, flags, id)
		local show_breathbar = flags.breathbar and enable_damage and
				player:get_armor_groups().immortal ~= 1
		if id then
			-- The element will not prematurely be removed by update_element
			-- (but may still be instantly removed if the flag changed)
			return show_breathbar
		end
		-- Don't add the element if the breath is full
		local breath_relevant = player:get_breath() < player:get_properties().breath_max
		return show_breathbar and breath_relevant
	end,
	update_def = function(player, elem_def)
		elem_def.item = elem_def.item or elem_def.number or core.PLAYER_MAX_BREATH_DEFAULT
		elem_def.number = scale_to_hud_max(player, "breath")
	end,
	update_elem = function(player, id)
		player:hud_change(id, "number", scale_to_hud_max(player, "breath"))

		local player_name = player:get_player_name()
		local breath_relevant = player:get_breath() < player:get_properties().breath_max

		if not breath_relevant then
			if not breathbar_removal_jobs[player_name] then
				-- The breathbar stays for some time and then gets removed.
				breathbar_removal_jobs[player_name] = core.after(1, function()
					local player = core.get_player_by_name(player_name)
					local player_hud_ids = hud_ids[player_name]
					if player and player_hud_ids and player_hud_ids.breath then
						player:hud_remove(player_hud_ids.breath)
						player_hud_ids.breath = nil
					end
					breathbar_removal_jobs[player_name] = nil
				end)
			end
		else
			-- Cancel removal
			local job = breathbar_removal_jobs[player_name]
			if job then
				job:cancel()
				breathbar_removal_jobs[player_name] = nil
			end
		end
	end,
})

--- Minimap

register_builtin_hud_element("minimap", {
	elem_def = {
		type = "minimap",
		position = {x = 1, y = 0},
		alignment = {x = -1, y = 1},
		offset = {x = -10, y = 10},
		size = {x = 256, y = 256},
	},
	show_elem = function(player, flags)
		-- Don't add a minimap for clients which already have it hardcoded in C++.
		return flags.minimap and
				core.get_player_information(player:get_player_name()).protocol_version >= 44
	end,
})

--- Hotbar

register_builtin_hud_element("hotbar", {
	elem_def = {
		type = "hotbar",
		position = {x = 0.5, y = 1},
		direction = 0,
		alignment = {x = 0, y = -1},
		offset = {x = 0, y = -4}, -- Extra padding below.
	},
	show_elem = function(player, flags)
		return flags.hotbar
	end,
})
