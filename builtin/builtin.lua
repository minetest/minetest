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
dofile(minetest.get_modpath("__builtin").."/enableRequire.lua")
minetest.require("__builtin","serialize")
minetest.require("__builtin","misc_helpers")
minetest.require("__builtin","item")
minetest.require("__builtin","misc_register")
minetest.require("__builtin","item_entity")
minetest.require("__builtin","deprecated")
minetest.require("__builtin","misc")
minetest.require("__builtin","privileges")
minetest.require("__builtin","auth")
minetest.require("__builtin","chatcommands")
minetest.require("__builtin","static_spawn")
