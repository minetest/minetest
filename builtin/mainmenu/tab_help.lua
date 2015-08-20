--Minetest
--Copyright (C) 2015 shadowzone
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

tab_help = {
	name = "help",
	caption = fgettext("Help"),
	cbf_formspec = function (tabview, name, tabdata)
			local logofile = defaulttexturedir .. "logo.png"
			return 
				"label[0.3,-0.25;Helpful tips and links.]" ..
				"image[1,0.25;" .. core.formspec_escape(playerimg) .. "]" ..
				"label[0.5,2.4;Minetest Engine]" ..
				"label[0.38,2.7;Version " .. core.get_version() .. "]" ..
				"label[0.4,3;http://minetest.net]" ..
				"image[0.5,3.375;" .. core.formspec_escape(logofile) .. "]" ..
				"textlist[3.5,-0.25;8.5,5.8;list_help;" ..
				"#FFFF00" .. gettext("Getting Started") .. "," ..
				"#FFFF00" .. gettext("===============") .. "," ..
				"#FFFF00" .. gettext("Default Controls") .. "," ..
				"Dig = Left mouse Button," ..
				"Place = Right Mouse Button," ..
				"Forward = W     Backwards = S," ..
				"Left = A              Right = D," ..
				"Jump = Space   Sneak = Left Shift," ..
				"Toggle Fly = K   Toggle Noclip = H," ..
				"Toggle Fast = J Toggle View Range = R," ..
				"Use = E             Drop = Q," ..
				"Inventory = I       Command = /," ..
				"Chat = T            Console = F10," ..
				"," ..
				"#FFFF00" .. gettext("Chat Commands") .. "," ..
				"The game has a set of 'privs' or privileges that allow the player," ..
				" to do different things. To view your privileges type '/privs' ," ..
				"To grant yourself privileges type '/grant singleplayer fly' or ," ..
				" '/grant singleplayer fast' and so on depending on the desired ," ..
				" privilege. ," ..
				"," ..
				"To give an item you can type '/giveme <item_name> <amount>' ," ..
				" or '/give <player> <item_name> <amount> '," ..
				"," ..
				"For a list of all chat commands type '/help' or '/help <cmd>' ," ..
				" for a specific command. ," ..
				"," ..
				"http://wiki.minetest.net/Getting_Started," ..
				"," ..
				"#FFFF00" .. gettext("Minetest Wiki") .. "," ..
				"http://wiki.minetest.net," ..
				"," ..
				"#FFFF00" .. gettext("Minetest IRC help and chat") .. "," ..
				"Freenode #minetest," ..
				"Development channel #minetest-dev," ..
				"Inchra.net #minetest," ..
				"," ..
				"#FFFF00" .. gettext("Minetest Forums") .. "," ..
				"http://forum.minetest.net," ..
				"," ..
				"#FFFF00" .. gettext("Minetest downloads") .. "," ..
				"For Downloads for most OS, Android or a newer version," ..
				"http://minetest.net/downloads," ..
				"or on Github," ..
				"http://github.com/minetest/minetest," ..
				"," ..
				"#FFFF00" .. gettext("Minetest Developer Wiki") .. "," ..
				"http://dev.minetest.net," ..
				"," ..
				";0;true]"
			end
	}
