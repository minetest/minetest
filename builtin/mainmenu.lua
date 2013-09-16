print = engine.debug
math.randomseed(os.time())
os.setlocale("C", "numeric")

local errorfct = error
error = function(text)
	print(debug.traceback(""))
	errorfct(text)
end

local scriptpath = engine.get_scriptdir()

mt_color_grey  = "#AAAAAA"
mt_color_blue  = "#0000DD"
mt_color_green = "#00DD00"
mt_color_dark_green = "#003300"

--for all other colors ask sfan5 to complete his worK!

dofile(scriptpath .. DIR_DELIM .. "misc_helpers.lua")
dofile(scriptpath .. DIR_DELIM .. "filterlist.lua")
dofile(scriptpath .. DIR_DELIM .. "modmgr.lua")
dofile(scriptpath .. DIR_DELIM .. "modstore.lua")
dofile(scriptpath .. DIR_DELIM .. "gamemgr.lua")
dofile(scriptpath .. DIR_DELIM .. "mm_textures.lua")
dofile(scriptpath .. DIR_DELIM .. "mm_menubar.lua")

menu = {}
local tabbuilder = {}
local worldlist = nil

--------------------------------------------------------------------------------
local function filter_texture_pack_list(list)
	retval = {"None"}
	for _,i in ipairs(list) do
		if i~="base" then
			table.insert(retval, i)
		end
	end
	return retval
end

--------------------------------------------------------------------------------
function menu.render_favorite(spec,render_details)
	local text = ""
	
	if spec.name ~= nil then
		text = text .. engine.formspec_escape(spec.name:trim())
		
--		if spec.description ~= nil and
--			engine.formspec_escape(spec.description):trim() ~= "" then
--			text = text .. " (" .. engine.formspec_escape(spec.description) .. ")"
--		end
	else
		if spec.address ~= nil then
			text = text .. spec.address:trim()
			
			if spec.port ~= nil then
				text = text .. ":" .. spec.port
			end
		end
	end
	
	if not render_details then
		return text
	end
	
	local details = ""
	if spec.password == true then
		details = details .. "*"
	else
		details = details .. "_"
	end
	
	if spec.creative then
		details = details .. "C"
	else
		details = details .. "_"
	end
	
	if spec.damage then
		details = details .. "D"
	else
		details = details .. "_"
	end
	
	if spec.pvp then
		details = details .. "P"
	else
		details = details .. "_"
	end
	details = details .. " "
	
	local playercount = ""
	
	if spec.clients ~= nil and
		spec.clients_max ~= nil then
		playercount = string.format("%03d",spec.clients) .. "/" ..
						string.format("%03d",spec.clients_max) .. " "
	end
	
	return playercount .. engine.formspec_escape(details) ..  text
end

--------------------------------------------------------------------------------
os.tempfolder = function()
	local filetocheck = os.tmpname()
	os.remove(filetocheck)
	
	local randname = "MTTempModFolder_" .. math.random(0,10000)
	if DIR_DELIM == "\\" then
		local tempfolder = os.getenv("TEMP")
		return tempfolder .. filetocheck
	else
		local backstring = filetocheck:reverse()
		return filetocheck:sub(0,filetocheck:len()-backstring:find(DIR_DELIM)+1) ..randname
	end

end

--------------------------------------------------------------------------------
function init_globals()
	--init gamedata
	gamedata.worldindex = 0
	
	worldlist = filterlist.create(
					engine.get_worlds,
					compare_worlds,
					function(element,uid)
						if element.name == uid then
							return true
						end
						return false
					end, --unique id compare fct
					function(element,gameid)
						if element.gameid == gameid then
							return true
						end
						return false
					end --filter fct
					)
					
	filterlist.add_sort_mechanism(worldlist,"alphabetic",sort_worlds_alphabetic)
	filterlist.set_sortmode(worldlist,"alphabetic")
					
end

--------------------------------------------------------------------------------
function update_menu()

	local formspec = "size[12,5.2]"
	
	-- handle errors
	if gamedata.errormessage ~= nil then
		formspec = formspec ..
			"field[1,2;10,2;;ERROR: " ..
			gamedata.errormessage .. 
			";]"..
			"button[4.5,4.2;3,0.5;btn_error_confirm;" .. fgettext("Ok") .. "]"
	else
		formspec = formspec .. tabbuilder.gettab()
	end

	engine.update_formspec(formspec)
end

--------------------------------------------------------------------------------
function menu.render_world_list()
	local retval = ""
	
	local current_worldlist = filterlist.get_list(worldlist)
	
	for i,v in ipairs(current_worldlist) do
		if retval ~= "" then
			retval = retval ..","
		end
		
		retval = retval .. engine.formspec_escape(v.name) ..
					" \\[" .. engine.formspec_escape(v.gameid) .. "\\]"
	end

	return retval
end

--------------------------------------------------------------------------------
function menu.render_texture_pack_list(list)
	local retval = ""

	for i,v in ipairs(list) do
		if retval ~= "" then
			retval = retval ..","
		end

		retval = retval .. v
	end

	return retval
end

