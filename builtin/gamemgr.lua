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

gamemgr = {}

--------------------------------------------------------------------------------
function gamemgr.dialog_new_game()
	local retval = 
		"label[2,2;" .. fgettext("Game Name") .. "]"..
		"field[4.5,2.4;6,0.5;te_game_name;;]" ..
		"button[5,4.2;2.6,0.5;new_game_confirm;" .. fgettext("Create") .. "]" ..
		"button[7.5,4.2;2.8,0.5;new_game_cancel;" .. fgettext("Cancel") .. "]"

	return retval
end

--------------------------------------------------------------------------------
function gamemgr.handle_games_buttons(fields)
	if fields["gamelist"] ~= nil then
		local event = engine.explode_textlist_event(fields["gamelist"])
		gamemgr.selected_game = event.index
	end
	
	if fields["btn_game_mgr_edit_game"] ~= nil then
		return {
			is_dialog = true,
			show_buttons = false,
			current_tab = "dialog_edit_game"
		}
	end
	
	if fields["btn_game_mgr_new_game"] ~= nil then
		return {
			is_dialog = true,
			show_buttons = false,
			current_tab = "dialog_new_game"
		}
	end
	
	return nil
end

--------------------------------------------------------------------------------
function gamemgr.handle_new_game_buttons(fields)

	if fields["new_game_confirm"] and
		fields["te_game_name"] ~= nil and
		fields["te_game_name"] ~= "" then
		local gamepath = engine.get_gamepath()
		
		if gamepath ~= nil and
			gamepath ~= "" then
			local gamefolder = cleanup_path(fields["te_game_name"])
			
			--TODO check for already existing first
			engine.create_dir(gamepath .. DIR_DELIM .. gamefolder)
			engine.create_dir(gamepath .. DIR_DELIM .. gamefolder .. DIR_DELIM .. "mods")
			engine.create_dir(gamepath .. DIR_DELIM .. gamefolder .. DIR_DELIM .. "menu")
			
			local gameconf = 
				io.open(gamepath .. DIR_DELIM .. gamefolder .. DIR_DELIM .. "game.conf","w")
			
			if gameconf then
				gameconf:write("name = " .. fields["te_game_name"])
				gameconf:close()
			end
		end
	end

	return {
		is_dialog = false,
		show_buttons = true,
		current_tab = engine.setting_get("main_menu_tab")
		}
end

--------------------------------------------------------------------------------
function gamemgr.handle_edit_game_buttons(fields)
	local current_game = gamemgr.get_game(gamemgr.selected_game)
	
	if fields["btn_close_edit_game"] ~= nil or
		current_game == nil then
		return {
			is_dialog = false,
			show_buttons = true,
			current_tab = engine.setting_get("main_menu_tab")
			}
	end

	if fields["btn_remove_mod_from_game"] ~= nil then
		gamemgr.delete_mod(current_game,engine.get_textlist_index("mods_current"))
	end
	
	if fields["btn_add_mod_to_game"] ~= nil then
		local modindex = engine.get_textlist_index("mods_available")
		
		local mod = modmgr.get_global_mod(modindex)
		if mod ~= nil then
			
			local sourcepath = mod.path
			
			if not gamemgr.add_mod(current_game,sourcepath) then
				gamedata.errormessage =
					fgettext("Gamemgr: Unable to copy mod \"$1\" to game \"$2\"", mod.name, current_game.id)
			end
		end
	end
	
	return nil
end

--------------------------------------------------------------------------------
function gamemgr.add_mod(gamespec,sourcepath)
	if gamespec.gamemods_path ~= nil and
		gamespec.gamemods_path ~= "" then
		
		local modname = get_last_folder(sourcepath)
		
		return engine.copy_dir(sourcepath,gamespec.gamemods_path .. DIR_DELIM .. modname);
	end
	
	return false
end

--------------------------------------------------------------------------------
function gamemgr.delete_mod(gamespec,modindex)
	if gamespec.gamemods_path ~= nil and
		gamespec.gamemods_path ~= "" then
		local game_mods = {}
		get_mods(gamespec.gamemods_path,game_mods)
		
		if modindex > 0 and
			#game_mods >= modindex then

			if game_mods[modindex].path:sub(0,gamespec.gamemods_path:len()) 
					== gamespec.gamemods_path then
				engine.delete_dir(game_mods[modindex].path)
			end
		end
	end
end

--------------------------------------------------------------------------------
function gamemgr.find_by_gameid(gameid)
	for i=1,#gamemgr.games,1 do		
		if gamemgr.games[i].id == gameid then
			return gamemgr.games[i], i
		end
	end
	return nil, nil
end

--------------------------------------------------------------------------------
function gamemgr.get_game_mods(gamespec, retval)
	if gamespec ~= nil and
		gamespec.gamemods_path ~= nil and
		gamespec.gamemods_path ~= "" then
		get_mods(gamespec.gamemods_path, retval)
	end
