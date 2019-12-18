--
-- Minimal Development Test
-- Mod: test
--


-- Try out PseudoRandom
pseudo = PseudoRandom(13)
assert(pseudo:next() == 22290)
assert(pseudo:next() == 13854)

local modpath = minetest.get_modpath("test")
dofile(modpath .. "/player.lua")
dofile(modpath .. "/formspec.lua")
dofile(modpath .. "/crafting.lua")
