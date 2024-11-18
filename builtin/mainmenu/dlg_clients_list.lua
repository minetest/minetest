--Luanti
--Copyright (C) 2024 siliconsniffer
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


local function clients_list_formspec(dialogdata)
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
		"formspec_version[8]" ,
		"size[6,9.5]" ,
		"position[0.5,0.5]" ,
		"hypertext[0,0;6,1.5;;<global margin=5 halign=center valign=middle>" ,
		fgettext("This is the list of clients from \n$1", "<b>" ..
		core.hypertext_escape(servername) .. "</b>") .. "]" ,
		"textlist[0.5,1.5;5,6.8;;" .. fmt_formspec_list(clients_list) .. "]" ,
		"button[1.5,8.5;3,0.8;quit;OK]"
	}
	return table.concat(formspec, "")
end


local function clients_list_buttonhandler(this, fields)
	if fields.quit then
        this:delete()
        serverlistmgr.sync()
		return true
	end
	return false
end


function create_clientslist_dialog(server)
	local retval = dialog_create("dlg_clients_list",
		clients_list_formspec,
		clients_list_buttonhandler,
		nil)
	retval.data.server = server
	return retval
end
