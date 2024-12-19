-- Luanti
-- Copyright (C) 2024 siliconsniffer
-- SPDX-License-Identifier: LGPL-2.1-or-later


local function clients_list_formspec(dialogdata)
	local TOUCH_GUI = core.settings:get_bool("touch_gui")
	local clients_list = dialogdata.server.clients_list
	local servername   = dialogdata.server.name

	local function fmt_formspec_list(clients_list)
		local escaped = {}
		for i, str in ipairs(clients_list) do
			escaped[i] = core.formspec_escape(str)
		end
		return table.concat(escaped, ",")
	end

	local formspec = {
		"formspec_version[8]",
		"size[6,9.5]",
		TOUCH_GUI and "padding[0.01,0.01]" or "",
		"hypertext[0,0;6,1.5;;<global margin=5 halign=center valign=middle>",
			fgettext("This is the list of clients connected to\n$1",
				"<b>" .. core.hypertext_escape(servername) .. "</b>") .. "]",
		"textlist[0.5,1.5;5,6.8;clients;" .. fmt_formspec_list(clients_list) .. "]",
		"button[1.5,8.5;3,0.8;quit;OK]"
	}
	return table.concat(formspec, "")
end


local function clients_list_buttonhandler(this, fields)
	if fields.quit then
        this:delete()
		return true
	end

	if fields.clients then
		local exploded = core.explode_textlist_event(fields.clients)
		local server = this.data.server
		local tabdata = this.data.tabdata
		if exploded.type == "DCL" then
			local player_key = "player:" .. server.clients_list[exploded.index]
			if not tabdata.search_for:find(player_key) then
				tabdata.search_for = (#tabdata.search_for == 0 and "" or tabdata.search_for .. " ") ..
					player_key
			end
			this:delete()
			return true
		end
	end

	return false
end


function create_clientslist_dialog(server, tabdata)
	local retval = dialog_create("dlg_clients_list",
		clients_list_formspec,
		clients_list_buttonhandler,
		nil)
	retval.data.server = server
	retval.data.tabdata = tabdata
	return retval
end
