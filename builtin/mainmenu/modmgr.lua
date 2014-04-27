--Minetest
--Copyright (C) 2013 sapier
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
function get_mods(path,retval,modpack)

	local mods = engine.get_dirlist(path,true)
	for i=1,#mods,1 do
		local toadd = {}
		local modpackfile = nil

		toadd.name		= mods[i]
		toadd.path		= path .. DIR_DELIM .. mods[i] .. DIR_DELIM
		if modpack ~= nil and
			modpack ~= "" then
			toadd.modpack	= modpack
		else
			local filename = path .. DIR_DELIM .. mods[i] .. DIR_DELIM .. "modpack.txt"
			local error = nil
			modpackfile,error = io.open(filename,"r")
		end

		if modpackfile ~= nil then
			modpackfile:close()
			toadd.is_modpack = true
			table.insert(retval,toadd)
			get_mods(path .. DIR_DELIM .. mods[i],retval,mods[i])
		else
			table.insert(retval,toadd)
		end
	end
end

--modmanager implementation
modmgr = {}

--------------------------------------------------------------------------------
function modmgr.extract(modfile)
	if modfile.type == "zip" then
		local tempfolder = os.tempfolder()

		if tempfolder ~= nil and
			tempfolder ~= "" then
			engine.create_dir(tempfolder)
			if engine.extract_zip(modfile.name,tempfolder) then
				return tempfolder
			end
		end
	end
	return nil
end

-------------------------------------------------------------------------------
function modmgr.getbasefolder(temppath)

	if temppath == nil then
		return {
		type = "invalid",
		path = ""
		}
	end

	local testfile = io.open(temppath .. DIR_DELIM .. "init.lua","r")
	if testfile ~= nil then
		testfile:close()
		return {
				type="mod",
				path=temppath
				}
	end

	testfile = io.open(temppath .. DIR_DELIM .. "modpack.txt","r")
	if testfile ~= nil then
		testfile:close()
		return {
				type="modpack",
				path=temppath
				}
	end

	local subdirs = engine.get_dirlist(temppath,true)

	--only single mod or modpack allowed
	if #subdirs ~= 1 then
		return {
			type = "invalid",
			path = ""
			}
	end

	testfile =
	io.open(temppath .. DIR_DELIM .. subdirs[1] ..DIR_DELIM .."init.lua","r")
	if testfile ~= nil then
		testfile:close()
		return {
			type="mod",
			path= temppath .. DIR_DELIM .. subdirs[1]
			}
	end

	testfile =
	io.open(temppath .. DIR_DELIM .. subdirs[1] ..DIR_DELIM .."modpack.txt","r")
	if testfile ~= nil then
		testfile:close()
		return {
			type="modpack",
			path=temppath ..  DIR_DELIM .. subdirs[1]
			}
	end

	return {
		type = "invalid",
		path = ""
		}
end

--------------------------------------------------------------------------------
function modmgr.isValidModname(modpath)
	if modpath:find("-") ~= nil then
		return false
	end

	return true
end

--------------------------------------------------------------------------------
function modmgr.parse_register_line(line)
	local pos1 = line:find("\"")
	local pos2 = nil
	if pos1 ~= nil then
		pos2 = line:find("\"",pos1+1)
	end

	if pos1 ~= nil and pos2 ~= nil then
		local item = line:sub(pos1+1,pos2-1)

		if item ~= nil and
			item ~= "" then
			local pos3 = item:find(":")

			if pos3 ~= nil then
				local retval = item:sub(1,pos3-1)
				if retval ~= nil and
					retval ~= "" then
					return retval
				end
			end
		end
	end
	return nil
end

--------------------------------------------------------------------------------
function modmgr.parse_dofile_line(modpath,line)
	local pos1 = line:find("\"")
	local pos2 = nil
	if pos1 ~= nil then
		pos2 = line:find("\"",pos1+1)
	end

	if pos1 ~= nil and pos2 ~= nil then
		local filename = line:sub(pos1+1,pos2-1)

		if filename ~= nil and
			filename ~= "" and
			filename:find(".lua") then
			return modmgr.identify_modname(modpath,filename)
		end
	end
	return nil
