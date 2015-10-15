--Minetest
--Copyright (C) 2015 PilzAdam
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

local FILENAME = "settingtypes.txt"

local function parse_setting_line(settings, line)
	-- empty lines
	if line:match("^[%s]*$") then
		-- clear current_comment so only comments directly above a setting are bound to it
		settings.current_comment = ""
		return
	end

	-- category
	local category = line:match("^%[([^%]]+)%]$")
	if category then
		local level = 0
		local index = 1
		while category:sub(index, index) == "*" do
			level = level + 1
			index = index + 1
		end
		category = category:sub(index, -1)
		table.insert(settings, {
			name = category,
			level = level,
			type = "category",
		})
		return
	end

	-- comment
	local comment = line:match("^#[%s]*(.*)$")
	if comment then
		if settings.current_comment == "" then
			settings.current_comment = comment
		else
			settings.current_comment = settings.current_comment .. "\n" .. comment
		end
		return
	end

	-- settings
	local first_part, name, readable_name, setting_type =
			line:match("^(([%w%._-]+)[%s]+%(([^%)]*)%)[%s]+([%w_]+)[%s]*)")

	if first_part then
		if readable_name == "" then
			readable_name = nil
		end
		local remaining_line = line:sub(first_part:len() + 1)

		if setting_type == "int" then
			local default, min, max = remaining_line:match("^([%d]+)[%s]*([%d]*)[%s]*([%d]*)$")
			if default and tonumber(default) then
				if min == "" then
					min = nil
				end
				if max == "" then
					max = nil
				end
				table.insert(settings, {
					name = name,
					readable_name = readable_name,
					type = "int",
					default = default,
					min = min,
					max = max,
					comment = settings.current_comment,
				})
			else
				core.log("error", "Found invalid int in " .. FILENAME .. ": " .. line)
			end

		elseif setting_type == "string" or setting_type == "flags" or setting_type == "noise_params" then
			local default = remaining_line:match("^[%s]*(.*)$")
			if default then
				table.insert(settings, {
					name = name,
					readable_name = readable_name,
					type = setting_type,
					default = default,
					comment = settings.current_comment,
				})
			else
				core.log("error", "Found invalid string in " .. FILENAME .. ": " .. line)
			end

		elseif setting_type == "bool" then
			if remaining_line == "false" or remaining_line == "true" then
				table.insert(settings, {
					name = name,
					readable_name = readable_name,
					type = "bool",
					default = remaining_line,
					comment = settings.current_comment,
				})
			else
				core.log("error", "Found invalid bool in " .. FILENAME .. ": " .. line)
			end

		elseif setting_type == "float" then
			local default, min, max
					= remaining_line:match("^([%d%.]+)[%s]*([%d%.]*)[%s]*([%d%.]*)$")
			if default and tonumber(default) then
				if min == "" then
					min = nil
				end
				if max == "" then
					max = nil
				end
				table.insert(settings, {
					name = name,
					readable_name = readable_name,
					type = "float",
					default = default,
					min = min,
					max = max,
					comment = settings.current_comment,
				})
			else
				core.log("error", "Found invalid float in " .. FILENAME .. ": " .. line)
			end

		elseif setting_type == "enum" then
			local default, values = remaining_line:match("^([^%s]+)[%s]+(.+)$")
			if default and values ~= "" then
				table.insert(settings, {
					name = name,
					readable_name = readable_name,
					type = "enum",
					default = default,
					values = values:split(","),
					comment = settings.current_comment,
				})
			else
				core.log("error", "Found invalid enum in " .. FILENAME .. ": " .. line)
			end

		elseif setting_type == "path" then
			local default = remaining_line:match("^[%s]*(.*)$")
			if default then
				table.insert(settings, {
					name = name,
					readable_name = readable_name,
					type = "path",
					default = default,
					comment = settings.current_comment,
				})
			else
				core.log("error", "Found invalid path in " .. FILENAME .. ": " .. line)
			end

		elseif setting_type == "key" then
			--ignore keys, since we have a special dialog for them

		-- TODO: flags, noise_params (, struct)

		else
			core.log("error", "Found setting with invalid setting type in " .. FILENAME .. ": " .. line)
		end
	else
		core.log("error", "Found invalid line in " .. FILENAME .. ": " .. line)
	end
	-- clear current_comment since we just used it
	-- if we not just used it, then clear it since we only want comments
	--  directly above the setting to be bound to it
	settings.current_comment = ""
end

local function parse_config_file()
	local file = io.open(core.get_builtin_path() .. DIR_DELIM .. FILENAME, "r")
	local settings = {}
	if not file then
		core.log("error", "Can't load " .. FILENAME)
		return settings
	end

	-- store this helper variable in the table so it's easier to pass to parse_setting_line()
	settings.current_comment = ""

	local line = file:read("*line")
	while line do
		parse_setting_line(settings, line)
		line = file:read("*line")
	end

	settings.current_comment = nil

	file:close()
	return settings
end

local settings = parse_config_file()
local selected_setting = 1

