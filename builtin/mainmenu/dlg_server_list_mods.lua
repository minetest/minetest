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
		"hypertext[0,0;6,1.5;;<global margin=5 halign=center valign=middle>",
			fgettext("The $1 server uses " .. (server.gameid and "a game called $2 and " or "") ..
					"the following mods:",
				"<b>" .. core.hypertext_escape(server.name) .. "</b>",
				"<style font=mono>" .. (server.gameid or "") .. "</style>") .. "]",
		"textlist[0.5,1.5;5,6.8;;" .. mods .. "]",
		"button[1.5,8.5;3,0.8;quit;OK]"
	}
	return table.concat(formspec, "")
end

local function buttonhandler(this, fields)
	if fields.quit then
        this:delete()
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
