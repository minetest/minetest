local SIZE_TAG = "size[11,5.5,true]"

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
	
	if fields.btn_continue then
		core.unpause()
	elseif fields.btn_key_config then
		core.key_config() -- Don't want this
	elseif fields.btn_change_password then
		core.change_password()
	elseif fields.btn_settings then
		core.show_settings()
	elseif fields.btn_exit_menu then
		core.disconnect()
	elseif fields.btn_exit_os then
		core.exit_to_os()
	end
	
	return
end)

local scriptpath = core.get_builtin_path()
local path = scriptpath.."mainmenu"..DIR_DELIM.."settings"

function core.get_mainmenu_path()
	return scriptpath.."mainmenu"
end

defaulttexturedir = ""
dofile(path .. DIR_DELIM .. "settingtypes.lua")
dofile(path .. DIR_DELIM .. "dlg_change_mapgen_flags.lua")
dofile(path .. DIR_DELIM .. "dlg_settings.lua")

function dialog_create(name, spec, buttonhandler, eventhandler)
	minetest.show_formspec(name, spec({}))
end


local settings_data = {}
settings_data.data = {
	leftscroll = 0,
	query = "",
	rightscroll = 0,
	components = {},
	page_id = "accessibility"
}

core.register_on_formspec_input(function(formname, fields)
	if formname ~= "builtin:MT_PAUSE_MENU_SETTINGS" then return true end
	--local this = data
	--buttonhandler(settings_data, fields)
	local dialogdata = settings_data.data
	dialogdata.leftscroll = core.explode_scrollbar_event(fields.leftscroll).value or dialogdata.leftscroll
	dialogdata.rightscroll = core.explode_scrollbar_event(fields.rightscroll).value or dialogdata.rightscroll
	dialogdata.query = fields.search_query
	local update = false

	if fields.back then
		this:delete()
		return true
	end

	if fields.show_technical_names ~= nil then
		local value = core.is_yes(fields.show_technical_names)
		core.settings:set_bool("show_technical_names", value)
		write_settings_early()
		update = true
		--return true
	end

	if fields.show_advanced ~= nil then
		local value = core.is_yes(fields.show_advanced)
		core.settings:set_bool("show_advanced", value)
		write_settings_early()
		core.show_settings()
		update = true
	end

	-- enable_touch is a checkbox in a setting component. We handle this
	-- setting differently so we can hide/show pages using the next if-statement
	if fields.enable_touch ~= nil then
		local value = core.is_yes(fields.enable_touch)
		core.settings:set_bool("enable_touch", value)
		write_settings_early()
		core.show_settings()
		update = true
	end

	if fields.show_advanced ~= nil or fields.enable_touch ~= nil then
		local suggested_page_id = update_filtered_pages(dialogdata.query)

		dialogdata.components = nil

		if not filtered_page_by_id[dialogdata.page_id] then
			dialogdata.leftscroll = 0
			dialogdata.rightscroll = 0

			dialogdata.page_id = suggested_page_id
		end

		return true
	end

	if fields.search or fields.key_enter_field == "search_query" then
		dialogdata.components = nil
		dialogdata.leftscroll = 0
		dialogdata.rightscroll = 0

		dialogdata.page_id = update_filtered_pages(dialogdata.query)

		return true
	end
	if fields.search_clear then
		dialogdata.query = ""
		dialogdata.components = nil
		dialogdata.leftscroll = 0
		dialogdata.rightscroll = 0

		dialogdata.page_id = update_filtered_pages("")
		return true
	end

	for _, page in ipairs(all_pages) do
		if fields["page_" .. page.id] then
			dialogdata.page_id = page.id
			dialogdata.components = nil
			dialogdata.rightscroll = 0
			core.show_settings()
			return true
		end
	end

	if dialogdata.components then
	for i, comp in ipairs(dialogdata.components) do
		if comp.on_submit and comp:on_submit(fields, this) then
			write_settings_early()
			core.show_settings()

			-- Clear components so they regenerate
			--dialogdata.components = nil
			return true
		end
		if comp.setting and fields["reset_" .. i] then
			core.settings:remove(comp.setting.name)
			write_settings_early()
			core.show_settings()

			-- Clear components so they regenerate
			--dialogdata.components = nil
			return true
		end
	end
	end
	
	if update then
		core.show_settings()
	end
	
	return false
end)

load(true, false)
--settings_data.data.page_id = update_filtered_pages("")

function core.show_settings()
	show_settings_client_formspec("builtin:MT_PAUSE_MENU_SETTINGS", settings_data.data)
	core.unpause()
end