--------------------------------------------------------------------------------
function menu.init()
	--init menu data
	gamemgr.update_gamelist()
	
	menu.last_game	= tonumber(engine.setting_get("main_menu_last_game_idx"))
	
	if type(menu.last_game) ~= "number" then
		menu.last_game = 1
	end

	if engine.setting_getbool("public_serverlist") then
		menu.favorites = engine.get_favorites("online")
	else
		menu.favorites = engine.get_favorites("local")
	end
	
	menu.defaulttexturedir = engine.get_texturepath() .. DIR_DELIM .. "base" .. 
					DIR_DELIM .. "pack" .. DIR_DELIM
end

--------------------------------------------------------------------------------
function menu.lastgame()
	if menu.last_game > 0 and menu.last_game <= #gamemgr.games then
		return gamemgr.games[menu.last_game]
	end
	
	if #gamemgr.games >= 1 then
		menu.last_game = 1
		return gamemgr.games[menu.last_game]
	end
	
	--error case!!
	return nil
end

--------------------------------------------------------------------------------
function menu.update_last_game()

	local current_world = filterlist.get_raw_element(worldlist,
							engine.setting_get("mainmenu_last_selected_world")
							)
							
	if current_world == nil then
		return
	end
	
	local gamespec, i = gamemgr.find_by_gameid(current_world.gameid)
	if i ~= nil then
		menu.last_game = i
		engine.setting_set("main_menu_last_game_idx",menu.last_game)
	end
end

--------------------------------------------------------------------------------
function menu.handle_key_up_down(fields,textlist,settingname)

	if fields["key_up"] then
		local oldidx = engine.get_textlist_index(textlist)
		
		if oldidx > 1 then
			local newidx = oldidx -1
			engine.setting_set(settingname,
				filterlist.get_raw_index(worldlist,newidx))
		end
	end
	
	if fields["key_down"] then
		local oldidx = engine.get_textlist_index(textlist)
		
		if oldidx < filterlist.size(worldlist) then
			local newidx = oldidx + 1
			engine.setting_set(settingname,
				filterlist.get_raw_index(worldlist,newidx))
		end
	end
end

--------------------------------------------------------------------------------
function tabbuilder.dialog_create_world()
	local mapgens = {"v6", "v7", "indev", "singlenode", "math"}

	local current_mg = engine.setting_get("mg_name")

	local mglist = ""
	local selindex = 1
	local i = 1
	for k,v in pairs(mapgens) do
		if current_mg == v then
			selindex = i
		end
		i = i + 1
		mglist = mglist .. v .. ","
	end
	mglist = mglist:sub(1, -2)

	local retval = 
		"label[2,0;" .. fgettext("World name") .. "]"..
		"label[2,1;" .. fgettext("Mapgen") .. "]"..
		"field[4.5,0.4;6,0.5;te_world_name;;]" ..
		"label[2,2;" .. fgettext("Game") .. "]"..
		"button[5,4.5;2.6,0.5;world_create_confirm;" .. fgettext("Create") .. "]" ..
		"button[7.5,4.5;2.8,0.5;world_create_cancel;" .. fgettext("Cancel") .. "]" ..
		"dropdown[4.2,1;6.3;dd_mapgen;" .. mglist .. ";" .. selindex .. "]" ..
		"textlist[4.2,1.9;5.8,2.3;games;" ..
		gamemgr.gamelist() ..
		";" .. menu.last_game .. ";true]"

	return retval
end

--------------------------------------------------------------------------------
function tabbuilder.dialog_delete_world()
	return	"label[2,2;" ..
			fgettext("Delete World \"$1\"?", filterlist.get_raw_list(worldlist)[menu.world_to_del].name) .. "]"..
			"button[3.5,4.2;2.6,0.5;world_delete_confirm;" .. fgettext("Yes").. "]" ..
			"button[6,4.2;2.8,0.5;world_delete_cancel;" .. fgettext("No") .. "]"
end

--------------------------------------------------------------------------------
function tabbuilder.gettab()
	local retval = ""
	
	if tabbuilder.show_buttons then
		retval = retval .. tabbuilder.tab_header()
	end

	if tabbuilder.current_tab == "singleplayer" then
		retval = retval .. tabbuilder.tab_singleplayer()
	end
	
	if tabbuilder.current_tab == "multiplayer" then
		retval = retval .. tabbuilder.tab_multiplayer()
	end

	if tabbuilder.current_tab == "server" then
		retval = retval .. tabbuilder.tab_server()
	end
	
	if tabbuilder.current_tab == "settings" then
		retval = retval .. tabbuilder.tab_settings()
	end
	
	if tabbuilder.current_tab == "texture_packs" then
		retval = retval .. tabbuilder.tab_texture_packs()
	end
	
	if tabbuilder.current_tab == "credits" then
		retval = retval .. tabbuilder.tab_credits()
	end
	
	if tabbuilder.current_tab == "dialog_create_world" then
		retval = retval .. tabbuilder.dialog_create_world()
	end
	
	if tabbuilder.current_tab == "dialog_delete_world" then
		retval = retval .. tabbuilder.dialog_delete_world()
	end
	
	retval = retval .. modmgr.gettab(tabbuilder.current_tab)
	retval = retval .. gamemgr.gettab(tabbuilder.current_tab)
	retval = retval .. modstore.gettab(tabbuilder.current_tab)

	return retval
end

