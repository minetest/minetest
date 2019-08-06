--Minetest
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


local function change_game_formspec(dialogdata)
	gamemenu.set_game(nil)

	local fs = {
		"size[11,11]real_coordinates[true]",
		"style_type[button;border=false]",
		"style_type[image_button;border=false]",
		"bgcolor[#00000000]",
		"button[0,0;11,1;;", fgettext("Select a Game"), "]",
		"container[0,1]", -- TODO: convert to scroll_container when added
	}

	local x = 0
	local y = 0

	local games = pkgmgr.games
	for i=1, #games do
		local game = games[i]
		fs[#fs + 1] = ("container[%f,%f]"):format(x * 2.25, y * 3.05)

		local icon = game.menuicon_path or ""

		fs[#fs + 1] = "image_button[0,0;2,2;"
		fs[#fs + 1] = icon
		fs[#fs + 1] = ";game_"
		fs[#fs + 1] = game.id
		fs[#fs + 1] = ";]"

		fs[#fs + 1] = "button[0,2;2,0.8;game_"
		fs[#fs + 1] = game.id
		fs[#fs + 1] = ";"
		fs[#fs + 1] = minetest.formspec_escape(game.name)
		fs[#fs + 1] = "]"

		fs[#fs + 1] = "container_end[]"

		if x < 4 then
			x = x + 1
		else
			x = 0
			y = y + 1
		end

		if y > 2 then
			break
		end
	end

	fs[#fs + 1] = "container_end[]"

	fs[#fs + 1] = "container[0,9]"
	fs[#fs + 1] = "box[0,0;11,1;#53AC56CC]"
	fs[#fs + 1] = "label[0.3,0.5;"
	fs[#fs + 1] = fgettext("You can install more games from the content repository")
	fs[#fs + 1] = "]container_end[]"


	fs[#fs + 1] = "container[0,10.2]style_type[button;border=true]"

	fs[#fs + 1] = "button[0,0;3,0.8;play_online;"
	fs[#fs + 1] = fgettext("Play Online")
	fs[#fs + 1] = "]"

	fs[#fs + 1] = "button[4,0;3,0.8;content;"
	fs[#fs + 1] = fgettext("Content")
	fs[#fs + 1] = "]"

	fs[#fs + 1] = "button[8,0;3,0.8;settings;"
	fs[#fs + 1] = fgettext("Settings")
	fs[#fs + 1] = "]"

	fs[#fs + 1] = "container_end[]"

	return table.concat(fs, "")
end

local function change_game_buttonhandler(this, fields)
	local games = pkgmgr.games
	for i=1, #games do
		if fields["game_" .. games[i].id] then
			gamemenu.set_game(games[i])
			this:delete()
			return true
		end
	end
	return false
end

function change_game_dlg()
	return dialog_create("change_game",
			change_game_formspec,
			change_game_buttonhandler,
			nil)
end
