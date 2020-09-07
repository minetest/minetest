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
end)
