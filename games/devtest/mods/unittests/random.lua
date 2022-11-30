-- PseudoRandom

local function test_pseudo_random_no_range()
	local rng = PseudoRandom(13)
	assert(rng:next() == 22290)
	assert(rng:next() == 13854)
end
unittests.register("test_pseudo_random_no_range", test_pseudo_random_no_range)

local function test_pseudo_random_range()
	local rng = PseudoRandom(13)
	assert(rng:next(-5, 0) == -5)
	assert(rng:next(240000, 245000.5) == 243852)
	assert(rng:next(-1.9, 32766) == 11621)
	assert(rng:next(0, 6553.9) == 1618)
	assert(rng:next(2.1, 2) == 2)
end
unittests.register("test_pseudo_random_range", test_pseudo_random_range)

local function test_pseudo_random_invalid_range()
	local rng = PseudoRandom(13)
	assert(not pcall(rng.next, rng, 0, -1))
	assert(not pcall(rng.next, rng, -1, 6553))
	assert(not pcall(rng.next, rng, 0, 32766))
	-- The state should not be affected by errors:
	assert(rng:next() == PseudoRandom(13):next())
end
unittests.register("test_pseudo_random_invalid_range", test_pseudo_random_invalid_range)

local function test_pseudo_random_overflow()
	local rng = PseudoRandom(13)
	assert(rng:next(0x80000000, 0x180000000) == -0x80000000)
end
unittests.register("test_pseudo_random_overflow", test_pseudo_random_overflow)

-- PcgRandom

local function test_pcg_random_no_range()
	local rng = PcgRandom(13)
	assert(rng:next() == -1525672764)
	assert(rng:next() == -319105642)
end
unittests.register("test_pcg_random_no_range", test_pcg_random_no_range)

local function test_pcg_random_range()
	local rng = PcgRandom(13)
	assert(rng:next(-5.9, 0.1) == -3)
	assert(rng:next(1.5, 2^31 - 1) == 1828378007)
	assert(rng:next(2.1, 2) == 2)
end
unittests.register("test_pcg_random_range", test_pcg_random_range)

local function test_pcg_random_invalid_range()
	local rng = PcgRandom(13)
	assert(not pcall(rng.next, rng, 0, -1))
	assert(not pcall(rng.rand_normal_dist, rng, 0, -1))
	-- The state should not be affected by errors:
	assert(rng:next() == PcgRandom(13):next())
end
unittests.register("test_pcg_random_invalid_range", test_pcg_random_invalid_range)

local function test_pcg_random_overflow()
	local rng = PcgRandom(13)
	assert(rng:next(0x80000000, 0x100000000) == -1349792524)
end
unittests.register("test_pcg_random_overflow", test_pcg_random_overflow)

local function test_pcg_random_normal_dist()
	local rng = PcgRandom(13)
	assert(rng:rand_normal_dist(0, 1000) == PcgRandom(13):rand_normal_dist(0, 1000, 6))
	assert(rng:rand_normal_dist(-1000, 1000, 7) == 20)
	assert(rng:rand_normal_dist(-1000, 0, 20) == -512)
end
unittests.register("test_pcg_random_normal_dist", test_pcg_random_normal_dist)
