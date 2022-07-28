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

-- https://github.com/orgs/minetest/teams/engine/members

local core_developers = {
	"Perttu Ahola (celeron55) <celeron55@gmail.com> [Project founder]",
	"sfan5 <sfan5@live.de>",
	"ShadowNinja <shadowninja@minetest.net>",
	"Nathanaëlle Courant (Nore/Ekdohibs) <nore@mesecons.net>",
	"Loic Blot (nerzhul/nrz) <loic.blot@unix-experience.fr>",
	"Andrew Ward (rubenwardy) <rw@rubenwardy.com>",
	"Krock/SmallJoker <mk939@ymail.com>",
	"Lars Hofhansl <larsh@apache.org>",
	"v-rob <robinsonvincent89@gmail.com>",
	"hecks",
	"Hugues Ross <hugues.ross@gmail.com>",
	"Dmitry Kostenko (x2048) <codeforsmile@gmail.com>",
}

local core_team = {
	"Zughy [Issue triager]",
}

-- For updating active/previous contributors, see the script in ./util/gather_git_credits.py

local active_contributors = {
	"Wuzzy [Features, translations, devtest]",
	"Lars Müller [Lua optimizations and fixes]",
	"Jude Melton-Houghton [Optimizations, bugfixes]",
	"paradust7 [Performance, fixes, Irrlicht refactoring]",
	"Desour [Fixes]",
	"ROllerozxa [Main menu]",
	"savilli [Bugfixes]",
	"Lexi Hale [Particlespawner animation]",
	"Liso [Shadow Mapping]",
	"JosiahWI [Fixes, build system]",
	"numzero [Graphics and rendering]",
	"HybridDog [Fixes]",
	"NeroBurner [Joystick]",
	"pecksin [Clickable web links]",
	"Daroc Alden [Fixes]",
	"Jean-Patrick Guerrero (kilbith) [Fixes]",
}

local previous_core_developers = {
	"BlockMen",
	"Maciej Kasatkin (RealBadAngel) [RIP]",
	"Lisa Milne (darkrose) <lisa@ltmnet.com>",
	"proller",
	"Ilya Zhuravlev (xyz) <xyz@minetest.net>",
	"PilzAdam <pilzadam@minetest.net>",
	"est31 <MTest31@outlook.com>",
	"kahrl <kahrl@gmx.net>",
	"Ryan Kwolek (kwolekr) <kwolekr@minetest.net>",
	"sapier",
	"Zeno",
	"Auke Kok (sofar) <sofar@foo-projects.org>",
	"Aaron Suen <warr1024@gmail.com>",
	"paramat",
	"Pierre-Yves Rollo <dev@pyrollo.com>",
}

local previous_contributors = {
	"Nils Dagsson Moskopp (erlehmann) <nils@dieweltistgarnichtso.net> [Minetest logo]",
	"red-001 <red-001@outlook.ie>",
	"Giuseppe Bilotta",
	"ClobberXD",
	"Dániel Juhász (juhdanad) <juhdanad@gmail.com>",
	"MirceaKitsune <mirceakitsune@gmail.com>",
	"MoNTE48",
	"Constantin Wenger (SpeedProg)",
	"Ciaran Gultnieks (CiaranG)",
	"Paul Ouellette (pauloue)",
	"stujones11",
	"srifqi",
	"Rogier <rogier777@gmail.com>",
	"Gregory Currie (gregorycu)",
	"JacobF",
	"Jeija <jeija@mesecons.net>",
}

local function buildCreditList(source)
	local ret = {}
	for i = 1, #source do
		ret[i] = core.formspec_escape(source[i])
	end
	return table.concat(ret, ",,")
end

return {
	name = "about",
	caption = fgettext("About"),
	cbf_formspec = function(tabview, name, tabdata)
		local logofile = defaulttexturedir .. "logo.png"
		local version = core.get_version()
		local fs = "image[0.75,0.5;2.2,2.2;" .. core.formspec_escape(logofile) .. "]" ..
			"style[label_button;border=false]" ..
			"button[0.5,2;2.5,2;label_button;" .. version.project .. " " .. version.string .. "]" ..
			"button[0.75,2.75;2,2;homepage;minetest.net]" ..
			"tablecolumns[color;text]" ..
			"tableoptions[background=#00000000;highlight=#00000000;border=false]" ..
			"table[3.5,-0.25;8.5,6.05;list_credits;" ..
			"#FFFF00," .. fgettext("Core Developers") .. ",," ..
			buildCreditList(core_developers) .. ",,," ..
			"#FFFF00," .. fgettext("Core Team") .. ",," ..
			buildCreditList(core_team) .. ",,," ..
			"#FFFF00," .. fgettext("Active Contributors") .. ",," ..
			buildCreditList(active_contributors) .. ",,," ..
			"#FFFF00," .. fgettext("Previous Core Developers") ..",," ..
			buildCreditList(previous_core_developers) .. ",,," ..
			"#FFFF00," .. fgettext("Previous Contributors") .. ",," ..
			buildCreditList(previous_contributors) .. "," ..
			";1]"

		-- Render information
		fs = fs .. "label[0.75,4.9;" ..
			fgettext("Active renderer:") .. "\n" ..
			core.formspec_escape(core.get_screen_info().render_info) .. "]"

		if PLATFORM == "Android" then
			fs = fs .. "button[0,4;3.5,1;share_debug;" .. fgettext("Share debug log") .. "]"
		else
			fs = fs .. "tooltip[userdata;" ..
					fgettext("Opens the directory that contains user-provided worlds, games, mods,\n" ..
							"and texture packs in a file manager / explorer.") .. "]"
			fs = fs .. "button[0,4;3.5,1;userdata;" .. fgettext("Open User Data Directory") .. "]"
		end

		return fs
	end,
	cbf_button_handler = function(this, fields, name, tabdata)
		if fields.homepage then
			core.open_url("https://www.minetest.net")
		end

		if fields.share_debug then
			local path = core.get_user_path() .. DIR_DELIM .. "debug.txt"
			core.share_file(path)
		end

		if fields.userdata then
			core.open_dir(core.get_user_path())
		end
	end,
}