--------------------------------------------------------------------------------
function tabbuilder.handle_create_world_buttons(fields)
	
	if fields["world_create_confirm"] or
		fields["key_enter"] then
		
		local worldname = fields["te_world_name"]
		local gameindex = engine.get_textlist_index("games")
		
		if gameindex > 0 and
			worldname ~= "" then
			
			local message = nil
			
			if not filterlist.uid_exists_raw(worldlist,worldname) then
				engine.setting_set("mg_name",fields["dd_mapgen"])
				message = engine.create_world(worldname,gameindex)
			else
				message = fgettext("A world named \"$1\" already exists", worldname)
			end
			
			if message ~= nil then
				gamedata.errormessage = message
			else
				menu.last_game = gameindex
				engine.setting_set("main_menu_last_game_idx",gameindex)
				
				filterlist.refresh(worldlist)
				engine.setting_set("mainmenu_last_selected_world",
									filterlist.raw_index_by_uid(worldlist,worldname))
			end
		else
			gamedata.errormessage = 
				fgettext("No worldname given or no game selected")
		end
	end
	
	if fields["games"] then
		tabbuilder.skipformupdate = true
		return
	end
	
	--close dialog
	tabbuilder.is_dialog = false
	tabbuilder.show_buttons = true
	tabbuilder.current_tab = engine.setting_get("main_menu_tab")
end

--------------------------------------------------------------------------------
function tabbuilder.handle_delete_world_buttons(fields)
	
	if fields["world_delete_confirm"] then
		if menu.world_to_del > 0 and 
			menu.world_to_del <= #filterlist.get_raw_list(worldlist) then
			engine.delete_world(menu.world_to_del)
			menu.world_to_del = 0
			filterlist.refresh(worldlist)
		end
	end
	
	tabbuilder.is_dialog = false
	tabbuilder.show_buttons = true
	tabbuilder.current_tab = engine.setting_get("main_menu_tab")
end

--------------------------------------------------------------------------------
function tabbuilder.handle_multiplayer_buttons(fields)
	
	if fields["te_name"] ~= nil then
		gamedata.playername = fields["te_name"]
		engine.setting_set("name", fields["te_name"])
	end
	
	if fields["favourites"] ~= nil then
		local event = explode_textlist_event(fields["favourites"])
		if event.typ == "DCL" then
			gamedata.address = menu.favorites[event.index].address
			gamedata.port = menu.favorites[event.index].port
			gamedata.playername		= fields["te_name"]
			if fields["te_pwd"] ~= nil then
				gamedata.password		= fields["te_pwd"]
			end
			gamedata.selected_world = 0
			
			if menu.favorites ~= nil then
				gamedata.servername = menu.favorites[event.index].name
				gamedata.serverdescription = menu.favorites[event.index].description
			end
			
			if gamedata.address ~= nil and
				gamedata.port ~= nil then
				
				engine.start()
			end
		end
		
		if event.typ == "CHG" then
			local address = menu.favorites[event.index].address
			local port = menu.favorites[event.index].port
			
			if address ~= nil and
				port ~= nil then
				engine.setting_set("address",address)
				engine.setting_set("port",port)
			end
			
			menu.fav_selected = event.index
		end
		return
	end
	
	if fields["key_up"] ~= nil or
		fields["key_down"] ~= nil then
		
		local fav_idx = engine.get_textlist_index("favourites")
		
		if fields["key_up"] ~= nil and fav_idx > 1 then
			fav_idx = fav_idx -1
		else if fields["key_down"] and fav_idx < #menu.favorites then
			fav_idx = fav_idx +1
		end end
		
		local address = menu.favorites[fav_idx].address
		local port = menu.favorites[fav_idx].port
		
		if address ~= nil and
			port ~= nil then
			engine.setting_set("address",address)
			engine.setting_set("port",port)
		end
		
		menu.fav_selected = fav_idx
		return
	end
	
	if fields["cb_public_serverlist"] ~= nil then
		engine.setting_set("public_serverlist", fields["cb_public_serverlist"])
			
		if engine.setting_getbool("public_serverlist") then
			menu.favorites = engine.get_favorites("online")
		else
			menu.favorites = engine.get_favorites("local")
		end
		menu.fav_selected = nil
		return
	end

	if fields["btn_delete_favorite"] ~= nil then
		local current_favourite = engine.get_textlist_index("favourites")
		engine.delete_favorite(current_favourite)
		menu.favorites = engine.get_favorites()
		menu.fav_selected = nil
		
		engine.setting_set("address","")
		engine.setting_get("port","")
		
		return
	end

	if fields["btn_mp_connect"] ~= nil or
		fields["key_enter"] then
		
		gamedata.playername		= fields["te_name"]
		gamedata.password		= fields["te_pwd"]
		gamedata.address		= fields["te_address"]
		gamedata.port			= fields["te_port"]
		
		local fav_idx = engine.get_textlist_index("favourites")
		
		if fav_idx > 0 and fav_idx <= #menu.favorites and
			menu.favorites[fav_idx].address == fields["te_address"] and
			menu.favorites[fav_idx].port == fields["te_port"] then
			
			gamedata.servername			= menu.favorites[fav_idx].name
			gamedata.serverdescription	= menu.favorites[fav_idx].description
		else
			gamedata.servername = ""
			gamedata.serverdescription = ""
		end

		gamedata.selected_world = 0
		
		engine.start()
		return
	end
end

