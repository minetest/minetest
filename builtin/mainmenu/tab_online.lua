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
	local selected
	if menudata.search_result then
		selected = menudata.search_result[tabdata.selected]
	else
		selected = serverlistmgr.servers[tabdata.selected]
	end

	if not tabdata.search_for then
		tabdata.search_for = ""
	end

	local retval =
		-- Search
		"field[0.15,0.075;5.91,1;te_search;;" .. core.formspec_escape(tabdata.search_for) .. "]" ..
		"image_button[5.63,-.165;.83,.83;" .. core.formspec_escape(defaulttexturedir .. "search.png") .. ";btn_mp_search;]" ..
		"image_button[6.3,-.165;.83,.83;" .. core.formspec_escape(defaulttexturedir .. "clear.png") .. ";btn_mp_clear;]" ..
		"image_button[6.97,-.165;.83,.83;" .. core.formspec_escape(defaulttexturedir .. "refresh.png")
			.. ";btn_mp_refresh;]" ..

		-- Address / Port
		"label[7.75,-0.25;" .. fgettext("Address / Port") .. "]" ..
		"field[8,0.65;3.25,0.5;te_address;;" ..
			core.formspec_escape(core.settings:get("address")) .. "]" ..
		"field[11.1,0.65;1.4,0.5;te_port;;" ..
			core.formspec_escape(core.settings:get("remote_port")) .. "]" ..

		-- Name / Password
		"label[7.75,0.95;" .. fgettext("Name / Password") .. "]" ..
		"field[8,1.85;2.9,0.5;te_name;;" ..
			core.formspec_escape(core.settings:get("name")) .. "]" ..
		"pwdfield[10.73,1.85;1.77,0.5;te_pwd;]" ..

		-- Description Background
		"box[7.73,2.25;4.25,2.6;#999999]"..

		-- Connect
		"button[9.88,4.9;2.3,1;btn_mp_connect;" .. fgettext("Connect") .. "]"

	if tabdata.selected and selected then
		if gamedata.fav then
			retval = retval .. "button[7.73,4.9;2.3,1;btn_delete_favorite;" ..
				fgettext("Del. Favorite") .. "]"
		end
		if selected.description then
			retval = retval .. "textarea[8.1,2.3;4.23,2.9;;;" ..
				core.formspec_escape((gamedata.serverdescription or ""), true) .. "]"
		end
	end

	--favorites
	retval = retval .. "tablecolumns[" ..
		image_column(fgettext("Favorite"), "favorite") .. ";" ..
		image_column(fgettext("Ping")) .. ",padding=0.25;" ..
		"color,span=3;" ..
		"text,align=right;" ..                -- clients
		"text,align=center,padding=0.25;" ..  -- "/"
		"text,align=right,padding=0.25;" ..   -- clients_max
		image_column(fgettext("Creative mode"), "creative") .. ",padding=1;" ..
		image_column(fgettext("Damage enabled"), "damage") .. ",padding=0.25;" ..
		--~ PvP = Player versus Player
		image_column(fgettext("PvP enabled"), "pvp") .. ",padding=0.25;" ..
		"color,span=1;" ..
		"text,padding=1]" ..
		"table[-0.15,0.6;7.75,5.15;favorites;"

	if menudata.search_result then
		local favs = serverlistmgr.get_favorites()
		for i = 1, #menudata.search_result do
			local server = menudata.search_result[i]
			for fav_id = 1, #favs do
				if server.address == favs[fav_id].address and
						server.port == favs[fav_id].port then
					server.is_favorite = true
				end
			end

			if i ~= 1 then
				retval = retval .. ","
			end

			retval = retval .. render_serverlist_row(server, server.is_favorite)
		end
	elseif #serverlistmgr.servers > 0 then
		local favs = serverlistmgr.get_favorites()
		if #favs > 0 then
			for i = 1, #favs do
				for j = 1, #serverlistmgr.servers do
					if serverlistmgr.servers[j].address == favs[i].address and
							serverlistmgr.servers[j].port == favs[i].port then
						table.insert(serverlistmgr.servers, i, table.remove(serverlistmgr.servers, j))
					end
				end
				if favs[i].address ~= serverlistmgr.servers[i].address then
					table.insert(serverlistmgr.servers, i, favs[i])
				end
			end
		end

		retval = retval .. render_serverlist_row(serverlistmgr.servers[1], (#favs > 0))
		for i = 2, #serverlistmgr.servers do
			retval = retval .. "," .. render_serverlist_row(serverlistmgr.servers[i], (i <= #favs))
		end
	end

	if tabdata.selected then
		retval = retval .. ";" .. tabdata.selected .. "]"
	else
		retval = retval .. ";0]"
	end

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)
	local serverlist = menudata.search_result or serverlistmgr.servers

	if fields.te_name then
		gamedata.playername = fields.te_name
		core.settings:set("name", fields.te_name)
	end

	if fields.favorites then
		local event = core.explode_table_event(fields.favorites)
		local fav = serverlist[event.row]

		if event.type == "DCL" then
			if event.row <= #serverlist then
				if not is_server_protocol_compat_or_error(
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
					core.settings:set("address", gamedata.address)
					core.settings:set("remote_port", gamedata.port)
					core.start()
				end
			end
			return true
		end

		if event.type == "CHG" then
			if event.row <= #serverlist then
				gamedata.fav = false
				local favs = serverlistmgr.get_favorites()
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
				tabdata.selected = event.row
			end
			return true
		end
	end

	if fields.key_up or fields.key_down then
		local fav_idx = core.get_table_index("favorites")
		local fav = serverlist[fav_idx]

		if fav_idx then
			if fields.key_up and fav_idx > 1 then
				fav_idx = fav_idx - 1
			elseif fields.key_down and fav_idx < #serverlistmgr.servers then
				fav_idx = fav_idx + 1
			end
		else
			fav_idx = 1
		end

		if not serverlistmgr.servers or not fav then
			tabdata.selected = 0
			return true
		end

		local address = fav.address
		local port    = fav.port
		gamedata.serverdescription = fav.description
		if address and port then
			core.settings:set("address", address)
			core.settings:set("remote_port", port)
		end

		tabdata.selected = fav_idx
		return true
	end

	if fields.btn_delete_favorite then
		local current_favorite = core.get_table_index("favorites")
		if not current_favorite then return end

		serverlistmgr.delete_favorite(serverlistmgr.servers[current_favorite])
		serverlistmgr.sync()
		tabdata.selected = nil

		core.settings:set("address", "")
		core.settings:set("remote_port", "30000")
		return true
	end

	if fields.btn_mp_clear then
		tabdata.search_for = ""
		menudata.search_result = nil
		return true
	end

	if fields.btn_mp_search or fields.key_enter_field == "te_search" then
		tabdata.selected = 1
		local input = fields.te_search:lower()
		tabdata.search_for = fields.te_search

		if #serverlistmgr.servers < 2 then
			return true
		end

		menudata.search_result = {}

		-- setup the keyword list
		local keywords = {}
		for word in input:gmatch("%S+") do
			word = word:gsub("(%W)", "%%%1")
			table.insert(keywords, word)
		end

		if #keywords == 0 then
			menudata.search_result = nil
			return true
		end

		-- Search the serverlist
		local search_result = {}
		for i = 1, #serverlistmgr.servers do
			local server = serverlistmgr.servers[i]
			local found = 0
			for k = 1, #keywords do
				local keyword = keywords[k]
				if server.name then
					local sername = server.name:lower()
					local _, count = sername:gsub(keyword, keyword)
					found = found + count * 4
				end

				if server.description then
					local desc = server.description:lower()
					local _, count = desc:gsub(keyword, keyword)
					found = found + count * 2
				end
			end
			if found > 0 then
				local points = (#serverlistmgr.servers - i) / 5 + found
				server.points = points
				table.insert(search_result, server)
			end
		end
		if #search_result > 0 then
			table.sort(search_result, function(a, b)
				return a.points > b.points
			end)
			menudata.search_result = search_result
			local first_server = search_result[1]
			core.settings:set("address",     first_server.address)
			core.settings:set("remote_port", first_server.port)
			gamedata.serverdescription = first_server.description
		end
		return true
	end

	if fields.btn_mp_refresh then
		serverlistmgr.sync()
		return true
	end

	if (fields.btn_mp_connect or fields.key_enter)
			and fields.te_address ~= "" and fields.te_port then
		gamedata.playername = fields.te_name
		gamedata.password   = fields.te_pwd
		gamedata.address    = fields.te_address
		gamedata.port       = tonumber(fields.te_port)
		gamedata.selected_world = 0
		local fav_idx = core.get_table_index("favorites")
		local fav = serverlist[fav_idx]

		if fav_idx and fav_idx <= #serverlist and
				fav.address == gamedata.address and
				fav.port    == gamedata.port then

			serverlistmgr.add_favorite(fav)

			gamedata.servername        = fav.name
			gamedata.serverdescription = fav.description

			if not is_server_protocol_compat_or_error(
						fav.proto_min, fav.proto_max) then
				return true
			end
		else
			gamedata.servername        = ""
			gamedata.serverdescription = ""

			serverlistmgr.add_favorite({
				address = gamedata.address,
				port = gamedata.port,
			})
		end

		core.settings:set("address",     gamedata.address)
		core.settings:set("remote_port", gamedata.port)

		core.start()
		return true
	end
	return false
end

local function on_change(type, old_tab, new_tab)
	if type == "LEAVE" then return end
	serverlistmgr.sync()
end

--------------------------------------------------------------------------------
return {
	name = "online",
	caption = fgettext("Join Game"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
