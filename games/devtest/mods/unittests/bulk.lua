
-- Test bulk get and set
local function test_bulk_get_set(_,pos)
	local positions = {}
	for n = 0, 10, 1 do
		table.insert(positions, vector.add(pos, vector.new(0, n, 0)))
	end
	local nodes = minetest.bulk_get_node(positions)

	for ipos, npos in pairs(positions) do
		local node = minetest.get_node(npos)
		assert(node.name == nodes[ipos].name)
		assert(node.param1 == nodes[ipos].param1)
		assert(node.param2 == nodes[ipos].param2)
	end

	minetest.bulk_set_node(positions, {name="basenodes:stone"})

	for _, npos in pairs(positions) do
		local node = minetest.get_node(npos)
		assert(node.name == "basenodes:stone")
		assert(node.param1 == 0)
		assert(node.param2 == 0)
	end

	for ipos, npos in pairs(positions) do
		minetest.set_node(npos, nodes[ipos])
	end
end
unittests.register("test_bulk_get_set", test_bulk_get_set, {map=true})
