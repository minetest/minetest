--Minetest
--Copyright (C) 2014 sapier
--Copyright (C) 2020 Yaman Qalieh
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

--------------------------------------------------------------------------------

local function confirm_exit_formspec(dialogdata)
	local retval =
		"size[11.5,4.5,true]" ..
		"label[4.2,2;" .. fgettext("Are you sure you want to exit?") .. "]"..
		"style[dlg_confirm_exit_accept;bgcolor=red]" ..
		"button[3.25,3.5;2.5,0.5;dlg_confirm_exit_accept;" .. fgettext("Yes") .. "]" ..
		"button[5.75,3.5;2.5,0.5;dlg_confirm_exit_cancel;" .. fgettext("Cancel") .. "]"

	return retval
end

--------------------------------------------------------------------------------
local function confirm_exit_buttonhandler(this, fields)
	if fields["dlg_confirm_exit_accept"] ~= nil then
		core.close()
		return true
	end

	if fields["dlg_confirm_exit_cancel"] then
		this:delete()
		return true
	end

	return false
end

--------------------------------------------------------------------------------
local function confirm_exit_event_handler(event)
	if event == "MenuQuit" then
		core.close()
		return true
	end
	return false
end

--------------------------------------------------------------------------------
function create_confirm_exit_dlg()

	local retval = dialog_create("dlg_confirm_exit",
					confirm_exit_formspec,
					confirm_exit_buttonhandler,
					confirm_exit_event_handler)
	return retval
end
