function unittests.test_random()
	-- Try out PseudoRandom
	minetest.log("action", "[unittests] Testing PseudoRandom ...")
	local pseudo = PseudoRandom(13)
	assert(pseudo:next() == 22290)
	assert(pseudo:next() == 13854)
	minetest.log("action", "[unittests] PseudoRandom test passed!")
	return true
end

