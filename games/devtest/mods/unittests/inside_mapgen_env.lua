core.log("info", "Hello World")

local function do_tests()
	assert(core == minetest)
	-- stuff that should not be here
	assert(not core.get_player_by_name)
	assert(not core.object_refs)
	-- stuff that should be here
	assert(core.register_on_generated)
	assert(core.get_node or true) -- TODO
	assert(ItemStack)
	assert(core.registered_items[""])
	-- alias handling
	assert(core.registered_items["unittests:steel_ingot_alias"].name ==
		"unittests:steel_ingot")
end

-- there's no (usable) communcation path between mapgen and the regular env
-- so we just run the test unconditionally :shrug:
do_tests()
