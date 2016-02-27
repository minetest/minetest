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
		"textlist[0,0.25;5.1,4.35;modlist;" ..
		modmgr.render_modlist(modmgr.global_mods) ..
		";" .. tabdata.selected_mod .. "]"

	retval = retval ..
--		"label[0.8,4.2;" .. fgettext("Add mod:") .. "]" ..
--		TODO Disabled due to upcoming release 0.4.8 and irrlicht messing up localization
--		"button[0.75,4.85;1.8,0.5;btn_mod_mgr_install_local;".. fgettext("Local install") .. "]" ..

--		TODO Disabled due to service being offline, and not likely to come online again, in this form
--		"button[0,4.85;5.25,0.5;btn_modstore;".. fgettext("Online mod repository") .. "]"
		""

	local selected_mod = nil

	if filterlist.size(modmgr.global_mods) >= tabdata.selected_mod then
		selected_mod = modmgr.global_mods:get_list()[tabdata.selected_mod]
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
			table.insert(descriptionlines,fgettext("No mod description available"))
		end

		retval = retval ..
			"label[5.5,1.7;".. fgettext("Mod information:") .. "]" ..
			"textlist[5.5,2.2;6.2,2.4;description;"

		for i=1,#descriptionlines,1 do
			retval = retval .. core.formspec_escape(descriptionlines[i]) .. ","
		end


		if selected_mod.is_modpack then
			retval = retval .. ";0]" ..
				"button[10,4.85;2,0.5;btn_mod_mgr_rename_modpack;" ..
				fgettext("Rename") .. "]"
			retval = retval .. "button[5.5,4.85;4.5,0.5;btn_mod_mgr_delete_mod;"
				.. fgettext("Uninstall selected modpack") .. "]"
		else
			--show dependencies

			retval = retval .. "," .. fgettext("Depends:") .. ","

			local toadd = modmgr.get_dependencies(selected_mod.path)

			retval = retval .. toadd .. ";0]"

			retval = retval .. "button[5.5,4.85;4.5,0.5;btn_mod_mgr_delete_mod;"
				.. fgettext("Uninstall selected mod") .. "]"
		end
	end
	return retval
end

--------------------------------------------------------------------------------
local function handle_buttons(tabview, fields, tabname, tabdata)
	if fields["modlist"] ~= nil then
		local event = core.explode_textlist_event(fields["modlist"])
		tabdata.selected_mod = event.index
		return true
	end

	if fields["btn_mod_mgr_install_local"] ~= nil then
		core.show_file_open_dialog("mod_mgt_open_dlg",fgettext("Select Mod File:"))
		return true
	end

	if fields["btn_modstore"] ~= nil then
		local modstore_ui = ui.find_by_name("modstore")
		if modstore_ui ~= nil then
			tabview:hide()
			modstore.update_modlist()
			modstore_ui:show()
		else
			print("modstore ui element not found")
		end
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
		modmgr.installmod(fields["mod_mgt_open_dlg_accepted"],nil)
		return true
	end

	return false
end

--------------------------------------------------------------------------------
tab_mods = {
	name = "mods",
	caption = fgettext("Mods"),
	cbf_formspec = get_formspec,
	cbf_button_handler = handle_buttons,
	on_change = gamemgr.update_gamelist
}
