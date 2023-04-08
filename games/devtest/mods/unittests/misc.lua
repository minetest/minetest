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

local function test_compress()
	-- This text should be compressible, to make sure the results are... normal
	local text = "The\000 icey canoe couldn't move very well on the\128 lake. The\000 ice was too stiff and the icey canoe's paddles simply wouldn't punch through."
	local methods = {
		"deflate",
		"zstd",
		-- "noodle", -- for warning alarm test
	}
	local zstd_magic = string.char(0x28, 0xB5, 0x2F, 0xFD)
	for _, method in ipairs(methods) do
		local compressed = core.compress(text, method)
		assert(core.decompress(compressed, method) == text, "input/output mismatch for compression method " .. method)
		local has_zstd_magic = compressed:sub(1, 4) == zstd_magic
		if method == "zstd" then
			assert(has_zstd_magic, "zstd magic number not in zstd method")
		else
			assert(not has_zstd_magic, "zstd magic number in method " .. method .. " (which is not zstd)")
		end
	end
end
unittests.register("test_compress", test_compress)

local function test_game_info()
	local info = minetest.get_game_info()
	local game_conf = Settings(info.path .. "/game.conf")
	assert(info.id == "devtest")
	assert(info.title == game_conf:get("title"))
end
unittests.register("test_game_info", test_game_info)

local function test_mapgen_edges(cb)
	-- Test that the map can extend to the expected edges and no further.
	local min_edge, max_edge = minetest.get_mapgen_edges()
	local min_finished = {}
	local max_finished = {}
	local function finish()
		if #min_finished ~= 1 then
			return cb("Expected 1 block to emerge around mapgen minimum edge")
		end
		if min_finished[1] ~= (min_edge / minetest.MAP_BLOCKSIZE):floor() then
			return cb("Expected block within minimum edge to emerge")
		end
		if #max_finished ~= 1 then
			return cb("Expected 1 block to emerge around mapgen maximum edge")
		end
		if max_finished[1] ~= (max_edge / minetest.MAP_BLOCKSIZE):floor() then
			return cb("Expected block within maximum edge to emerge")
		end
		return cb()
	end
	local emerges_left = 2
	local function emerge_block(blockpos, action, blocks_left, finished)
		if action ~= minetest.EMERGE_CANCELLED then
			table.insert(finished, blockpos)
		end
		if blocks_left == 0 then
			emerges_left = emerges_left - 1
			if emerges_left == 0 then
				return finish()
			end
		end
	end
	minetest.emerge_area(min_edge:subtract(1), min_edge, emerge_block, min_finished)
	minetest.emerge_area(max_edge, max_edge:add(1), emerge_block, max_finished)
end
unittests.register("test_mapgen_edges", test_mapgen_edges, {map=true, async=true})

local finish_test_on_mapblocks_changed
minetest.register_on_mapblocks_changed(function(modified_blocks, modified_block_count)
	if finish_test_on_mapblocks_changed then
		finish_test_on_mapblocks_changed(modified_blocks, modified_block_count)
		finish_test_on_mapblocks_changed = nil
	end
end)
local function test_on_mapblocks_changed(cb, player, pos)
	local bp1 = (pos / minetest.MAP_BLOCKSIZE):floor()
	local bp2 = bp1:add(1)
	for _, bp in ipairs({bp1, bp2}) do
		-- Make a modification in the block.
		local p = bp * minetest.MAP_BLOCKSIZE
		minetest.load_area(p)
		local meta = minetest.get_meta(p)
		meta:set_int("test_on_mapblocks_changed", meta:get_int("test_on_mapblocks_changed") + 1)
	end
	finish_test_on_mapblocks_changed = function(modified_blocks, modified_block_count)
		if modified_block_count < 2 then
			return cb("Expected at least two mapblocks to be recorded as modified")
		end
		if not modified_blocks[minetest.hash_node_position(bp1)] or
				not modified_blocks[minetest.hash_node_position(bp2)] then
			return cb("The expected mapblocks were not recorded as modified")
		end
		cb()
	end
end
unittests.register("test_on_mapblocks_changed", test_on_mapblocks_changed, {map=true, async=true})
