local function test_itemstack_equals()
	local i1 = ItemStack("default:stone")
	local i2 = ItemStack("default:stone")
	local i3 = ItemStack("default:stone")

	local m1 = i1:get_meta()
	local m2 = i2:get_meta()
	local m3 = i3:get_meta()

	local keys = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p"}
	local values = {}

	for i, key in pairs(keys) do
		m1:set_int(key, i)
		m3:set_int(key, i)
		values[key] = i
	end

	m3:set_int("a", 999)

	for key, i in pairs(values) do
		m2:set_int(key, i)
	end

	assert(i1:equals(i2))
	assert(i1 == i2)
	assert(not i1:equals(i3))
	assert(i1 ~= i3)
end

unittests.register("test_itemstack_equals", test_itemstack_equals)
