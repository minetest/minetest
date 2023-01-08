benchmarks = {}

local using_ffi = core.global_exists("jit") and core.settings:get_bool("secure.use_ffi", true)

-- Safeguard against too much optimization. This way the results cannot be optimized
-- away, but they can be garbage collected (due to __mode = "k").
benchmarks.content_ids_data = setmetatable({}, {__mode = "k"})

local function bench_name2content()
	local t = {}
	benchmarks.content_ids_data[t] = true

	local get_content_id = minetest.get_content_id

	local start = minetest.get_us_time()

	for i = 1, 200 do
		for name in pairs(minetest.registered_nodes) do
			t[#t + 1] = get_content_id(name)
		end
	end

	local finish = minetest.get_us_time()

	return (finish - start) / 1000
end

local function bench_content2name()
	local t = {}
	benchmarks.content_ids_data[t] = true

	-- Try to estimate the highest content ID that's used
	-- (not accurate but good enough for this test)
	local n = 0
	for _ in pairs(minetest.registered_nodes) do
		n = n + 1
	end

	local get_name_from_content_id = minetest.get_name_from_content_id

	local start = minetest.get_us_time()

	for i = 1, 200 do
		for j = 0, n do
			t[#t + 1] = get_name_from_content_id(j)
		end
	end

	local finish = minetest.get_us_time()

	return (finish - start) / 1000
end

minetest.register_chatcommand("bench_name2content", {
	params = "",
	description = "Benchmark: Conversion from node names to content IDs",
	func = function(name, param)
		minetest.chat_send_player(name, "Benchmarking minetest.get_content_id. Warming up ...")
		bench_name2content()
		minetest.chat_send_player(name, "Warming up finished, now benchmarking ...")
		local time = bench_name2content()
		return true, ("Time: %.2f ms"):format(time)
	end,
})

minetest.register_chatcommand("bench_content2name", {
	params = "",
	description = "Benchmark: Conversion from content IDs to node names",
	func = function(name, param)
		minetest.chat_send_player(name, "Benchmarking minetest.get_name_from_content_id. Warming up ...")
		bench_content2name()
		minetest.chat_send_player(name, "Warming up finished, now benchmarking ...")
		local time = bench_content2name()
		return true, ("Time: %.2f ms"):format(time)
	end,
})

minetest.register_chatcommand("bench_bulk_set_node", {
	params = "",
	description = "Benchmark: Bulk-set 99×99×99 stone nodes",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local pos_list = {}
		local ppos = player:get_pos()
		local i = 1
		for x=2,100 do
			for y=2,100 do
				for z=2,100 do
					pos_list[i] = {x=ppos.x + x,y = ppos.y + y,z = ppos.z + z}
					i = i + 1
				end
			end
		end

		minetest.chat_send_player(name, "Benchmarking minetest.bulk_set_node. Warming up ...");

		-- warm up with stone to prevent having different callbacks
		-- due to different node topology
		minetest.bulk_set_node(pos_list, {name = "mapgen_stone"})

		minetest.chat_send_player(name, "Warming up finished, now benchmarking ...");

		local start_time = minetest.get_us_time()
		for i=1,#pos_list do
			minetest.set_node(pos_list[i], {name = "mapgen_stone"})
		end
		local middle_time = minetest.get_us_time()
		minetest.bulk_set_node(pos_list, {name = "mapgen_stone"})
		local end_time = minetest.get_us_time()
		local msg = string.format("Benchmark results: minetest.set_node loop: %.2f ms; minetest.bulk_set_node: %.2f ms",
			((middle_time - start_time)) / 1000,
			((end_time - middle_time)) / 1000
		)
		return true, msg
	end,
})

local vm_data_buf, vm_light_data_buf, vm_param2_data_buf = {}, {}, {}
-- Theoretically prevent the results from being optimized away:
benchmarks.vm_data = {vm_data_buf, vm_light_data_buf, vm_param2_data_buf}

local math_random, math_min, math_max = math.random, math.min, math.max
local function bench_vm_bulk_accessors()
	local vm = VoxelManip(vector.new(-50, -50, -50), vector.new(50, 50, 50))

	local start = minetest.get_us_time()

	local data = vm:get_data(vm_data_buf)
	local light_data = vm:get_light_data(vm_light_data_buf)
	local param2_data = vm:get_param2_data(vm_param2_data_buf)
	for i = 1, #data do
		data[i] = math_max(0, math_min(0x7FFF, data[i] + math_random(-5, 5)))
		light_data[i] = math_max(0, math_min(0xFF, light_data[i] + math_random(-5, 5)))
		param2_data[i] = math_max(0, math_min(0xFF, param2_data[i] + math_random(-5, 5)))
	end
	vm:set_data(data)
	vm:set_light_data(light_data)
	vm:set_param2_data(param2_data)

	local finish = minetest.get_us_time()

	return (finish - start) / 1000
end

minetest.register_chatcommand("bench_vm_bulk_accessors", {
	params = "",
	description = "Benchmark: VoxelManip bulk accessor functions",
	func = function(name, param)
		minetest.chat_send_player(name, "Benchmarking bulk accessors with FFI " ..
			(using_ffi and "enabled" or "disabled"))

		minetest.chat_send_player(name, "Warming up ...")

		bench_vm_bulk_accessors()

		minetest.chat_send_player(name, "Warming up finished, now benchmarking ...");

		local time = bench_vm_bulk_accessors()

		return true, string.format("Benchmark results: %.2f ms", time)
	end,
})
