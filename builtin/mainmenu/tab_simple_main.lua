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
	-- Update the cached supported proto info,
	-- it may have changed after a change by the settings menu.
	common_update_cached_supp_proto()
	local fav_selected = menudata.favorites[tabdata.fav_selected]

	local retval =
		"label[9.5,0;".. fgettext("Name / Password") .. "]" ..
		"field[0.25,3.35;5.5,0.5;te_address;;" ..
			core.formspec_escape(core.settings:get("address")) .."]" ..
		"field[5.75,3.35;2.25,0.5;te_port;;" ..
			core.formspec_escape(core.settings:get("remote_port")) .."]" ..
		"button[10,2.6;2,1.5;btn_mp_connect;".. fgettext("Connect") .. "]" ..
		"field[9.8,1;2.6,0.5;te_name;;" ..
			core.formspec_escape(core.settings:get("name")) .."]" ..
		"pwdfield[9.8,2;2.6,0.5;te_pwd;]"


	if tabdata.fav_selected and fav_selected then
		if gamedata.fav then
			retval = retval .. "button[7.7,2.6;2.3,1.5;btn_delete_favorite;" ..
				fgettext("Del. Favorite") .. "]"
		end
	end

	retval = retval .. "tablecolumns[" ..
		image_column(fgettext("Favorite"), "favorite") .. ";" ..
		image_column(fgettext("Ping"), "") .. ",padding=0.25;" ..
		"color,span=3;" ..
		"text,align=right;" ..                -- clients
		"text,align=center,padding=0.25;" ..  -- "/"
		"text,align=right,padding=0.25;" ..   -- clients_max
		image_column(fgettext("Creative mode"), "creative") .. ",padding=1;" ..
		image_column(fgettext("Damage enabled"), "damage") .. ",padding=0.25;" ..
		image_column(fgettext("PvP enabled"), "pvp") .. ",padding=0.25;" ..
		"color,span=1;" ..
		"text,padding=1]" ..                  -- name
		"table[-0.05,0;9.2,2.75;favourites;"

	if #menudata.favorites > 0 then
		local favs = core.get_favorites("local")
		if #favs > 0 then
			for i = 1, #favs do
				for j = 1, #menudata.favorites do
					if menudata.favorites[j].address == favs[i].address and
							menudata.favorites[j].port == favs[i].port then
						table.insert(menudata.favorites, i,
							table.remove(menudata.favorites, j))
					end
				end
				if favs[i].address ~= menudata.favorites[i].address then
					table.insert(menudata.favorites, i, favs[i])
				end
			end
		end
		retval = retval .. render_serverlist_row(menudata.favorites[1], (#favs > 0))
		for i = 2, #menudata.favorites do
			retval = retval .. "," .. render_serverlist_row(menudata.favorites[i], (i <= #favs))
		end
	end

	if tabdata.fav_selected then
		retval = retval .. ";" .. tabdata.fav_selected .. "]"
	else
		retval = retval .. ";0]"
	end

	-- separator
	retval = retval .. "box[-0.28,3.75;12.4,0.1;#FFFFFF]"

	-- checkboxes
	retval = retval ..
		"checkbox[8.0,3.9;cb_creative;".. fgettext("Creative Mode") .. ";" ..
			dump(core.settings:get_bool("creative_mode")) .. "]"..
		"checkbox[8.0,4.4;cb_damage;".. fgettext("Enable Damage") .. ";" ..
			dump(core.settings:get_bool("enable_damage")) .. "]"
	-- buttons
	retval = retval ..
		"button[0,3.7;8,1.5;btn_start_singleplayer;" .. fgettext("Start Singleplayer") .. "]" ..
		"button[0,4.5;8,1.5;btn_config_sp_world;" .. fgettext("Config mods") .. "]"

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)
	if fields.btn_start_singleplayer then
		gamedata.selected_world	= gamedata.worldindex
		gamedata.singleplayer	= true
		core.start()
		return true
	end

	if fields.favourites then
		local event = core.explode_table_event(fields.favourites)
		if event.type == "CHG" then
			if event.row <= #menudata.favorites then
				gamedata.fav = false
				local favs = core.get_favorites("local")
				local fav = menudata.favorites[event.row]
				local address = fav.address
				local port    = fav.port
				gamedata.serverdescription = fav.description

				for i = 1, #favs do
					if fav.address == favs[i].address and
							fav.port == favs[i].port then
						gamedata.fav = true
					end
				end

				if address and port then
					core.settings:set("address", address)
					core.settings:set("remote_port", port)
				end
				tabdata.fav_selected = event.row
			end
			return true
		end
	end

	if fields.btn_delete_favorite then
		local current_favourite = core.get_table_index("favourites")
		if not current_favourite then return end

		core.delete_favorite(current_favourite)
		asyncOnlineFavourites()
		tabdata.fav_selected = nil

		core.settings:set("address", "")
		core.settings:set("remote_port", "30000")
		return true
	end

	if fields.cb_creative then
		core.settings:set("creative_mode", fields.cb_creative)
		return true
	end

	if fields.cb_damage then
		core.settings:set("enable_damage", fields.cb_damage)
		return true
	end

	if fields.btn_mp_connect or fields.key_enter then
		gamedata.playername = fields.te_name
		gamedata.password   = fields.te_pwd
		gamedata.address    = fields.te_address
		gamedata.port	    = fields.te_port
		local fav_idx = core.get_textlist_index("favourites")

		if fav_idx and fav_idx <= #menudata.favorites and
				menudata.favorites[fav_idx].address == fields.te_address and
				menudata.favorites[fav_idx].port    == fields.te_port then
			local fav = menudata.favorites[fav_idx]
			gamedata.servername        = fav.name
			gamedata.serverdescription = fav.description

			if menudata.favorites_is_public and
					not is_server_protocol_compat_or_error(
						fav.proto_min, fav.proto_max) then
				return true
			end
		else
			gamedata.servername	   = ""
			gamedata.serverdescription = ""
		end

		gamedata.selected_world = 0

		core.settings:set("address", fields.te_address)
		core.settings:set("remote_port", fields.te_port)

		core.start()
		return true
	end

	if fields.btn_config_sp_world then
		local configdialog = create_configure_world_dlg(1)
		if configdialog then
			configdialog:set_parent(tabview)
			tabview:hide()
			configdialog:show()
		end
		return true
	end
end

--------------------------------------------------------------------------------
local function on_activate(type,old_tab,new_tab)
	if type == "LEAVE" then return end
	asyncOnlineFavourites()
end

--------------------------------------------------------------------------------
return {
	name = "main",
	caption = fgettext("Main"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_activate
}
