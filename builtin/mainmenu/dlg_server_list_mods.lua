-- Luanti
-- Copyright (C) 2024 cx384
-- SPDX-License-Identifier: LGPL-2.1-or-later

local function get_formspec(dialogdata)
	local TOUCH_GUI = core.settings:get_bool("touch_gui")
	local server = dialogdata.server

	-- A wrongly behaving server may send ill formed mod names
	local mods = {}
	table.sort(server.mods)
	for i, m in ipairs(server.mods) do
		mods[i] = core.formspec_escape(m)
	end
	mods = table.concat(mods, ",")

	local formspec = {
		"formspec_version[8]",
		"size[6,9.5]",
		TOUCH_GUI and "padding[0.01,0.01]" or "",
		"hypertext[0,0;6,1.5;gameid;<global margin=5 halign=center valign=middle>",
			fgettext("The $1 server uses " .. (server.gameid and "a game called $2 and " or "") ..
					"the following mods:",
				"<b>" .. core.hypertext_escape(server.name) .. "</b>",
				"<action name=click><style color=white font=mono>" .. (server.gameid or "") .. "</style></action>") .. "]",
		"textlist[0.5,1.5;5,6.8;mods;" .. mods .. "]",
		"button[1.5,8.5;3,0.8;quit;OK]"
	}
	return table.concat(formspec, "")
end

local function buttonhandler(this, fields)
	if fields.quit then
        this:delete()
		return true
	end

	local server = this.data.server
	local tabdata = this.data.tabdata

	if fields.gameid then
		local found_start, found_end = tabdata.search_for:find("game:[^%s\"]*")
		if found_start then -- Replace instead
			tabdata.search_for = tabdata.search_for:sub(0, found_start+4) ..
				server.gameid .. tabdata.search_for:sub(found_end+1)
		else
			tabdata.search_for = (#tabdata.search_for == 0 and "" or tabdata.search_for .. " ") ..
				"game:" .. server.gameid
		end
		this:delete()
		return true
	end

	if fields.mods then
		local exploded = core.explode_textlist_event(fields.mods)
		if exploded.type == "DCL" then
			local mod_key = "mod:" .. server.mods[exploded.index]
			if not tabdata.search_for:find(mod_key) then
				tabdata.search_for = (#tabdata.search_for == 0 and "" or tabdata.search_for .. " ") ..
					mod_key
			end
			this:delete()
			return true
		end
	end

	return false
end

function create_server_list_mods_dialog(server, tabdata)
	local retval = dialog_create("dlg_server_list_mods",
		get_formspec,
		buttonhandler,
		nil)
	retval.data.server = server
	retval.data.tabdata = tabdata
	return retval
end
