local SIZE_TAG = "size[11,5.5,true]"

local settingspath = core.get_builtin_path().."settings"

function core.get_mainmenu_path()
	return settingspath
end

defaulttexturedir = ""
dofile(settingspath .. DIR_DELIM .. "settingtypes.lua")
dofile(settingspath .. DIR_DELIM .. "gui_change_mapgen_flags.lua")
dofile(settingspath .. DIR_DELIM .. "gui_settings.lua")


local function avoid_noid()
	return "label[1,1;Avoid the Noid!]"
end

local function menu_formspec(simple_singleplayer_mode, is_touchscreen, address)
	local ypos = simple_singleplayer_mode and 0.7 or 0.1
	local control_text = ""
	
	if is_touchscreen then
		control_text = fgettext([[Controls:
No menu open:
- slide finger: look around
- tap: place/punch/use (default)
- long tap: dig/use (default)
Menu/inventory open:
- double tap (outside):
 --> close
- touch stack, touch slot:
 --> move stack
- touch&drag, tap 2nd finger
 --> place single item to slot
]])
	end
	
	local fs = {
		"formspec_version[1]",
		SIZE_TAG,
		("button_exit[4,%f;3,0.5;btn_continue;%s]"):format(ypos, fgettext("Continue"))
	}
	ypos = ypos + 1
	
	if not simple_singleplayer_mode then
		fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_change_password;%s]"):format(
			ypos, fgettext("Change Password"))
	else
		fs[#fs + 1] = "field[4.95,0;5,1.5;;" .. fgettext("Game paused") .. ";]"
	end
	
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_key_config;%s]"):format(
		ypos, fgettext("Controls"))
	ypos = ypos + 1
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_settings;%s]"):format(
		ypos, fgettext("Settings"))
	ypos = ypos + 1
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_exit_menu;%s]"):format(
		ypos, fgettext("Exit to Menu"))
	ypos = ypos + 1
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_exit_os;%s]"):format(
		ypos, fgettext("Exit to OS"))
	ypos = ypos + 1
	
	-- Controls
	if control_text ~= "" then
		fs[#fs + 1] = ("textarea[7.5,0.25;3.9,6.25;;%s;]"):format(control_text)
	end
	
	-- Server info
	local info = minetest.get_version()
	fs[#fs + 1] = ("textarea[0.4,0.25;3.9,6.25;;%s %s\n\n%s\n"):format(
		info.project, info.hash or info.string, fgettext("Game info:"))
	
	fs[#fs + 1] = "- Mode: " .. (simple_singleplayer_mode and "Singleplayer" or
		((not address) and "Hosting server" or "Remote server"))
	
	if not address then
		local enable_damage = minetest.settings:get_bool("enable_damage")
		local enable_pvp = minetest.settings:get_bool("enable_pvp")
		local server_announce = minetest.settings:get_bool("server_announce")
		local server_name = minetest.settings:get("server_name")
		table.insert_all(fs, {
			"\n",
			enable_damage and
				("- PvP: " .. (enable_pvp and "On" or "Off")) or "",
			"\n",
			"- Public: " .. (server_announce and "On" or "Off"),
			"\n",
			(server_announce and server_name) and  
				("- Server Name: " .. minetest.formspec_escape(server_name)) or ""
		})
	end
		
	fs[#fs + 1] = ";]"
	
	
	return table.concat(fs, "")
end

function core.show_pause_menu(is_singleplayer, is_touchscreen, address)
	minetest.show_formspec("builtin:MT_PAUSE_MENU", menu_formspec(is_singleplayer, is_touchscreen, address))
end

core.register_on_formspec_input(function(formname, fields)
	if formname ~= "builtin:MT_PAUSE_MENU" then return end
	
	if fields.btn_key_config then
		core.key_config() -- Don't want this
	elseif fields.btn_change_password then
		core.change_password()
	elseif fields.btn_settings then
		core.show_settings()
	elseif fields.btn_exit_menu then
		core.disconnect()
	elseif fields.btn_exit_os then
		core.exit_to_os()
	elseif fields.quit or fields.btn_continue then
		core.unpause()
	end
	
	return
end)

-- Override a dlg function to just exit
function settings.delete()
	core.unpause()
end

core.register_on_formspec_input(function(formname, fields)
	if formname ~= "builtin:MT_PAUSE_MENU_SETTINGS" then return true end
	
	settings:buttonhandler(fields)
	if settings.refresh_page then
		core.show_settings()
		core.reload_graphics()
		settings.refresh_page = false
	end
	return true
end)

settings.load(true, false)

function core.show_settings()
	settings.show_client_formspec("builtin:MT_PAUSE_MENU_SETTINGS", settings)
	core.unpause()
end
