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


mm_game_theme = {}

--------------------------------------------------------------------------------
function mm_game_theme.init()
	mm_game_theme.defaulttexturedir = core.get_texturepath_share() .. DIR_DELIM .. "base" ..
						DIR_DELIM .. "pack" .. DIR_DELIM
	mm_game_theme.basetexturedir = mm_game_theme.defaulttexturedir

	mm_game_theme.texturepack = core.settings:get("texture_path")

	mm_game_theme.gameid = nil

	mm_game_theme.music_handle = nil
end

--------------------------------------------------------------------------------
function mm_game_theme.update(tab,gamedetails)
	if tab ~= "singleplayer" then
		mm_game_theme.reset()
		return
	end

	if gamedetails == nil then
		return
	end

	mm_game_theme.update_game(gamedetails)
end

--------------------------------------------------------------------------------
function mm_game_theme.reset()
	mm_game_theme.gameid = nil
	local have_bg      = false
	local have_overlay = mm_game_theme.set_generic("overlay")

	if not have_overlay then
		have_bg = mm_game_theme.set_generic("background")
	end

	mm_game_theme.clear("header")
	mm_game_theme.clear("footer")
	core.set_clouds(false)

	mm_game_theme.set_generic("footer")
	mm_game_theme.set_generic("header")

	if not have_bg then
		if core.settings:get_bool("menu_clouds") then
			core.set_clouds(true)
		else
			mm_game_theme.set_dirt_bg()
		end
	end

	if mm_game_theme.music_handle ~= nil then
		core.sound_stop(mm_game_theme.music_handle)
	end
end

--------------------------------------------------------------------------------
function mm_game_theme.update_game(gamedetails)
	if mm_game_theme.gameid == gamedetails.id then
		return
	end

	local have_bg      = false
	local have_overlay = mm_game_theme.set_game("overlay",gamedetails)

	if not have_overlay then
		have_bg = mm_game_theme.set_game("background",gamedetails)
	end

	mm_game_theme.clear("header")
	mm_game_theme.clear("footer")
	core.set_clouds(false)

	if not have_bg then

		if core.settings:get_bool("menu_clouds") then
			core.set_clouds(true)
		else
			mm_game_theme.set_dirt_bg()
		end
	end

	mm_game_theme.set_game("footer",gamedetails)
	mm_game_theme.set_game("header",gamedetails)

	mm_game_theme.gameid = gamedetails.id
end

--------------------------------------------------------------------------------
function mm_game_theme.clear(identifier)
	core.set_background(identifier,"")
end

--------------------------------------------------------------------------------
function mm_game_theme.set_generic(identifier)
	--try texture pack first
	if mm_game_theme.texturepack ~= nil then
		local path = mm_game_theme.texturepack .. DIR_DELIM .."menu_" ..
										identifier .. ".png"
		if core.set_background(identifier,path) then
			return true
		end
	end

	if mm_game_theme.defaulttexturedir ~= nil then
		local path = mm_game_theme.defaulttexturedir .. DIR_DELIM .."menu_" ..
										identifier .. ".png"
		if core.set_background(identifier,path) then
			return true
		end
	end

	return false
end

--------------------------------------------------------------------------------
function mm_game_theme.set_game(identifier, gamedetails)

	if gamedetails == nil then
		return false
	end

	mm_game_theme.set_music(gamedetails)

	if mm_game_theme.texturepack ~= nil then
		local path = mm_game_theme.texturepack .. DIR_DELIM ..
			gamedetails.id .. "_menu_" .. identifier .. ".png"
		if core.set_background(identifier, path) then
			return true
		end
	end

	-- Find out how many randomized textures the game provides
	local n = 0
	local filename
	local menu_files = core.get_dir_list(gamedetails.path .. DIR_DELIM .. "menu", false)
	for i = 1, #menu_files do
		filename = identifier .. "." .. i .. ".png"
		if table.indexof(menu_files, filename) == -1 then
			n = i - 1
			break
		end
	end
	-- Select random texture, 0 means standard texture
	n = math.random(0, n)
	if n == 0 then
		filename = identifier .. ".png"
	else
		filename = identifier .. "." .. n .. ".png"
	end

	local path = gamedetails.path .. DIR_DELIM .. "menu" ..
		DIR_DELIM .. filename
	if core.set_background(identifier, path) then
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function mm_game_theme.set_dirt_bg()
	if mm_game_theme.texturepack ~= nil then
		local path = mm_game_theme.texturepack .. DIR_DELIM .."default_dirt.png"
		if core.set_background("background", path, true, 128) then
			return true
		end
	end

	-- Use universal fallback texture in textures/base/pack
	local minimalpath = defaulttexturedir .. "menu_bg.png"
	core.set_background("background", minimalpath, true, 128)
end

--------------------------------------------------------------------------------
function mm_game_theme.set_music(gamedetails)
	if mm_game_theme.music_handle ~= nil then
		core.sound_stop(mm_game_theme.music_handle)
	end
	local music_path = gamedetails.path .. DIR_DELIM .. "menu" .. DIR_DELIM .. "theme"
	mm_game_theme.music_handle = core.sound_play(music_path, true)
end
