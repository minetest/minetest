--Minetest
--Copyright (C) 2018-24 rubenwardy
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

local function get_formspec(data)
	local selected_game, selected_game_idx = pkgmgr.find_by_gameid(core.settings:get("menu_last_game"))
	if not selected_game_idx then
		selected_game_idx = 1
		selected_game = pkgmgr.games[1]
	end

	local game_list = {}
	for i, game in ipairs(pkgmgr.games) do
		game_list[i] = core.formspec_escape(game.title)
	end

	local package = data.package
	local will_install_deps = data.will_install_deps

	local deps_to_install = 0
	local deps_not_found = 0

	data.dependencies = contentdb.resolve_dependencies(package, selected_game)
	local formatted_deps = {}
	for _, dep in pairs(data.dependencies) do
		formatted_deps[#formatted_deps + 1] = "#fff"
		formatted_deps[#formatted_deps + 1] = core.formspec_escape(dep.name)
		if dep.installed then
			formatted_deps[#formatted_deps + 1] = "#ccf"
			formatted_deps[#formatted_deps + 1] = fgettext("Already installed")
		elseif dep.package then
			formatted_deps[#formatted_deps + 1] = "#cfc"
			formatted_deps[#formatted_deps + 1] = fgettext("$1 by $2", dep.package.title, dep.package.author)
			deps_to_install = deps_to_install + 1
		else
			formatted_deps[#formatted_deps + 1] = "#f00"
			formatted_deps[#formatted_deps + 1] = fgettext("Not found")
			deps_not_found = deps_not_found + 1
		end
	end

	local message_bg = "#3333"
	local message
	if will_install_deps then
		message = fgettext("$1 and $2 dependencies will be installed.", package.title, deps_to_install)
	else
		message = fgettext("$1 will be installed, and $2 dependencies will be skipped.", package.title, deps_to_install)
	end
	if deps_not_found > 0 then
		message = fgettext("$1 required dependencies could not be found.", deps_not_found) ..
				" " .. fgettext("Please check that the base game is correct.", deps_not_found) ..
				"\n" .. message
		message_bg = mt_color_orange
	end

	local formspec = {
		"formspec_version[3]",
		"size[7,7.85]",
		"style[title;border=false]",
		"box[0,0;7,0.5;#3333]",
		"button[0,0;7,0.5;title;", fgettext("Install $1", package.title) , "]",

		"container[0.375,0.70]",

		"label[0,0.25;", fgettext("Base Game:"), "]",
		"dropdown[2,0;4.25,0.5;selected_game;", table.concat(game_list, ","), ";", selected_game_idx, "]",

		"label[0,0.8;", fgettext("Dependencies:"), "]",

		"tablecolumns[color;text;color;text]",
		"table[0,1.1;6.25,3;packages;", table.concat(formatted_deps, ","), "]",

		"container_end[]",

		"checkbox[0.375,5.1;will_install_deps;",
		fgettext("Install missing dependencies"), ";",
		will_install_deps and "true" or "false", "]",

		"box[0,5.4;7,1.2;", message_bg, "]",
		"textarea[0.375,5.5;6.25,1;;;", message, "]",

		"container[1.375,6.85]",
		"button[0,0;2,0.8;install_all;", fgettext("Install"), "]",
		"button[2.25,0;2,0.8;cancel;", fgettext("Cancel"), "]",
		"container_end[]",
	}

	return table.concat(formspec)
end


local function handle_submit(this, fields)
	local data = this.data
	if fields.cancel then
		this:delete()
		return true
	end

	if fields.will_install_deps ~= nil then
		data.will_install_deps = core.is_yes(fields.will_install_deps)
		return true
	end

	if fields.install_all then
		contentdb.queue_download(data.package, contentdb.REASON_NEW)

		if data.will_install_deps then
			for _, dep in pairs(data.dependencies) do
				if not dep.is_optional and not dep.installed and dep.package then
					contentdb.queue_download(dep.package, contentdb.REASON_DEPENDENCY)
				end
			end
		end

		this:delete()
		return true
	end

	if fields.selected_game then
		for _, game in pairs(pkgmgr.games) do
			if game.title == fields.selected_game then
				core.settings:set("menu_last_game", game.id)
				break
			end
		end
		return true
	end

	return false
end


function create_install_dialog(package)
	local dlg = dialog_create("install_dialog", get_formspec, handle_submit, nil)
	dlg.data.dependencies = nil
	dlg.data.package = package
	dlg.data.will_install_deps = true
	return dlg
end
