--Minetest
--Copyright (C) 2014 sapier
--Copyright (C) 2016 celeron55
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

local function event_handler(tabview, event)
	if event == "MenuQuit" then
		tabview:delete()
		return true
	end
	return false
end

local function button_handler(tabview, fields)
	if fields["back"] ~= nil then
		tabview:delete()
		return true
	end
end

function create_internet()
	local tv_main = tabview_create("internet_tabview",{x=12,y=5.9},{x=0,y=0})
	tv_main.append_to_formspec = "button[0,5.7;2.6,0.5;back;< ".. fgettext("Back") .. "]" ..
			"bgcolor[#0000;true]"
	tv_main:set_global_event_handler(event_handler)
	tv_main.custom_button_handler = button_handler
	tv_main:set_fixed_size(false)

	tv_main:add(tab_multiplayer)
	tv_main:add(tab_server)

	-- TODO: Restore previously open tab
	tv_main:set_tab("multiplayer")

	return tv_main
end
