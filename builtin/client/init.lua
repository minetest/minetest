-- Minetest: builtin/client/init.lua
local scriptpath = core.get_builtin_path()..DIR_DELIM
local clientpath = scriptpath.."client"..DIR_DELIM

dofile(clientpath .. "register.lua")

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_shutdown(function()
	print("shutdown client")
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_receiving_chat_messages(function(message)
	print("Received message " .. message)
	return false
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_sending_chat_messages(function(message)
	print("Sending message " .. message)
	return false
end)
