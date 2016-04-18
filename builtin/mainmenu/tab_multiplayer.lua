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
	-- Update the cached supported proto info,
	-- it may have changed after a change by the settings menu.
	common_update_cached_supp_proto()
	local fav_selected = menudata.favorites[tabdata.fav_selected]

	local retval =
		"label[7.75,-0.15;" .. fgettext("Address / Port") .. "]" ..
		"label[7.75,1.05;" .. fgettext("Name / Password") .. "]" ..
		"field[8,0.75;3.3,0.5;te_address;;" ..
			core.formspec_escape(core.setting_get("address")) .. "]" ..
		"field[11.15,0.75;1.4,0.5;te_port;;" ..
			core.formspec_escape(core.setting_get("remote_port")) .. "]" ..
		"button[10.1,4.9;2,0.5;btn_mp_connect;" .. fgettext("Connect") .. "]" ..
		"field[8,1.95;2.95,0.5;te_name;;" ..
			core.formspec_escape(core.setting_get("name")) .. "]" ..
		"pwdfield[10.78,1.95;1.77,0.5;te_pwd;]" ..
		"box[7.73,2.35;4.3,2.28;#999999]"

	if tabdata.fav_selected and fav_selected then
		if gamedata.fav then
			retval = retval .. "button[7.85,4.9;2.3,0.5;btn_delete_favorite;" ..
				fgettext("Del. Favorite") .. "]"
		end
		if fav_selected.description then
			retval = retval .. "textarea[8.1,2.4;4.26,2.6;;" ..
				core.formspec_escape((gamedata.serverdescription or ""), true) .. ";]"
		end
	end

	--favourites
	retval = retval .. "tablecolumns[" ..
		image_column(fgettext("Favorite"), "favorite") .. ";" ..
		"color,span=3;" ..
		"text,align=right;" ..                -- clients
		"text,align=center,padding=0.25;" ..  -- "/"
		"text,align=right,padding=0.25;" ..   -- clients_max
		image_column(fgettext("Creative mode"), "creative") .. ",padding=1;" ..
		image_column(fgettext("Damage enabled"), "damage") .. ",padding=0.25;" ..
		image_column(fgettext("PvP enabled"), "pvp") .. ",padding=0.25;" ..
		"color,span=1;" ..
		"text,padding=1]" ..
		"table[-0.15,-0.1;7.75,5.5;favourites;"

	if #menudata.favorites > 0 then
		local favs = core.get_favorites("local")
		if #favs > 0 then
			for i = 1, #favs do
			for j = 1, #menudata.favorites do
				if menudata.favorites[j].address == favs[i].address and
						menudata.favorites[j].port == favs[i].port then
					table.insert(menudata.favorites, i, table.remove(menudata.favorites, j))
				end
			end
				if favs[i].address ~= menudata.favorites[i].address then
					table.insert(menudata.favorites, i, favs[i])
				end
			end
		end
		retval = retval .. render_favorite(menudata.favorites[1], (#favs > 0))
		for i = 2, #menudata.favorites do
			retval = retval .. "," .. render_favorite(menudata.favorites[i], (i <= #favs))
		end
	end

	if tabdata.fav_selected then
		retval = retval .. ";" .. tabdata.fav_selected .. "]"
	else
		retval = retval .. ";0]"
	end

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)
	if fields.te_name then
		gamedata.playername = fields.te_name
		core.setting_set("name", fields.te_name)
	end

	if fields.favourites then
		local event = core.explode_table_event(fields.favourites)
		local fav = menudata.favorites[event.row]

		if event.type == "DCL" then
			if event.row <= #menudata.favorites then
				if menudata.favorites_is_public and
						not is_server_protocol_compat_or_error(
							fav.proto_min, fav.proto_max) then
					return true
				end

				gamedata.address    = fav.address
				gamedata.port       = fav.port
				gamedata.playername = fields.te_name
				gamedata.selected_world = 0

				if fields.te_pwd then
					gamedata.password = fields.te_pwd
				end

				gamedata.servername        = fav.name
				gamedata.serverdescription = fav.description

				if gamedata.address and gamedata.port then
					core.setting_set("address", gamedata.address)
					core.setting_set("remote_port", gamedata.port)
					core.start()
				end
			end
			return true
		end

		if event.type == "CHG" then
			if event.row <= #menudata.favorites then
				gamedata.fav = false
				local favs = core.get_favorites("local")
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
					core.setting_set("address", address)
					core.setting_set("remote_port", port)
				end
				tabdata.fav_selected = event.row
			end
			return true
		end
	end

	if fields.key_up or fields.key_down then
		local fav_idx = core.get_table_index("favourites")
		local fav = menudata.favorites[fav_idx]

		if fav_idx then
			if fields.key_up and fav_idx > 1 then
				fav_idx = fav_idx - 1
			elseif fields.key_down and fav_idx < #menudata.favorites then
				fav_idx = fav_idx + 1
			end
		else
			fav_idx = 1
		end

		if not menudata.favorites or not fav then
			tabdata.fav_selected = 0
			return true
		end

		local address = fav.address
		local port    = fav.port

		if address and port then
			core.setting_set("address", address)
			core.setting_set("remote_port", port)
		end

		tabdata.fav_selected = fav_idx
		return true
	end

	if fields.btn_delete_favorite then
		local current_favourite = core.get_table_index("favourites")
		if not current_favourite then return end

		core.delete_favorite(current_favourite)
		asyncOnlineFavourites()
		tabdata.fav_selected = nil

		core.setting_set("address", "")
		core.setting_set("remote_port", "30000")
		return true
	end

	if (fields.btn_mp_connect or fields.key_enter) and fields.te_address and fields.te_port then
		gamedata.playername = fields.te_name
		gamedata.password   = fields.te_pwd
		gamedata.address    = fields.te_address
		gamedata.port       = fields.te_port
		gamedata.selected_world = 0
		local fav_idx = core.get_table_index("favourites")
		local fav = menudata.favorites[fav_idx]

		if fav_idx and fav_idx <= #menudata.favorites and
				fav.address == fields.te_address and
				fav.port    == fields.te_port then

			gamedata.servername        = fav.name
			gamedata.serverdescription = fav.description

			if menudata.favorites_is_public and
					not is_server_protocol_compat_or_error(
						fav.proto_min, fav.proto_max) then
				return true
			end
		else
			gamedata.servername        = ""
			gamedata.serverdescription = ""
		end

		core.setting_set("address",     fields.te_address)
		core.setting_set("remote_port", fields.te_port)

		core.start()
		return true
	end
	return false
end

local function on_change(type, old_tab, new_tab)
	if type == "LEAVE" then return end
	asyncOnlineFavourites()
end

--------------------------------------------------------------------------------
return {
	name = "multiplayer",
	caption = fgettext("Client"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
