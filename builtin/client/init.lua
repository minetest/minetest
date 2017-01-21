-- Minetest: builtin/client/init.lua
local scriptpath = core.get_builtin_path()..DIR_DELIM
local clientpath = scriptpath.."client"..DIR_DELIM

dofile(clientpath .. "register.lua")

core.register_on_shutdown(function()
	print("coucou")
end)
