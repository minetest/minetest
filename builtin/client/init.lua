
local scriptpath = core.get_builtin_path()..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM
local gamepath = scriptpath.."game"..DIR_DELIM
local clientpath = scriptpath.."client"..DIR_DELIM

dofile(commonpath.."vector.lua")

dofile(gamepath.."constants.lua")
dofile(gamepath.."item.lua")

if core.setting_getbool("profiler.load") then
	profiler = dofile(scriptpath.."profiler"..DIR_DELIM.."init.lua")
end

dofile(gamepath.."detached_inventory.lua")
dofile(gamepath.."features.lua")
dofile(gamepath.."voxelarea.lua")

profiler = nil
