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
function get_mods(path,retval,basefolder)

	local mods = engine.get_dirlist(path,true)
			
	for i=1,#mods,1 do
		local filename = path .. DIR_DELIM .. mods[i] .. DIR_DELIM .. "modpack.txt"
		local modpackfile,error = io.open(filename,"r")
		
		local name = mods[i]
		if basefolder ~= nil and
			basefolder ~= "" then
			name = basefolder .. DIR_DELIM .. mods[i]
		end
			
		if modpackfile ~= nil then
			modpackfile:close()
			table.insert(retval,name .. " <MODPACK>")
			get_mods(path .. DIR_DELIM .. name,retval,name)
		else

			table.insert(retval,name)
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
			tempfodler ~= "" then
			engine.create_dir(tempfolder)
			engine.extract_zip(modfile.name,tempfolder)
			return tempfolder
		end
	end
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
				return item:sub(1,pos3-1)
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
function modmgr.update_global_mods()
	local modpath = engine.get_modpath()
	modmgr.global_mods = {}
	if modpath ~= nil and
		modpath ~= "" then
		get_mods(modpath,modmgr.global_mods)
	end
end

--------------------------------------------------------------------------------
function modmgr.get_mods_list()
	local toadd = ""
	
	modmgr.update_global_mods()
	
	if modmgr.global_mods ~= nil then
		for i=1,#modmgr.global_mods,1 do
			if toadd ~= "" then
				toadd = toadd..","
			end
			toadd = toadd .. modmgr.global_mods[i]
		end 
	end
	
	return toadd
end

--------------------------------------------------------------------------------
function modmgr.mod_exists(basename)
	modmgr.update_global_mods()
	
	if modmgr.global_mods ~= nil then
		for i=1,#modmgr.global_mods,1 do
			if modmgr.global_mods[i] == basename then
				return true
			end
		end
	end
	
	return false
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
	if modmgr.selected_mod == nil then
		modmgr.selected_mod = 1
	end
	
	local retval = 
		"vertlabel[0,-0.25;MODS]" ..
		"label[0.8,-0.25;Installed Mods:]" ..
		"textlist[0.75,0.25;4.5,4.3;modlist;" ..
		modmgr.get_mods_list() .. 
		";" .. modmgr.selected_mod .. "]"

	retval = retval ..
		"button[1,4.85;2,0.5;btn_mod_mgr_install_local;Install]" ..
		"button[3,4.85;2,0.5;btn_mod_mgr_download;Download]"
		
	if #modmgr.global_mods >= modmgr.selected_mod and
		modmgr.global_mods[modmgr.selected_mod]:find("<MODPACK>") then
		retval = retval .. "button[10,4.85;2,0.5;btn_mod_mgr_rename_modpack;Rename]"
	end
	
	if #modmgr.global_mods >= modmgr.selected_mod then
		local modpath = engine.get_modpath()
		--show dependencys
		if modmgr.global_mods[modmgr.selected_mod]:find("<MODPACK>") == nil then
			retval = retval .. 
				"label[6,1.9;Depends:]" ..
				"textlist[6,2.4;5.7,2;deplist;"
				
			toadd = modmgr.get_dependencys(modpath .. DIR_DELIM .. 
						modmgr.global_mods[modmgr.selected_mod])
			
			retval = retval .. toadd .. ";0;true,false]"
			
			--TODO read modinfo
		end
		--show delete button
		retval = retval .. "button[8,4.85;2,0.5;btn_mod_mgr_delete_mod;Delete]"
	end
	return retval
end

--------------------------------------------------------------------------------
function modmgr.dialog_rename_modpack()

	local modname = modmgr.global_mods[modmgr.selected_mod]
	modname = modname:sub(0,modname:find("<") -2)
	
	local retval = 
		"label[1.75,1;Rename Modpack:]"..
		"field[4.5,1.4;6,0.5;te_modpack_name;;" ..
		modname ..
		"]" ..
		"button[5,4.2;2.6,0.5;dlg_rename_modpack_confirm;Accept]" ..
		"button[7.5,4.2;2.8,0.5;dlg_rename_modpack_cancel;Cancel]"

	return retval
