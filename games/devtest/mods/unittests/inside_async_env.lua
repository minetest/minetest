unittests = {}

core.log("info", "Hello World")

local function do_tests()
	assert(core == minetest)
	-- stuff that should not be here
	assert(not core.get_player_by_name)
	assert(not core.set_node)
	assert(not core.object_refs)
	-- stuff that should be here
	assert(ItemStack)
	assert(core.registered_items[""])
	-- alias handling
	assert(core.registered_items["unittests:steel_ingot_alias"].name ==
		"unittests:steel_ingot")
end

function unittests.async_test()
	local ok, err = pcall(do_tests)
	if not ok then
		core.log("error", err)
	end
	return ok
end