local function get_current_value(setting)
	local value = core.setting_get(setting.name)
	if value == nil then
		value = setting.default
	end
	return value
end

local function create_change_setting_formspec(dialogdata)
	local setting = settings[selected_setting]
	local formspec = "size[10,5.2,true]" ..
			"button[5,4.5;2,1;btn_done;" .. fgettext("Save") .. "]" ..
			"button[3,4.5;2,1;btn_cancel;" .. fgettext("Cancel") .. "]" ..
			"tablecolumns[color;text]" ..
			"tableoptions[background=#00000000;highlight=#00000000;border=false]" ..
			"table[0,0;10,3;info;"

	if setting.readable_name then
		formspec = formspec .. "#FFFF00," .. fgettext(setting.readable_name)
				.. " (" .. core.formspec_escape(setting.name) .. "),"
	else
		formspec = formspec .. "#FFFF00," .. core.formspec_escape(setting.name) .. ","
	end

	formspec = formspec .. ",,"

	for _, comment_line in ipairs(fgettext_ne(setting.comment):split("\n")) do
		formspec = formspec .. "," .. core.formspec_escape(comment_line) .. ","
	end

	formspec = formspec .. ";1]"

	if setting.type == "bool" then
		local selected_index
		if core.is_yes(get_current_value(setting)) then
			selected_index = 2
		else
			selected_index = 1
		end
		formspec = formspec .. "dropdown[0.5,3.5;3,1;dd_setting_value;"
				.. fgettext("Disabled") .. "," .. fgettext("Enabled") .. ";" .. selected_index .. "]"

	elseif setting.type == "enum" then
		local selected_index = 0
		formspec = formspec .. "dropdown[0.5,3.5;3,1;dd_setting_value;"
		for index, value in ipairs(setting.values) do
			-- translating value is not possible, since it's the value
			--  that we set the setting to
			formspec = formspec ..  core.formspec_escape(value) .. ","
			if get_current_value(setting) == value then
				selected_index = index
			end
		end
		if #setting.values > 0 then
			formspec = formspec:sub(1, -2) -- remove trailing comma
		end
		formspec = formspec .. ";" .. selected_index .. "]"

	elseif setting.type == "path" then
		local current_value = dialogdata.selected_path
		if not current_value then
			current_value = get_current_value(setting)
		end
		formspec = formspec .. "field[0.5,4;7.5,1;te_setting_value;;"
				.. core.formspec_escape(current_value) .. "]"
				.. "button[8,3.75;2,1;btn_browser_path;" .. fgettext("Browse") .. "]"

	else
		-- TODO: fancy input for float, int, flags, noise_params
		formspec = formspec .. "field[0.5,4;9.5,1;te_setting_value;;"
				.. core.formspec_escape(get_current_value(setting)) .. "]"
	end
	return formspec
end

local function handle_change_setting_buttons(this, fields)
	if fields["btn_done"] or fields["key_enter"] then
		local setting = settings[selected_setting]
		if setting.type == "bool" then
			local new_value = fields["dd_setting_value"]
			-- Note: new_value is the actual (translated) value shown in the dropdown
			core.setting_setbool(setting.name, new_value == fgettext("Enabled"))

		elseif setting.type == "enum" then
			local new_value = fields["dd_setting_value"]
			core.setting_set(setting.name, new_value)

		else
			local new_value = fields["te_setting_value"]
			core.setting_set(setting.name, new_value)
		end
		core.setting_save()
		this:delete()
		return true
	end

	if fields["btn_cancel"] then
		this:delete()
		return true
	end

	if fields["btn_browser_path"] then
		core.show_file_open_dialog("dlg_browse_path", fgettext_ne("Select path"))
	end

	if fields["dlg_browse_path_accepted"] then
		this.data.selected_path = fields["dlg_browse_path_accepted"]
		core.update_formspec(this:get_formspec())
	end

	return false
end

local function create_settings_formspec(tabview, name, tabdata)
	local formspec = "tablecolumns[color;tree;text;text]" ..
					"tableoptions[background=#00000000;border=false]" ..
					"table[0,0;12,4.5;list_settings;"

	local current_level = 0
	for _, entry in ipairs(settings) do
		local name
		if not core.setting_getbool("main_menu_technical_settings") and entry.readable_name then
			name = fgettext_ne(entry.readable_name)
		else
			name = entry.name
		end
		
		if entry.type == "category" then
			current_level = entry.level
			formspec = formspec .. "#FFFF00," .. current_level .. "," .. core.formspec_escape(name) .. ",,"
		elseif entry.type == "bool" then
			local value = get_current_value(entry)
			if core.is_yes(value) then
				value = fgettext("Enabled")
			else
				value = fgettext("Disabled")
			end
			formspec = formspec .. "," .. (current_level + 1) .. "," .. core.formspec_escape(name) .. ","
					.. value .. ","
		else
			formspec = formspec .. "," .. (current_level + 1) .. "," .. core.formspec_escape(name) .. ","
					.. core.formspec_escape(get_current_value(entry)) .. ","
		end
	end

	if #settings > 0 then
		formspec = formspec:sub(1, -2) -- remove trailing comma
	end
	formspec = formspec .. ";" .. selected_setting .. "]" ..
			"button[4,4.5;3,1;btn_change_keys;".. fgettext("Change keys") .. "]" ..
			"button[10,4.5;2,1;btn_edit;" .. fgettext("Edit") .. "]" ..
			"button[7,4.5;3,1;btn_restore;" .. fgettext("Restore Default") .. "]" ..
			"checkbox[0,4.5;cb_tech_settings;" .. fgettext("Show technical names") .. ";"
					.. dump(core.setting_getbool("main_menu_technical_settings")) .. "]"

	return formspec
