--Minetest
--Copyright (C) 2020 rubenwardy
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

serverlistmgr = {}

--------------------------------------------------------------------------------
local function order_favorite_list(list)
	local res = {}
	--orders the favorite list after support
	for i = 1, #list do
		local fav = list[i]
		if is_server_protocol_compat(fav.proto_min, fav.proto_max) then
			res[#res + 1] = fav
		end
	end
	for i = 1, #list do
		local fav = list[i]
		if not is_server_protocol_compat(fav.proto_min, fav.proto_max) then
			res[#res + 1] = fav
		end
	end
	return res
end

local public_downloading = false

--------------------------------------------------------------------------------
function serverlistmgr.sync()
	if not serverlistmgr.servers then
		serverlistmgr.servers = {{
			name = fgettext("Loading..."),
			description = fgettext_ne("Try reenabling public serverlist and check your internet connection.")
		}}
	end

	if public_downloading then
		return
	end
	public_downloading = true

	core.handle_async(
		function(param)
			local http = core.get_http_api()
			local url = ("%s/list?proto_version_min=%d&proto_version_max=%d"):format(
				core.settings:get("serverlist_url"),
				core.get_min_supp_proto(),
				core.get_max_supp_proto())

			local response = http.fetch_sync({ url = url })
			if not response.succeeded then
				return {}
			end

			local retval = core.parse_json(response.data)
			return retval and retval.list or {}
		end,
		nil,
		function(result)
			public_downloading = nil
			local favs = order_favorite_list(result)
			if favs[1] then
				serverlistmgr.servers = favs
			end
			core.event_handler("Refresh")
		end
	)
end

--------------------------------------------------------------------------------
local function get_favorites_path()
	local base = core.get_user_path() .. DIR_DELIM .. "client" .. DIR_DELIM .. "serverlist" .. DIR_DELIM
	return base .. core.settings:get("serverlist_file")
end

--------------------------------------------------------------------------------
local function save_favorites(favorites)
	local filename = core.settings:get("serverlist_file")
	if filename:sub(#filename - 3):lower() == ".txt" then
		core.settings:set("serverlist_file", filename:sub(1, #filename - 4) .. ".json")
	end

	local path = get_favorites_path()
	core.create_dir(path)
	core.safe_file_write(path, core.write_json(favorites))
end

--------------------------------------------------------------------------------
local function read_favorites()
	local path = get_favorites_path()

	if path:sub(#path - 4):lower() == ".json" then
		local file = io.open(path, "r")
		if file then
			local json = file:read("*all")
			file:close()
			return core.parse_json(json)
		end

		path = path:sub(1, #path - 5) .. ".txt"
	end

	local file = io.open(path, "r")
	if file then
		local lines = {}
		for line in file:lines() do
			lines[#lines + 1] = line
		end
		file:close()

		local favorites = {}

		local i = 1
		while i < #lines do
			local function pop()
				local line = lines[i]
				i = i + 1
				return line and line:trim()
			end

			if pop():lower() == "[server]" then
				local name = pop()
				local address = pop()
				local port = tonumber(pop())
				local description = pop()

				if name == "" then
					name = nil
				end

				if description == "" then
					description = nil
				end

				if not address or #address < 3 then
					core.log("warning", "Malformed favorites file, missing address at line " .. i)
				end
				if not port or port < 1 or port > 65535 then
					core.log("warning", "Malformed favorites file, missing port at line " .. i)
				end
				if (name and name:upper() == "[SERVER]") or
						(address and address:upper() == "[SERVER]") or
						(description and description:upper() == "[SERVER]") then
					core.log("warning", "Potentially malformed favorites file, overran at line " .. i)
				end

				favorites[#favorites + 1] = {
					name = name,
					address = address,
					port = port,
					description = description
				}
			end
		end

		return favorites
	end

	return nil
end

--------------------------------------------------------------------------------
local function delete_favorite(favorites, current_favorite)
	for i=1, #favorites do
		local fav = favorites[i]
		if fav.address == current_favorite.address and fav.port == current_favorite.port then
			table.remove(favorites, i)
			return
		end
	end

	core.log("warning", "Could not delete favorite, it was not found.")
end

--------------------------------------------------------------------------------
function serverlistmgr.get_favorites()
	if serverlistmgr.favorites then
		return serverlistmgr.favorites
	end

	serverlistmgr.favorites = read_favorites()
	return serverlistmgr.favorites or {}
end

--------------------------------------------------------------------------------
function serverlistmgr.add_favorite(current_favorite)
	local favorites = serverlistmgr.get_favorites()
	delete_favorite(favorites, current_favorite)
	table.insert(favorites, {
		name = current_favorite.name,
		address = current_favorite.address,
		port = tonumber(current_favorite.port),
		description = current_favorite.description,
	})
	save_favorites(favorites)
end

--------------------------------------------------------------------------------
function serverlistmgr.delete_favorite(current_favorite)
	local favorites = serverlistmgr.get_favorites()
	delete_favorite(favorites, current_favorite)
	save_favorites(favorites)
end
