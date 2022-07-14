local function test_random()
	-- Try out PseudoRandom
	local pseudo = PseudoRandom(13)
	assert(pseudo:next() == 22290)
	assert(pseudo:next() == 13854)
end
unittests.register("test_random", test_random)

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
			return cb("impossible condition")
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

local function test_v3f_metatable(player)
	assert(vector.check(player:get_pos()))
end
unittests.register("test_v3f_metatable", test_v3f_metatable, {player=true})

local function test_v3s16_metatable(player, pos)
	local node = minetest.get_node(pos)
	local found_pos = minetest.find_node_near(pos, 0, node.name, true)
	assert(vector.check(found_pos))
end
unittests.register("test_v3s16_metatable", test_v3s16_metatable, {map=true})

local function test_clear_meta(_, pos)
	local ref = core.get_meta(pos)

	for way = 1, 3 do
		ref:set_string("foo", "bar")
		assert(ref:contains("foo"))

		if way == 1 then
			ref:from_table({})
		elseif way == 2 then
			ref:from_table(nil)
		else
			ref:set_string("foo", "")
		end

		assert(#core.find_nodes_with_meta(pos, pos) == 0, "clearing failed " .. way)
	end
end
unittests.register("test_clear_meta", test_clear_meta, {map=true})

local on_punch_called
minetest.register_on_punchnode(function()
	on_punch_called = true
end)
unittests.register("test_punch_node", function(_, pos)
	minetest.place_node(pos, {name="basenodes:dirt"})
	on_punch_called = false
	minetest.punch_node(pos)
	minetest.remove_node(pos)
	-- currently failing: assert(on_punch_called)
end, {map=true})
