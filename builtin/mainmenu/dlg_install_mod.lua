--Minetest
--Copyright (C) 2017 octacian
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

local function install_mod_formspec(dialogdata)
	local retval =
		"size[11.5,4.5,true]" ..
		"label[2,2;" ..
		fgettext(dialogdata.text) .. "]"..
		"button[4.5,3.5;2.5,0.5;dlg_install_mod_close;" .. fgettext("OK") .. "]"

	return retval
end

--------------------------------------------------------------------------------
local function install_mod_buttonhandler(this, fields)
	if fields["dlg_install_mod_close"] then
		this:delete()
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function create_install_mod_dlg(text)

	local retval = dialog_create("dlg_install_mod",
					install_mod_formspec,
					install_mod_buttonhandler,
					nil)
	retval.data.text = text
	return retval
end
