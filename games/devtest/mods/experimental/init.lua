--
-- Experimental things
--

experimental = {}

dofile(minetest.get_modpath("experimental").."/detached.lua")
dofile(minetest.get_modpath("experimental").."/items.lua")
dofile(minetest.get_modpath("experimental").."/commands.lua")

function experimental.print_to_everything(msg)
	minetest.log("action", msg)
	minetest.chat_send_all(msg)
end


