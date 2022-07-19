-- Tests of generic metadata functionality

local function test_metadata(meta)
	local compare_meta = ItemStack("unittests:iron_lump"):get_meta()
	compare_meta:from_table({
		fields = {
			a = "1",
			b = "2",
			c = "3",
			d = "4",
			e = "e",
		},
	})

	meta:from_table({fields = {a = 1, b = "2"}})
	meta:set_string("c", "3")
	meta:set_int("d", 4)
	meta:set_string("e", "e")

	meta:set_string("", "!")
	meta:set_string("", "")

	assert(meta:equals(compare_meta))

	local tab = meta:to_table()
	assert(tab.fields.a == "1")
	assert(tab.fields.b == "2")
	assert(tab.fields.c == "3")
	assert(tab.fields.d == "4")
	assert(tab.fields.e == "e")

	assert(not meta:contains(""))
	assert(meta:contains("a"))
	assert(meta:contains("b"))
	assert(meta:contains("c"))
	assert(meta:contains("d"))
	assert(meta:contains("e"))

	assert(meta:get("") == nil)
	assert(meta:get_string("") == "")
	assert(meta:get_int("") == 0)
	assert(meta:get_float("") == 0.0)
	assert(meta:get("a") == "1")
	assert(meta:get_string("a") == "1")
	assert(meta:get_int("a") == 1)
	assert(meta:get_float("a") == 1.0)
	assert(meta:get_int("e") == 0)
	assert(meta:get_float("e") == 0.0)
	meta:set_float("f", 1.1)
	assert(meta:get_float("f") > 1)

	meta:from_table()
	assert(next(meta:to_table().fields) == nil)

	assert(not meta:equals(compare_meta))
end

local storage = core.get_mod_storage()
local function test_mod_storage()
	test_metadata(storage)
end
unittests.register("test_mod_storage", test_mod_storage)

local function test_item_metadata()
	test_metadata(ItemStack("unittest:coal_lump"):get_meta())
end
unittests.register("test_item_metadata", test_item_metadata)

local function test_node_metadata(player, pos)
	test_metadata(minetest.get_meta(pos))
end
unittests.register("test_node_metadata", test_node_metadata, {map=true})
