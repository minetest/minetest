_G.vector = {}
dofile("builtin/common/vector.lua")

describe("vector", function()
	describe("new()", function()
		it("constructs", function()
			assert.same({ x = 0, y = 0, z = 0 }, vector.new())
			assert.same({ x = 1, y = 2, z = 3 }, vector.new(1, 2, 3))
			assert.same({ x = 3, y = 2, z = 1 }, vector.new({ x = 3, y = 2, z = 1 }))

			local input = vector.new({ x = 3, y = 2, z = 1 })
			local output = vector.new(input)
			assert.same(input, output)
			assert.are_not.equal(input, output)
		end)

		it("throws on invalid input", function()
			assert.has.errors(function()
				vector.new({ x = 3 })
			end)

			assert.has.errors(function()
				vector.new({ d = 3 })
			end)
		end)
	end)

	it("equal()", function()
			local function assertE(a, b)
				assert.is_true(vector.equals(a, b))
			end
			local function assertNE(a, b)
				assert.is_false(vector.equals(a, b))
			end

			assertE({x = 0, y = 0, z = 0}, {x = 0, y = 0, z = 0})
			assertE({x = -1, y = 0, z = 1}, {x = -1, y = 0, z = 1})
			local a = { x = 2, y = 4, z = -10 }
			assertE(a, a)
			assertNE({x = -1, y = 0, z = 1}, a)
	end)

	it("add()", function()
		assert.same({ x = 2, y = 4, z = 6 }, vector.add(vector.new(1, 2, 3), { x = 1, y = 2, z = 3 }))
	end)

	it("offset()", function()
		assert.same({ x = 41, y = 52, z = 63 }, vector.offset(vector.new(1, 2, 3), 40, 50, 60))
	end)

	-- This function is needed because of floating point imprecision.
	local function almost_equal(a, b)
		if type(a) == "number" then
			return math.abs(a - b) < 0.00000000001
		end
		return vector.distance(a, b) < 0.000000000001
	end

	describe("rotate_around_axis()", function()
		it("rotates", function()
			assert.True(almost_equal({x = -1, y = 0, z = 0},
				vector.rotate_around_axis({x = 1, y = 0, z = 0}, {x = 0, y = 1, z = 0}, math.pi)))
			assert.True(almost_equal({x = 0, y = 1, z = 0},
				vector.rotate_around_axis({x = 0, y = 0, z = 1}, {x = 1, y = 0, z = 0}, math.pi / 2)))
			assert.True(almost_equal({x = 4, y = 1, z = 1},
				vector.rotate_around_axis({x = 4, y = 1, z = 1}, {x = 4, y = 1, z = 1}, math.pi / 6)))
		end)
		it("keeps distance to axis", function()
			local rotate1 = {x = 1, y = 3, z = 1}
			local axis1 = {x = 1, y = 3, z = 2}
			local rotated1 = vector.rotate_around_axis(rotate1, axis1, math.pi / 13)
			assert.True(almost_equal(vector.distance(axis1, rotate1), vector.distance(axis1, rotated1)))
			local rotate2 = {x = 1, y = 1, z = 3}
			local axis2 = {x = 2, y = 6, z = 100}
			local rotated2 = vector.rotate_around_axis(rotate2, axis2, math.pi / 23)
			assert.True(almost_equal(vector.distance(axis2, rotate2), vector.distance(axis2, rotated2)))
			local rotate3 = {x = 1, y = -1, z = 3}
			local axis3 = {x = 2, y = 6, z = 100}
			local rotated3 = vector.rotate_around_axis(rotate3, axis3, math.pi / 2)
			assert.True(almost_equal(vector.distance(axis3, rotate3), vector.distance(axis3, rotated3)))
		end)
		it("rotates back", function()
			local rotate1 = {x = 1, y = 3, z = 1}
			local axis1 = {x = 1, y = 3, z = 2}
			local rotated1 = vector.rotate_around_axis(rotate1, axis1, math.pi / 13)
			rotated1 = vector.rotate_around_axis(rotated1, axis1, -math.pi / 13)
			assert.True(almost_equal(rotate1, rotated1))
			local rotate2 = {x = 1, y = 1, z = 3}
			local axis2 = {x = 2, y = 6, z = 100}
			local rotated2 = vector.rotate_around_axis(rotate2, axis2, math.pi / 23)
			rotated2 = vector.rotate_around_axis(rotated2, axis2, -math.pi / 23)
			assert.True(almost_equal(rotate2, rotated2))
			local rotate3 = {x = 1, y = -1, z = 3}
			local axis3 = {x = 2, y = 6, z = 100}
			local rotated3 = vector.rotate_around_axis(rotate3, axis3, math.pi / 2)
			rotated3 = vector.rotate_around_axis(rotated3, axis3, -math.pi / 2)
			assert.True(almost_equal(rotate3, rotated3))
		end)
		it("is right handed", function()
			local v_before1 = {x = 0, y = 1, z = -1}
			local v_after1 = vector.rotate_around_axis(v_before1, {x = 1, y = 0, z = 0}, math.pi / 4)
			assert.True(almost_equal(vector.normalize(vector.cross(v_after1, v_before1)), {x = 1, y = 0, z = 0}))

			local v_before2 = {x = 0, y = 3, z = 4}
			local v_after2 = vector.rotate_around_axis(v_before2, {x = 1, y = 0, z = 0},  2 * math.pi / 5)
			assert.True(almost_equal(vector.normalize(vector.cross(v_after2, v_before2)), {x = 1, y = 0, z = 0}))

			local v_before3 = {x = 1, y = 0, z = -1}
			local v_after3 = vector.rotate_around_axis(v_before3, {x = 0, y = 1, z = 0}, math.pi / 4)
			assert.True(almost_equal(vector.normalize(vector.cross(v_after3, v_before3)), {x = 0, y = 1, z = 0}))

			local v_before4 = {x = 3, y = 0, z = 4}
			local v_after4 = vector.rotate_around_axis(v_before4, {x = 0, y = 1, z = 0}, 2 * math.pi / 5)
			assert.True(almost_equal(vector.normalize(vector.cross(v_after4, v_before4)), {x = 0, y = 1, z = 0}))

			local v_before5 = {x = 1, y = -1, z = 0}
			local v_after5 = vector.rotate_around_axis(v_before5, {x = 0, y = 0, z = 1}, math.pi / 4)
			assert.True(almost_equal(vector.normalize(vector.cross(v_after5, v_before5)), {x = 0, y = 0, z = 1}))

			local v_before6 = {x = 3, y = 4, z = 0}
			local v_after6 = vector.rotate_around_axis(v_before6, {x = 0, y = 0, z = 1}, 2 * math.pi / 5)
			assert.True(almost_equal(vector.normalize(vector.cross(v_after6, v_before6)), {x = 0, y = 0, z = 1}))
		end)
	end)

	describe("rotate()", function()
		it("rotates", function()
			assert.True(almost_equal({x = -1, y = 0, z = 0},
				vector.rotate({x = 1, y = 0, z = 0}, {x = 0, y = math.pi, z = 0})))
			assert.True(almost_equal({x = 0, y = -1, z = 0},
				vector.rotate({x = 1, y = 0, z = 0}, {x = 0, y = 0, z = math.pi / 2})))
			assert.True(almost_equal({x = 1, y = 0, z = 0},
				vector.rotate({x = 1, y = 0, z = 0}, {x = math.pi / 123, y = 0, z = 0})))
		end)
		it("is counterclockwise", function()
			local v_before1 = {x = 0, y = 1, z = -1}
			local v_after1 = vector.rotate(v_before1, {x = math.pi / 4, y = 0, z = 0})
			assert.True(almost_equal(vector.normalize(vector.cross(v_after1, v_before1)), {x = 1, y = 0, z = 0}))

			local v_before2 = {x = 0, y = 3, z = 4}
			local v_after2 = vector.rotate(v_before2, {x = 2 * math.pi / 5, y = 0, z = 0})
			assert.True(almost_equal(vector.normalize(vector.cross(v_after2, v_before2)), {x = 1, y = 0, z = 0}))

			local v_before3 = {x = 1, y = 0, z = -1}
			local v_after3 = vector.rotate(v_before3, {x = 0, y = math.pi / 4, z = 0})
			assert.True(almost_equal(vector.normalize(vector.cross(v_after3, v_before3)), {x = 0, y = 1, z = 0}))

			local v_before4 = {x = 3, y = 0, z = 4}
			local v_after4 = vector.rotate(v_before4, {x = 0, y = 2 * math.pi / 5, z = 0})
			assert.True(almost_equal(vector.normalize(vector.cross(v_after4, v_before4)), {x = 0, y = 1, z = 0}))

			local v_before5 = {x = 1, y = -1, z = 0}
			local v_after5 = vector.rotate(v_before5, {x = 0, y = 0, z = math.pi / 4})
			assert.True(almost_equal(vector.normalize(vector.cross(v_after5, v_before5)), {x = 0, y = 0, z = 1}))

			local v_before6 = {x = 3, y = 4, z = 0}
			local v_after6 = vector.rotate(v_before6, {x = 0, y = 0, z = 2 * math.pi / 5})
			assert.True(almost_equal(vector.normalize(vector.cross(v_after6, v_before6)), {x = 0, y = 0, z = 1}))
		end)
	end)

	it("dir_to_rotation()", function()
		-- Comparing rotations (pitch, yaw, roll) is hard because of certain ambiguities,
		-- e.g. (pi, 0, pi) looks exactly the same as (0, pi, 0)
		-- So instead we convert the rotation back to vectors and compare these.
		local function forward_at_rot(rot)
			return vector.rotate(vector.new(0, 0, 1), rot)
		end
		local function up_at_rot(rot)
			return vector.rotate(vector.new(0, 1, 0), rot)
		end
		local rot1 = vector.dir_to_rotation({x = 1, y = 0, z = 0}, {x = 0, y = 1, z = 0})
		assert.True(almost_equal({x = 1, y = 0, z = 0}, forward_at_rot(rot1)))
		assert.True(almost_equal({x = 0, y = 1, z = 0}, up_at_rot(rot1)))
		local rot2 = vector.dir_to_rotation({x = 1, y = 1, z = 0}, {x = 0, y = 0, z = 1})
		assert.True(almost_equal({x = 1/math.sqrt(2), y = 1/math.sqrt(2), z = 0}, forward_at_rot(rot2)))
		assert.True(almost_equal({x = 0, y = 0, z = 1}, up_at_rot(rot2)))
		for i = 1, 1000 do
			local rand_vec = vector.new(math.random(), math.random(), math.random())
			if vector.length(rand_vec) ~= 0 then
				local rot_1 = vector.dir_to_rotation(rand_vec)
				local rot_2 = {
					x = math.atan2(rand_vec.y, math.sqrt(rand_vec.z * rand_vec.z + rand_vec.x * rand_vec.x)),
					y = -math.atan2(rand_vec.x, rand_vec.z),
					z = 0
				}
				assert.True(almost_equal(rot_1, rot_2))
			end
		end

	end)
end)
