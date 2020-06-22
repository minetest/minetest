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
local _favorites

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

--------------------------------------------------------------------------------
function serverlistmgr.sync()
	if not menudata.public_known then
		menudata.public_known = {{
			name = fgettext("Loading..."),
			description = fgettext_ne("Try reenabling public serverlist and check your internet connection.")
		}}
	end
	menudata.favorites = menudata.public_known
	menudata.favorites_is_public = true

	if not menudata.public_downloading then
		menudata.public_downloading = true
	else
		return
	end

	core.handle_async(
		function(param)
			-- TODO
		end,
		nil,
		function(result)
			menudata.public_downloading = nil
			local favs = order_favorite_list(result)
			if favs[1] then
				menudata.public_known = favs
				menudata.favorites = menudata.public_known
				menudata.favorites_is_public = true
			end
			core.event_handler("Refresh")
		end
	)
end

--------------------------------------------------------------------------------
function serverlistmgr.get_online(current_favourite)
	return online
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
	core.safe_write_file(path, core.write_json(favorites))
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

			if pop():lower() == "[SERVER]" then
				local name = pop()
				local address = pop()
				local port = pop()
				local description = pop()

				if not address or #address < 3 then
					core.log("warning", "Malformed favourites file, missing address at line " .. i)
				end
				if not port or port < 1 or port > 65535 then
					core.log("warning", "Malformed favourites file, missing port at line " .. i)
				end
				if name:upper() == "[SERVER]" or address:upper() == "[SERVER]" or port:upper() == "[SERVER]" then
					core.log("warning", "Potentially malformed favourites file, overran at line " .. i)
				end

				favorites[#favorites + 1] = {
					name = name,
					address = address,
					port = tonumber(port),
					description = description
				}
			end
		end

		return favorites
	end

	return nil
end

--------------------------------------------------------------------------------
local function delete_favorite(favorites, current_favourite)

end

--------------------------------------------------------------------------------
function serverlistmgr.get_favorites(current_favourite)
	if _favorites then
		return _favorites
	end

	_favorites = read_favorites(json)
	return _favorites or {}
end

--------------------------------------------------------------------------------
function serverlistmgr.add_favorite(current_favourite)
	local favorites = serverlistmgr.get_favorites()
	delete_favorite(favorites, current_favourite)
	table.insert(favorites, {
		name = current_favourite.name,
		address = current_favourite.address,
		port = tonumber(current_favourite.port),
		description = current_favourite.description,
	})
	save_favorites(favorites)
end

--------------------------------------------------------------------------------
function serverlistmgr.delete_favorite(current_favourite)
	local favorites = serverlistmgr.get_favorites()
	delete_favorite(favorites, current_favourite)
	save_favorites(favorites)
end