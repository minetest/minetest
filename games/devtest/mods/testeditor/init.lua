local S = minetest.get_translator("testeditor")
local F = minetest.formspec_escape

local function editor_to_string(editor)
	if type(editor) == "string" then
		return "\"" .. editor .. "\""
	elseif type(editor) == "table" then
		return tostring(dump(editor)):gsub("\n", "")
	else
		return tostring(editor)
	end
end

local editor_formspecs = {}

local function get_object_properties_form(read_data, playername)
	if not playername then return "" end
	local formspec = editor_formspecs[playername]
	formspec.index_to_key = {}
	local str = ""
	local datalist = {}
	for k,_ in pairs(read_data) do
		table.insert(datalist, k)
	end
	table.sort(datalist)
	for p=1, #datalist do
		local k = datalist[p]
		local v = read_data[k]
		local newline = ""
		newline = k .. " = "
		newline = newline .. editor_to_string(v)
		str = str .. F(newline)
		if p < #datalist then
			str = str .. ","
		end
		table.insert(formspec.index_to_key, k)
	end
	return str
end

local function editor_formspec(playername)
	local formspec = editor_formspecs[playername]
	local sel = formspec.selindex or ""
	local key = formspec.index_to_key[sel]
	local value = ""
	if formspec.data[key] then
		value = editor_to_string(formspec.data[key])
	end
	local title = formspec.title
	if not formspec.actual then
		title = S("@1 - NOT APPLIED CHANGES", title)
	end
	minetest.show_formspec(playername, "testeditor:editor",
		"size[11,9]"..
		"label[0,0;"..F(title).."]"..
		"textlist[0,0.5;11,6.5;editor_data;"..formspec.list..";"..sel..";false]"..
		"field[0.2,7.75;7,1;key;"..F(S("Key"))..";"..F(formspec.key).."]"..
		"field_close_on_enter[key;false]"..
		"button[8,7.5;3,1;submit_key;"..F(S("Add/Change key")).."]"..
		"field[0.2,8.75;8,1;value;"..F(S("Value"))..";"..F(value).."]"..
		"field_close_on_enter[value;false]"..
		"button[8,8.5;3,1;submit_value;"..F(S("Submit and apply")).."]"
	)
end

local function editor_formspec_create(playername, title, read_cb, write_cb)
	local data = read_cb(playername)
	editor_formspecs[playername] = {
		title = title,
		read_cb = read_cb,
		write_cb = write_cb,
		data = data,
		key = "",
		actual = true,
	}
	editor_formspecs[playername].list = get_object_properties_form(data, playername)
	editor_formspec(playername)
end

-- Use loadstring to parse param as a Lua value
local function use_loadstring(param, player)
	-- For security reasons, require 'server' priv, just in case
	-- someone is actually crazy enough to run this on a public server.
	local privs = minetest.get_player_privs(player:get_player_name())
	if not privs.server then
		return false, "You need 'server' privilege to change object properties!"
	end
	if not param then
		return false, "Failed: parameter is nil"
	end
	--[[ DANGER ZONE ]]
	-- Interpret string as Lua value
	local func, errormsg = loadstring("return (" .. param .. ")")
	if not func then
		return false, "Failed: " .. errormsg
	end

	-- Apply sandbox here using setfenv
	setfenv(func, {})

	-- Run it
	local good, errOrResult = pcall(func)
	if not good then
		-- A Lua error was thrown
		return false, "Failed: " .. errOrResult
	end

	-- errOrResult will be the value
	return true, errOrResult
end

minetest.register_on_player_receive_fields(function(player, formname, fields)
	if not (player and player:is_player()) then
		return
	end
	if formname == "testeditor:editor" then
		local name = player:get_player_name()
		local formspec = editor_formspecs[name]
		if not formspec then
			return
		end
		if fields.editor_data then
			local expl = minetest.explode_textlist_event(fields.editor_data)
			if expl.type == "DCL" or expl.type == "CHG" then
				formspec.selindex = expl.index
				formspec.key = formspec.index_to_key[expl.index]
				editor_formspec(name)
				return
			end
		end
		if fields.key_enter_field == "key" or fields.submit_key then
			local success, str = use_loadstring(fields.value, player)
			if success then
				local key = fields.key
				formspec.data[key] = str
				formspec.list = get_object_properties_form(formspec.data, name)
				formspec.actual = false
			else
				minetest.chat_send_player(name, str)
				return
			end
			editor_formspec(name)
			if fields.submit_value then
				formspec.write_cb(name, formspec.data)
			end
			return
		end
		if fields.key_enter_field == "value" or fields.submit_value then
			local success, str = use_loadstring(fields.value, player)
			if success then
				local key = formspec.index_to_key[formspec.selindex]
				formspec.data[key] = str
				formspec.list = get_object_properties_form(formspec.data, name)
				formspec.actual = false
			else
				minetest.chat_send_player(name, str)
				return
			end
			editor_formspec(name)
			if fields.submit_value then
				formspec.write_cb(name, formspec.data)
				formspec.data = formspec.read_cb(name)
				formspec.list = get_object_properties_form(formspec.data, name)
				formspec.actual = true
			end
			return
		end
	end
end)

