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
	local render_details = core.is_yes(core.setting_getbool("public_serverlist"))
	
	local retval =
		"vertlabel[0,-0.25;".. fgettext("CLIENT") .. "]" ..
		"label[1,-0.25;".. fgettext("Favorites:") .. "]"..
		"label[1,4.25;".. fgettext("Address/Port") .. "]"..
		"label[9,2.75;".. fgettext("Name/Password") .. "]" ..
		"field[1.25,5.25;5.5,0.5;te_address;;" ..core.setting_get("address") .."]" ..
		"field[6.75,5.25;2.25,0.5;te_port;;" ..core.setting_get("remote_port") .."]" ..
		"checkbox[1,3.6;cb_public_serverlist;".. fgettext("Public Serverlist") .. ";" ..
		dump(core.setting_getbool("public_serverlist")) .. "]"

	if not core.setting_getbool("public_serverlist") then
		retval = retval ..
		"button[6.45,3.95;2.25,0.5;btn_delete_favorite;".. fgettext("Delete") .. "]"
	end

	retval = retval ..
		"button[9,4.95;2.5,0.5;btn_mp_connect;".. fgettext("Connect") .. "]" ..
		"field[9.3,3.75;2.5,0.5;te_name;;" ..core.setting_get("name") .."]" ..
		"pwdfield[9.3,4.5;2.5,0.5;te_pwd;]" ..
		"textarea[9.3,0.25;2.5,2.75;;"
		
	if tabdata.fav_selected ~= nil and
		menudata.favorites[tabdata.fav_selected] ~= nil and
		menudata.favorites[tabdata.fav_selected].description ~= nil then
		retval = retval ..
			core.formspec_escape(menudata.favorites[tabdata.fav_selected].description,true)
	end

	retval = retval ..
		";]"
		
	--favourites
	retval = retval ..
		"textlist[1,0.35;7.5,3.35;favourites;"

	if #menudata.favorites > 0 then
		retval = retval .. render_favorite(menudata.favorites[1],render_details)

		for i=2,#menudata.favorites,1 do
			retval = retval .. "," .. render_favorite(menudata.favorites[i],render_details)
		end
	end

	if tabdata.fav_selected ~= nil then
		retval = retval .. ";" .. tabdata.fav_selected .. "]"
	else
		retval = retval .. ";0]"
	end

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)

	if fields["te_name"] ~= nil then
		gamedata.playername = fields["te_name"]
		core.setting_set("name", fields["te_name"])
	end

	if fields["favourites"] ~= nil then
		local event = core.explode_textlist_event(fields["favourites"])
		if event.type == "DCL" then
			if event.index <= #menudata.favorites then
				gamedata.address    = menudata.favorites[event.index].address
				gamedata.port       = menudata.favorites[event.index].port
				gamedata.playername = fields["te_name"]
				if fields["te_pwd"] ~= nil then
					gamedata.password		= fields["te_pwd"]
				end
				gamedata.selected_world = 0

				if menudata.favorites ~= nil then
					gamedata.servername        = menudata.favorites[event.index].name
					gamedata.serverdescription = menudata.favorites[event.index].description
				end

				if gamedata.address ~= nil and
					gamedata.port ~= nil then
					core.setting_set("address",gamedata.address)
					core.setting_set("remote_port",gamedata.port)
					core.start()
				end
			end
			return true
		end

		if event.type == "CHG" then
			if event.index <= #menudata.favorites then
				local address = menudata.favorites[event.index].address
				local port    = menudata.favorites[event.index].port

				if address ~= nil and
					port ~= nil then
					core.setting_set("address",address)
					core.setting_set("remote_port",port)
				end

				tabdata.fav_selected = event.index
			end
			
			return true
		end
	end

	if fields["key_up"] ~= nil or
		fields["key_down"] ~= nil then

		local fav_idx = core.get_textlist_index("favourites")

		if fav_idx ~= nil then
			if fields["key_up"] ~= nil and fav_idx > 1 then
				fav_idx = fav_idx -1
			else if fields["key_down"] and fav_idx < #menudata.favorites then
				fav_idx = fav_idx +1
			end end
		else
			fav_idx = 1
		end
		
		if menudata.favorites == nil or
			menudata.favorites[fav_idx] == nil then
			tabdata.fav_selected = 0
			return true
		end
	
		local address = menudata.favorites[fav_idx].address
		local port    = menudata.favorites[fav_idx].port

		if address ~= nil and
			port ~= nil then
			core.setting_set("address",address)
			core.setting_set("remote_port",port)
		end

		tabdata.fav_selected = fav_idx
		return true
	end

	if fields["cb_public_serverlist"] ~= nil then
		core.setting_set("public_serverlist", fields["cb_public_serverlist"])

		if core.setting_getbool("public_serverlist") then
			asyncOnlineFavourites()
		else
			menudata.favorites = core.get_favorites("local")
		end
		tabdata.fav_selected = nil
		return true
	end

	if fields["btn_delete_favorite"] ~= nil then
		local current_favourite = core.get_textlist_index("favourites")
		if current_favourite == nil then return end
		core.delete_favorite(current_favourite)
		menudata.favorites   = core.get_favorites()
		tabdata.fav_selected = nil

		core.setting_set("address","")
		core.setting_set("remote_port","30000")

		return true
	end

	if fields["btn_mp_connect"] ~= nil or
		fields["key_enter"] ~= nil then

		gamedata.playername     = fields["te_name"]
		gamedata.password       = fields["te_pwd"]
		gamedata.address        = fields["te_address"]
		gamedata.port           = fields["te_port"]

		local fav_idx = core.get_textlist_index("favourites")

		if fav_idx ~= nil and fav_idx <= #menudata.favorites and
			menudata.favorites[fav_idx].address == fields["te_address"] and
			menudata.favorites[fav_idx].port    == fields["te_port"] then

			gamedata.servername        = menudata.favorites[fav_idx].name
			gamedata.serverdescription = menudata.favorites[fav_idx].description
		else
			gamedata.servername        = ""
			gamedata.serverdescription = ""
		end

		gamedata.selected_world = 0

		core.setting_set("address",    fields["te_address"])
		core.setting_set("remote_port",fields["te_port"])

		core.start()
		return true
	end
	return false
end

local function on_change(type,old_tab,new_tab)
	if type == "LEAVE" then
		return
	end
	if core.setting_getbool("public_serverlist") then
		asyncOnlineFavourites()
	else
		menudata.favorites = core.get_favorites("local")
	end
end

--------------------------------------------------------------------------------
tab_multiplayer = {
	name = "multiplayer",
	caption = fgettext("Client"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
	}