end

--------------------------------------------------------------------------------
function modmgr.identify_modname(modpath,filename)
	local testfile = io.open(modpath .. DIR_DELIM .. filename,"r")
	if testfile ~= nil then
		local line = testfile:read()

		while line~= nil do
			local modname = nil

			if line:find("minetest.register_tool") then
				modname = modmgr.parse_register_line(line)
			end

			if line:find("minetest.register_craftitem") then
				modname = modmgr.parse_register_line(line)
			end


			if line:find("minetest.register_node") then
				modname = modmgr.parse_register_line(line)
			end

			if line:find("dofile") then
				modname = modmgr.parse_dofile_line(modpath,line)
			end

			if modname ~= nil then
				testfile:close()
				return modname
			end

			line = testfile:read()
		end
		testfile:close()
	end

	return nil
end

--------------------------------------------------------------------------------
function modmgr.tab()

	if modmgr.global_mods == nil then
		modmgr.refresh_globals()
	end

	if modmgr.selected_mod == nil then
		modmgr.selected_mod = 1
	end

	local retval =
		"vertlabel[0,-0.25;".. fgettext("MODS") .. "]" ..
		"label[0.8,-0.25;".. fgettext("Installed Mods:") .. "]" ..
		"textlist[0.75,0.25;4.5,4;modlist;" ..
		modmgr.render_modlist(modmgr.global_mods) ..
		";" .. modmgr.selected_mod .. "]"

	retval = retval ..
		"label[0.8,4.2;" .. fgettext("Add mod:") .. "]" ..
--		TODO Disabled due to upcoming release 0.4.8 and irrlicht messing up localization
--		"button[0.75,4.85;1.8,0.5;btn_mod_mgr_install_local;".. fgettext("Local install") .. "]" ..
		"button[2.45,4.85;3.05,0.5;btn_mod_mgr_download;".. fgettext("Online mod repository") .. "]"

	local selected_mod = nil

	if filterlist.size(modmgr.global_mods) >= modmgr.selected_mod then
		selected_mod = filterlist.get_list(modmgr.global_mods)[modmgr.selected_mod]
	end

	if selected_mod ~= nil then
		local modscreenshot = nil

		--check for screenshot beeing available
		local screenshotfilename = selected_mod.path .. DIR_DELIM .. "screenshot.png"
		local error = nil
		screenshotfile,error = io.open(screenshotfilename,"r")
		if error == nil then
			screenshotfile:close()
			modscreenshot = screenshotfilename
		end

		if modscreenshot == nil then
				modscreenshot = modstore.basetexturedir .. "no_screenshot.png"
		end

		retval = retval
				.. "image[5.5,0;3,2;" .. engine.formspec_escape(modscreenshot) .. "]"
				.. "label[8.25,0.6;" .. selected_mod.name .. "]"

		local descriptionlines = nil
		error = nil
		local descriptionfilename = selected_mod.path .. "description.txt"
		descriptionfile,error = io.open(descriptionfilename,"r")
		if error == nil then
			descriptiontext = descriptionfile:read("*all")

			descriptionlines = engine.splittext(descriptiontext,42)
			descriptionfile:close()
		else
			descriptionlines = {}
			table.insert(descriptionlines,fgettext("No mod description available"))
		end

		retval = retval ..
			"label[5.5,1.7;".. fgettext("Mod information:") .. "]" ..
			"textlist[5.5,2.2;6.2,2.4;description;"

		for i=1,#descriptionlines,1 do
			retval = retval .. engine.formspec_escape(descriptionlines[i]) .. ","
		end


		if selected_mod.is_modpack then
			retval = retval .. ";0]" ..
				"button[10,4.85;2,0.5;btn_mod_mgr_rename_modpack;" ..
				fgettext("Rename") .. "]"
			retval = retval .. "button[5.5,4.85;4.5,0.5;btn_mod_mgr_delete_mod;"
				.. fgettext("Uninstall selected modpack") .. "]"
		else
			--show dependencies

			retval = retval .. ",Depends:,"

			toadd = modmgr.get_dependencies(selected_mod.path)

			retval = retval .. toadd .. ";0]"

			retval = retval .. "button[5.5,4.85;4.5,0.5;btn_mod_mgr_delete_mod;"
				.. fgettext("Uninstall selected mod") .. "]"
		end
	end
	return retval
