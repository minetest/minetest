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

local function create_world_formspec(dialog_data)
	local values = {
		seed = core.settings:get("fixed_map_seed") or "",

		-- The ID which is also the name
		map_generator = core.settings:get("mg_name"),

		-- The ID
		game = core.settings:get("menu_last_game")
	}

	local map_generator_textlist = {}
	local map_generator_index = 1
	for i, name in ipairs(core.get_mapgen_names()) do
		if name == values.map_generator then
			map_generator_index = i
		end
		table.insert(map_generator_textlist, core.formspec_escape(name))
	end

	-- Cannot use gamemgr.gamelist() as it doesn't escape game names
	local game_dropdown = {}
	local game_index = 1
	for i, game in ipairs(gamemgr.games) do
		if game.id == values.game then
			game_index = i
		end
		table.insert(game_dropdown, core.formspec_escape(game.name))
	end

	local formspec =
		"size[11.5,6.5,true]" ..

		"label[2,0;" .. fgettext("World name") .. "]" ..
		"field[4.5,0.4;6,0.5;name;;]" ..

		"label[2,1;" .. fgettext("Seed") .. "]" ..
		"field[4.5,1.4;6,0.5;seed;;".. core.formspec_escape(values.seed) .. "]" ..

		"label[2,2;" .. fgettext("Mapgen") .. "]" ..
		"dropdown[4.2,2;6.3;map_generator;" ..
		table.concat(map_generator_textlist, ",") .. ";" ..
		map_generator_index .. "]" ..

		"label[2,3;" .. fgettext("Game") .. "]" ..
		"textlist[4.2,3;5.8,2.3;game;" ..
		table.concat(game_dropdown, ",") .. ";" ..
		game_index .. ";true]" ..

		"button[3.25,6;2.5,0.5;create;" .. fgettext("Create") .. "]" ..
		"button[5.75,6;2.5,0.5;cancel;" .. fgettext("Cancel") .. "]"
		
	if #gamemgr.games == 0 then
		formspec = formspec ..
			"box[2,4;8,1;#ff8800]" ..

			"label[2.25,4;" ..
			fgettext("You have no subgames installed.") .. "]" ..

			"label[2.25,4.4;" ..
			fgettext("Download one from minetest.net") .. "]"
	elseif #gamemgr.games == 1 and gamemgr.games[1].id == "minimal" then
		formspec = formspec ..
			"box[1.75,4;8.7,1;#ff8800]" ..

			"label[2,4;" ..
			fgettext("Warning: The minimal development test is meant for developers.") .. "]" ..

			"label[2,4.4;" ..
			fgettext("Download a subgame, such as minetest_game, from minetest.net") .. "]"
	end
	return formspec
end


local function create_world_button_handler(this, fields)
	local buttons = {
		create = fields["create"] or fields["key_enter"],
		cancel = fields["cancel"]
	}
	local values = {
		name = fields["name"],
		game_index = core.get_textlist_index("game"),
		seed = fields["seed"],
		map_generator = fields["map_generator"]
	}

	if values.name == "" then
		values.name = fgettext_ne("Unnamed")
	end
	while menudata.worldlist:uid_exists_raw(values.name) do
		local words = {}
		for word in string.gmatch(values.name, "[^ ]+") do
			table.insert(words, word)
		end
		local number = tonumber(words[#words])
		if number == nil then
			values.name = values.name .. " 2"
		else
			words[#words] = tostring(number + 1)
			values.name = table.concat(words, " ")
		end
	end

	if buttons.create then
		if values.game_index == nil then
			gamedata.errormessage =
				fgettext("No game selected")
			return true
		end

		core.settings:set("fixed_map_seed", values.seed)
		core.settings:set("mg_name", values.map_generator)
		local create_world_error =
			core.create_world(values.name, values.game_index)
		if create_world_error ~= nil then
			gamedata.errormessage = create_world_error
			return true
		end

		local game_id = gamemgr.games[values.game_index].id
		core.settings:set("menu_last_game", game_id)
		if this.data.update_worldlist_filter then
			menudata.worldlist:set_filtercriteria(game_id)
			mm_texture.update("singleplayer", game_id)
		end
		menudata.worldlist:refresh()
		local raw_worldlist_index =
			menudata.worldlist:raw_index_by_uid(values.name);
		core.settings:set(
			"mainmenu_last_selected_world",
			raw_worldlist_index
		)
		this:delete()
		return true
	end

	if values.game then
		return true
	end
	
	if buttons.cancel then
		this:delete()
		return true
	end

	return false
end


function create_create_world_dlg(update_worldlist_filter)
	local dialog = dialog_create(
		"sp_create_world",
		create_world_formspec,
		create_world_button_handler,
		nil
	)
	dialog.update_worldlist_filter = update_worldlist_filter
	return dialog
end