-- ARMOR GROUPS
local function read_armor(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_armor_groups()
	end
	return {}
end
local function write_armor(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_armor_groups(data)
	end
end

-- NAMETAG
local function read_nametag(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_nametag_attributes()
	end
	return {}
end
local function write_nametag(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_nametag_attributes(data)
	end
end

-- PSYCHICS OVERRIDE
local function read_physics(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_physics_override()
	end
	return {}
end
local function write_physics(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_physics_override(data)
	end
end

-- HUD FLAGS
local function read_hud_flags(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:hud_get_flags()
	end
	return {}
end
local function write_hud_flags(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:hud_set_flags(data)
	end
end

-- SKY
local function read_sky(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_sky(true)
	end
	return {}
end
local function write_sky(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_sky(data)
	end
end

-- SUN
local function read_sun(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_sun()
	end
	return {}
end
local function write_sun(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_sun(data)
	end
end

-- MOON
local function read_moon(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_moon()
	end
	return {}
end
local function write_moon(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_moon(data)
	end
end

-- STARS
local function read_stars(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_stars()
	end
	return {}
end
local function write_stars(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_stars(data)
	end
end

-- CLOUDS
local function read_clouds(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_clouds()
	end
	return {}
end
local function write_clouds(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_clouds(data)
	end
end

-- LIGHTING
local function read_lighting(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_lighting()
	end
	return {}
end
local function write_lighting(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_lighting(data)
	end
end

-- FLAGS
local function read_flags(name)
	local player = minetest.get_player_by_name(name)
	if player then
		return player:get_flags()
	end
	return {}
end
local function write_flags(name, data)
	local player = minetest.get_player_by_name(name)
	if player then
		player:set_flags(data)
	end
end

local editor_params = "armor|nametag|physics|hud_flags|sky|sun|moon|stars|clouds|lighting|flags"

minetest.register_chatcommand("player_editor", {
	params = "<"..editor_params..">",
	description = "Open editor for some player data",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		if param == "armor" then
			local title = S("Properties editor of armor groups (get_armor_groups/set_armor_groups)")
			editor_formspec_create(name, title, read_armor, write_armor)
		elseif param == "nametag" then
			local title = S("Properties editor of nametag (get_nametag/set_nametag)")
			editor_formspec_create(name, title, read_nametag, write_nametag)
		elseif param == "physics" then
			local title = S("Properties editor of physics_override (get_physics_override/set_physics_override)")
			editor_formspec_create(name, title, read_physics, write_physics)
		elseif param == "hud_flags" then
			local title = S("Properties editor of hud_flags (hud_get_flags/hud_set_flags)")
			editor_formspec_create(name, title, read_hud_flags, write_hud_flags)
		elseif param == "sky" then
			local title = S("Properties editor of sky (get_sky/set_sky)")
			editor_formspec_create(name, title, read_sky, write_sky)
		elseif param == "sun" then
			local title = S("Properties editor of sun (get_sun/set_sun)")
			editor_formspec_create(name, title, read_sun, write_sun)
		elseif param == "moon" then
			local title = S("Properties editor of moon (get_moon/set_moon)")
			editor_formspec_create(name, title, read_moon, write_moon)
		elseif param == "stars" then
			local title = S("Properties editor of stars (get_stars/set_stars)")
			editor_formspec_create(name, title, read_stars, write_stars)
		elseif param == "clouds" then
			local title = S("Properties editor of clouds (get_clouds/set_clouds)")
			editor_formspec_create(name, title, read_clouds, write_clouds)
		elseif param == "lighting" then
			local title = S("Properties editor of lighting (get_lighting/set_lighting)")
			editor_formspec_create(name, title, read_lighting, write_lighting)
		elseif param == "flags" then
			local title = S("Properties editor of flags (get_flags/set_flags)")
			editor_formspec_create(name, title, read_flags, write_flags)
		else
			return false, S("Use with @1.", editor_params)
		end
		return true
	end,
})

