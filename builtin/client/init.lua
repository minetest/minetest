-- Minetest: builtin/client/init.lua
local scriptpath = core.get_builtin_path()..DIR_DELIM
local clientpath = scriptpath.."client"..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM

dofile(clientpath .. "register.lua")
dofile(commonpath .. "after.lua")
dofile(commonpath .. "chatcommands.lua")
dofile(clientpath .. "preview.lua")

core.register_on_death(function()
	core.display_chat_message("You died.")
end)
