-- Minetest: builtin/client/init.lua
local scriptpath = core.get_builtin_path()
local clientpath = scriptpath.."client"..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM

dofile(commonpath .. "after.lua")
dofile(commonpath .. "chatcommands.lua")
dofile(commonpath .. "vector.lua")
dofile(clientpath .. "register.lua")
dofile(clientpath .. "chatcommands.lua")
dofile(clientpath .. "death_formspec.lua")
