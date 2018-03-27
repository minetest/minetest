--
-- Minimal Development Test
-- Mod: test
--

-- Try out PseudoRandom
pseudo = PseudoRandom(13)
assert(pseudo:next() == 22290)
assert(pseudo:next() == 13854)


--
-- HP Change Reasons
--
local expect = nil
minetest.register_on_joinplayer(function(player)
	expect = { type = "set_hp", from = "mod" }
	player:set_hp(3)
	assert(expect == nil)

	expect = { a = 234, type = "set_hp", from = "mod" }
	player:set_hp(10, { a= 234 })
	assert(expect == nil)

	expect = { df = 3458973454, type = "fall", from = "mod" }
	player:set_hp(10, { type = "fall", df = 3458973454 })
	assert(expect == nil)
end)

minetest.register_on_player_hpchange(function(player, hp, reason)
	for key, value in pairs(reason) do
		assert(expect[key] == value)
	end

	for key, value in pairs(expect) do
		assert(reason[key] == value)
	end

	expect = nil
end)
