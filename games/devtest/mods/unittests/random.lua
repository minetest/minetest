local function test_random()
	-- Try out PseudoRandom
	local pseudo = PseudoRandom(13)
	assert(pseudo:next() == 22290)
	assert(pseudo:next() == 13854)
end
unittests.register("test_random", test_random)
