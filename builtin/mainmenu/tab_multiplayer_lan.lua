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
	local retval =
		"label[7.75,-0.15;" .. fgettext("Address / Port :") .. "]" ..
		"label[7.75,1.05;" .. fgettext("Name / Password :") .. "]" ..
		"field[8,0.75;3.4,0.5;te_address;;" ..
		core.formspec_escape(core.setting_get("address")) .. "]" ..
		"field[11.25,0.75;1.3,0.5;te_port;;" ..
		core.formspec_escape(core.setting_get("remote_port")) .. "]"

	retval = retval ..
		"button[10,4.9;2,0.5;btn_mp_connect;" .. fgettext("Connect") .. "]" ..
		"field[8,1.95;2.95,0.5;te_name;;" ..
		core.formspec_escape(core.setting_get("name")) .. "]" ..
		"pwdfield[10.78,1.95;1.77,0.5;te_pwd;]" ..
		"box[7.73,2.35;4.3,2.28;#999999]" ..
		"textarea[8.1,2.4;4.26,2.6;;"
		
	retval = retval .. ";]"

	-- TODO: Search for LAN games automatically (the server should do UDP
	--       broadcast or something)
	retval = retval .. "tablecolumns[text;text]"
	retval = retval .. "table[-0.15,-0.1;7.75,5;lan_games;"
	retval = retval .. core.formspec_escape("127.0.0.1:30000") .. "," .. core.formspec_escape("Advertised somehow by server")
	retval = retval .. ";0]"

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)
	if fields["te_name"] ~= nil then
		gamedata.playername = fields["te_name"]
		core.setting_set("name", fields["te_name"])
	end

	if fields["favourites"] ~= nil then
		local event = core.explode_table_event(fields["favourites"])
		if event.type == "DCL" then
			if event.row <= #menudata.lan_games then
				if not is_server_protocol_compat_or_error(menudata.lan_games[event.row].proto_min,
						menudata.lan_games[event.row].proto_max) then
					return true
				end
				gamedata.address    = menudata.lan_games[event.row].address
				gamedata.port       = menudata.lan_games[event.row].port
				gamedata.playername = fields["te_name"]
				if fields["te_pwd"] ~= nil then
					gamedata.password		= fields["te_pwd"]
				end
				gamedata.selected_world = 0

				if menudata.lan_games ~= nil then
					gamedata.servername        = menudata.lan_games[event.row].name
					gamedata.serverdescription = menudata.lan_games[event.row].description
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
			if event.row <= #menudata.lan_games then
				local address = menudata.lan_games[event.row].address
				local port    = menudata.lan_games[event.row].port

				if address ~= nil and
					port ~= nil then
					core.setting_set("address",address)
					core.setting_set("remote_port",port)
				end

				tabdata.fav_selected = event.row
			end
			
			return true
		end
	end

	if fields["key_up"] ~= nil or
		fields["key_down"] ~= nil then

		local fav_idx = core.get_table_index("favourites")

		if fav_idx ~= nil then
			if fields["key_up"] ~= nil and fav_idx > 1 then
				fav_idx = fav_idx -1
			else if fields["key_down"] and fav_idx < #menudata.lan_games then
				fav_idx = fav_idx +1
			end end
		else
			fav_idx = 1
		end
		
		if menudata.lan_games == nil or
			menudata.lan_games[fav_idx] == nil then
			tabdata.fav_selected = 0
			return true
		end
	
		local address = menudata.lan_games[fav_idx].address
		local port    = menudata.lan_games[fav_idx].port

		if address ~= nil and
			port ~= nil then
			core.setting_set("address",address)
			core.setting_set("remote_port",port)
		end

		tabdata.fav_selected = fav_idx
		return true
	end

	if (fields["btn_mp_connect"] ~= nil or
		fields["key_enter"] ~= nil) and fields["te_address"] ~= nil and
		fields["te_port"] ~= nil then

		gamedata.playername     = fields["te_name"]
		gamedata.password       = fields["te_pwd"]
		gamedata.address        = fields["te_address"]
		gamedata.port           = fields["te_port"]

		local fav_idx = core.get_table_index("favourites")

		if fav_idx ~= nil and fav_idx <= #menudata.favorites and
			menudata.favorites[fav_idx].address == fields["te_address"] and
			menudata.favorites[fav_idx].port    == fields["te_port"] then

			gamedata.servername        = menudata.favorites[fav_idx].name
			gamedata.serverdescription = menudata.favorites[fav_idx].description

			if not is_server_protocol_compat_or_error(menudata.favorites[fav_idx].proto_min,
					menudata.favorites[fav_idx].proto_max)then
				return true
			end
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
tab_multiplayer_lan = {
	name = "multiplayer_lan",
	caption = fgettext("LAN (TBD)"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
	}

