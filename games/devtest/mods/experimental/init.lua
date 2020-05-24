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

minetest.log("info", "[experimental] modname="..dump(minetest.get_current_modname()))
minetest.log("info", "[experimental] modpath="..dump(minetest.get_modpath("experimental")))
minetest.log("info", "[experimental] worldpath="..dump(minetest.get_worldpath()))


minetest.register_on_mods_loaded(function()
	minetest.log("action", "[experimental] on_mods_loaded()")
end)
