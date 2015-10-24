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
			return "label[0.5,3.2;Minetest " .. core.get_version() .. "]" ..
				"label[0.5,3.5;http://minetest.net]" ..
				"image[0.5,1;" .. core.formspec_escape(logofile) .. "]" ..
				"tablecolumns[color;text]" ..
				"tableoptions[background=#00000000;highlight=#00000000;border=false]" ..
				"table[3.5,-0.25;8.5,5.8;list_credits;" ..
				"#FFFF00," .. fgettext("Core Developers") .."," ..
				",Perttu Ahola (celeron55) <celeron55@gmail.com>,"..
				",Ryan Kwolek (kwolekr) <kwolekr@minetest.net>,"..
				",PilzAdam <pilzadam@minetest.net>," ..
				",sfan5 <sfan5@live.de>,"..
				",kahrl <kahrl@gmx.net>,"..
				",sapier,"..
				",ShadowNinja <shadowninja@minetest.net>,"..
				",Nathanael Courant (Nore/Ekdohibs) <nore@mesecons.net>,"..
				",BlockMen,"..
				",Craig Robbins (Zeno),"..
				",Loic Blot (nerzhul/nrz) <loic.blot@unix-experience.fr>,"..
				",Mat Gregory (paramat),"..
				",est31 <MTest31@outlook.com>," ..
				",,"..
				"#FFFF00," .. fgettext("Active Contributors") .. "," ..
				",SmallJoker <mk939@ymail.com>," ..
				",Andrew Ward (rubenwardy) <rubenwardy@gmail.com>," ..
				",Aaron Suen <warr1024@gmail.com>," ..
				",Sokomine <wegwerf@anarres.dyndns.org>," ..
				",Břetislav Štec (t0suj4/TBC_x)," ..
				",TeTpaAka," ..
				",Jean-Patrick G (kilbith) <jeanpatrick.guerrero@gmail.com>," ..
				",Diego Martinez (kaeza) <kaeza@users.sf.net>," ..
				",," ..
				"#FFFF00," .. fgettext("Previous Core Developers") .."," ..
				",Maciej Kasatkin (RealBadAngel) <maciej.kasatkin@o2.pl>,"..
				",Lisa Milne (darkrose) <lisa@ltmnet.com>," ..
				",proller," ..
				",Ilya Zhuravlev (xyz) <xyz@minetest.net>," ..
				",," ..
				"#FFFF00," .. fgettext("Previous Contributors") .. "," ..
				",Vanessa Ezekowitz (VanessaE) <vanessaezekowitz@gmail.com>,"..
				",Jurgen Doser (doserj) <jurgen.doser@gmail.com>,"..
				",Gregory Currie (gregorycu)," ..
				",Jeija <jeija@mesecons.net>,"..
				",MirceaKitsune <mirceakitsune@gmail.com>,"..
				",dannydark <the_skeleton_of_a_child@yahoo.co.uk>,"..
				",0gb.us <0gb.us@0gb.us>,"..
				",Guiseppe Bilotta (Oblomov) <guiseppe.bilotta@gmail.com>,"..
				",Jonathan Neuschafer <j.neuschaefer@gmx.net>,"..
				",Nils Dagsson Moskopp (erlehmann) <nils@dieweltistgarnichtso.net>,"..
				",Constantin Wenger (SpeedProg) <constantin.wenger@googlemail.com>,"..
				",matttpt <matttpt@gmail.com>,"..
				",JacobF <queatz@gmail.com>,"..
				",TriBlade9 <triblade9@mail.com>,"..
				",Zefram <zefram@fysh.org>,"..
				";1]"
			end
	}
