--
-- This file contains built-in stuff in Minetest implemented in Lua.
--
-- It is always loaded and executed after registration of the C API,
-- before loading and running any mods.
--

-- Initialize some very basic things
print = minetest.debug
math.randomseed(os.time())

-- Load other files
dofile(minetest.get_modpath("__builtin").."/serialize.lua")
dofile(minetest.get_modpath("__builtin").."/misc_helpers.lua")
dofile(minetest.get_modpath("__builtin").."/item.lua")
dofile(minetest.get_modpath("__builtin").."/misc_register.lua")
dofile(minetest.get_modpath("__builtin").."/item_entity.lua")
dofile(minetest.get_modpath("__builtin").."/deprecated.lua")
dofile(minetest.get_modpath("__builtin").."/misc.lua")
dofile(minetest.get_modpath("__builtin").."/privileges.lua")
dofile(minetest.get_modpath("__builtin").."/auth.lua")
dofile(minetest.get_modpath("__builtin").."/chatcommands.lua")
dofile(minetest.get_modpath("__builtin").."/static_spawn.lua")
dofile(minetest.get_modpath("__builtin").."/detached_inventory.lua")

