--
-- Minimal Development Test
-- Mod: test
--

-- Try out PseudoRandom
describe("PseudoRandom", function()
	it("generates", function()
		local pseudo = PseudoRandom(13)
		assert(pseudo:next() == 22290)
		assert(pseudo:next() == 13854)
	end)
end)



--
-- HP Change Reasons
--
local expect = nil
local function run_hpchangereason_tests(player)
	describe("hpchange", function()
		it("plain", function()
			expect = { type = "set_hp", from = "mod" }
			player:set_hp(3)
			assert(expect == nil)
		end)

		it("pass Lua reference", function()
			expect = { a = 234, type = "set_hp", from = "mod" }
			player:set_hp(10, { a= 234 })
			assert(expect == nil)
		end)

		it("pass Lua reference and different type", function()
			expect = { df = 3458973454, type = "fall", from = "mod" }
			player:set_hp(10, { type = "fall", df = 3458973454 })
			assert(expect == nil)
		end)
	end)
end
minetest.register_on_player_hpchange(function(player, hp, reason)
	if not expect then
		return
	end

	assert.same(expect, reason)
	expect = nil
end)



local function run_player_meta_tests(player)
	describe("player meta", function()
		local meta, meta2

		it("can set", function()
			meta = player:get_meta()
			meta:set_string("foo", "bar")
			assert(meta:contains("foo"))
			assert(meta:get_string("foo") == "bar")
			assert(meta:get("foo") == "bar")
		end)

		it("multiple handles are the same", function()
			meta2 = player:get_meta()
			assert(meta2:get_string("foo") == "bar")
			assert(meta2:get("foo") == "bar")
			assert(meta:equals(meta2))
		end)

		it("get attribute works", function()
			assert(player:get_attribute("foo") == "bar")
		end)

		it("setting one sets another handle", function()
			meta:set_string("bob", "dillan")
			assert(meta:get_string("foo") == "bar")
			assert(meta:get_string("bob") == "dillan")
			assert(meta:get("bob") == "dillan")
			assert(meta2:get_string("foo") == "bar")
			assert(meta2:get_string("bob") == "dillan")
			assert(meta2:get("bob") == "dillan")
			assert(meta:equals(meta2))
			assert(player:get_attribute("foo") == "bar")
			assert(player:get_attribute("bob") == "dillan")
		end)

		it("can delete", function()
			meta:set_string("foo", "")
			assert(not meta:contains("foo"))
			assert(meta:get("foo") == nil)
			assert(meta:get_string("foo") == "")
			assert(meta:equals(meta2))
		end)
	end)
end

local function run_player_tests(player)
	run_hpchangereason_tests(player)
	run_player_meta_tests(player)
	if core.has_failed_tests() then
		error("Some tests failed")
	else
		minetest.chat_send_all("All tests pass!")
	end
end
minetest.register_on_joinplayer(run_player_tests)