--------------------------------------------------------------------------------
function tabbuilder.handle_server_buttons(fields)

	local world_doubleclick = false

	if fields["srv_worlds"] ~= nil then
		local event = explode_textlist_event(fields["srv_worlds"])
		
		if event.typ == "DCL" then
			world_doubleclick = true
		end
		if event.typ == "CHG" then
			engine.setting_set("mainmenu_last_selected_world",
				filterlist.get_raw_index(worldlist,engine.get_textlist_index("srv_worlds")))
		end
	end
	
	menu.handle_key_up_down(fields,"srv_worlds","mainmenu_last_selected_world")
	
	if fields["cb_creative_mode"] then
		engine.setting_set("creative_mode", fields["cb_creative_mode"])
	end
	
	if fields["cb_enable_damage"] then
		engine.setting_set("enable_damage", fields["cb_enable_damage"])
	end

	if fields["cb_server_announce"] then
		engine.setting_set("server_announce", fields["cb_server_announce"])
	end
	
	if fields["start_server"] ~= nil or
		world_doubleclick or
		fields["key_enter"] then
		local selected = engine.get_textlist_index("srv_worlds")
		if selected > 0 then
			gamedata.playername		= fields["te_playername"]
			gamedata.password		= fields["te_passwd"]
			gamedata.port			= fields["te_serverport"]
			gamedata.address		= ""
			gamedata.selected_world	= filterlist.get_raw_index(worldlist,selected)
			
			menu.update_last_game(gamedata.selected_world)
			engine.start()
		end
	end
	
	if fields["world_create"] ~= nil then
		tabbuilder.current_tab = "dialog_create_world"
		tabbuilder.is_dialog = true
		tabbuilder.show_buttons = false
	end
	
	if fields["world_delete"] ~= nil then
		local selected = engine.get_textlist_index("srv_worlds")
		if selected > 0 and
			selected <= filterlist.size(worldlist) then
			local world = filterlist.get_list(worldlist)[selected]
			if world ~= nil and
				world.name ~= nil and
				world.name ~= "" then
				menu.world_to_del = filterlist.get_raw_index(worldlist,selected)
				tabbuilder.current_tab = "dialog_delete_world"
				tabbuilder.is_dialog = true
				tabbuilder.show_buttons = false
			else
				menu.world_to_del = 0
			end
		end
	end
	
	if fields["world_configure"] ~= nil then
		selected = engine.get_textlist_index("srv_worlds")
		if selected > 0 then
			modmgr.world_config_selected_world = filterlist.get_raw_index(worldlist,selected)
			if modmgr.init_worldconfig() then
				tabbuilder.current_tab = "dialog_configure_world"
				tabbuilder.is_dialog = true
				tabbuilder.show_buttons = false
			end
		end
	end
end

--------------------------------------------------------------------------------
function tabbuilder.handle_settings_buttons(fields)
	if fields["cb_fancy_trees"] then
		engine.setting_set("new_style_leaves", fields["cb_fancy_trees"])
	end
	if fields["cb_smooth_lighting"] then
		engine.setting_set("smooth_lighting", fields["cb_smooth_lighting"])
	end
	if fields["cb_3d_clouds"] then
		engine.setting_set("enable_3d_clouds", fields["cb_3d_clouds"])
	end
	if fields["cb_opaque_water"] then
		engine.setting_set("opaque_water", fields["cb_opaque_water"])
	end
	
	if fields["cb_mipmapping"] then
		engine.setting_set("mip_map", fields["cb_mipmapping"])
	end
	if fields["cb_anisotrophic"] then
		engine.setting_set("anisotropic_filter", fields["cb_anisotrophic"])
	end
	if fields["cb_bilinear"] then
		engine.setting_set("bilinear_filter", fields["cb_bilinear"])
	end
	if fields["cb_trilinear"] then
		engine.setting_set("trilinear_filter", fields["cb_trilinear"])
	end
			
	if fields["cb_shaders"] then
		engine.setting_set("enable_shaders", fields["cb_shaders"])
	end
	if fields["cb_pre_ivis"] then
		engine.setting_set("preload_item_visuals", fields["cb_pre_ivis"])
	end
	if fields["cb_particles"] then
		engine.setting_set("enable_particles", fields["cb_particles"])
	end
	if fields["cb_finite_liquid"] then
		engine.setting_set("liquid_finite", fields["cb_finite_liquid"])
	end

	if fields["btn_change_keys"] ~= nil then
		engine.show_keys_menu()
	end
end

