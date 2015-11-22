---------------------------------------------------------------------------------
-- Minetest
-- Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License along
-- with this program; if not, write to the Free Software Foundation, Inc.,
-- 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
---------------------------------------------------------------------------------

local scriptpath = core.get_builtin_path()..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM
local gamepath = scriptpath.."game"..DIR_DELIM

dofile(commonpath.."vector.lua")

dofile(gamepath.."constants.lua")
dofile(gamepath.."item.lua")
dofile(gamepath.."register.lua")

if core.setting_getbool("mod_profiling") then
	dofile(gamepath.."mod_profiling.lua")
end

dofile(gamepath.."item_entity.lua")
dofile(gamepath.."deprecated.lua")
dofile(gamepath.."misc.lua")
dofile(gamepath.."privileges.lua")
dofile(gamepath.."auth.lua")
dofile(gamepath.."chatcommands.lua")
dofile(gamepath.."static_spawn.lua")
dofile(gamepath.."detached_inventory.lua")
dofile(gamepath.."falling.lua")
dofile(gamepath.."features.lua")
dofile(gamepath.."voxelarea.lua")
dofile(gamepath.."forceloading.lua")
dofile(gamepath.."statbars.lua")

if core.setting_getbool("enable_hunger") then
	dofile(gamepath .. "hunger.lua")
end