end

--------------------------------------------------------------------------------
function modmgr.dialog_rename_modpack()

	local mod = filterlist.get_list(modmgr.global_mods)[modmgr.selected_mod]

	local retval =
		"label[1.75,1;".. fgettext("Rename Modpack:") .. "]"..
		"field[4.5,1.4;6,0.5;te_modpack_name;;" ..
		mod.name ..
		"]" ..
		"button[5,4.2;2.6,0.5;dlg_rename_modpack_confirm;"..
				fgettext("Accept") .. "]" ..
		"button[7.5,4.2;2.8,0.5;dlg_rename_modpack_cancel;"..
				fgettext("Cancel") .. "]"

	return retval
end

--------------------------------------------------------------------------------
function modmgr.precheck()

	if modmgr.world_config_selected_world == nil then
		modmgr.world_config_selected_world = 1
	end

	if modmgr.world_config_selected_mod == nil then
		modmgr.world_config_selected_mod = 1
	end

	if modmgr.hide_gamemods == nil then
		modmgr.hide_gamemods = true
	end

	if modmgr.hide_modpackcontents == nil then
		modmgr.hide_modpackcontents = true
	end
end

--------------------------------------------------------------------------------
function modmgr.render_modlist(render_list)
	local retval = ""

	if render_list == nil then
		if modmgr.global_mods == nil then
			modmgr.refresh_globals()
		end
		render_list = modmgr.global_mods
	end

	local list = filterlist.get_list(render_list)
	local last_modpack = nil

	for i,v in ipairs(list) do
		if retval ~= "" then
			retval = retval ..","
		end

		local color = ""

		if v.is_modpack then
			local rawlist = filterlist.get_raw_list(render_list)

			local all_enabled = true
			for j=1,#rawlist,1 do
				if rawlist[j].modpack == list[i].name and
					rawlist[j].enabled ~= true then
						all_enabled = false
						break
				end
			end

			if all_enabled == false then
				color = mt_color_grey
			else
				color = mt_color_dark_green
			end
		end

		if v.typ == "game_mod" then
			color = mt_color_blue
		else
			if v.enabled then
				color = mt_color_green
			end
		end

		retval = retval .. color
		if v.modpack  ~= nil then
			retval = retval .. "    "
		end
		retval = retval .. v.name
	end

	return retval
end