--------------------------------------------------------------------------------
function tabbuilder.handle_singleplayer_buttons(fields)

	local world_doubleclick = false

	if fields["sp_worlds"] ~= nil then
		local event = explode_textlist_event(fields["sp_worlds"])
		
		if event.typ == "DCL" then
			world_doubleclick = true
		end
		
		if event.typ == "CHG" then
			engine.setting_set("mainmenu_last_selected_world",
				filterlist.get_raw_index(worldlist,engine.get_textlist_index("sp_worlds")))
		end
	end
	
	menu.handle_key_up_down(fields,"sp_worlds","mainmenu_last_selected_world")
	
	if fields["cb_creative_mode"] then
		engine.setting_set("creative_mode", fields["cb_creative_mode"])
	end
	
	if fields["cb_enable_damage"] then
		engine.setting_set("enable_damage", fields["cb_enable_damage"])
	end

	if fields["play"] ~= nil or
		world_doubleclick or
		fields["key_enter"] then
		local selected = engine.get_textlist_index("sp_worlds")
		if selected > 0 then
			gamedata.selected_world	= filterlist.get_raw_index(worldlist,selected)
			gamedata.singleplayer	= true
			
			menu.update_last_game(gamedata.selected_world)
			
			engine.start()
		end
	end
	
	if fields["world_create"] ~= nil then
		tabbuilder.current_tab = "dialog_create_world"
		tabbuilder.is_dialog = true
		tabbuilder.show_buttons = false
	end
	
	if fields["world_delete"] ~= nil then
		local selected = engine.get_textlist_index("sp_worlds")
		if selected > 0 and
			selected <= filterlist.size(worldlist) then
			local world = filterlist.get_list(worldlist)[selected]
			if world ~= nil and
				world.name ~= nil and
				world.name ~= "" then
				menu.world_to_del = filterlist.get_raw_index(worldlist,selected)
				tabbuilder.current_tab = "dialog_delete_world"
				tabbuilder.is_dialog = true
				tabbuilder.show_buttons = false
			else
				menu.world_to_del = 0
			end
		end
	end
	
	if fields["world_configure"] ~= nil then
		selected = engine.get_textlist_index("sp_worlds")
		if selected > 0 then
			modmgr.world_config_selected_world = filterlist.get_raw_index(worldlist,selected)
			if modmgr.init_worldconfig() then
				tabbuilder.current_tab = "dialog_configure_world"
				tabbuilder.is_dialog = true
				tabbuilder.show_buttons = false
			end
		end
	end
end

--------------------------------------------------------------------------------
function tabbuilder.handle_texture_pack_buttons(fields)
	if fields["TPs"] ~= nil then
		local event = explode_textlist_event(fields["TPs"])
		if event.typ == "CHG" or event.typ=="DCL" then
			local index = engine.get_textlist_index("TPs")
			engine.setting_set("mainmenu_last_selected_TP",
				index)
			local list = filter_texture_pack_list(engine.get_dirlist(engine.get_texturepath(), true))
			local current_index = engine.get_textlist_index("TPs")
			if #list >= current_index then
				local new_path = engine.get_texturepath()..DIR_DELIM..list[current_index]
				if list[current_index] == "None" then new_path = "" end
				
				engine.setting_set("texture_path", new_path)
			end
		end
	end
end

--------------------------------------------------------------------------------
function tabbuilder.tab_header()

	if tabbuilder.last_tab_index == nil then
		tabbuilder.last_tab_index = 1
	end
	
	local toadd = ""
	
	for i=1,#tabbuilder.current_buttons,1 do
		
		if toadd ~= "" then
			toadd = toadd .. ","
		end
		
		toadd = toadd .. tabbuilder.current_buttons[i].caption
	end
	return "tabheader[-0.3,-0.99;main_tab;" .. toadd ..";" .. tabbuilder.last_tab_index .. ";true;false]"
end

--------------------------------------------------------------------------------
function tabbuilder.handle_tab_buttons(fields)

	if fields["main_tab"] then
		local index = tonumber(fields["main_tab"])
		tabbuilder.last_tab_index = index
		tabbuilder.current_tab = tabbuilder.current_buttons[index].name
		
		engine.setting_set("main_menu_tab",tabbuilder.current_tab)
	end
	
	--handle tab changes
	if tabbuilder.current_tab ~= tabbuilder.old_tab then
		if tabbuilder.current_tab ~= "singleplayer" then
			menu.update_gametype(true)
		end
	end
	
	if tabbuilder.current_tab == "singleplayer" then
		menu.update_gametype()
	end
	
	tabbuilder.old_tab = tabbuilder.current_tab
end

--------------------------------------------------------------------------------
function tabbuilder.init()
	tabbuilder.current_tab = engine.setting_get("main_menu_tab")
	
	if tabbuilder.current_tab == nil or
		tabbuilder.current_tab == "" then
		tabbuilder.current_tab = "singleplayer"
		engine.setting_set("main_menu_tab",tabbuilder.current_tab)
	end
	
	--initialize tab buttons
	tabbuilder.last_tab = nil
	tabbuilder.show_buttons = true
	
	tabbuilder.current_buttons = {}
	table.insert(tabbuilder.current_buttons,{name="singleplayer", caption=fgettext("Singleplayer")})
	table.insert(tabbuilder.current_buttons,{name="multiplayer", caption=fgettext("Client")})
	table.insert(tabbuilder.current_buttons,{name="server", caption=fgettext("Server")})
	table.insert(tabbuilder.current_buttons,{name="settings", caption=fgettext("Settings")})
	table.insert(tabbuilder.current_buttons,{name="texture_packs", caption=fgettext("Texture Packs")})
	
	if engine.setting_getbool("main_menu_game_mgr") then
		table.insert(tabbuilder.current_buttons,{name="game_mgr", caption=fgettext("Games")})
	end
	
	if engine.setting_getbool("main_menu_mod_mgr") then
		table.insert(tabbuilder.current_buttons,{name="mod_mgr", caption=fgettext("Mods")})
	end
	table.insert(tabbuilder.current_buttons,{name="credits", caption=fgettext("Credits")})
	
	
	for i=1,#tabbuilder.current_buttons,1 do
		if tabbuilder.current_buttons[i].name == tabbuilder.current_tab then
			tabbuilder.last_tab_index = i
		end
	end
	
	if tabbuilder.current_tab ~= "singleplayer" then
		menu.update_gametype(true)
	else
		menu.update_gametype()
	end
