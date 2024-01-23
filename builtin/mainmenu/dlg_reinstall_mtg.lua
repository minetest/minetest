--Minetest
--Copyright (C) 2023 Gregor Parzefall
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

function check_reinstall_mtg()
	if core.settings:get_bool("no_mtg_notification") then
		return
	end

	local games = core.get_games()
	for _, game in ipairs(games) do
		if game.id == "minetest" then
			core.settings:set_bool("no_mtg_notification", true)
			return
		end
	end

	local mtg_world_found = false
	local worlds = core.get_worlds()
	for _, world in ipairs(worlds) do
		if world.gameid == "minetest" then
			mtg_world_found = true
			break
		end
	end
	if not mtg_world_found then
		core.settings:set_bool("no_mtg_notification", true)
		return
	end

	local maintab = ui.find_by_name("maintab")

	local dlg = create_reinstall_mtg_dlg()
	dlg:set_parent(maintab)
	maintab:hide()
	dlg:show()
	ui.update()
end

local function get_formspec(dialogdata)
	local markup = table.concat({
		"<big>", fgettext("Minetest Game is no longer installed by default"), "</big>\n",
		fgettext("For a long time, the Minetest engine shipped with a default game called \"Minetest Game\". " ..
				"Since Minetest 5.8.0, Minetest ships without a default game."), "\n",
		fgettext("If you want to continue playing in your Minetest Game worlds, you need to reinstall Minetest Game."),
	})

	return table.concat({
		"formspec_version[6]",
		"size[12.8,7]",
		"hypertext[0.375,0.375;12.05,5.2;text;", minetest.formspec_escape(markup), "]",
		"container[0.375,5.825]",
		"style[dismiss;bgcolor=red]",
		"button[0,0;4,0.8;dismiss;", fgettext("Dismiss"), "]",
		"button[4.25,0;8,0.8;reinstall;", fgettext("Reinstall Minetest Game"), "]",
		"container_end[]",
	})
end

local function buttonhandler(this, fields)
	if fields.reinstall then
		-- Don't set "no_mtg_notification" here so that the dialog will be shown
		-- again if downloading MTG fails for whatever reason.
		this:delete()

		local maintab = ui.find_by_name("maintab")

		local dlg = create_store_dlg(nil, "minetest/minetest")
		dlg:set_parent(maintab)
		maintab:hide()
		dlg:show()

		return true
	end

	if fields.dismiss then
		core.settings:set_bool("no_mtg_notification", true)
		this:delete()
		return true
	end
end

local function eventhandler(event)
	if event == "DialogShow" then
		mm_game_theme.set_engine()
		return true
	elseif event == "MenuQuit" then
		-- Don't allow closing the dialog with ESC, but still allow exiting
		-- Minetest.
		core.close()
		return true
	end
	return false
end

function create_reinstall_mtg_dlg()
	local dlg = dialog_create("dlg_reinstall_mtg", get_formspec,
			buttonhandler, eventhandler)
	return dlg
end


