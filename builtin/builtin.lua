--
-- This file contains built-in stuff in Minetest implemented in Lua.
--
-- It is always loaded and executed after registration of the C API,
-- before loading and running any mods.
--

-- Initialize some very basic things
print = minetest.debug
math.randomseed(os.time())
os.setlocale("C", "numeric")

-- Load other files
local modpath = minetest.get_modpath("__builtin")
dofile(modpath.."/serialize.lua")
dofile(modpath.."/misc_helpers.lua")
dofile(modpath.."/item.lua")
dofile(modpath.."/misc_register.lua")
dofile(modpath.."/item_entity.lua")
dofile(modpath.."/deprecated.lua")
dofile(modpath.."/misc.lua")
dofile(modpath.."/privileges.lua")
dofile(modpath.."/auth.lua")
dofile(modpath.."/chatcommands.lua")
dofile(modpath.."/static_spawn.lua")
dofile(modpath.."/detached_inventory.lua")
dofile(modpath.."/falling.lua")
dofile(modpath.."/features.lua")
dofile(modpath.."/voxelarea.lua")
dofile(modpath.."/vector.lua")
dofile(modpath.."/forceloading.lua")
