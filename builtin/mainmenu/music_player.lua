--Minetest
--Copyright (C) 2014 sapier
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

--local static variables
local current_file = ""
local current_music_handle = nil

-- Stops current music, if not the same as the requested file, then plays the new file on repeat
function menu_music_play(file)
	if file ~= current_file then
		if current_music_handle ~= nil then
			core.sound_stop(current_music_handle)
		end
		minetest.log("playing: " .. file)
		current_music_handle = core.sound_play(file, true)
		current_file = file
	end
end