end

--------------------------------------------------------------------------------
function tabbuilder.tab_multiplayer()

	local retval =
		"vertlabel[0,-0.25;".. fgettext("CLIENT") .. "]" ..
		"label[1,-0.25;".. fgettext("Favorites:") .. "]"..
		"label[1,4.25;".. fgettext("Address/Port") .. "]"..
		"label[9,2.75;".. fgettext("Name/Password") .. "]" ..
		"field[1.25,5.25;5.5,0.5;te_address;;" ..engine.setting_get("address") .."]" ..
		"field[6.75,5.25;2.25,0.5;te_port;;" ..engine.setting_get("port") .."]" ..
		"checkbox[1,3.6;cb_public_serverlist;".. fgettext("Public Serverlist") .. ";" ..
		dump(engine.setting_getbool("public_serverlist")) .. "]"
		
	if not engine.setting_getbool("public_serverlist") then
		retval = retval .. 
		"button[6.45,3.95;2.25,0.5;btn_delete_favorite;".. fgettext("Delete") .. "]"
	end
	
	retval = retval ..
		"button[9,4.95;2.5,0.5;btn_mp_connect;".. fgettext("Connect") .. "]" ..
		"field[9.3,3.75;2.5,0.5;te_name;;" ..engine.setting_get("name") .."]" ..
		"pwdfield[9.3,4.5;2.5,0.5;te_pwd;]" ..
		"textarea[9.3,0.25;2.5,2.75;;"
	if menu.fav_selected ~= nil and 
		menu.favorites[menu.fav_selected].description ~= nil then
		retval = retval .. 
			engine.formspec_escape(menu.favorites[menu.fav_selected].description,true)
	end
	
	retval = retval .. 
		";]" ..
		"textlist[1,0.35;7.5,3.35;favourites;"

	local render_details = engine.setting_getbool("public_serverlist")

	if #menu.favorites > 0 then
		retval = retval .. menu.render_favorite(menu.favorites[1],render_details)
		
		for i=2,#menu.favorites,1 do
			retval = retval .. "," .. menu.render_favorite(menu.favorites[i],render_details)
		end
	end

	if menu.fav_selected ~= nil then
		retval = retval .. ";" .. menu.fav_selected .. "]"
	else
		retval = retval .. ";0]"
	end

	return retval
end

--------------------------------------------------------------------------------
function tabbuilder.tab_server()

	local index = filterlist.get_current_index(worldlist,
				tonumber(engine.setting_get("mainmenu_last_selected_world"))
				)
	
	local retval = 
		"button[4,4.15;2.6,0.5;world_delete;".. fgettext("Delete") .. "]" ..
		"button[6.5,4.15;2.8,0.5;world_create;".. fgettext("New") .. "]" ..
		"button[9.2,4.15;2.55,0.5;world_configure;".. fgettext("Configure") .. "]" ..
		"button[8.5,4.9;3.25,0.5;start_server;".. fgettext("Start Game") .. "]" ..
		"label[4,-0.25;".. fgettext("Select World:") .. "]"..
		"vertlabel[0,-0.25;".. fgettext("START SERVER") .. "]" ..
		"checkbox[0.5,0.25;cb_creative_mode;".. fgettext("Creative Mode") .. ";" ..
		dump(engine.setting_getbool("creative_mode")) .. "]"..
		"checkbox[0.5,0.7;cb_enable_damage;".. fgettext("Enable Damage") .. ";" ..
		dump(engine.setting_getbool("enable_damage")) .. "]"..
		"checkbox[0.5,1.15;cb_server_announce;".. fgettext("Public") .. ";" ..
		dump(engine.setting_getbool("server_announce")) .. "]"..
		"field[0.8,3.2;3,0.5;te_playername;".. fgettext("Name") .. ";" ..
		engine.setting_get("name") .. "]" ..
		"pwdfield[0.8,4.2;3,0.5;te_passwd;".. fgettext("Password") .. "]" ..
		"field[0.8,5.2;3,0.5;te_serverport;".. fgettext("Server Port") .. ";30000]" ..
		"textlist[4,0.25;7.5,3.7;srv_worlds;" ..
		menu.render_world_list() ..
		";" .. index .. "]"
		
	return retval
end

