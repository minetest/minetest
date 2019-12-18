local function test_clear_craft()
	minetest.log("info", "Testing clear_craft")
	-- Clearing by output
	minetest.register_craft({
		output = "foo",
		recipe = {{"bar"}}
	})
	minetest.register_craft({
		output = "foo 4",
		recipe = {{"foo", "bar"}}
	})
	assert(#minetest.get_all_craft_recipes("foo") == 2)
	minetest.clear_craft({output="foo"})
	assert(minetest.get_all_craft_recipes("foo") == nil)
	-- Clearing by input
	minetest.register_craft({
		output = "foo 4",
		recipe = {{"foo", "bar"}}
	})
	assert(#minetest.get_all_craft_recipes("foo") == 1)
	minetest.clear_craft({recipe={{"foo", "bar"}}})
	assert(minetest.get_all_craft_recipes("foo") == nil)
end
test_clear_craft()

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
