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

gamemgr = {}

--------------------------------------------------------------------------------
function gamemgr.find_by_gameid(gameid)
	for i=1,#gamemgr.games,1 do
		if gamemgr.games[i].id == gameid then
			return gamemgr.games[i], i
		end
	end
	return nil, nil
end

--------------------------------------------------------------------------------
function gamemgr.get_game_mods(gamespec, retval)
	if gamespec ~= nil and
		gamespec.gamemods_path ~= nil and
		gamespec.gamemods_path ~= "" then
		get_mods(gamespec.gamemods_path, retval)
	end
end

--------------------------------------------------------------------------------
function gamemgr.get_game_modlist(gamespec)
	local retval = ""
	local game_mods = {}
	gamemgr.get_game_mods(gamespec, game_mods)
	for i=1,#game_mods,1 do
		if retval ~= "" then
			retval = retval..","
		end
		retval = retval .. game_mods[i].name
	end
	return retval
end

--------------------------------------------------------------------------------
function gamemgr.get_game(index)
	if index > 0 and index <= #gamemgr.games then
		return gamemgr.games[index]
	end

	return nil
end

--------------------------------------------------------------------------------
function gamemgr.update_gamelist()
	gamemgr.games = core.get_games()
end

--------------------------------------------------------------------------------
function gamemgr.gamelist()
	local retval = ""
	if #gamemgr.games > 0 then
		retval = retval .. gamemgr.games[1].name

		for i=2,#gamemgr.games,1 do
			retval = retval .. "," .. gamemgr.games[i].name
		end
	end
	return retval
end

--------------------------------------------------------------------------------
-- read initial data
--------------------------------------------------------------------------------
gamemgr.update_gamelist()