--------------------------------------------------------------------------------
function modmgr.dialog_configure_world()
	modmgr.precheck()

	local worldspec = engine.get_worlds()[modmgr.world_config_selected_world]
	local mod = filterlist.get_list(modmgr.modlist)[modmgr.world_config_selected_mod]

	local retval =
		"size[11,6.5,true]" ..
		"label[0.5,-0.25;" .. fgettext("World:") .. "]" ..
		"label[1.75,-0.25;" .. worldspec.name .. "]"

	if modmgr.hide_gamemods then
		retval = retval .. "checkbox[0,5.75;cb_hide_gamemods;" .. fgettext("Hide Game") .. ";true]"
	else
		retval = retval .. "checkbox[0,5.75;cb_hide_gamemods;" .. fgettext("Hide Game") .. ";false]"
	end

	if modmgr.hide_modpackcontents then
		retval = retval .. "checkbox[2,5.75;cb_hide_mpcontent;" .. fgettext("Hide mp content") .. ";true]"
	else
		retval = retval .. "checkbox[2,5.75;cb_hide_mpcontent;" .. fgettext("Hide mp content") .. ";false]"
	end

	if mod == nil then
		mod = {name=""}
	end
	retval = retval ..
		"label[0,0.45;" .. fgettext("Mod:") .. "]" ..
		"label[0.75,0.45;" .. mod.name .. "]" ..
		"label[0,1;" .. fgettext("Depends:") .. "]" ..
		"textlist[0,1.5;5,4.25;world_config_depends;" ..
		modmgr.get_dependencies(mod.path) .. ";0]" ..
		"button[9.25,6.35;2,0.5;btn_config_world_save;" .. fgettext("Save") .. "]" ..
		"button[7.4,6.35;2,0.5;btn_config_world_cancel;" .. fgettext("Cancel") .. "]"

	if mod ~= nil and mod.name ~= "" and mod.typ ~= "game_mod" then
		if mod.is_modpack then
			local rawlist = filterlist.get_raw_list(modmgr.modlist)

			local all_enabled = true
			for j=1,#rawlist,1 do
				if rawlist[j].modpack == mod.name and
					rawlist[j].enabled ~= true then
						all_enabled = false
						break
				end
			end

			if all_enabled == false then
				retval = retval .. "button[5.5,-0.125;2,0.5;btn_mp_enable;" .. fgettext("Enable MP") .. "]"
			else
				retval = retval .. "button[5.5,-0.125;2,0.5;btn_mp_disable;" .. fgettext("Disable MP") .. "]"
			end
		else
			if mod.enabled then
				retval = retval .. "checkbox[5.5,-0.375;cb_mod_enable;" .. fgettext("enabled") .. ";true]"
			else
				retval = retval .. "checkbox[5.5,-0.375;cb_mod_enable;" .. fgettext("enabled") .. ";false]"
			end
		end
	end

	retval = retval ..
		"button[8.5,-0.125;2.5,0.5;btn_all_mods;" .. fgettext("Enable all") .. "]" ..
		"textlist[5.5,0.5;5.5,5.75;world_config_modlist;"

	retval = retval .. modmgr.render_modlist(modmgr.modlist)

	retval = retval .. ";" .. modmgr.world_config_selected_mod .."]"

	return retval
end

--------------------------------------------------------------------------------
function modmgr.handle_buttons(tab,fields)

	local retval = nil

	if tab == "mod_mgr" then
		retval = modmgr.handle_modmgr_buttons(fields)
	end

	if tab == "dialog_rename_modpack" then
		retval = modmgr.handle_rename_modpack_buttons(fields)
	end

	if tab == "dialog_delete_mod" then
		retval = modmgr.handle_delete_mod_buttons(fields)
	end

	if tab == "dialog_configure_world" then
		retval = modmgr.handle_configure_world_buttons(fields)
	end

	return retval
end

--------------------------------------------------------------------------------
function modmgr.get_dependencies(modfolder)
	local toadd = ""
	if modfolder ~= nil then
		local filename = modfolder ..
					DIR_DELIM .. "depends.txt"

		local dependencyfile = io.open(filename,"r")

		if dependencyfile then
			local dependency = dependencyfile:read("*l")
			while dependency do
				if toadd ~= "" then
					toadd = toadd .. ","
				end
				toadd = toadd .. dependency
				dependency = dependencyfile:read()
			end
			dependencyfile:close()
		end
	end

	return toadd
end


--------------------------------------------------------------------------------
function modmgr.get_worldconfig(worldpath)
	local filename = worldpath ..
				DIR_DELIM .. "world.mt"

	local worldfile = Settings(filename)

	local worldconfig = {}
	worldconfig.global_mods = {}
	worldconfig.game_mods = {}

	for key,value in pairs(worldfile:to_table()) do
		if key == "gameid" then
			worldconfig.id = value
		else
			worldconfig.global_mods[key] = engine.is_yes(value)
		end
	end

	--read gamemods
	local gamespec = gamemgr.find_by_gameid(worldconfig.id)
	gamemgr.get_game_mods(gamespec, worldconfig.game_mods)

	return worldconfig
