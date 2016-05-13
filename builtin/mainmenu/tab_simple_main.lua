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
local function get_formspec(tabview, name, tabdata)
	local retval = ""

	local render_details = dump(core.setting_getbool("public_serverlist"))

	retval = retval ..
		"label[8,0.5;".. fgettext("Name/Password") .. "]" ..
		"field[0.25,3.25;5.5,0.5;te_address;;" ..
		core.formspec_escape(core.setting_get("address")) .."]" ..
		"field[5.75,3.25;2.25,0.5;te_port;;" ..
		core.formspec_escape(core.setting_get("remote_port")) .."]" ..
		"checkbox[8,-0.25;cb_public_serverlist;".. fgettext("Public Serverlist") .. ";" ..
		render_details .. "]"

	retval = retval ..
		"button[8,2.5;4,1.5;btn_mp_connect;".. fgettext("Connect") .. "]" ..
		"field[8.75,1.5;3.5,0.5;te_name;;" ..
		core.formspec_escape(core.setting_get("name")) .."]" ..
		"pwdfield[8.75,2.3;3.5,0.5;te_pwd;]"
		
	if render_details then
		retval = retval .. "tablecolumns[" ..
			"color,span=3;" ..
			"text,align=right;" ..                -- clients
			"text,align=center,padding=0.25;" ..  -- "/"
			"text,align=right,padding=0.25;" ..   -- clients_max
			image_column(fgettext("Creative mode"), "creative") .. ",padding=1;" ..
			image_column(fgettext("Damage enabled"), "damage") .. ",padding=0.25;" ..
			image_column(fgettext("PvP enabled"), "pvp") .. ",padding=0.25;" ..
			"color,span=1;" ..
			"text,padding=1]"                               -- name
	else
		retval = retval .. "tablecolumns[text]"
	end
	retval = retval ..
		"table[-0.05,0;7.55,2.75;favourites;"

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

	-- separator
	retval = retval ..
		"box[-0.28,3.75;12.4,0.1;#FFFFFF]"

	-- checkboxes
	retval = retval ..
		"checkbox[8.0,3.9;cb_creative;".. fgettext("Creative Mode") .. ";" ..
		dump(core.setting_getbool("creative_mode")) .. "]"..
		"checkbox[8.0,4.4;cb_damage;".. fgettext("Enable Damage") .. ";" ..
		dump(core.setting_getbool("enable_damage")) .. "]"
	-- buttons
	retval = retval ..
		"button[0,3.7;8,1.5;btn_start_singleplayer;" .. fgettext("Start Singleplayer") .. "]" ..
		"button[0,4.5;8,1.5;btn_config_sp_world;" .. fgettext("Config mods") .. "]"

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)

	if fields["btn_start_singleplayer"] then
		gamedata.selected_world	= gamedata.worldindex
		gamedata.singleplayer	= true
		core.start()
		return true
	end

	if fields["favourites"] ~= nil then
		local event = core.explode_table_event(fields["favourites"])

		if event.type == "CHG" then
			if event.row <= #menudata.favorites then
				local address = menudata.favorites[event.row].address
				local port = menudata.favorites[event.row].port

				if address ~= nil and
					port ~= nil then
					core.setting_set("address",address)
					core.setting_set("remote_port",port)
				end

				tabdata.fav_selected = event.row
			end
		end
		return true
	end

	if fields["cb_public_serverlist"] ~= nil then
		core.setting_set("public_serverlist", fields["cb_public_serverlist"])

		if core.setting_getbool("public_serverlist") then
			asyncOnlineFavourites()
		else
			menudata.favorites = core.get_favorites("local")
		end
		return true
	end

	if fields["cb_creative"] then
		core.setting_set("creative_mode", fields["cb_creative"])
		return true
	end

	if fields["cb_damage"] then
		core.setting_set("enable_damage", fields["cb_damage"])
		return true
	end

	if fields["btn_mp_connect"] ~= nil or
		fields["key_enter"] ~= nil then

		gamedata.playername		= fields["te_name"]
		gamedata.password		= fields["te_pwd"]
		gamedata.address		= fields["te_address"]
		gamedata.port			= fields["te_port"]

		local fav_idx = core.get_textlist_index("favourites")

		if fav_idx ~= nil and fav_idx <= #menudata.favorites and
			menudata.favorites[fav_idx].address == fields["te_address"] and
			menudata.favorites[fav_idx].port    == fields["te_port"] then

			gamedata.servername			= menudata.favorites[fav_idx].name
			gamedata.serverdescription	= menudata.favorites[fav_idx].description

			if not is_server_protocol_compat_or_error(menudata.favorites[fav_idx].proto_min,
					menudata.favorites[fav_idx].proto_max) then
				return true
			end
		else
			gamedata.servername			= ""
			gamedata.serverdescription	= ""
		end

		gamedata.selected_world = 0

		core.setting_set("address",fields["te_address"])
		core.setting_set("remote_port",fields["te_port"])

		core.start()
		return true
	end

	if fields["btn_config_sp_world"] ~= nil then
		local configdialog = create_configure_world_dlg(1)

		if (configdialog ~= nil) then
			configdialog:set_parent(tabview)
			tabview:hide()
			configdialog:show()
		end
		return true
	end
end

--------------------------------------------------------------------------------
local function on_activate(type,old_tab,new_tab)
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
tab_simple_main = {
	name = "main",
	caption = fgettext("Main"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_activate
	}