end

local function handle_settings_buttons(this, fields, tabname, tabdata)
	local list_enter = false
	if fields["list_settings"] then
		selected_setting = core.get_table_index("list_settings")
		if  core.explode_table_event(fields["list_settings"]).type == "DCL" then
			-- Directly toggle booleans
			local setting = settings[selected_setting]
			if setting.type == "bool" then
				local current_value = get_current_value(setting)
				core.setting_setbool(setting.name, not core.is_yes(current_value))
				core.setting_save()
				return true
			else
				list_enter = true
			end
		else
			return true
		end
	end
	
	if fields["btn_edit"] or list_enter then
		local setting = settings[selected_setting]
		if setting.type ~= "category" then
			local edit_dialog = dialog_create("change_setting", create_change_setting_formspec,
					handle_change_setting_buttons)
			edit_dialog:set_parent(this)
			this:hide()
			edit_dialog:show()
		end
		return true
	end

	if fields["btn_restore"] then
		local setting = settings[selected_setting]
		if setting.type ~= "category" then
			core.setting_set(setting.name, setting.default)
			core.setting_save()
			core.update_formspec(this:get_formspec())
		end
		return true
	end

	if fields["btn_change_keys"] then
		core.show_keys_menu()
		return true
	end

	if fields["cb_tech_settings"] then
		core.setting_set("main_menu_technical_settings", fields["cb_tech_settings"])
		core.setting_save()
		core.update_formspec(this:get_formspec())
		return true
	end

	return false
end

local function create_minetest_conf_example()
	local result = "#    This file contains a list of all available settings and their default value for minetest.conf\n" ..
			"\n" ..
			"#    By default, all the settings are commented and not functional.\n" ..
			"#    Uncomment settings by removing the preceding #.\n" ..
			"\n" ..
			"#    minetest.conf is read by default from:\n" ..
			"#    ../minetest.conf\n" ..
			"#    ../../minetest.conf\n" ..
			"#    Any other path can be chosen by passing the path as a parameter\n" ..
			"#    to the program, eg. \"minetest.exe --config ../minetest.conf.example\".\n" ..
			"\n" ..
			"#    Further documentation:\n" ..
			"#    http://wiki.minetest.net/\n" ..
			"\n"

	for _, entry in ipairs(settings) do
		if entry.type == "category" then
			if entry.level == 0 then
				result = result .. "#\n# " .. entry.name .. "\n#\n\n"
			else
				for i = 1, entry.level do
					result = result .. "#"
				end
				result = result .. "# " .. entry.name .. "\n\n"
			end
		else
			if entry.comment_line ~= "" then
				for _, comment_line in ipairs(entry.comment:split("\n")) do
					result = result .."#    " .. comment_line .. "\n"
				end
			end
			result = result .. "#    type: " .. entry.type
			if entry.min then
				result = result .. " min: " .. entry.min
			end
			if entry.max then
				result = result .. " max: " .. entry.max
			end
			if entry.values then
				result = result .. " values: " .. table.concat(entry.values, ", ")
			end
			result = result .. "\n"
			result = result .. "# " .. entry.name .. " = ".. entry.default .. "\n\n"
		end
	end
	return result
end

local function create_translation_file()
	local result = "// This file is automatically generated\n" ..
			"// It conatins a bunch of fake gettext calls, to tell xgettext about the strings in config files\n" ..
			"// To update it, refer to the bottom of builtin/mainmenu/tab_settings.lua\n\n" ..
			"fake_function() {\n"
	for _, entry in ipairs(settings) do
		if entry.type == "category" then
			result = result .. "\tgettext(\"" .. entry.name .. "\");\n"
		else
			if entry.readable_name then
				result = result .. "\tgettext(\"" .. entry.readable_name .. "\");\n"
			end
			if entry.comment ~= "" then
				local comment = entry.comment:gsub("\n", "\\n")
				result = result .. "\tgettext(\"" .. comment .. "\");\n"
			end
		end
	end
	result = result .. "}\n"
	return result
end

if false then
	local file = io.open("minetest.conf.example", "w")
	if file then
		file:write(create_minetest_conf_example())
		file:close()
	end
end

if false then
	local file = io.open("src/settings_translation_file.c", "w")
	if file then
		file:write(create_translation_file())
		file:close()
	end
end

tab_settings = {
	name = "settings",
	caption = fgettext("Settings"),
	cbf_formspec = create_settings_formspec,
	cbf_button_handler = handle_settings_buttons,
}
