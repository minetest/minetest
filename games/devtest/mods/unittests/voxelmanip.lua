local function test_vm_empty()
	local vm = VoxelManip()

	assert(#vm:get_data() == 0)
	assert(#vm:get_light_data() == 0)
	assert(#vm:get_param2_data() == 0)

	vm:set_data(vm:get_data())

	local pos = vector.new(0, 0, 0)
	local node = vm:get_node_at(pos)
	assert(node.name == "ignore")
	assert(node.param1 == 0)
	assert(node.param2 == 0)
	vm:set_node_at(pos, node)
end
unittests.register("test_vm_empty", test_vm_empty, {map=true})

local function test_vm_accessors()
	local vm = VoxelManip(vector.new(-10, -10, -10), vector.new(20, 20, 20))
	local emin, emax = vm:get_emerged_area()
	local area = VoxelArea:new{MinEdge = emin, MaxEdge = emax}

	local pos = vector.new(-4, 5, 10)
	local index = area:indexp(pos)

	vm:set_node_at(pos, {name = "basenodes:desert_stone", param2 = 3})

	local data = {}
	assert(data == vm:get_data(data))
	local light_data = vm:get_light_data()
	local param2_data = {}
	assert(param2_data == vm:get_param2_data(param2_data))
	assert(#data == area:getVolume())
	assert(#light_data == area:getVolume())
	assert(#param2_data == area:getVolume())

	assert(data[index] == core.get_content_id("basenodes:desert_stone"))
	assert(light_data[index] == 0)
	assert(param2_data[index] == 3)

	data[index] = core.get_content_id("basenodes:ice")
	light_data[index] = 3
	param2_data[index] = 255
	vm:set_data(data)
	vm:set_light_data(light_data)
	vm:set_param2_data(param2_data)

	local node = vm:get_node_at(pos)
	assert(node.name == "basenodes:ice")
	assert(node.param1 == 3)
	assert(node.param2 == 255)

	vm:set_node_at(pos:add(0.51), {name = "basenodes:desert_stone"})
	assert(vm:get_node_at(pos:add(0.49)).name == "basenodes:ice")
	assert(vm:get_node_at(pos:add(1)).name == "basenodes:desert_stone")

	data[index] = 0xFFFF
	vm:set_data(data)
	assert(vm:get_node_at(pos).name == "unknown")
end
unittests.register("test_vm_accessors", test_vm_accessors, {map=true})

local function test_vm_invalid_input()
	local vm = VoxelManip(vector.new(-10, -10, -10), vector.new(20, 20, 20))
	local emin, emax = vm:get_emerged_area()
	local area = VoxelArea:new{MinEdge = emin, MaxEdge = emax}

	local pos = vector.new(-4, 5, 10)
	local index = area:indexp(pos)

	assert(not pcall(vm.get_node_at))
	assert(not pcall(vm.get_node_at, {}))
	assert(not pcall(vm.set_node_at, vm, pos, {name = "odfnfj", param2 = 3}))
	assert(not pcall(vm.get_node_at, vm, nil))
	assert(not pcall(vm.set_node_at, vm, {}, {name = "air"}))
	assert(not pcall(vm.set_data, vm, ""))

	vm:set_node_at(pos, {name = "basenodes:desert_stone", param1 = true, param2 = 4.5})

	local data = vm:get_data(1)
	local light_data = vm:get_light_data(2)
	local param2_data = vm:get_param2_data(3)

	assert(data[index] == core.get_content_id("basenodes:desert_stone"))
	assert(light_data[index] == 0)
	assert(param2_data[index] ~= 0)

	-- Unspecified results:
	data[index] = -1.23
	vm:set_data(data)
	data[index] = 0 / 0
	vm:set_data(data)
	data[index] = 1 / 0
	vm:set_data(data)
	data[index] = -1 / 0
	vm:set_data(data)

	data[index] = "basenodes:ice"
	light_data[index] = "d"
	param2_data[index] = "4"
	vm:set_data(data)
	vm:set_light_data(light_data)
	vm:set_param2_data(param2_data)

	local node = vm:get_node_at(pos)
	assert(node.name == core.get_name_from_content_id(0))
	assert(node.param1 == 0)
	assert(node.param2 == 4)
end
unittests.register("test_vm_invalid_input", test_vm_invalid_input, {map=true})
