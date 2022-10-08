local modname = minetest.get_current_modname()
local prefix = "["..modname.."] "

-- Startup info
minetest.log("action", prefix.."modname="..dump(modname))
minetest.log("action", prefix.."modpath="..dump(minetest.get_modpath(modname)))
minetest.log("action", prefix.."worldpath="..dump(minetest.get_worldpath()))

-- Callback info
minetest.register_on_mods_loaded(function()
	minetest.log("action", prefix.."Callback: on_mods_loaded()")
end)

minetest.register_on_chatcommand(function(name, command, params)
	minetest.log("action", prefix.."Caught command '"..command.."', issued by '"..name.."'. Parameters: '"..params.."'")
end)
