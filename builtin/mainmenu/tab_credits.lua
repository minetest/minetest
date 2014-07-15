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

tab_credits = {
	name = "credits",
	caption = fgettext("Credits"),
	cbf_formspec = function (tabview, name, tabdata)
			local logofile = defaulttexturedir .. "logo.png"
			return	"vertlabel[0,-0.25;CREDITS]" ..
				"label[0.5,3;Minetest " .. core.get_version() .. "]" ..
				"label[0.5,3.3;http://minetest.net]" ..
				"image[0.5,1;" .. core.formspec_escape(logofile) .. "]" ..
				"textlist[3.5,-0.25;8.5,5.8;list_credits;" ..
				"#FFFF00" .. fgettext("Core Developers") .."," ..
				"Perttu Ahola (celeron55) <celeron55@gmail.com>,"..
				"Ryan Kwolek (kwolekr) <kwolekr@minetest.net>,"..
				"PilzAdam <pilzadam@minetest.net>," ..
				"Ilya Zhuravlev (xyz) <xyz@minetest.net>,"..
				"Lisa Milne (darkrose) <lisa@ltmnet.com>,"..
				"Maciej Kasatkin (RealBadAngel) <mk@realbadangel.pl>,"..
				"sfan5 <sfan5@live.de>,"..
				"kahrl <kahrl@gmx.net>,"..
				"sapier,"..
				"ShadowNinja <shadowninja@minetest.net>,"..
				"Nathanael Courant (Nore/Novatux) <nore@mesecons.net>,"..
				"BlockMen,"..
				","..
				"#FFFF00" .. fgettext("Active Contributors") .. "," ..
				"Vanessa Ezekowitz (VanessaE) <vanessaezekowitz@gmail.com>,"..
				"Jurgen Doser (doserj) <jurgen.doser@gmail.com>,"..
				"Jeija <jeija@mesecons.net>,"..
				"MirceaKitsune <mirceakitsune@gmail.com>,"..
				"dannydark <the_skeleton_of_a_child@yahoo.co.uk>,"..
				"0gb.us <0gb.us@0gb.us>,"..
				"," ..
				"#FFFF00" .. fgettext("Previous Contributors") .. "," ..
				"Guiseppe Bilotta (Oblomov) <guiseppe.bilotta@gmail.com>,"..
				"Jonathan Neuschafer <j.neuschaefer@gmx.net>,"..
				"Nils Dagsson Moskopp (erlehmann) <nils@dieweltistgarnichtso.net>,"..
				"Constantin Wenger (SpeedProg) <constantin.wenger@googlemail.com>,"..
				"matttpt <matttpt@gmail.com>,"..
				"JacobF <queatz@gmail.com>,"..
				";0;true]"
			end
	}
