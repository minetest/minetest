-- Test json with vector (can not be done in builtin unittests because the core
-- provides write_json from C++)

local function assert_vector_in_json(v, should)
	-- convert to json and back
	local json, err = core.write_json(v)
	assert(json, "could not write vector to json: " .. tostring(err))
	local after, err = core.parse_json(json)
	assert(after, "could not parse vector from json: " .. tostring(err))
	-- compare for equality
	local are_same = true
	for k, val in pairs(v) do
		if after[k] ~= val then
			are_same = false
		end
	end
	for k, val in pairs(after) do
		if v[k] ~= val then
			are_same = false
		end
	end
	assert(are_same, "vector-in-json error, should be: " .. dump(should) ..
		" but was: " .. dump(after))
end

function unittests.test_vector_json()
	local v = vector.new(1, 2, 3)
	assert_vector_in_json(v, {x = 1, y = 2, z = 3})

	v.a = "foo"
	assert_vector_in_json(v, {x = 1, y = 2, z = 3, a = "foo"})
end

