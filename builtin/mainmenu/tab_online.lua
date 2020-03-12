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

local server_lookup
local function get_formspec(tabview, name, tabdata)
	-- Update the cached supported proto info,
	-- it may have changed after a change by the settings menu.
	common_update_cached_supp_proto()
	local fav_selected
	if menudata.search_result then
		fav_selected = menudata.search_result[tabdata.fav_selected]
	else
		fav_selected = menudata.favorites[tabdata.fav_selected]
	end

	if not tabdata.search_for then
		tabdata.search_for = ""
	end

	local retval =
		-- Search
		"image_button[0.25,0.25;0.75,0.75;" .. core.formspec_escape(defaulttexturedir .. "mainmenu_clear.png")
			.. ";btn_mp_clear;]" ..
		"field[1,0.25;6,0.75;te_search;;" .. core.formspec_escape(tabdata.search_for) .. "]" ..
		"image_button[7,0.25;0.75,0.75;" .. core.formspec_escape(defaulttexturedir .. "mainmenu_search.png")
			.. ";btn_mp_search;]" ..
		"image_button[7.75,0.25;0.75,0.75;" .. core.formspec_escape(defaulttexturedir .. "refresh.png")
			.. ";btn_mp_refresh;]" ..
		"tooltip[btn_mp_search;" .. fgettext("Clear") .. "]"..
		"tooltip[btn_mp_search;" .. fgettext("Search") .. "]"..
		"tooltip[btn_mp_refresh;" .. fgettext("Refresh") .. "]"..

		"box[8.625,0;4.5,7;"..mt_color_green.."]"..

		-- Address / Port
		"label[8.75,0.35;" .. fgettext("Address / Port") .. "]" ..
		"field[8.75,0.5;3.25,0.5;te_address;;" ..
			core.formspec_escape(core.settings:get("address")) .. "]" ..
		"field[12,0.5;1,0.5;te_port;;" ..
			core.formspec_escape(core.settings:get("remote_port")) .. "]" ..

		-- Name / Password
		"label[8.75,1.55;" .. fgettext("Name / Password") .. "]" ..
		"field[8.75,1.75;2.75,0.5;te_name;;" ..
			core.formspec_escape(core.settings:get("name")) .. "]" ..
		"pwdfield[11.5,1.75;1.5,0.5;te_pwd;]" ..

		-- Description Background
		"label[8.75,3.05;" .. fgettext("Server Description") .. "]" ..
		"box[8.75,3.25;4.25,2.5;#999999]"..

		-- Connect
		"button[11,6;2,0.75;btn_mp_connect;" .. fgettext("Connect") .. "]"

	if tabdata.fav_selected and fav_selected then
		if gamedata.fav then
			retval = retval .. "button[8.75,6;2,0.75;btn_delete_favorite;" ..
				fgettext("Del. Favorite") .. "]"
		end
		if fav_selected.description then
			retval = retval .. "textarea[8.75,3.25;4.25,2.5;;;" ..
				core.formspec_escape(gamedata.serverdescription or "") .. "]"
		end
	end

	retval = retval .. "tablecolumns[" ..
		"image,tooltip=" .. fgettext("Ping") .. "," ..
		"0=" .. core.formspec_escape(defaulttexturedir .. "blank.png") .. "," ..
		"1=" .. core.formspec_escape(defaulttexturedir .. "server_ping_4.png") .. "," ..
		"2=" .. core.formspec_escape(defaulttexturedir .. "server_ping_3.png") .. "," ..
		"3=" .. core.formspec_escape(defaulttexturedir .. "server_ping_2.png") .. "," ..
		"4=" .. core.formspec_escape(defaulttexturedir .. "server_ping_1.png").. "," ..
		"5=" .. core.formspec_escape(defaulttexturedir .. "server_flags_favorite.png") .. "," ..
		"6=" .. core.formspec_escape(defaulttexturedir .. "server_divider_discover.png") .. "," ..
		"7=" .. core.formspec_escape(defaulttexturedir .. "server_divider_incompatible.png") .. "," ..
		"padding=0.25;"..
		"color,span=1;" ..
		"text,align=inline;"..
		"color,span=1;" ..
		"text,align=inline,width=4;" ..
		"image,tooltip=" .. fgettext("Gamemode") .. "," ..
		"0=" .. core.formspec_escape(defaulttexturedir .. "blank.png") .. "," ..
		"1=" .. core.formspec_escape(defaulttexturedir .."server_flags_creative.png") .. "," ..
		"2=" .. core.formspec_escape(defaulttexturedir .."server_flags_damage.png") .. "," ..
		"3=" .. core.formspec_escape(defaulttexturedir .."server_flags_pvp.png") .. "," ..
		"align=inline,padding=1,width=2;"..
		"color,align=inline,span=1;" ..
		"text,align=inline,padding=1]" ..
		"table[0.25,1;8.25,5.75;servers;"

	local dividers = {
		fav = "5,#ffff00," .. fgettext("Favorite Servers") .. ",,,0,,",
		discover = "6,"..mt_color_green.."," .. fgettext("Discover Servers") .. ",,,0,,",
		incompatible = "7,"..mt_color_grey.."," .. fgettext("Incompatible Servers") .. ",,,0,,"
	}
	local servers = {
		fav = {},
		discover = {},
		incompatible = {}
	}
	local favs = core.get_favorites("local")
	local taken_favs = {}
	local result = menudata.search_result or menudata.favorites
	for _, server in ipairs(result) do
		for index, fav in ipairs(favs) do
			if server.address == fav.address and server.port == fav.port then
				taken_favs[index] = true
				server.is_favorite = true
				break
			end
		end
		server.is_compatible = is_server_protocol_compat(server.proto_min, server.proto_max)
		if server.is_favorite then
			table.insert(servers.fav, server)
		elseif server.is_compatible then
			table.insert(servers.discover, server)
		else
			table.insert(servers.incompatible, server)
		end
	end

	if not menudata.search_result then
		for index, fav in ipairs(favs) do
			if not taken_favs[index] then
				table.insert(servers.fav, fav)
			end
		end
	end

	local order = {
		"fav", "discover", "incompatible"
	}

	server_lookup = {}
	local rows = {}
	for _, section in ipairs(order) do
		local section_servers = servers[section]
		if next(section_servers) then
			table.insert(server_lookup, false)
			table.insert(rows, dividers[section])
			for _, server in ipairs(section_servers) do
				table.insert(server_lookup, server)
				table.insert(rows, render_serverlist_row(server))
			end
		end
	end

	retval = retval..table.concat(rows, ",")

	if tabdata.fav_selected then
		retval = retval .. ";" .. tabdata.fav_selected .. "]"
	else
		retval = retval .. ";0]"
	end

	return retval, "size[13.125,7,false]real_coordinates[true]"
