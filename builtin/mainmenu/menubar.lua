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

menubar = {}

--------------------------------------------------------------------------------
function menubar.handle_buttons(fields)
	for i=1,#menubar.buttons,1 do
		if fields[menubar.buttons[i].btn_name] ~= nil then
			menu.last_game = menubar.buttons[i].index
			engine.setting_set("main_menu_last_game_idx",menu.last_game)
			menu.update_gametype()
		end
	end
end

--------------------------------------------------------------------------------
function menubar.refresh()

	menubar.formspec = "box[-0.3,5.625;12.4,1.2;#000000]" ..
					   "box[-0.3,5.6;12.4,0.05;#FFFFFF]"
	menubar.buttons = {}

	local button_base = -0.08
	
	local maxbuttons = #gamemgr.games
	
	if maxbuttons > 11 then
		maxbuttons = 11
	end
	
	for i=1,maxbuttons,1 do

		local btn_name = "menubar_btn_" .. gamemgr.games[i].id
		local buttonpos = button_base + (i-1) * 1.1
		if gamemgr.games[i].menuicon_path ~= nil and
			gamemgr.games[i].menuicon_path ~= "" then

			menubar.formspec = menubar.formspec ..
				"image_button[" .. buttonpos ..  ",5.72;1.165,1.175;"  ..
				engine.formspec_escape(gamemgr.games[i].menuicon_path) .. ";" ..
				btn_name .. ";;true;false]"
		else
		
			local part1 = gamemgr.games[i].id:sub(1,5)
			local part2 = gamemgr.games[i].id:sub(6,10)
			local part3 = gamemgr.games[i].id:sub(11)
			
			local text = part1 .. "\n" .. part2
			if part3 ~= nil and
				part3 ~= "" then
				text = text .. "\n" .. part3
			end
			menubar.formspec = menubar.formspec ..
				"image_button[" .. buttonpos ..  ",5.72;1.165,1.175;;" ..btn_name ..
				";" .. text .. ";true;true]"
		end
		
		local toadd = {
			btn_name = btn_name,
			index = i,
		}
		
		table.insert(menubar.buttons,toadd)
	end
end
