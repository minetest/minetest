--
-- Minimal Development Test
-- Mod: test
--

--
-- HP Change Reasons
--
local expect = nil
local function run_hpchangereason_tests(player)
	expect = { type = "set_hp", from = "mod" }
	player:set_hp(3)
	assert(expect == nil)

	expect = { a = 234, type = "set_hp", from = "mod" }
	player:set_hp(7, { a= 234 })
	assert(expect == nil)

	expect = { df = 3458973454, type = "fall", from = "mod" }
	player:set_hp(10, { type = "fall", df = 3458973454 })
	assert(expect == nil)
end
minetest.register_on_player_hpchange(function(player, hp, reason)
	if not expect then
		return
	end

	for key, value in pairs(reason) do
		assert(expect[key] == value)
	end

	for key, value in pairs(expect) do
		assert(reason[key] == value)
	end

	expect = nil
end)


local function run_player_meta_tests(player)
	local meta = player:get_meta()
	meta:set_string("foo", "bar")
	assert(meta:contains("foo"))
	assert(meta:get_string("foo") == "bar")
	assert(meta:get("foo") == "bar")

	local meta2 = player:get_meta()
	assert(meta2:get_string("foo") == "bar")
	assert(meta2:get("foo") == "bar")
	assert(meta:equals(meta2))
	assert(player:get_attribute("foo") == "bar")

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

	meta:set_string("foo", "")
	assert(not meta:contains("foo"))
	assert(meta:get("foo") == nil)
	assert(meta:get_string("foo") == "")
	assert(meta:equals(meta2))
end

local function run_player_tests(player)
	run_hpchangereason_tests(player)
	run_player_meta_tests(player)
	minetest.chat_send_all("All tests pass!")
end
minetest.register_on_joinplayer(run_player_tests)


local function test_get_craft_result()
	minetest.log("info", "test_get_craft_result()")
	-- normal
	local input = {
		method = "normal",
		width = 2,
		items = {"", "default:coal_lump", "", "default:stick"}
	}
	minetest.log("info", "torch crafting input: "..dump(input))
	local output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "torch crafting output: "..dump(output))
	minetest.log("info", "torch crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	minetest.log("info", "torch crafting output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "default:torch")
	assert(output.item:get_count() == 4)
	-- fuel
	local input = {
		method = "fuel",
		width = 1,
		items = {"default:coal_lump"}
	}
	minetest.log("info", "coal fuel input: "..dump(input))
	local output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "coal fuel output: "..dump(output))
	minetest.log("info", "coal fuel decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)
	-- cook
	local input = {
		method = "cooking",
		width = 1,
		items = {"default:cobble"}
	}
	minetest.log("info", "cobble cooking input: "..dump(output))
	local output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "cobble cooking output: "..dump(output))
	minetest.log("info", "cobble cooking decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)
	assert(output.item)
	minetest.log("info", "cobble cooking output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "default:stone")
	assert(output.item:get_count() == 1)
end
test_get_craft_result()
