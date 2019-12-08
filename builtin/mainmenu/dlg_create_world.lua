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

local worldname = ""

local function create_world_formspec(dialogdata)
	local mapgens = core.get_mapgen_names()

	local current_seed = core.settings:get("fixed_map_seed") or ""
	local current_mg   = core.settings:get("mg_name")
	local gameid = core.settings:get("menu_last_game")

	local gameidx = 0
	if gameid ~= nil then
		local _
		_, gameidx = pkgmgr.find_by_gameid(gameid)

		if gameidx == nil then
			gameidx = 0
		end
	end

	local game_by_gameidx = core.get_game(gameidx)
	if game_by_gameidx ~= nil then
		local gamepath = game_by_gameidx.path
		local gameconfig = Settings(gamepath.."/game.conf")

		local disallowed_mapgens = (gameconfig:get("disallowed_mapgens") or ""):split()
		for key, value in pairs(disallowed_mapgens) do
			disallowed_mapgens[key] = value:trim()
		end

		if disallowed_mapgens then
			for i = #mapgens, 1, -1 do
				if table.indexof(disallowed_mapgens, mapgens[i]) > 0 then
					table.remove(mapgens, i)
				end
			end
		end
	end

	local mglist = ""
	local selindex = 1
	local i = 1
	for k,v in pairs(mapgens) do
		if current_mg == v then
			selindex = i
		end
		i = i + 1
		mglist = mglist .. v .. ","
	end
	mglist = mglist:sub(1, -2)

	current_seed = core.formspec_escape(current_seed)
	local retval =
		"size[11.5,6.5,true]" ..
		"label[2,0;" .. fgettext("World name") .. "]"..
		"field[4.5,0.4;6,0.5;te_world_name;;" .. minetest.formspec_escape(worldname) .. "]" ..

		"label[2,1;" .. fgettext("Seed") .. "]"..
		"field[4.5,1.4;6,0.5;te_seed;;".. current_seed .. "]" ..

		"label[2,2;" .. fgettext("Mapgen") .. "]"..
		"dropdown[4.2,2;6.3;dd_mapgen;" .. mglist .. ";" .. selindex .. "]" ..

		"label[2,3;" .. fgettext("Game") .. "]"..
		"textlist[4.2,3;5.8,2.3;games;" .. pkgmgr.gamelist() ..
		";" .. gameidx .. ";true]" ..

		"button[3.25,6;2.5,0.5;world_create_confirm;" .. fgettext("Create") .. "]" ..
		"button[5.75,6;2.5,0.5;world_create_cancel;" .. fgettext("Cancel") .. "]"

	if #pkgmgr.games == 0 then
		retval = retval .. "box[2,4;8,1;#ff8800]label[2.25,4;" ..
				fgettext("You have no games installed.") .. "]label[2.25,4.4;" ..
				fgettext("Download one from minetest.net") .. "]"
	elseif #pkgmgr.games == 1 and pkgmgr.games[1].id == "minimal" then
		retval = retval .. "box[1.75,4;8.7,1;#ff8800]label[2,4;" ..
				fgettext("Warning: The minimal development test is meant for developers.") .. "]label[2,4.4;" ..
				fgettext("Download a game, such as Minetest Game, from minetest.net") .. "]"
	end

	return retval

end

local function create_world_buttonhandler(this, fields)

	if fields["world_create_confirm"] or
		fields["key_enter"] then

		local worldname = fields["te_world_name"]
		local gameindex = core.get_textlist_index("games")

		if gameindex ~= nil then
			if worldname == "" then
				local random_number = math.random(10000, 99999)
				local random_world_name = "Unnamed" .. random_number
				worldname = random_world_name
			end

			core.settings:set("fixed_map_seed", fields["te_seed"])

			local message
			if not menudata.worldlist:uid_exists_raw(worldname) then
				core.settings:set("mg_name",fields["dd_mapgen"])
				message = core.create_world(worldname,gameindex)
			else
				message = fgettext("A world named \"$1\" already exists", worldname)
			end

			if message ~= nil then
				gamedata.errormessage = message
			else
				core.settings:set("menu_last_game",pkgmgr.games[gameindex].id)
				if this.data.update_worldlist_filter then
					menudata.worldlist:set_filtercriteria(pkgmgr.games[gameindex].id)
					mm_texture.update("singleplayer", pkgmgr.games[gameindex].id)
				end
				menudata.worldlist:refresh()
				core.settings:set("mainmenu_last_selected_world",
									menudata.worldlist:raw_index_by_uid(worldname))
			end
		else
			gamedata.errormessage = fgettext("No game selected")
		end
		this:delete()
		return true
	end

	worldname = fields.te_world_name

	if fields["games"] then
		local gameindex = core.get_textlist_index("games")
		core.settings:set("menu_last_game", pkgmgr.games[gameindex].id)
		return true
	end

	if fields["world_create_cancel"] then
		this:delete()
		return true
	end

	return false
end


function create_create_world_dlg(update_worldlistfilter)
	worldname = ""
	local retval = dialog_create("sp_create_world",
					create_world_formspec,
					create_world_buttonhandler,
					nil)
	retval.update_worldlist_filter = update_worldlistfilter

	return retval
end
