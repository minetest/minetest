dofile(core.get_modpath(core.get_current_modname()) .. "/crafting_prepare.lua")

-- Test minetest.clear_craft function
local function test_clear_craft()
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
unittests.register("test_clear_craft", test_clear_craft)

-- Test minetest.get_craft_result function
local function test_get_craft_result()
	-- normal
	local input = {
		method = "normal",
		width = 2,
		items = {"", "unittests:coal_lump", "", "unittests:stick"}
	}
	minetest.log("info", "[unittests] torch crafting input: "..dump(input))
	local output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "[unittests] torch crafting output: "..dump(output))
	minetest.log("info", "[unittests] torch crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	minetest.log("info", "[unittests] torch crafting output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "unittests:torch")
	assert(output.item:get_count() == 4)

	-- fuel
	input = {
		method = "fuel",
		width = 1,
		items = {"unittests:coal_lump"}
	}
	minetest.log("info", "[unittests] coal fuel input: "..dump(input))
	output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "[unittests] coal fuel output: "..dump(output))
	minetest.log("info", "[unittests] coal fuel decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)

	-- cooking
	input = {
		method = "cooking",
		width = 1,
		items = {"unittests:iron_lump"}
	}
	minetest.log("info", "[unittests] iron lump cooking input: "..dump(output))
	output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "[unittests] iron lump cooking output: "..dump(output))
	minetest.log("info", "[unittests] iron lump cooking decremented input: "..dump(decremented_input))
	assert(output.time)
	assert(output.time > 0)
	assert(output.item)
	minetest.log("info", "[unittests] iron lump cooking output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "unittests:steel_ingot")
	assert(output.item:get_count() == 1)

	-- tool repair (repairable)
	input = {
		method = "normal",
		width = 2,
		-- Using a wear of 60000
		items = {"unittests:repairable_tool 1 60000", "unittests:repairable_tool 1 60000"}
	}
	minetest.log("info", "[unittests] repairable tool crafting input: "..dump(input))
	output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "[unittests] repairable tool crafting output: "..dump(output))
	minetest.log("info", "[unittests] repairable tool crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	minetest.log("info", "[unittests] repairable tool crafting output.item:to_table(): "..dump(output.item:to_table()))
	assert(output.item:get_name() == "unittests:repairable_tool")
	-- Test the wear value.
	-- See src/craftdef.cpp in Minetest source code for the formula. The formula to calculate
	-- the value 51187 is:
	--    65536 - ((65536-60000)+(65536-60000)) + floor(additonal_wear * 65536 + 0.5) = 51187
	-- where additional_wear = 0.05
	assert(output.item:get_wear() == 51187)
	assert(output.item:get_count() == 1)

	-- failing tool repair (unrepairable)
	input = {
		method = "normal",
		width = 2,
		items = {"unittests:unrepairable_tool 1 60000", "unittests:unrepairable_tool 1 60000"}
	}
	minetest.log("info", "[unittests] unrepairable tool crafting input: "..dump(input))
	output, decremented_input = minetest.get_craft_result(input)
	minetest.log("info", "[unittests] unrepairable tool crafting output: "..dump(output))
	minetest.log("info", "[unittests] unrepairable tool crafting decremented input: "..dump(decremented_input))
	assert(output.item)
	minetest.log("info", "[unittests] unrepairable tool crafting output.item:to_table(): "..dump(output.item:to_table()))
	-- unrepairable tool must not yield any output
	assert(output.item:is_empty())
end
unittests.register("test_get_craft_result", test_get_craft_result)
