--
-- Minimal Development Test
-- Mod: test
--


-- Try out PseudoRandom
pseudo = PseudoRandom(13)
assert(pseudo:next() == 22290)
assert(pseudo:next() == 13854)

dofile(minetest.get_modpath("test") .. "/player.lua")
dofile(minetest.get_modpath("test") .. "/formspec.lua")
