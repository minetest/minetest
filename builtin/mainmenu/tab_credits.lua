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

--------------------------------------------------------------------------------

local core_developers = {
	"Perttu Ahola (celeron55) <celeron55@gmail.com>",
	"sfan5 <sfan5@live.de>",
	"ShadowNinja <shadowninja@minetest.net>",
	"Nathanaël Courant (Nore/Ekdohibs) <nore@mesecons.net>",
	"Loic Blot (nerzhul/nrz) <loic.blot@unix-experience.fr>",
	"paramat",
	"Auke Kok (sofar) <sofar@foo-projects.org>",
	"rubenwardy <rw@rubenwardy.com>",
	"Krock/SmallJoker <mk939@ymail.com>",
	"Lars Hofhansl <larsh@apache.org>",
}

local active_contributors = {
	"numberZero [Audiovisuals: meshgen]",
	"stujones11 [Android UX improvements]",
	"red-001 <red-001@outlook.ie> [CSM & Menu fixes]",
	"Paul Ouellette (pauloue) [Docs, fixes]",
	"Dániel Juhász (juhdanad) <juhdanad@gmail.com> [Audiovisuals: lighting]",
	"Hybrid Dog [API]",
	"srifqi [Android]",
	"Vincent Glize (Dumbeldor) [Cleanups, CSM APIs]",
	"Ben Deutsch [Rendering, Fixes, SQLite auth]",
	"Wuzzy [Translation, Slippery]",
	"ANAND (ClobberXD) [Docs, Fixes]",
	"Shara/Ezhh [Docs, Game API]",
	"DTA7 [Fixes, mute key]",
	"Thomas-S [Disconnected, Formspecs]",
	"Raymoo [Tool Capabilities]",
	"Elijah Duffy (octacian) [Mainmenu]",
	"noob3167 [Fixes]",
	"adelcoding1 [Formspecs]",
	"adrido [Windows Installer, Formspecs]",
	"Rui [Sound Pitch]",
	"Jean-Patrick G (kilbith) <jeanpatrick.guerrero@gmail.com> [Audiovisuals]",
	"Esteban (EXio4) [Cleanups]",
	"Vaughan Lapsley (vlapsley) [Carpathian mapgen]",
	"CoderForTheBetter [Add set_rotation]",
	"Quentin Bazin (Unarelith) [Cleanups]",
	"Maksim (MoNTE48) [Android]",
	"Gaël-de-Sailly [Mapgen, pitch fly]",
	"zeuner [Docs, Fixes]",
	"ThomasMonroe314 (tre) [Fixes]",
	"Rob Blanckaert (basicer) [Fixes]",
	"Jozef Behran (osjc) [Fixes]",
	"random-geek [Fixes]",
	"Pedro Gimeno (pgimeno) [Fixes]",
	"lisacvuk [Fixes]",
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
}

local previous_contributors = {
	"Gregory Currie (gregorycu) [optimisation]",
	"Diego Martínez (kaeza) <kaeza@users.sf.net>",
	"T4im [Profiler]",
	"TeTpaAka [Hand overriding, nametag colors]",
	"Duane Robertson <duane@duanerobertson.com> [MGValleys]",
	"neoascetic [OS X Fixes]",
	"TriBlade9 <triblade9@mail.com> [Audiovisuals]",
	"Jurgen Doser (doserj) <jurgen.doser@gmail.com> [Fixes]",
	"MirceaKitsune <mirceakitsune@gmail.com> [Audiovisuals]",
	"Guiseppe Bilotta (Oblomov) <guiseppe.bilotta@gmail.com> [Fixes]",
	"matttpt <matttpt@gmail.com> [Fixes]",
	"Nils Dagsson Moskopp (erlehmann) <nils@dieweltistgarnichtso.net> [Minetest Logo]",
	"Jeija <jeija@mesecons.net> [HTTP, particles]",
	"bigfoot547 [CSM]",
	"Rogier <rogier777@gmail.com> [Fixes]",
}

local function buildCreditList(source)
	local ret = {}
	for i = 1, #source do
		ret[i] = core.formspec_escape(source[i])
	end
	return table.concat(ret, ",,")
end

return {
	name = "credits",
	caption = fgettext("Credits"),
	cbf_formspec = function(tabview, name, tabdata)
		local logofile = defaulttexturedir .. "logo.png"
		local version = core.get_version()
		return "image[0.5,1;" .. core.formspec_escape(logofile) .. "]" ..
			"label[0.5,3.2;" .. version.project .. " " .. version.string .. "]" ..
			"label[0.5,3.5;http://minetest.net]" ..
			"tablecolumns[color;text]" ..
			"tableoptions[background=#00000000;highlight=#00000000;border=false]" ..
			"table[3.5,-0.25;8.5,6.05;list_credits;" ..
			"#FFFF00," .. fgettext("Core Developers") .. ",," ..
			buildCreditList(core_developers) .. ",,," ..
			"#FFFF00," .. fgettext("Active Contributors") .. ",," ..
			buildCreditList(active_contributors) .. ",,," ..
			"#FFFF00," .. fgettext("Previous Core Developers") ..",," ..
			buildCreditList(previous_core_developers) .. ",,," ..
			"#FFFF00," .. fgettext("Previous Contributors") .. ",," ..
			buildCreditList(previous_contributors) .. "," ..
			";1]"
	end
}