end

--------------------------------------------------------------------------------
function gamemgr.get_game_modlist(gamespec)
	local retval = ""
	local game_mods = {}
	gamemgr.get_game_mods(gamespec, game_mods)
	for i=1,#game_mods,1 do
		if retval ~= "" then
			retval = retval..","
		end
		retval = retval .. game_mods[i].name
	end 
	return retval
end

--------------------------------------------------------------------------------
function gamemgr.gettab(name)
	local retval = ""
	
	if name == "dialog_edit_game" then
		retval = retval .. gamemgr.dialog_edit_game()
	end
	
	if name == "dialog_new_game" then
		retval = retval .. gamemgr.dialog_new_game()
	end
	
	if name == "game_mgr" then
		retval = retval .. gamemgr.tab()
	end
	
	return retval
end

--------------------------------------------------------------------------------
function gamemgr.tab()
	if gamemgr.selected_game == nil then
		gamemgr.selected_game = 1
	end
	
	local retval = 
		"vertlabel[0,-0.25;" .. fgettext("GAMES") .. "]" ..
		"label[1,-0.25;" .. fgettext("Games") .. ":]" ..
		"textlist[1,0.25;4.5,4.4;gamelist;" ..
		gamemgr.gamelist() ..
		";" .. gamemgr.selected_game .. "]"
	
	local current_game = gamemgr.get_game(gamemgr.selected_game)
	
	if current_game ~= nil then
		if current_game.menuicon_path ~= nil and
			current_game.menuicon_path ~= "" then
			retval = retval .. 
				"image[5.8,-0.25;2,2;" ..
				engine.formspec_escape(current_game.menuicon_path) .. "]"
		end
		
		retval = retval ..
			"field[8,-0.25;6,2;;" .. current_game.name .. ";]"..
			"label[6,1.4;" .. fgettext("Mods:") .."]" ..
			"button[9.7,1.5;2,0.2;btn_game_mgr_edit_game;" .. fgettext("edit game") .. "]" ..
			"textlist[6,2;5.5,3.3;game_mgr_modlist;"
			.. gamemgr.get_game_modlist(current_game) ..";0]" ..
			"button[1,4.75;3.2,0.5;btn_game_mgr_new_game;" .. fgettext("new game") .. "]"
	end
	return retval
end

--------------------------------------------------------------------------------
function gamemgr.dialog_edit_game()
	local current_game = gamemgr.get_game(gamemgr.selected_game)
	if current_game ~= nil then
		local retval = 
			"vertlabel[0,-0.25;" .. fgettext("EDIT GAME") .."]" ..
			"label[0,-0.25;" .. current_game.name .. "]" ..
			"button[11.55,-0.2;0.75,0.5;btn_close_edit_game;x]"
		
		if current_game.menuicon_path ~= nil and
			current_game.menuicon_path ~= "" then
			retval = retval .. 
				"image[5.25,0;2,2;" ..
				engine.formspec_escape(current_game.menuicon_path) .. "]"
		end
		
		retval = retval .. 
			"textlist[0.5,0.5;4.5,4.3;mods_current;"
			.. gamemgr.get_game_modlist(current_game) ..";0]"
			
			
		retval = retval .. 
			"textlist[7,0.5;4.5,4.3;mods_available;"
			.. modmgr.render_modlist() .. ";0]"

		retval = retval ..
			"button[0.55,4.95;4.7,0.5;btn_remove_mod_from_game;" .. fgettext("Remove selected mod") .."]"
			
		retval = retval ..
			"button[7.05,4.95;4.7,0.5;btn_add_mod_to_game;" .. fgettext("<<-- Add mod") .."]"
		
		return retval
	end
end

--------------------------------------------------------------------------------
function gamemgr.handle_buttons(tab,fields)
	local retval = nil
	
	if tab == "dialog_edit_game" then
		retval = gamemgr.handle_edit_game_buttons(fields)
	end
	
	if tab == "dialog_new_game" then
		retval = gamemgr.handle_new_game_buttons(fields)
	end
	
	if tab == "game_mgr" then
		retval = gamemgr.handle_games_buttons(fields)
	end
	
	return retval
end

--------------------------------------------------------------------------------
function gamemgr.get_game(index) 
	if index > 0 and index <= #gamemgr.games then
		return gamemgr.games[index]
	end
	
	return nil
end

--------------------------------------------------------------------------------
function gamemgr.update_gamelist()
	gamemgr.games = engine.get_games()
end

--------------------------------------------------------------------------------
function gamemgr.gamelist()
	local retval = ""
	if #gamemgr.games > 0 then
		retval = retval .. gamemgr.games[1].id
		
		for i=2,#gamemgr.games,1 do
			retval = retval .. "," .. gamemgr.games[i].name
		end
	end
	return retval
end
