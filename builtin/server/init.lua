
local scriptpath = core.get_builtin_path()..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM
local gamepath = scriptpath.."game"..DIR_DELIM
local serverpath = scriptpath.."server"..DIR_DELIM

-- Shared between builtin files, but
-- not exposed to outer context
local builtin_shared = {}

dofile(commonpath.."vector.lua")

dofile(gamepath.."constants.lua")
assert(loadfile(gamepath.."item.lua"))(builtin_shared)
dofile(serverpath.."register.lua")

if core.setting_getbool("profiler.load") then
	profiler = dofile(scriptpath.."profiler"..DIR_DELIM.."init.lua")
end

dofile(serverpath.."item_entity.lua")
dofile(serverpath.."deprecated.lua")
dofile(serverpath.."misc.lua")
dofile(serverpath.."privileges.lua")
dofile(serverpath.."auth.lua")
dofile(serverpath.."chatcommands.lua")
dofile(serverpath.."static_spawn.lua")
dofile(gamepath.."detached_inventory.lua")
assert(loadfile(serverpath.."falling.lua"))(builtin_shared)
dofile(gamepath.."features.lua")
dofile(gamepath.."voxelarea.lua")
dofile(serverpath.."forceloading.lua")
dofile(serverpath.."statbars.lua")

profiler = nil