end
--------------------------------------------------------------------------------
function modmgr.handle_modmgr_buttons(fields)
	local retval = {
			tab = nil,
			is_dialog = nil,
			show_buttons = nil,
		}

	if fields["modlist"] ~= nil then
		local event = engine.explode_textlist_event(fields["modlist"])
		modmgr.selected_mod = event.index
	end

	if fields["btn_mod_mgr_install_local"] ~= nil then
		engine.show_file_open_dialog("mod_mgt_open_dlg",fgettext("Select Mod File:"))
	end

	if fields["btn_mod_mgr_download"] ~= nil then
		modstore.update_modlist()
		retval.current_tab = "dialog_modstore_unsorted"
		retval.is_dialog = true
		retval.show_buttons = false
		return retval
	end

	if fields["btn_mod_mgr_rename_modpack"] ~= nil then
		retval.current_tab = "dialog_rename_modpack"
		retval.is_dialog = true
		retval.show_buttons = false
		return retval
	end

	if fields["btn_mod_mgr_delete_mod"] ~= nil then
		retval.current_tab = "dialog_delete_mod"
		retval.is_dialog = true
		retval.show_buttons = false
		return retval
	end

	if fields["mod_mgt_open_dlg_accepted"] ~= nil and
		fields["mod_mgt_open_dlg_accepted"] ~= "" then
		modmgr.installmod(fields["mod_mgt_open_dlg_accepted"],nil)
	end

	return nil;
end

--------------------------------------------------------------------------------
function modmgr.installmod(modfilename,basename)
	local modfile = modmgr.identify_filetype(modfilename)
	local modpath = modmgr.extract(modfile)

	if modpath == nil then
		gamedata.errormessage = fgettext("Install Mod: file: \"$1\"", modfile.name) ..
			fgettext("\nInstall Mod: unsupported filetype \"$1\" or broken archive", modfile.type)
		return
	end


	local basefolder = modmgr.getbasefolder(modpath)

	if basefolder.type == "modpack" then
		local clean_path = nil

		if basename ~= nil then
			clean_path = "mp_" .. basename
		end

		if clean_path == nil then
			clean_path = get_last_folder(cleanup_path(basefolder.path))
		end

		if clean_path ~= nil then
			local targetpath = engine.get_modpath() .. DIR_DELIM .. clean_path
			if not engine.copy_dir(basefolder.path,targetpath) then
				gamedata.errormessage = fgettext("Failed to install $1 to $2", basename, targetpath)
			end
		else
			gamedata.errormessage = fgettext("Install Mod: unable to find suitable foldername for modpack $1", modfilename)
		end
	end

	if basefolder.type == "mod" then
		local targetfolder = basename

		if targetfolder == nil then
			targetfolder = modmgr.identify_modname(basefolder.path,"init.lua")
		end

		--if heuristic failed try to use current foldername
		if targetfolder == nil then
			targetfolder = get_last_folder(basefolder.path)
		end

		if targetfolder ~= nil and modmgr.isValidModname(targetfolder) then
			local targetpath = engine.get_modpath() .. DIR_DELIM .. targetfolder
			engine.copy_dir(basefolder.path,targetpath)
		else
			gamedata.errormessage = fgettext("Install Mod: unable to find real modname for: $1", modfilename)
		end
	end

	engine.delete_dir(modpath)

	modmgr.refresh_globals()

end

