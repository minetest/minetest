unittests = {}

core.log("info", "Hello World")

function unittests.async_test()
	assert(core == minetest)
	-- stuff that should not be here
	assert(not core.get_player_by_name)
	assert(not core.set_node)
	assert(not core.object_refs)
	-- stuff that should be here
	assert(ItemStack)
	assert(core.registered_items[""])
	return true
end
