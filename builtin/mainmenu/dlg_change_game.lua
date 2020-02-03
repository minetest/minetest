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

	fs[#fs + 1] = ("button[0,0;%f,%f;bg;]"):format(w, h)

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
		"formspec_version[3]",
		"size[9.75,10]",
		"style[title;border=false]",
		"button[0,0;9.75,1;title;", fgettext("Select a Game"), "]",
		"scrollbar[8.875,1;0.5,7.625;vertical;scrollbar;0]",
		"container[0.375,1]",
	}

	-- TODO: convert to scroll_container when added
	-- this will also allow removing the game limit below

	local games = pkgmgr.games
	for i=1, math.min(4, #games) do
		fs[#fs + 1] = ("container[%f,%f]"):format(0, (i - 1) * 1.75)
		render_button(fs, games[i], 8.25, 1.5)
		fs[#fs + 1] = "container_end[]"
	end

	fs[#fs + 1] = "container_end[]"

	if #games <= 1 then
		fs[#fs + 1] = "container[0,7.575]"
		fs[#fs + 1] = "box[0,0;11,1;#53AC56CC]"
		fs[#fs + 1] = "label[0.3,0.5;"
		fs[#fs + 1] = fgettext("You can install more games from the content repository")
		fs[#fs + 1] = "]container_end[]"
	end

	fs[#fs + 1] = "container[0.375,8.825]style_type[button;border=true]"

	local num_buttons = 3
	local w = (9.75 - 2*0.375 - (num_buttons - 1) * 0.25) / num_buttons

	fs[#fs + 1] = "button[0,0;"
	fs[#fs + 1] = tostring(w)
	fs[#fs + 1] = ",0.8;play_online;"
	fs[#fs + 1] = fgettext("Play Online")
	fs[#fs + 1] = "]"

	fs[#fs + 1] = "button["
	fs[#fs + 1] = tostring(w + 0.25)
	fs[#fs + 1] = ",0;"
	fs[#fs + 1] = tostring(w)
	fs[#fs + 1] = ",0.8;content;"
	fs[#fs + 1] = fgettext("Content")
	fs[#fs + 1] = "]"

	fs[#fs + 1] = "button["
	fs[#fs + 1] = tostring(2 *(w + 0.25))
	fs[#fs + 1] = ",0;"
	fs[#fs + 1] = tostring(w)
	fs[#fs + 1] =",0.8;settings;"
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