--------------------------------------------------------------------------------
function modmgr.handle_rename_modpack_buttons(fields)

	if fields["dlg_rename_modpack_confirm"] ~= nil then
		local mod = filterlist.get_list(modmgr.global_mods)[modmgr.selected_mod]
		local oldpath = engine.get_modpath() .. DIR_DELIM .. mod.name
		local targetpath = engine.get_modpath() .. DIR_DELIM .. fields["te_modpack_name"]
		engine.copy_dir(oldpath,targetpath,false)
		modmgr.refresh_globals()
		modmgr.selected_mod = filterlist.get_current_index(modmgr.global_mods,
			filterlist.raw_index_by_uid(modmgr.global_mods, fields["te_modpack_name"]))
	end

	return {
		is_dialog = false,
		show_buttons = true,
		current_tab = engine.setting_get("main_menu_tab")
		}
end
--------------------------------------------------------------------------------
function modmgr.handle_configure_world_buttons(fields)
	if fields["world_config_modlist"] ~= nil then
		local event = engine.explode_textlist_event(fields["world_config_modlist"])
		modmgr.world_config_selected_mod = event.index

		if event.type == "DCL" then
			modmgr.world_config_enable_mod(nil)
		end
	end

	if fields["key_enter"] ~= nil then
		modmgr.world_config_enable_mod(nil)
	end

	if fields["cb_mod_enable"] ~= nil then
		local toset = engine.is_yes(fields["cb_mod_enable"])
		modmgr.world_config_enable_mod(toset)
	end

	if fields["btn_mp_enable"] ~= nil or
		fields["btn_mp_disable"] then
		local toset = (fields["btn_mp_enable"] ~= nil)
		modmgr.world_config_enable_mod(toset)
	end

	if fields["cb_hide_gamemods"] ~= nil then
		local current = filterlist.get_filtercriteria(modmgr.modlist)

		if current == nil then
			current = {}
		end

		if engine.is_yes(fields["cb_hide_gamemods"]) then
			current.hide_game = true
			modmgr.hide_gamemods = true
		else
			current.hide_game = false
			modmgr.hide_gamemods = false
		end

		filterlist.set_filtercriteria(modmgr.modlist,current)
	end

		if fields["cb_hide_mpcontent"] ~= nil then
		local current = filterlist.get_filtercriteria(modmgr.modlist)

		if current == nil then
			current = {}
		end

		if engine.is_yes(fields["cb_hide_mpcontent"]) then
			current.hide_modpackcontents = true
			modmgr.hide_modpackcontents = true
		else
			current.hide_modpackcontents = false
			modmgr.hide_modpackcontents = false
		end

		filterlist.set_filtercriteria(modmgr.modlist,current)
	end

	if fields["btn_config_world_save"] then
		local worldspec = engine.get_worlds()[modmgr.world_config_selected_world]

		local filename = worldspec.path ..
				DIR_DELIM .. "world.mt"

		local worldfile = Settings(filename)
		local mods = worldfile:to_table()

		local rawlist = filterlist.get_raw_list(modmgr.modlist)

		local i,mod
		for i,mod in ipairs(rawlist) do
			if not mod.is_modpack and
					mod.typ ~= "game_mod" then
				if mod.enabled then
					worldfile:set("load_mod_"..mod.name, "true")
				else
					worldfile:set("load_mod_"..mod.name, "false")
				end
				mods["load_mod_"..mod.name] = nil
			end
		end

		-- Remove mods that are not present anymore
		for key,value in pairs(mods) do
			if key:sub(1,9) == "load_mod_" then
				worldfile:remove(key)
			end
		end

		if not worldfile:write() then
			engine.log("error", "Failed to write world config file")
		end

		modmgr.modlist = nil
		modmgr.worldconfig = nil

		return {
			is_dialog = false,
			show_buttons = true,
			current_tab = engine.setting_get("main_menu_tab")
		}
	end

	if fields["btn_config_world_cancel"] then

		modmgr.worldconfig = nil

		return {
			is_dialog = false,
			show_buttons = true,
			current_tab = engine.setting_get("main_menu_tab")
		}
	end

	if fields["btn_all_mods"] then
		local list = filterlist.get_raw_list(modmgr.modlist)

		for i=1,#list,1 do
			if list[i].typ ~= "game_mod" and
				not list[i].is_modpack then
				list[i].enabled = true
			end
		end
	end



	return nil
