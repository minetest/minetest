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
local function get_formspec(tabview, name, tabdata)

	if modmgr.global_mods == nil then
		modmgr.refresh_globals()
	end

	if tabdata.selected_mod == nil then
		tabdata.selected_mod = 1
	end

	local retval =
		"label[0.05,-0.25;".. fgettext("Installed Mods:") .. "]" ..
		"tablecolumns[color;tree;text]" ..
		"table[0,0.25;5.1,4.4;modlist;" ..
		modmgr.render_modlist(modmgr.global_mods) ..
		";" .. tabdata.selected_mod .. "]"

	local selected_mod = nil

	if filterlist.size(modmgr.global_mods) >= tabdata.selected_mod then
		selected_mod = modmgr.global_mods:get_list()[tabdata.selected_mod]
	end

	if selected_mod == nil then
		retval = retval ..
				"button[0,4.9;5.33,0.5;btn_mod_mgr_install_local;".. fgettext("+ Install mod from ZIP") .. "]"
	else
		retval = retval ..
				"button[0,4.9;2.6,0.5;btn_mod_mgr_install_local;".. fgettext("+ Install mod") .. "]" ..
				"tooltip[btn_mod_mgr_install_local;".. fgettext("Install mod from ZIP") .."]"
	end

	if selected_mod ~= nil then
		local modscreenshot = nil

		--check for screenshot beeing available
		local screenshotfilename = selected_mod.path .. DIR_DELIM .. "screenshot.png"
		local error = nil
		local screenshotfile,error = io.open(screenshotfilename,"r")
		if error == nil then
			screenshotfile:close()
			modscreenshot = screenshotfilename
		end

		if modscreenshot == nil then
				modscreenshot = defaulttexturedir .. "no_screenshot.png"
		end

		retval = retval
				.. "image[5.5,0;3,2;" .. core.formspec_escape(modscreenshot) .. "]"
				.. "label[8.25,0.6;" .. selected_mod.name .. "]"

		local descriptionlines = nil
		error = nil
		local descriptionfilename = selected_mod.path .. "description.txt"
		local descriptionfile,error = io.open(descriptionfilename,"r")
		if error == nil then
			local descriptiontext = descriptionfile:read("*all")

			descriptionlines = core.splittext(descriptiontext,42)
			descriptionfile:close()
		else
			descriptionlines = {}
			descriptionlines[#descriptionlines + 1] = fgettext("No mod description available")
		end

		retval = retval ..
			"label[5.5,1.7;".. fgettext("Mod information:") .. "]" ..
			"textlist[5.5,2.2;6.2,3.22;description;"

		for i=1,#descriptionlines,1 do
			retval = retval .. core.formspec_escape(descriptionlines[i]) .. ","
		end


		if selected_mod.is_modpack then
			retval = retval .. ";0]" ..
				"button[10,4.85;2,0.5;btn_mod_mgr_rename_modpack;" ..
				fgettext("Rename") .. "]"
				retval = retval .. "button[2.53,4.9;2.8,0.5;btn_mod_mgr_delete_mod;"
						.. fgettext("Uninstall modpack") .. "]" ..
						"tooltip[btn_mod_mgr_delete_mod;".. fgettext("Uninstall selected modpack") .."]"
		else
			--show dependencies
			local toadd_hard, toadd_soft = modmgr.get_dependencies(selected_mod.path)
			if toadd_hard == "" and toadd_soft == "" then
				retval = retval .. "," .. fgettext("No dependencies.")
			else
				if toadd_hard ~= "" then
					retval = retval .. "," .. fgettext("Dependencies:") .. ","
					retval = retval .. toadd_hard
				end
				if toadd_soft ~= "" then
					if toadd_hard ~= "" then
						retval = retval .. ","
					end
					retval = retval .. "," .. fgettext("Optional dependencies:") .. ","
					retval = retval .. toadd_soft
				end
			end

			retval = retval .. ";0]"

			retval = retval .. "button[2.53,4.9;2.8,0.5;btn_mod_mgr_delete_mod;"
					.. fgettext("Uninstall mod") .. "]" ..
					"tooltip[btn_mod_mgr_delete_mod;".. fgettext("Uninstall selected mod") .."]"
		end
	end
	return retval
end

--------------------------------------------------------------------------------
local function prepare_install_mod_dlg(tabview, text)
	local dlg_install = create_install_mod_dlg(text)
	dlg_install:set_parent(tabview)
	tabview:hide()
	dlg_install:show()
end

--------------------------------------------------------------------------------
local function handle_buttons(tabview, fields, tabname, tabdata)
	if fields["modlist"] ~= nil then
		local event = core.explode_table_event(fields["modlist"])
		tabdata.selected_mod = event.row
		return true
	end

	if fields["btn_mod_mgr_install_local"] ~= nil then
		core.show_file_open_dialog("mod_mgt_open_dlg", fgettext("Select Mod File:"))
		return true
	end

	if fields["btn_mod_mgr_rename_modpack"] ~= nil then
		local dlg_renamemp = create_rename_modpack_dlg(tabdata.selected_mod)
		dlg_renamemp:set_parent(tabview)
		tabview:hide()
		dlg_renamemp:show()
		return true
	end

	if fields["btn_mod_mgr_delete_mod"] ~= nil then
		local dlg_delmod = create_delete_mod_dlg(tabdata.selected_mod)
		dlg_delmod:set_parent(tabview)
		tabview:hide()
		dlg_delmod:show()
		return true
	end

	if fields["mod_mgt_open_dlg_accepted"] ~= nil and
			fields["mod_mgt_open_dlg_accepted"] ~= "" then
		if modmgr.installmod(fields["mod_mgt_open_dlg_accepted"], nil) then
			prepare_install_mod_dlg(tabview, "Successfully installed mod")
		else
			prepare_install_mod_dlg(tabview, "Failed to install mod")
		end
		return true
	elseif fields["mod_mgt_open_dlg_cancelled"] or
			fields["mod_mgt_open_dlg_accepted"] == "" then
		prepare_install_mod_dlg(tabview, "Failed to install mod")
		return true
	end

	return false
end

--------------------------------------------------------------------------------
return {
	name = "mods",
	caption = fgettext("Mods"),
	cbf_formspec = get_formspec,
	cbf_button_handler = handle_buttons,
	on_change = gamemgr.update_gamelist
}