end

local function main_button_handler(tabview, fields, name, tabdata)
	local serverlist = menudata.search_result or menudata.favorites

	if fields.te_name then
		gamedata.playername = fields.te_name
		core.settings:set("name", fields.te_name)
	end

	if fields.servers then
		local event = core.explode_table_event(fields.servers)
		local server = server_lookup[event.row]

		if server then
			if event.type == "DCL" then
				if menudata.favorites_is_public and
						not is_server_protocol_compat_or_error(
							server.proto_min, server.proto_max) then
					return true
				end

				gamedata.address    = server.address
				gamedata.port       = server.port
				gamedata.playername = fields.te_name
				gamedata.selected_world = 0

				if fields.te_pwd then
					gamedata.password = fields.te_pwd
				end

				gamedata.servername        = server.name
				gamedata.serverdescription = server.description

				if gamedata.address and gamedata.port then
					core.settings:set("address", gamedata.address)
					core.settings:set("remote_port", gamedata.port)
					core.start()
				end
				return true
			end
			if event.type == "CHG" then
				gamedata.fav = false
				local favs = core.get_favorites("local")
				local address = server.address
				local port    = server.port
				gamedata.serverdescription = server.description

				for _, fav in ipairs(favs) do
					if server.address == fav.address and
							server.port == fav.port then
						gamedata.fav = true
					end
				end

				if address and port then
					core.settings:set("address", address)
					core.settings:set("remote_port", port)
				end
				tabdata.fav_selected = event.row
				return true
			end
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

	if fields.btn_mp_clear then
		tabdata.search_for = ""
		menudata.search_result = nil
		return true
	end

	if fields.btn_mp_search or fields.key_enter_field == "te_search" then
		tabdata.fav_selected = 1
		local input = fields.te_search:lower()
		tabdata.search_for = fields.te_search

		if #menudata.favorites < 2 then
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
		for i = 1, #menudata.favorites do
			local server = menudata.favorites[i]
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
				local points = (#menudata.favorites - i) / 5 + found
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
		asyncOnlineFavourites()
		return true
	end

	if (fields.btn_mp_connect or fields.key_enter)
			and fields.te_address ~= "" and fields.te_port then
		gamedata.playername = fields.te_name
		gamedata.password   = fields.te_pwd
		gamedata.address    = fields.te_address
		gamedata.port       = fields.te_port
		gamedata.selected_world = 0
		local fav_idx = core.get_table_index("favourites")
		local fav = serverlist[fav_idx]

		if fav_idx and fav_idx <= #serverlist and
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

		core.settings:set("address",     fields.te_address)
		core.settings:set("remote_port", fields.te_port)

		core.start()
		return true
	end
	return false
end

local function on_change(type, old_tab, new_tab)
	if type == "LEAVE" then return end
	asyncOnlineFavourites()
end

return {
	name = "online",
	caption = fgettext("Join Game"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