--------------------------------------------------------------------------------
function tabbuilder.tab_settings()
	return	"vertlabel[0,0;" .. fgettext("SETTINGS") .. "]" ..
			"checkbox[1,0.75;cb_fancy_trees;".. fgettext("Fancy trees") .. ";" 
					.. dump(engine.setting_getbool("new_style_leaves")) .. "]"..
			"checkbox[1,1.25;cb_smooth_lighting;".. fgettext("Smooth Lighting") 
					.. ";".. dump(engine.setting_getbool("smooth_lighting")) .. "]"..
			"checkbox[1,1.75;cb_3d_clouds;".. fgettext("3D Clouds") .. ";"
					.. dump(engine.setting_getbool("enable_3d_clouds")) .. "]"..
			"checkbox[1,2.25;cb_opaque_water;".. fgettext("Opaque Water") .. ";"
					.. dump(engine.setting_getbool("opaque_water")) .. "]"..
			
			"checkbox[4,0.75;cb_mipmapping;".. fgettext("Mip-Mapping") .. ";"
					.. dump(engine.setting_getbool("mip_map")) .. "]"..
			"checkbox[4,1.25;cb_anisotrophic;".. fgettext("Anisotropic Filtering") .. ";"
					.. dump(engine.setting_getbool("anisotropic_filter")) .. "]"..
			"checkbox[4,1.75;cb_bilinear;".. fgettext("Bi-Linear Filtering") .. ";"
					.. dump(engine.setting_getbool("bilinear_filter")) .. "]"..
			"checkbox[4,2.25;cb_trilinear;".. fgettext("Tri-Linear Filtering") .. ";"
					.. dump(engine.setting_getbool("trilinear_filter")) .. "]"..
			
			"checkbox[7.5,0.75;cb_shaders;".. fgettext("Shaders") .. ";"
					.. dump(engine.setting_getbool("enable_shaders")) .. "]"..
			"checkbox[7.5,1.25;cb_pre_ivis;".. fgettext("Preload item visuals") .. ";"
					.. dump(engine.setting_getbool("preload_item_visuals"))	.. "]"..
			"checkbox[7.5,1.75;cb_particles;".. fgettext("Enable Particles") .. ";"
					.. dump(engine.setting_getbool("enable_particles"))	.. "]"..
			"checkbox[7.5,2.25;cb_finite_liquid;".. fgettext("Finite Liquid") .. ";"
					.. dump(engine.setting_getbool("liquid_finite")) .. "]"..
			
			"button[1,4.25;2.25,0.5;btn_change_keys;".. fgettext("Change keys") .. "]"
end

--------------------------------------------------------------------------------
function tabbuilder.tab_singleplayer()
	
	local index = filterlist.get_current_index(worldlist,
				tonumber(engine.setting_get("mainmenu_last_selected_world"))
				)

	return	"button[4,4.15;2.6,0.5;world_delete;".. fgettext("Delete") .. "]" ..
			"button[6.5,4.15;2.8,0.5;world_create;".. fgettext("New") .. "]" ..
			"button[9.2,4.15;2.55,0.5;world_configure;".. fgettext("Configure") .. "]" ..
			"button[8.5,4.95;3.25,0.5;play;".. fgettext("Play") .. "]" ..
			"label[4,-0.25;".. fgettext("Select World:") .. "]"..
			"vertlabel[0,-0.25;".. fgettext("SINGLE PLAYER") .. "]" ..
			"checkbox[0.5,0.25;cb_creative_mode;".. fgettext("Creative Mode") .. ";" ..
			dump(engine.setting_getbool("creative_mode")) .. "]"..
			"checkbox[0.5,0.7;cb_enable_damage;".. fgettext("Enable Damage") .. ";" ..
			dump(engine.setting_getbool("enable_damage")) .. "]"..
			"textlist[4,0.25;7.5,3.7;sp_worlds;" ..
			menu.render_world_list() ..
			";" .. index .. "]" ..
			menubar.formspec
end

--------------------------------------------------------------------------------
function tabbuilder.tab_texture_packs()
	local retval = "label[4,-0.25;".. fgettext("Select texture pack:") .. "]"..
			"vertlabel[0,-0.25;".. fgettext("TEXTURE PACKS") .. "]" ..
			"textlist[4,0.25;7.5,5.0;TPs;"

	local current_texture_path = engine.setting_get("texture_path")	
	local list = filter_texture_pack_list(engine.get_dirlist(engine.get_texturepath(), true))	
	local index = tonumber(engine.setting_get("mainmenu_last_selected_TP"))
	
	if index == nil then index = 1 end
	
	if current_texture_path == "" then
		retval = retval ..
			menu.render_texture_pack_list(list) ..
			";" .. index .. "]"
		return retval
	end
	
	local infofile = current_texture_path ..DIR_DELIM.."info.txt"
	local infotext = ""
	local f = io.open(infofile, "r")
	if f==nil then
		infotext = fgettext("No information available")
	else
		infotext = f:read("*all")
		f:close()
	end
	
	local screenfile = current_texture_path..DIR_DELIM.."screenshot.png"
	local no_screenshot = nil
	if not file_exists(screenfile) then
		screenfile = nil
		no_screenshot = engine.get_texturepath()..DIR_DELIM..
					"base"..DIR_DELIM.."pack"..DIR_DELIM.."no_screenshot.png"
	end

	return	retval ..
			menu.render_texture_pack_list(list) ..
			";" .. index .. "]" ..
			"image[0.65,0.25;4.0,3.7;"..engine.formspec_escape(screenfile or no_screenshot).."]"..
			"textarea[1.0,3.25;3.7,1.5;;"..engine.formspec_escape(infotext or "")..";]"
end

