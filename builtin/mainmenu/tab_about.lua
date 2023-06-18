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
	"Hugues Ross <hugues.ross@gmail.com>",
	"Dmitry Kostenko (x2048) <codeforsmile@gmail.com>",
	"Desour",
}

-- currently only https://github.com/orgs/minetest/teams/triagers/members

local core_team = {
	"Zughy [Issue triager]",
	"wsor [Issue triager]",
	"Hugo Locurcio (Calinou) [Issue triager]",
}

-- For updating active/previous contributors, see the script in ./util/gather_git_credits.py

local active_contributors = {
	"Wuzzy [Features, translations, devtest]",
	"Lars Müller [Bugfixes and entity features]",
	"paradust7 [Bugfixes]",
	"ROllerozxa [Bugfixes, Android]",
	"srifqi [Android, translations]",
	"Lexi Hale [Particlespawner animation]",
	"savilli [Bugfixes]",
	"fluxionary [Bugfixes]",
	"Gregor Parzefall [Bugfixes]",
	"Abdou-31 [Documentation]",
	"pecksin [Bouncy physics]",
	"Daroc Alden [Fixes]",
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
	"hecks",
	"Jude Melton-Houghton (TurkeyMcMac) [RIP]",
}

local previous_contributors = {
	"Nils Dagsson Moskopp (erlehmann) <nils@dieweltistgarnichtso.net> [Minetest logo]",
	"red-001 <red-001@outlook.ie>",
	"Giuseppe Bilotta",
	"numzero",
	"HybridDog",
	"ClobberXD",
	"Dániel Juhász (juhdanad) <juhdanad@gmail.com>",
	"MirceaKitsune <mirceakitsune@gmail.com>",
	"Jean-Patrick Guerrero (kilbith)",
	"MoNTE48",
	"Constantin Wenger (SpeedProg)",
	"Ciaran Gultnieks (CiaranG)",
	"Paul Ouellette (pauloue)",
	"stujones11",
	"Rogier <rogier777@gmail.com>",
	"Gregory Currie (gregorycu)",
	"JacobF",
	"Jeija <jeija@mesecons.net>",
}

