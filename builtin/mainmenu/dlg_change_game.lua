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


local function render_button(fs, game, w, h)
	local icon = game.menuicon_path or ""
	local key = "game_" .. game.id

	local info = core.get_content_info(game.path)

	fs[#fs + 1] = ("box[0,0;%f,%f;#333]"):format(w, h)

	--fs[#fs + 1] = ("box[%f,%f;%f,%f;#f00]"):format(0.25, 0.25, h - 0.5, h - 0.5, icon)
	fs[#fs + 1] = ("image[%f,%f;%f,%f;%s]"):format(0.25, 0.25, h - 0.5, h - 0.5, icon)

	fs[#fs + 1] = "label["
	fs[#fs + 1] = tostring(h)
	fs[#fs + 1] = ",0.4;"
	fs[#fs + 1] = minetest.formspec_escape(game.name)
	fs[#fs + 1] = "]"

	--fs[#fs + 1] = ("box[%f,%f;%f,%f;#f00]"):format(h, 0.65, w - h - 0.25, h - 0.9)
	fs[#fs + 1] = ("textarea[%f,%f;%f,%f;;;%s]"):format(h, 0.65, w - h - 0.25, h - 0.9, info.description or "")

	fs[#fs + 1] = "style["
	fs[#fs + 1] = key
	fs[#fs + 1] = (";border=false;bgimg=%s;bgimg_hovered=%s;bgimg_pressed=%s]"):format(
			defaulttexturedir .. "blank.png",
			defaulttexturedir .. "highlight.png",
			defaulttexturedir .. "highlight.png")

	fs[#fs + 1] = ("button[0,0;%f,%f;%s;]"):format(w, h, key)
end

local function change_game_formspec(dialogdata)
	gamemenu.set_game(nil)

	local fs = {
		"formspec_version[3]size[13,11]",
		"bgcolor[#00000000]",
		"style[title;border=false]",
		"button[0,0;13,1;title;", fgettext("Select a Game"), "]",
		"container[0,1]", -- TODO: convert to scroll_container when added
	}

	local x = 0
	local y = 0

	local games = pkgmgr.games
	for i=1, #games do
		fs[#fs + 1] = ("container[%f,%f]"):format(x * 6.625, y * 2)
		render_button(fs, games[i], 6.375, 1.75)
		fs[#fs + 1] = "container_end[]"

		if x < 1 then
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

	--fs[#fs + 1] = "container[0,9]"
	--fs[#fs + 1] = "box[0,0;11,1;#53AC56CC]"
	--fs[#fs + 1] = "label[0.3,0.5;"
	--fs[#fs + 1] = fgettext("You can install more games from the content repository")
	--fs[#fs + 1] = "]container_end[]"


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