end
--------------------------------------------------------------------------------
function modmgr.world_config_enable_mod(toset)
	local mod = filterlist.get_list(modmgr.modlist)
		[engine.get_textlist_index("world_config_modlist")]

	if mod.typ == "game_mod" then
		-- game mods can't be enabled or disabled
	elseif not mod.is_modpack then
		if toset == nil then
			mod.enabled = not mod.enabled
		else
			mod.enabled = toset
		end
	else
		local list = filterlist.get_raw_list(modmgr.modlist)
		for i=1,#list,1 do
			if list[i].modpack == mod.name then
				if toset == nil then
					toset = not list[i].enabled
				end
				list[i].enabled = toset
			end
		end
	end
end
--------------------------------------------------------------------------------
function modmgr.handle_delete_mod_buttons(fields)
	local mod = filterlist.get_list(modmgr.global_mods)[modmgr.selected_mod]

	if fields["dlg_delete_mod_confirm"] ~= nil then

		if mod.path ~= nil and
			mod.path ~= "" and
			mod.path ~= engine.get_modpath() then
			if not engine.delete_dir(mod.path) then
				gamedata.errormessage = fgettext("Modmgr: failed to delete \"$1\"", mod.path)
			end
			modmgr.refresh_globals()
		else
			gamedata.errormessage = fgettext("Modmgr: invalid modpath \"$1\"", mod.path)
		end
	end

	return {
		is_dialog = false,
		show_buttons = true,
		current_tab = engine.setting_get("main_menu_tab")
		}
end

--------------------------------------------------------------------------------
function modmgr.dialog_delete_mod()

	local mod = filterlist.get_list(modmgr.global_mods)[modmgr.selected_mod]

	local retval =
		"field[1.75,1;10,3;;" .. fgettext("Are you sure you want to delete \"$1\"?", mod.name) ..  ";]"..
		"button[4,4.2;1,0.5;dlg_delete_mod_confirm;" .. fgettext("Yes") .. "]" ..
		"button[6.5,4.2;3,0.5;dlg_delete_mod_cancel;" .. fgettext("No of course not!") .. "]"

	return retval
end

--------------------------------------------------------------------------------
function modmgr.preparemodlist(data)
	local retval = {}

	local global_mods = {}
	local game_mods = {}

	--read global mods
	local modpath = engine.get_modpath()

	if modpath ~= nil and
		modpath ~= "" then
		get_mods(modpath,global_mods)
	end

	for i=1,#global_mods,1 do
		global_mods[i].typ = "global_mod"
		table.insert(retval,global_mods[i])
	end

	--read game mods
	local gamespec = gamemgr.find_by_gameid(data.gameid)
	gamemgr.get_game_mods(gamespec, game_mods)

	for i=1,#game_mods,1 do
		game_mods[i].typ = "game_mod"
		table.insert(retval,game_mods[i])
	end

	if data.worldpath == nil then
		return retval
	end

	--read world mod configuration
	local filename = data.worldpath ..
				DIR_DELIM .. "world.mt"

	local worldfile = Settings(filename)

	for key,value in pairs(worldfile:to_table()) do
		if key:sub(1, 9) == "load_mod_" then
			key = key:sub(10)
			local element = nil
			for i=1,#retval,1 do
				if retval[i].name == key then
					element = retval[i]
					break
				end
			end
			if element ~= nil then
				element.enabled = engine.is_yes(value)
			else
				engine.log("info", "Mod: " .. key .. " " .. dump(value) .. " but not found")
			end
		end
	end

	return retval
end