local function prepare_credits(dest, source)
	for _, s in ipairs(source) do
		-- if there's text inside brackets make it gray-ish
		s = s:gsub("%[.-%]", core.colorize("#aaa", "%1"))
		dest[#dest+1] = s
	end
end

local function build_hacky_list(items, spacing)
	spacing = spacing or 0.5
	local y = spacing / 2
	local ret = {}
	for _, item in ipairs(items) do
		if item ~= "" then
			ret[#ret+1] = ("label[0,%f;%s]"):format(y, core.formspec_escape(item))
		end
		y = y + spacing
	end
	return table.concat(ret, ""), y
end

local function read_text_file(path)
	local f = io.open(path, "r")
	if not f then
		return nil
	end
    local text = f:read("*all")
    f:close()
	return text
end

local function collect_debug_info()
	local version = core.get_version()
	local path_pre = core.get_user_path() .. DIR_DELIM
	local minetest_conf = read_text_file(path_pre .. "minetest.conf") or "[not available]\n"
	local debug_txt = read_text_file(path_pre .. "debug.txt") or "[not available]\n"

	return table.concat({
		version.project, " ", (version.hash or version.string), "\n",
		"Platform: ", PLATFORM, "\n",
		"Active Irrlicht device: ", core.get_active_irrlicht_device(), "\n",
		"Active renderer: ", core.get_active_renderer(), "\n\n",
		"minetest.conf\n",
		"-------------\n",
		minetest_conf, "\n",
		"debug.txt\n",
		"---------\n",
		debug_txt,
	})
end

return {
	name = "about",
	caption = fgettext("About"),

	cbf_formspec = function(tabview, name, tabdata)
		local logofile = defaulttexturedir .. "logo.png"
		local version = core.get_version()

		local credit_list = {}
		table.insert_all(credit_list, {
			core.colorize("#000", "Dedication of the current release"),
			"The 5.7.0 release is dedicated to the memory of",
			"Minetest developer Jude Melton-Houghton (TurkeyMcMac)",
			"who died on February 1, 2023.",
			"Our thoughts are with his family and friends.",
			"",
			core.colorize("#ff0", fgettext("Core Developers"))
		})
		prepare_credits(credit_list, core_developers)
		table.insert_all(credit_list, {
			"",
			core.colorize("#ff0", fgettext("Core Team"))
		})
		prepare_credits(credit_list, core_team)
		table.insert_all(credit_list, {
			"",
			core.colorize("#ff0", fgettext("Active Contributors"))
		})
		prepare_credits(credit_list, active_contributors)
		table.insert_all(credit_list, {
			"",
			core.colorize("#ff0", fgettext("Previous Core Developers"))
		})
		prepare_credits(credit_list, previous_core_developers)
		table.insert_all(credit_list, {
			"",
			core.colorize("#ff0", fgettext("Previous Contributors"))
		})
		prepare_credits(credit_list, previous_contributors)
		local credit_fs, scroll_height = build_hacky_list(credit_list)
		-- account for the visible portion
		scroll_height = math.max(0, scroll_height - 6.9)

		local TAB_H = 7.1
		local TAB_PADDING = 0.5
		local LOGO_SIZE = 2.5
		local BTN_H = 0.8
		local LABEL_BTN_H = 0.5

		local fs = {
			("scroll_container[5.5,0.1;%f,6.9;scroll_credits;vertical;%f]"):format(
					9.9 - SCROLLBAR_W, scroll_height / 1000),
			credit_fs,
			"scroll_container_end[]",
			("scrollbar[%f,0.1;%f,6.9;vertical;scroll_credits;0]"):format(
					15.4 - SCROLLBAR_W, SCROLLBAR_W),
		}

		-- Place the content of the left half from bottom to top.
		local pos_y = TAB_H - TAB_PADDING
		local show_userdata_btn = PLATFORM ~= "Android"

		if show_userdata_btn then
			pos_y = pos_y - BTN_H
			fs[#fs + 1] = "tooltip[userdata;" ..
					fgettext("Opens the directory that contains user-provided worlds, games, mods,\n" ..
							"and texture packs in a file manager / explorer.") .. "]"
			fs[#fs + 1] = ("button[0.5,%f;4.5,%f;userdata;%s]"):format(
					pos_y, BTN_H, fgettext("Open user data directory"))
			pos_y = pos_y - 0.1
		end

		pos_y = pos_y - BTN_H
		local debug_label = PLATFORM == "Android" and fgettext("Share debug info") or
				fgettext("Copy debug info")
		fs[#fs + 1] = ("button[0.5,%f;4.5,%f;share_debug;%s]"):format(pos_y, BTN_H, debug_label)
		pos_y = pos_y - (show_userdata_btn and 0.25 or 0.15)

		pos_y = pos_y - BTN_H
		fs[#fs + 1] = ("button[0.5,%f;4.5,%f;homepage;minetest.net]"):format(pos_y, BTN_H)
		pos_y = pos_y - 0.15

		pos_y = pos_y - LABEL_BTN_H
		fs[#fs + 1] = "style[label_button;border=false]"
		fs[#fs + 1] = ("button[0.1,%f;5.3,%f;label_button;%s]"):format(
				pos_y, LABEL_BTN_H, core.formspec_escape(version.project .. " " .. version.string))

		-- Place the logo in the middle of the remaining space.
		fs[#fs + 1] = ("image[1.5,%f;%f,%f;%s]"):format(
				pos_y / 2 - LOGO_SIZE / 2, LOGO_SIZE, LOGO_SIZE, core.formspec_escape(logofile))

		return table.concat(fs, "")
	end,

	cbf_button_handler = function(this, fields, name, tabdata)
		if fields.homepage then
			core.open_url("https://www.minetest.net")
		end

		if fields.share_debug then
			local info = get_debug_info()
			if PLATFORM == "Android" then
				core.share_text(info)
			else
				core.copy_text(info)
			end
		end

		if fields.userdata then
			core.open_dir(core.get_user_path())
		end
	end,
}
