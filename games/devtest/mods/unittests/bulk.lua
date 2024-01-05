
-- Test bulk get and set
local function test_bulk_get_set(_,pos)
	local positions = {}
	for n = 0, 10, 1 do
		table.insert(positions, vector.add(pos, vector.new(0, n, 0)))
	end
	local nodes = minetest.bulk_get_node(positions)

	minetest.bulk_set_node(positions, {name="basenodes:stone"})

	for index,pos in pairs(positions) do
		minetest.set_node(pos, nodes[index])
	end
end
unittests.register("test_bulk_get_set", test_bulk_get_set, {map=true})