--------------------------------------------------------------------------------
function modmgr.init_worldconfig()
	modmgr.precheck()
	local worldspec = engine.get_worlds()[modmgr.world_config_selected_world]

	if worldspec ~= nil then
		--read worldconfig
		modmgr.worldconfig = modmgr.get_worldconfig(worldspec.path)

		if modmgr.worldconfig.id == nil or
			modmgr.worldconfig.id == "" then
			modmgr.worldconfig = nil
			return false
		end

		modmgr.modlist = filterlist.create(
						modmgr.preparemodlist, --refresh
						modmgr.comparemod, --compare
						function(element,uid) --uid match
							if element.name == uid then
								return true
							end
						end,
						function(element,criteria)
							if criteria.hide_game and
								element.typ == "game_mod" then
									return false
							end

							if criteria.hide_modpackcontents and
								element.modpack ~= nil then
									return false
								end
							return true
						end, --filter
						{ worldpath= worldspec.path,
						  gameid = worldspec.gameid }
					)

		filterlist.set_filtercriteria(modmgr.modlist, {
									hide_game=modmgr.hide_gamemods,
									hide_modpackcontents= modmgr.hide_modpackcontents
									})
		filterlist.add_sort_mechanism(modmgr.modlist, "alphabetic", sort_mod_list)
		filterlist.set_sortmode(modmgr.modlist, "alphabetic")

		return true
	end

	return false
end

--------------------------------------------------------------------------------
function modmgr.comparemod(elem1,elem2)
	if elem1 == nil or elem2 == nil then
		return false
	end
	if elem1.name ~= elem2.name then
		return false
	end
	if elem1.is_modpack ~= elem2.is_modpack then
		return false
	end
	if elem1.typ ~= elem2.typ then
		return false
	end
	if elem1.modpack ~= elem2.modpack then
		return false
	end

	if elem1.path ~= elem2.path then
		return false
	end

	return true
end

--------------------------------------------------------------------------------
function modmgr.gettab(name)
	local retval = ""

	if name == "mod_mgr" then
		retval = retval .. modmgr.tab()
	end

	if name == "dialog_rename_modpack" then
		retval = retval .. modmgr.dialog_rename_modpack()
	end

	if name == "dialog_delete_mod" then
		retval = retval .. modmgr.dialog_delete_mod()
	end

	if name == "dialog_configure_world" then
		retval = retval .. modmgr.dialog_configure_world()
	end

	return retval
end

--------------------------------------------------------------------------------
function modmgr.mod_exists(basename)

	if modmgr.global_mods == nil then
		modmgr.refresh_globals()
	end

	if filterlist.raw_index_by_uid(modmgr.global_mods,basename) > 0 then
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function modmgr.get_global_mod(idx)

	if modmgr.global_mods == nil then
		return nil
	end

	if idx == nil or idx < 1 or idx > filterlist.size(modmgr.global_mods) then
		return nil
	end

	return filterlist.get_list(modmgr.global_mods)[idx]
end

--------------------------------------------------------------------------------
function modmgr.refresh_globals()
	modmgr.global_mods = filterlist.create(
					modmgr.preparemodlist, --refresh
					modmgr.comparemod, --compare
					function(element,uid) --uid match
						if element.name == uid then
							return true
						end
					end,
					nil, --filter
					{}
					)
	filterlist.add_sort_mechanism(modmgr.global_mods, "alphabetic", sort_mod_list)
	filterlist.set_sortmode(modmgr.global_mods, "alphabetic")
end

--------------------------------------------------------------------------------
function modmgr.identify_filetype(name)

	if name:sub(-3):lower() == "zip" then
		return {
				name = name,
				type = "zip"
				}
	end

	if name:sub(-6):lower() == "tar.gz" or
		name:sub(-3):lower() == "tgz"then
		return {
				name = name,
				type = "tgz"
				}
	end

	if name:sub(-6):lower() == "tar.bz2" then
		return {
				name = name,
				type = "tbz"
				}
	end

	if name:sub(-2):lower() == "7z" then
		return {
				name = name,
				type = "7z"
				}
	end

	return {
		name = name,
		type = "ukn"
	}
end
