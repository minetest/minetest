-- helper

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

	-- Circular key, should error
	local tmp = {"foo", "bar"}
	tmp[tmp] = true
	assert(not pcall(core.serialize_roundtrip, tmp))

	-- Circular value, should error
	local tmp = {"foo"}
	tmp[2] = tmp
	assert(not pcall(core.serialize_roundtrip, tmp))
end
unittests.register("test_object_passing", test_object_passing)

local function test_object_passing2()
	-- basic userdata passing
	local obj = table.copy(test_object.tiles[1])
	obj.test = ItemStack("default:cobble 99")
	local tmp = core.serialize_roundtrip(obj)
	assert(type(tmp.test) == "userdata")
	assert(obj.test:to_string() == tmp.test:to_string())

	-- object can't be passed, should error
	local obj = core.raycast(vector.new(), vector.new())
	assert(not pcall(core.serialize_roundtrip, obj))

	-- (add vmanip test here when ready)
end
unittests.register("test_object_passing2", test_object_passing2)

-- Asynchronous jobs

local function test_handle_async(cb)
	local func = function(x)
		return _VERSION, type(_G), jit and jit[x] or ("no " .. x)
	end

	local expect = {func("arch")}

	core.handle_async(func, function(...)
		if not deepequal(expect, {...}) then
			cb("Values did not equal")
		end
		cb()
	end, "arch")
end
unittests.register("test_handle_async", test_handle_async, {async=true})