--------------------------------------------------------------------------------
function tabbuilder.tab_credits()
	local logofile = menu.defaulttexturedir .. "logo.png"
	return	"vertlabel[0,-0.5;CREDITS]" ..
			"label[0.5,3;Minetest " .. engine.get_version() .. "]" ..
			"label[0.5,3.3;http://minetest.net]" .. 
			"image[0.5,1;" .. engine.formspec_escape(logofile) .. "]" ..
			"textlist[3.5,-0.25;8.5,5.8;list_credits;" ..
			"#FFFF00" .. fgettext("Core Developers") .."," ..
			"Perttu Ahola (celeron55) <celeron55@gmail.com>,"..
			"Ryan Kwolek (kwolekr) <kwolekr@minetest.net>,"..
			"PilzAdam <pilzadam@minetest.net>," ..
			"Ilya Zhuravlev (xyz) <xyz@minetest.net>,"..
			"Lisa Milne (darkrose) <lisa@ltmnet.com>,"..
			"Maciej Kasatkin (RealBadAngel) <mk@realbadangel.pl>,"..
			"proller <proler@gmail.com>,"..
			"sfan5 <sfan5@live.de>,"..
			"kahrl <kahrl@gmx.net>,"..
			","..
			"#FFFF00" .. fgettext("Active Contributors") .. "," ..
			"sapier,"..
			"Vanessa Ezekowitz (VanessaE) <vanessaezekowitz@gmail.com>,"..
			"Jurgen Doser (doserj) <jurgen.doser@gmail.com>,"..
			"Jeija <jeija@mesecons.net>,"..
			"MirceaKitsune <mirceakitsune@gmail.com>,"..
			"ShadowNinja,"..
			"dannydark <the_skeleton_of_a_child@yahoo.co.uk>,"..
			"0gb.us <0gb.us@0gb.us>,"..
			"," ..
			"#FFFF00" .. fgettext("Previous Contributors") .. "," ..
			"Guiseppe Bilotta (Oblomov) <guiseppe.bilotta@gmail.com>,"..
			"Jonathan Neuschafer <j.neuschaefer@gmx.net>,"..
			"Nils Dagsson Moskopp (erlehmann) <nils@dieweltistgarnichtso.net>,"..
			"Constantin Wenger (SpeedProg) <constantin.wenger@googlemail.com>,"..
			"matttpt <matttpt@gmail.com>,"..
			"JacobF <queatz@gmail.com>,"..
			";0;true]"
end

--------------------------------------------------------------------------------
function tabbuilder.checkretval(retval)

	if retval ~= nil then
		if retval.current_tab ~= nil then
			tabbuilder.current_tab = retval.current_tab
		end
		
		if retval.is_dialog ~= nil then
			tabbuilder.is_dialog = retval.is_dialog
		end
		
		if retval.show_buttons ~= nil then
			tabbuilder.show_buttons = retval.show_buttons
		end
		
		if retval.skipformupdate ~= nil then
			tabbuilder.skipformupdate = retval.skipformupdate
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- initialize callbacks
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
engine.button_handler = function(fields)
	--print("Buttonhandler: tab: " .. tabbuilder.current_tab .. " fields: " .. dump(fields))
	
	if fields["btn_error_confirm"] then
		gamedata.errormessage = nil
	end
	
	local retval = modmgr.handle_buttons(tabbuilder.current_tab,fields)
	tabbuilder.checkretval(retval)
	
	retval = gamemgr.handle_buttons(tabbuilder.current_tab,fields)
	tabbuilder.checkretval(retval)
	
	retval = modstore.handle_buttons(tabbuilder.current_tab,fields)
	tabbuilder.checkretval(retval)
	
	if tabbuilder.current_tab == "dialog_create_world" then
		tabbuilder.handle_create_world_buttons(fields)
	end
	
	if tabbuilder.current_tab == "dialog_delete_world" then
		tabbuilder.handle_delete_world_buttons(fields)
	end
	
	if tabbuilder.current_tab == "singleplayer" then
		tabbuilder.handle_singleplayer_buttons(fields)
	end
	
	if tabbuilder.current_tab == "texture_packs" then
		tabbuilder.handle_texture_pack_buttons(fields)
	end
	
	if tabbuilder.current_tab == "multiplayer" then
		tabbuilder.handle_multiplayer_buttons(fields)
	end
	
	if tabbuilder.current_tab == "settings" then
		tabbuilder.handle_settings_buttons(fields)
	end
	
	if tabbuilder.current_tab == "server" then
		tabbuilder.handle_server_buttons(fields)
	end
	
	--tab buttons
	tabbuilder.handle_tab_buttons(fields)
	
	--menubar buttons
	menubar.handle_buttons(fields)
	
	if not tabbuilder.skipformupdate then
		--update menu
		update_menu()
	else
		tabbuilder.skipformupdate = false
	end
end

--------------------------------------------------------------------------------
engine.event_handler = function(event)
	if event == "MenuQuit" then
		if tabbuilder.is_dialog then
			tabbuilder.is_dialog = false
			tabbuilder.show_buttons = true
			tabbuilder.current_tab = engine.setting_get("main_menu_tab")
			menu.update_gametype()
			update_menu()
		else
			engine.close()
		end
	end
end

--------------------------------------------------------------------------------
function menu.update_gametype(reset)
	if reset then
		mm_texture.reset()
		engine.set_topleft_text("")
		filterlist.set_filtercriteria(worldlist,nil)
	else
		local game = menu.lastgame()
		mm_texture.update(tabbuilder.current_tab,game)
		engine.set_topleft_text(game.name)
		filterlist.set_filtercriteria(worldlist,game.id)
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- menu startup
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
init_globals()
mm_texture.init()
menu.init()
tabbuilder.init()
menubar.refresh()
modstore.init()

engine.sound_play("main_menu", true)

update_menu()
