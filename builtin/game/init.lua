
local script_path = core.get_builtin_path()
local common_path = script_path .. "/common"
local game_path = script_path .. "/game"

dofile(common_path .. "/vector.lua")

dofile(game_path .. "/constants.lua")
dofile(game_path .. "/item.lua")
dofile(game_path .. "/register.lua")

if core.setting_getbool("profiler.load") then
	profiler = dofile(script_path .. "/profiler/init.lua")
end

dofile(game_path .. "/item_entity.lua")
dofile(game_path .. "/deprecated.lua")
dofile(game_path .. "/misc.lua")
dofile(game_path .. "/privileges.lua")
dofile(game_path .. "/auth.lua")
dofile(game_path .. "/chatcommands.lua")
dofile(game_path .. "/static_spawn.lua")
dofile(game_path .. "/detached_inventory.lua")
dofile(game_path .. "/falling.lua")
dofile(game_path .. "/features.lua")
dofile(game_path .. "/voxelarea.lua")
dofile(game_path .. "/forceloading.lua")
dofile(game_path .. "/statbars.lua")

profiler = nil
