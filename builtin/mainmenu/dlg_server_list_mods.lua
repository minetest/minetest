-- Luanti
-- Copyright (C) 2024 cx384
-- SPDX-License-Identifier: LGPL-2.1-or-later

local function get_formspec(dialogdata)
	local TOUCH_GUI = core.settings:get_bool("touch_gui")
	local server = dialogdata.server
	local filter_prefix = dialogdata.filter_prefix

	-- A wrongly behaving server may send ill formed mod names
	local mods = {}
	table.sort(server.mods)

	if filter_prefix == true then -- All prefixes
		mods = server.mods
	elseif filter_prefix then
		for _, mod in ipairs(server.mods) do
			if mod == filter_prefix or mod:sub(0, #filter_prefix + 1) == filter_prefix .."_" then
				table.insert(mods, core.formspec_escape(mod))
			end
		end
	else
		local last_prefix
		for i, mod in ipairs(server.mods) do
			local prefix = mod:match("([^_]*)_") or mod
			if prefix and last_prefix == prefix then
				mods[#mods] = "#BBBBBB▶ ".. core.formspec_escape(prefix)
			else
				table.insert(mods, core.formspec_escape(mod))
				last_prefix = prefix
			end
		end
	end
	dialogdata.mods = mods
	mods = table.concat(mods, ",")

	local prefix_button_label
	if not filter_prefix then
		prefix_button_label = fgettext("Expand all prefixes")
	elseif filter_prefix == true then
		prefix_button_label = fgettext("Group by prefix")
	else
		prefix_button_label = fgettext("Show all mods")
	end

	local heading
	if server.gameid then
		heading = fgettext("The $1 server uses a game called $2 and the following mods:",
			"<b>" .. core.hypertext_escape(server.name) .. "</b>",
			"<style font=mono>" .. core.hypertext_escape(server.gameid) .. "</style>")
	else
		heading = fgettext("The $1 server uses the following mods:",
				"<b>" .. core.hypertext_escape(server.name) .. "</b>")
	end

	local formspec = {
		"formspec_version[8]",
		"size[6,9.5]",
		TOUCH_GUI and "padding[0.01,0.01]" or "",
		"hypertext[0,0;6,1.5;;<global margin=5 halign=center valign=middle>".. heading .. "]",
		"textlist[0.5,1.5;5,6.8;mods;" .. mods .. "]",
		"button[0.5,8.5;3,0.8;prefix;" .. prefix_button_label .. "]",
		"button[3.5,8.5;2,0.8;quit;OK]"
	}
	return table.concat(formspec, "")
end

local function buttonhandler(this, fields)
	if fields.quit then
        this:delete()
		return true
	end

	if fields.mods then
		local exploded = core.explode_textlist_event(fields.mods)
		if exploded.type == "DCL" then
			local match = this.data.mods[exploded.index]:match("#BBBBBB▶ ([^_]*)")
			if match then
				this.data.filter_prefix = match
				return true
			end
		end
	end

	if fields.prefix then
		if this.data.filter_prefix then
			this.data.filter_prefix = nil
		else
			this.data.filter_prefix = true
		end
		return true
	end

	return false
end

function create_server_list_mods_dialog(server)
	local retval = dialog_create("dlg_server_list_mods",
		get_formspec,
		buttonhandler,
		nil)
	retval.data.server = server
	return retval
end