end

--------------------------------------------------------------------------------
function modmgr.precheck()
	if modmgr.global_mods == nil then
		modmgr.update_global_mods()
	end
	
	if modmgr.world_config_selected_world == nil then
		modmgr.world_config_selected_world = 1
	end
	
	if modmgr.world_config_selected_mod == nil then
		modmgr.world_config_selected_mod = 1
	end
	
	if modmgr.hide_gamemods == nil then
		modmgr.hide_gamemods = true
	end
end

--------------------------------------------------------------------------------
function modmgr.get_worldmod_idx()
	if not modmgr.hide_gamemods then
		return modmgr.world_config_selected_mod - #modmgr.worldconfig.game_mods
	else
		return modmgr.world_config_selected_mod
	end
end

--------------------------------------------------------------------------------
function modmgr.is_gamemod()
	if not modmgr.hide_gamemods then
		if modmgr.world_config_selected_mod <= #modmgr.worldconfig.game_mods then
			return true
		else
			return false
		end
	else
		return false
	end
end

--------------------------------------------------------------------------------
function modmgr.render_worldmodlist()
	local retval = ""
	
	for i=1,#modmgr.global_mods,1 do
		local parts = modmgr.global_mods[i]:split(DIR_DELIM)
		local shortname = parts[#parts]
		if modmgr.worldconfig.global_mods[shortname] then
			retval = retval .. "#GRN" .. modmgr.global_mods[i] .. ","
		else
			retval = retval .. modmgr.global_mods[i] .. ","
		end
	end
	
	return retval
end

--------------------------------------------------------------------------------
function modmgr.render_gamemodlist()
	local retval = ""
	for i=1,#modmgr.worldconfig.game_mods,1 do
		retval = retval ..
			"#BLU" .. modmgr.worldconfig.game_mods[i] .. ","
	end
	
	return retval
end

--------------------------------------------------------------------------------
function modmgr.dialog_configure_world()
	modmgr.precheck()
	
	local modpack_selected = false
	local gamemod_selected = modmgr.is_gamemod()
	local modname = ""
	local modfolder = ""
	local shortname = ""
	
	if not gamemod_selected then
		local worldmodidx = modmgr.get_worldmod_idx()
		modname = modmgr.global_mods[worldmodidx]

		if modname:find("<MODPACK>") ~= nil then
			modname = modname:sub(0,modname:find("<") -2)
			modpack_selected = true
		end
		
		local parts = modmgr.global_mods[worldmodidx]:split(DIR_DELIM)
		shortname = parts[#parts]
		
		modfolder = engine.get_modpath() .. DIR_DELIM .. modname
	end

	local worldspec = engine.get_worlds()[modmgr.world_config_selected_world]
	
	local retval =
		"size[11,6.5]" ..
		"label[1.5,-0.25;World: " .. worldspec.name .. "]"
		
	if modmgr.hide_gamemods then
		retval = retval .. "checkbox[5.5,6.15;cb_hide_gamemods;Hide Game;true]"
	else
		retval = retval .. "checkbox[5.5,6.15;cb_hide_gamemods;Hide Game;false]"
	end
	retval = retval ..
		"button[9.25,6.35;2,0.5;btn_config_world_save;Save]" ..
		"button[7.4,6.35;2,0.5;btn_config_world_cancel;Cancel]" ..
		"textlist[5.5,-0.25;5.5,6.5;world_config_modlist;"
		

	if not modmgr.hide_gamemods then
		retval = retval .. modmgr.render_gamemodlist()
	end
	
	retval = retval .. modmgr.render_worldmodlist()	

	retval = retval .. ";" .. modmgr.world_config_selected_mod .."]"
	
	if not gamemod_selected then
		retval = retval ..
			"label[0,0.45;Mod:]" ..
			"label[0.75,0.45;" .. modname .. "]" ..
			"label[0,1.5;depends on:]" ..
			"textlist[0,2;5,2;world_config_depends;" ..
			modmgr.get_dependencys(modfolder) .. ";0]" ..
			"label[0,4;depends on:]" ..
			"textlist[0,4.5;5,2;world_config_is_required;;0]"
	
		if modpack_selected then
			retval = retval ..
				"button[-0.05,1.05;2,0.5;btn_cfgw_enable_all;Enable All]" ..
				"button[3.25,1.05;2,0.5;btn_cfgw_disable_all;Disable All]"
		else
			retval = retval .. 
				"checkbox[0,0.8;cb_mod_enabled;enabled;"
			
			if modmgr.worldconfig.global_mods[shortname] then
				print("checkbox " .. shortname .. " enabled")
				retval = retval .. "true"
			else
				print("checkbox " .. shortname .. " disabled")
				retval = retval .. "false"
			end
			
			retval = retval .. "]"
		end
	end
	
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
function modmgr.get_dependencys(modfolder)
	local filename = modfolder ..
				DIR_DELIM .. "depends.txt"

	local dependencyfile = io.open(filename,"r")
	
	local toadd = ""
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
	else
		print(filename .. " not found")
	end

	return toadd
end


--------------------------------------------------------------------------------
function modmgr.get_worldconfig(worldpath)
	local filename = worldpath ..
				DIR_DELIM .. "world.mt"

	local worldfile = io.open(filename,"r")
	
	local worldconfig = {}
	worldconfig.global_mods = {}
	worldconfig.game_mods = {}
	
	if worldfile then
		local dependency = worldfile:read("*l")
		while dependency do
			local parts = dependency:split("=")

			local key = parts[1]:trim()

			if key == "gameid" then
				worldconfig.id = parts[2]:trim()
			else
				local key = parts[1]:trim():sub(10)
				if parts[2]:trim() == "true" then
					print("found enabled mod: >" .. key .. "<")
					worldconfig.global_mods[key] = true
				else
					print("found disabled mod: >" .. key .. "<")
					worldconfig.global_mods[key] = false
				end
			end
			dependency = worldfile:read("*l")
		end
		worldfile:close()
	else
		print(filename .. " not found")
	end
	
	--read gamemods
	local gamemodpath = engine.get_gamepath() .. DIR_DELIM .. worldconfig.id .. DIR_DELIM .. "mods"
	
	print("reading game mods from: " .. dump(gamemodpath))
	get_mods(gamemodpath,worldconfig.game_mods)

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
		local event = explode_textlist_event(fields["modlist"])
		modmgr.selected_mod = event.index
	end
	
	if fields["btn_mod_mgr_install_local"] ~= nil then
		engine.show_file_open_dialog("mod_mgt_open_dlg","Select Mod File:")
	end
	
	if fields["btn_mod_mgr_download"] ~= nil then
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
	local modfile = identify_filetype(modfilename)
	
	local modpath = modmgr.extract(modfile)
	
	if modpath == nil then
		gamedata.errormessage = "Install Mod: file: " .. modfile.name ..
			"\nInstall Mod: unsupported filetype \"" .. modfile.type .. "\""
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
			engine.copy_dir(basefolder.path,targetpath)
		else
			gamedata.errormessage = "Install Mod: unable to find suitable foldername for modpack " 
				.. modfilename
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
			gamedata.errormessage = "Install Mod: unable to find real modname for: " 
				.. modfilename
		end
	end
	
	engine.delete_dir(modpath)
end

--------------------------------------------------------------------------------
function modmgr.handle_rename_modpack_buttons(fields)
	local oldname = modmgr.global_mods[modmgr.selected_mod]
	oldname = oldname:sub(0,oldname:find("<") -2)
	
	if fields["dlg_rename_modpack_confirm"] ~= nil then
		local oldpath = engine.get_modpath() .. DIR_DELIM .. oldname
		local targetpath = engine.get_modpath() .. DIR_DELIM .. fields["te_modpack_name"]
		engine.copy_dir(oldpath,targetpath,false)
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
		local event = explode_textlist_event(fields["world_config_modlist"])
		modmgr.world_config_selected_mod = event.index
	end

	if fields["cb_mod_enabled"] ~= nil then
		local index = modmgr.get_worldmod_idx()
		local modname = modmgr.global_mods[index]
								
		local parts = modmgr.global_mods[index]:split(DIR_DELIM)
		local shortname = parts[#parts]
		
		if fields["cb_mod_enabled"] == "true" then
			modmgr.worldconfig.global_mods[shortname] = true
		else
			modmgr.worldconfig.global_mods[shortname] = false
		end
	end
	
	if fields["cb_hide_gamemods"] ~= nil then
		if fields["cb_hide_gamemods"] == "true" then
			modmgr.hide_gamemods = true
		else
			modmgr.hide_gamemods = false
		end
	end
	
	if fields["btn_config_world_save"] then
		local worldspec = engine.get_worlds()[modmgr.world_config_selected_world]
		
		local filename = worldspec.path ..
				DIR_DELIM .. "world.mt"

		local worldfile = io.open(filename,"w")
		
		if worldfile then
			worldfile:write("gameid = " .. modmgr.worldconfig.id .. "\n")
			for key,value in pairs(modmgr.worldconfig.global_mods) do
				if value then
					worldfile:write("load_mod_" .. key .. " = true" .. "\n")
				else
					worldfile:write("load_mod_" .. key .. " = false" .. "\n")
				end
			end
			
			worldfile:close()
		end
		
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
	
	if fields["btn_cfgw_enable_all"] then
		local worldmodidx = modmgr.get_worldmod_idx()
		modname = modmgr.global_mods[worldmodidx]

		modname = modname:sub(0,modname:find("<") -2)

		for i=1,#modmgr.global_mods,1 do
		
			if modmgr.global_mods[i]:find("<MODPACK>") == nil then
				local modpackpart = modmgr.global_mods[i]:sub(0,modname:len())
				
				if modpackpart == modname then
					local parts = modmgr.global_mods[i]:split(DIR_DELIM)
					local shortname = parts[#parts]
					modmgr.worldconfig.global_mods[shortname] = true
				end
			end
		end
	end
	
	if fields["btn_cfgw_disable_all"] then
		local worldmodidx = modmgr.get_worldmod_idx()
		modname = modmgr.global_mods[worldmodidx]

		modname = modname:sub(0,modname:find("<") -2)

		for i=1,#modmgr.global_mods,1 do
			local modpackpart = modmgr.global_mods[i]:sub(0,modname:len())
			
			if modpackpart == modname then
				local parts = modmgr.global_mods[i]:split(DIR_DELIM)
				local shortname = parts[#parts]
				modmgr.worldconfig.global_mods[shortname] = nil
			end
		end
	end
	
	return nil
end
--------------------------------------------------------------------------------
function modmgr.handle_delete_mod_buttons(fields)
	local modname = modmgr.global_mods[modmgr.selected_mod]
	
	if modname:find("<MODPACK>") ~= nil then
		modname = modname:sub(0,modname:find("<") -2)
	end
	
	if fields["dlg_delete_mod_confirm"] ~= nil then
		local oldpath = engine.get_modpath() .. DIR_DELIM .. modname
		
		if oldpath ~= nil and
			oldpath ~= "" and
			oldpath ~= engine.get_modpath() then
			engine.delete_dir(oldpath)
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

	local modname = modmgr.global_mods[modmgr.selected_mod]

	if modname:find("<MODPACK>") ~= nil then
		modname = modname:sub(0,modname:find("<") -2)
	end
	
	local retval = 
		"field[1.75,1;10,3;;Are you sure you want to delete ".. modname .. "?;]"..
		"button[4,4.2;1,0.5;dlg_delete_mod_confirm;Yes]" ..
		"button[6.5,4.2;3,0.5;dlg_delete_mod_cancel;No of course not!]"

	return retval
end

--------------------------------------------------------------------------------
function modmgr.init_worldconfig()

	local worldspec = engine.get_worlds()[modmgr.world_config_selected_world]
	
	if worldspec ~= nil then
		--read worldconfig
		modmgr.worldconfig = modmgr.get_worldconfig(worldspec.path)
		
		if modmgr.worldconfig.id == nil or
			modmgr.worldconfig.id == "" then
			modmgr.worldconfig = nil
			return false
		end
		
		return true	
	end

	return false
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
