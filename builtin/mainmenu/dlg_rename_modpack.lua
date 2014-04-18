--Minetest
--Copyright (C) 2014 sapier
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

local function rename_modpack_formspec(dialogdata)
	
	dialogdata.mod = modmgr.global_mods:get_list()[dialogdata.selected]

	local retval =
		"size[12.4,5,true]" ..
		"label[1.75,1;".. fgettext("Rename Modpack:") .. "]"..
		"field[4.5,1.4;6,0.5;te_modpack_name;;" ..
		dialogdata.mod.name ..
		"]" ..
		"button[5,4.2;2.6,0.5;dlg_rename_modpack_confirm;"..
				fgettext("Accept") .. "]" ..
		"button[7.5,4.2;2.8,0.5;dlg_rename_modpack_cancel;"..
				fgettext("Cancel") .. "]"
	
	return retval
end

--------------------------------------------------------------------------------
local function rename_modpack_buttonhandler(this, fields)
	if fields["dlg_rename_modpack_confirm"] ~= nil then
		local oldpath = core.get_modpath() .. DIR_DELIM .. this.data.mod.name
		local targetpath = core.get_modpath() .. DIR_DELIM .. fields["te_modpack_name"]
		core.copy_dir(oldpath,targetpath,false)
		modmgr.refresh_globals()
		modmgr.selected_mod = modmgr.global_mods:get_current_index(
			modmgr.global_mods:raw_index_by_uid(fields["te_modpack_name"]))
			
		this:delete()
		return true
	end
	
	if fields["dlg_rename_modpack_cancel"] then
		this:delete()
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function create_rename_modpack_dlg(selected_index)

	local retval = dialog_create("dlg_delete_mod",
					rename_modpack_formspec,
					rename_modpack_buttonhandler,
					nil)
	retval.data.selected = selected_index
	return retval
end
