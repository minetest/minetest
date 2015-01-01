
local scriptpath = minetest.get_builtin_path()..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM
local gamepath = scriptpath.."game"..DIR_DELIM

dofile(commonpath.."vector.lua")

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

