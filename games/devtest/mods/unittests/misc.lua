local function test_random()
	-- Try out PseudoRandom
	local pseudo = PseudoRandom(13)
	assert(pseudo:next() == 22290)
	assert(pseudo:next() == 13854)
end
unittests.register("test_random", test_random)

local function test_light_equivalent()
	local function assert_equivalence(a, b, equiv)
		if core.light_equivalent(a, b) ~= equiv then
			error(("%s %s light-wise equivalent to %s"):format(
				core.write_json(a),
				equiv and "is not" or "is",
				core.write_json(b)
			))
		end
	end
	local function test_noncommutatively(a, b, equiv)
		local node_a = {name = a, param1 = 0, param2 = 0}
		local node_b = {name = b, param1 = 0, param2 = 0}
		assert_equivalence(a, b, equiv)
		assert_equivalence(node_a, b, equiv)
		assert_equivalence(a, node_b, equiv)
		assert_equivalence(node_a, node_b, equiv)
	end
	local function test(a, b, equiv)
		test_noncommutatively(a, b, equiv)
		test_noncommutatively(b, a, equiv)
	end

	test("ignore", "ignore", true)
	test("basenodes:water_source", "basenodes:water_flowing", true)
	test("basenodes:water_source", "mapgen_water_source", true)
	test("basenodes:lava_source", "basenodes:lava_flowing", true)
	test("basenodes:water_source", "basenodes:lava_flowing", false)
	test("mapgen_water_source", "mapgen_lava_source", false)
	test("basenodes:stone", "basenodes:dirt_with_grass", true)
	test("basenodes:tree", "basenodes:leaves", false)
	test("basenodes:leaves", "basenodes:jungleleaves", true)
	test("basenodes:leaves", "basenodes:lava_source", false)
	test("basenodes:leaves", "basenodes:apple", false)
	test("basenodes:apple", "air", true)
end
unittests.register("test_light_equivalent", test_light_equivalent)

local function test_dynamic_media(cb, player)
	if core.get_player_information(player:get_player_name()).protocol_version < 40 then
		core.log("warning", "test_dynamic_media: Client too old, skipping test.")
		return cb()
	end

	-- Check that the client acknowledges media transfers
	local path = core.get_worldpath() .. "/test_media.obj"
	local f = io.open(path, "w")
	f:write("# contents don't matter\n")
	f:close()

	local call_ok = false
	local ok = core.dynamic_add_media({
		filepath = path,
		to_player = player:get_player_name(),
	}, function(name)
		if not call_ok then
			cb("impossible condition")
		end
		cb()
	end)
	if not ok then
		return cb("dynamic_add_media() returned error")
	end
	call_ok = true

	-- if the callback isn't called this test will just hang :shrug:
end
unittests.register("test_dynamic_media", test_dynamic_media, {async=true, player=true})
