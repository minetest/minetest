local player_font_huds = {}

local font_states = {
	{0, "Normal font"},
	{1, "Bold font"},
	{2, "Italic font"},
	{3, "Bold and italic font"},
	{4, "Monospace font"},
	{5, "Bold and monospace font"},
	{7, "ZOMG all the font styles"},
}


local font_default_def = {
	hud_elem_type = "text",
	position = {x = 0.5, y = 0.5},
	scale = {x = 2, y = 2},
	alignment = { x = 0, y = 0 },
	number = 0xFFFFFF,
}

local function add_font_hud(player, state)
	local def = table.copy(font_default_def)
	local statetbl = font_states[state]
	def.offset = {x = 0, y = 32 * state}
	def.style = statetbl[1]
	def.text = statetbl[2]
	return player:hud_add(def)
end

local font_etime = 0
local font_state = 0

minetest.register_globalstep(function(dtime)
	font_etime = font_etime + dtime
	if font_etime < 1 then
		return
	end
	font_etime = 0
	for _, player in ipairs(minetest.get_connected_players()) do
		local huds = player_font_huds[player:get_player_name()]
		if huds then
			for i, hud_id in ipairs(huds) do
				local statetbl = font_states[(font_state + i) % #font_states + 1]
				player:hud_change(hud_id, "style", statetbl[1])
				player:hud_change(hud_id, "text", statetbl[2])
			end
		end
	end
	font_state = font_state + 1
end)

minetest.register_chatcommand("hudfonts", {
	params = "[<HUD elements>]",
	description = "Show/Hide some text on the HUD with various font options",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		local param = tonumber(param) or 0
		param = math.min(math.max(param, 1), #font_states)
		if player_font_huds[name] == nil then
			player_font_huds[name] = {}
			for i = 1, param do
				table.insert(player_font_huds[name], add_font_hud(player, i))
			end
			minetest.chat_send_player(name, ("%d text HUD element(s) added."):format(param))
		else
			local huds = player_font_huds[name]
			if huds then
				for _, hud_id in ipairs(huds) do
					player:hud_remove(hud_id)
				end
				minetest.chat_send_player(name, "All text HUD elements removed.")
			end
			player_font_huds[name] = nil
		end
		return true
	end,
})

-- Testing waypoint capabilities

local player_waypoints = {}
minetest.register_chatcommand("hudwaypoints", {
	params = "[ add | add_change | remove ]",
	description = "Create HUD waypoints at your position for testing (add: Add waypoints and change them after 0.5s (default). add_change: Add waypoints and change immediately. remove: Remove all waypoints)",
	func = function(name, params)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		if params == "remove" then
			if player_waypoints[name] then
				for i=1, #player_waypoints[name] do
					player:hud_remove(player_waypoints[name][i])
				end
				player_waypoints[name] = {}
			end
			return true, "All waypoint HUD elements removed."
		end
		if not (params == "add_change" or params == "add" or params == "") then
			-- Incorrect syntax
			return false
		end
		local regular = player:hud_add {
			hud_elem_type = "waypoint",
			name = "regular waypoint",
			text = "m",
			number = 0xFFFFFF,
			world_pos = vector.add(player:get_pos(), {x = 0, y = 1.5, z = 0})
		}
		local reduced_precision = player:hud_add {
			hud_elem_type = "waypoint",
			name = "imprecise waypoint",
			text = "m (0.1 steps, precision = 10)",
			precision = 10,
			number = 0xFFFFFF,
			world_pos = vector.add(player:get_pos(), {x = 0, y = 1, z = 0})
		}
		local hidden_distance = player:hud_add {
			hud_elem_type = "waypoint",
			name = "waypoint with hidden distance",
			text = "this text is hidden as well (precision = 0)",
			precision = 0,
			number = 0xFFFFFF,
			world_pos = vector.add(player:get_pos(), {x = 0, y = 0.5, z = 0})
		}
		local function change(chplayer)
			if not (chplayer and chplayer:is_player()) then
				return
			end
			if regular then
				chplayer:hud_change(regular, "world_pos", vector.add(player:get_pos(), {x = 0, y = 3, z = 0}))
				chplayer:hud_change(regular, "number", 0xFF0000)
			end
			if reduced_precision then
				chplayer:hud_change(reduced_precision, "precision", 2)
				chplayer:hud_change(reduced_precision, "text", "m (0.5 steps, precision = 2)")
				chplayer:hud_change(reduced_precision, "number", 0xFFFF00)
			end
			if hidden_distance then
				chplayer:hud_change(hidden_distance, "number", 0x0000FF)
			end
			minetest.chat_send_player(chplayer:get_player_name(), "Waypoints changed.")
		end
		if params == "add_change" then
			-- change immediate
			change(player)
		else
			minetest.after(0.5, change, player)
		end
		local image_waypoint = player:hud_add {
			hud_elem_type = "image_waypoint",
			text = "testhud_waypoint.png",
			world_pos = player:get_pos(),
			scale = {x = 3, y = 3},
			offset = {x = 0, y = -32}
		}
		if not player_waypoints[name] then
			player_waypoints[name] = {}
		end
		if regular then
			table.insert(player_waypoints[name], regular)
		end
		if reduced_precision then
			table.insert(player_waypoints[name], reduced_precision)
		end
		if hidden_distance then
			table.insert(player_waypoints[name], hidden_distance)
		end
		if image_waypoint then
			table.insert(player_waypoints[name], image_waypoint)
		end
		regular = regular or "error"
		reduced_precision = reduced_precision or "error"
		hidden_distance = hidden_distance or "error"
		image_waypoint = image_waypoint or "error"
		return true, "Waypoints added. IDs: regular: " .. regular .. ", reduced precision: " .. reduced_precision ..
			", hidden distance: " .. hidden_distance .. ", image waypoint: " .. image_waypoint
	end
})

minetest.register_on_joinplayer(function(player)
	player:set_properties({zoom_fov = 15})
end)

minetest.register_chatcommand("zoomfov", {
	params = "[<FOV>]",
	description = "Set or display your zoom_fov",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		if param == "" then
			local fov = player:get_properties().zoom_fov
			return true, "zoom_fov = "..tostring(fov)
		end
		local fov = tonumber(param)
		if not fov then
			return false, "Missing or incorrect zoom_fov parameter!"
		end
		player:set_properties({zoom_fov = fov})
		fov = player:get_properties().zoom_fov
		return true, "zoom_fov = "..tostring(fov)
	end,
})

minetest.register_on_leaveplayer(function(player)
	player_font_huds[player:get_player_name()] = nil
	player_waypoints[player:get_player_name()] = nil
end)
