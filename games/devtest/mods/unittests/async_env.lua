-- helper

core.register_async_dofile(core.get_modpath(core.get_current_modname()) ..
	DIR_DELIM .. "inside_async_env.lua")

local function deepequal(a, b)
	if type(a) == "function" then
		return type(b) == "function"
	elseif type(a) ~= "table" then
		return a == b
	elseif type(b) ~= "table" then
		return false
	end
	for k, v in pairs(a) do
		if not deepequal(v, b[k]) then
			return false
		end
	end
	for k, v in pairs(b) do
		if not deepequal(a[k], v) then
			return false
		end
	end
	return true
end

-- Object Passing / Serialization

local test_object = {
	name = "stairs:stair_glass",
	type = "node",
	groups = {oddly_breakable_by_hand = 3, cracky = 3, stair = 1},
	description = "Glass Stair",
	sounds = {
		dig = {name = "default_glass_footstep", gain = 0.5},
		footstep = {name = "default_glass_footstep", gain = 0.3},
		dug = {name = "default_break_glass", gain = 1}
	},
	node_box = {
		fixed = {
			{-0.5, -0.5, -0.5, 0.5, 0, 0.5},
			{-0.5, 0, 0, 0.5, 0.5, 0.5}
		},
		type = "fixed"
	},
	tiles = {
		{name = "stairs_glass_split.png", backface_culling = true},
		{name = "default_glass.png", backface_culling = true},
		{name = "stairs_glass_stairside.png^[transformFX", backface_culling = true}
	},
	on_place = function(itemstack, placer)
		return core.is_player(placer)
	end,
	sunlight_propagates = true,
	is_ground_content = false,
	light_source = 0,
}

local function test_object_passing()
	local tmp = core.serialize_roundtrip(test_object)
	assert(deepequal(test_object, tmp))

	local circular_key = {"foo", "bar"}
	circular_key[circular_key] = true
	tmp = core.serialize_roundtrip(circular_key)
	assert(tmp[1] == "foo")
	assert(tmp[2] == "bar")
	assert(tmp[tmp] == true)

	local circular_value = {"foo"}
	circular_value[2] = circular_value
	tmp = core.serialize_roundtrip(circular_value)
	assert(tmp[1] == "foo")
	assert(tmp[2] == tmp)

	-- Two-segment cycle
	local cycle_seg_1, cycle_seg_2 = {}, {}
	cycle_seg_1[1] = cycle_seg_2
	cycle_seg_2[1] = cycle_seg_1
	tmp = core.serialize_roundtrip(cycle_seg_1)
	assert(tmp[1][1] == tmp)

	-- Duplicated value without a cycle
	local acyclic_dup_holder = {}
	tmp = ItemStack("")
	acyclic_dup_holder[tmp] = tmp
	tmp = core.serialize_roundtrip(acyclic_dup_holder)
	for k, v in pairs(tmp) do
		assert(rawequal(k, v))
	end
end
unittests.register("test_object_passing", test_object_passing)

local function test_userdata_passing(_, pos)
	-- basic userdata passing
	local obj = table.copy(test_object.tiles[1])
	obj.test = ItemStack("default:cobble 99")
	local tmp = core.serialize_roundtrip(obj)
	assert(type(tmp.test) == "userdata")
	assert(obj.test:to_string() == tmp.test:to_string())

	-- object can't be passed, should error
	obj = core.raycast(pos, pos)
	assert(not pcall(core.serialize_roundtrip, obj))

	-- VManip
	local vm = core.get_voxel_manip(pos, pos)
	local expect = vm:get_node_at(pos)
	local vm2 = core.serialize_roundtrip(vm)
	assert(deepequal(vm2:get_node_at(pos), expect))
end
unittests.register("test_userdata_passing", test_userdata_passing, {map=true})

-- Asynchronous jobs

local function test_handle_async(cb)
	-- Basic test including mod name tracking and unittests.async_test()
	-- which is defined inside_async_env.lua
	local func = function(x)
		return core.get_last_run_mod(), _VERSION, unittests[x]()
	end
	local expect = {core.get_last_run_mod(), _VERSION, true}

	core.handle_async(func, function(...)
		if not deepequal(expect, {...}) then
			return cb("Values did not equal")
		end
		if core.get_last_run_mod() ~= expect[1] then
			return cb("Mod name not tracked correctly")
		end

		-- Test passing of nil arguments and return values
		core.handle_async(function(a, b)
			return a, b
		end, function(a, b)
			if b ~= 123 then
				return cb("Argument went missing")
			end
			cb()
		end, nil, 123)
	end, "async_test")
end
unittests.register("test_handle_async", test_handle_async, {async=true})

local function test_userdata_passing2(cb, _, pos)
	-- VManip: check transfer into other env
	local vm = core.get_voxel_manip(pos, pos)
	local expect = vm:get_node_at(pos)

	core.handle_async(function(vm_, pos_)
		return vm_:get_node_at(pos_)
	end, function(ret)
		if not deepequal(expect, ret) then
			return cb("Node data mismatch (one-way)")
		end

		-- VManip: test a roundtrip
		core.handle_async(function(vm_)
			return vm_
		end, function(vm2)
			if not deepequal(expect, vm2:get_node_at(pos)) then
				return cb("Node data mismatch (roundtrip)")
			end
			cb()
		end, vm)
	end, vm, pos)
end
unittests.register("test_userdata_passing2", test_userdata_passing2, {map=true, async=true})
