--Minetest
--Copyright (C) 2013 sapier
--Copyright (C) 2019 rubenwardy
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


gamemenu = {}

--------------------------------------------------------------------------------
function gamemenu.init()
	gamemenu.defaulttexturedir = core.get_texturepath() .. DIR_DELIM .. "base" ..
						DIR_DELIM .. "pack" .. DIR_DELIM
	gamemenu.basetexturedir = gamemenu.defaulttexturedir

	gamemenu.texturepack = core.settings:get("texture_path")

	gamemenu.set_game(gamemenu.get_game())
end

--------------------------------------------------------------------------------
-- Selected game has changed, unload the current game and apply any properties
--------------------------------------------------------------------------------
function gamemenu.set_game(new_game)
	if type(new_game) == "string" then
		new_game = pkgmgr.find_by_gameid(new_game)
		assert(new_game)
	end

	local new_id = new_game and new_game.id
	if gamemenu.current_id == new_id then
		return
	end

	assert(type(new_id) == "string" or new_id == nil)
	core.settings:set("menu_current_game", new_id or "")
	gamemenu.current_id = new_id

	-- Update world filter list
	menudata.worldlist:set_filtercriteria(new_id)
	menudata.worldlist:refresh()

	gamemenu.update()
end

--------------------------------------------------------------------------------
-- Get current game
--------------------------------------------------------------------------------
function gamemenu.get_game()
	local gameid = core.settings:get("menu_current_game")
	return gameid and pkgmgr.find_by_gameid(gameid)
end

--------------------------------------------------------------------------------
-- Get current game id
--------------------------------------------------------------------------------
function gamemenu.get_game_id()
	return core.settings:get("menu_current_game")
end

--------------------------------------------------------------------------------
-- Reset the current selected game, and all properties set by it
-- This is called before entering engine dialogs such as Change Game
--------------------------------------------------------------------------------
function gamemenu.reset()
	gamemenu.gameid = nil
	gamemenu.update()
end

local function search_random(dir, identifier)
	if not dir then
		return nil
	end

	-- Find out how many randomized textures are provided
	local n = 0
	local menu_files = core.get_dir_list(dir, false)
	for j = 1, #menu_files do
		local filename = identifier .. "." .. j .. ".png"
		if table.indexof(menu_files, filename) == -1 then
			n = j - 1
			break
		end
	end

	-- Select random texture, 0 means standard texture
	n = math.random(0, n)
	if n == 0 then
		if file_exists(dir .. DIR_DELIM .. identifier .. ".png") then
			return dir .. DIR_DELIM  .. identifier .. ".png"
		else
			return nil
		end
	else
		return dir .. DIR_DELIM .. identifier .. "." .. n .. ".png"
	end
end


--------------------------------------------------------------------------------
-- Reset the current selected game, and all properties set by it
-- This is called before entering engine dialogs such as Change Game
--------------------------------------------------------------------------------
function gamemenu.get_properties()
	local properties = {}
	local game = gamemenu.get_game()
	if game then
		local menu_path = game.path .. DIR_DELIM .. "menu"

		local function check_set(identifier)
			properties[identifier] =
					search_random(gamemenu.texturepack, game.id .. "_menu_" .. identifier) or
					search_random(menu_path, identifier)
		end

		check_set("overlay")
		check_set("background")
		check_set("header")
		check_set("footer")
	else
		properties.overlay =
				search_random(gamemenu.texturepack, "menu_overlay")

		properties.background =
				search_random(gamemenu.texturepack, "menu_background")

		properties.header = gamemenu.basetexturedir .. "menu_header.png"
	end

	if properties.clouds == nil then
		properties.clouds = core.settings:get_bool("menu_clouds") and (not properties.background or properties.overlay)
	end

	return properties
end

--------------------------------------------------------------------------------
-- Set properties for selected game or engine
--------------------------------------------------------------------------------
function gamemenu.update()
	local properties = gamemenu.get_properties()
	core.set_background("overlay", properties.overlay or "")
	core.set_background("background", properties.background or "")
	core.set_background("header", properties.header or "")
	core.set_background("footer", properties.footer or "")
	core.set_clouds(properties.clouds)

	if not properties.has_bg and not properties.clouds then
		if gamemenu.texturepack ~= nil then
			local path = gamemenu.texturepack .. DIR_DELIM .."default_dirt.png"
			if core.set_background("background", path, true, 128) then
				return true
			end
		end

		-- Use universal fallback texture in textures/base/pack
		local minimalpath = defaulttexturedir .. "menu_bg.png"
		core.set_background("background", minimalpath, true, 128)
	end
end
