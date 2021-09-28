unittests = {}

local modpath = minetest.get_modpath("unittests")
dofile(modpath .. "/random.lua")
dofile(modpath .. "/player.lua")
dofile(modpath .. "/crafting_prepare.lua")
dofile(modpath .. "/crafting.lua")
dofile(modpath .. "/itemdescription.lua")

if minetest.settings:get_bool("devtest_unittests_autostart", false) then
	unittests.test_random()
	unittests.test_crafting()
	unittests.test_short_desc()
	minetest.register_on_joinplayer(function(player)
		unittests.test_player(player)
	end)
end

