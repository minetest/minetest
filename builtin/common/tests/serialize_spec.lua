_G.core = {}

_G.setfenv = require 'busted.compatibility'.setfenv

dofile("builtin/common/serialize.lua")
dofile("builtin/common/vector.lua")

describe("serialize", function()
	it("works", function()
		local test_in = {cat={sound="nyan", speed=400}, dog={sound="woof"}}
		local test_out = core.deserialize(core.serialize(test_in))

		assert.same(test_in, test_out)
	end)

	it("handles characters", function()
		local test_in = {escape_chars="\n\r\t\v\\\"\'", non_european="θשׁ٩∂"}
		local test_out = core.deserialize(core.serialize(test_in))
		assert.same(test_in, test_out)
	end)

	it("handles precise numbers", function()
		local test_in = 0.2695949158945771
		local test_out = core.deserialize(core.serialize(test_in))
		assert.same(test_in, test_out)
	end)

	it("handles big integers", function()
		local test_in = 269594915894577
		local test_out = core.deserialize(core.serialize(test_in))
		assert.same(test_in, test_out)
	end)

	it("handles recursive structures", function()
		local test_in = { hello = "world" }
		test_in.foo = test_in

		local test_out = core.deserialize(core.serialize(test_in))
		assert.same(test_in, test_out)
	end)

	it("strips functions in safe mode", function()
		local test_in = {
			func = function(a, b)
				error("test")
			end,
			foo = "bar"
		}

		local str = core.serialize(test_in)
		assert.not_nil(str:find("loadstring"))

		local test_out = core.deserialize(str, true)
		assert.is_nil(test_out.func)
		assert.equals(test_out.foo, "bar")
	end)

	it("vectors work", function()
		local v = vector.new(1, 2, 3)
		assert.same({{x = 1, y = 2, z = 3}}, core.deserialize(core.serialize({v})))
		assert.same({x = 1, y = 2, z = 3}, core.deserialize(core.serialize(v)))

		-- abuse
		v = vector.new(1, 2, 3)
		v.a = "bla"
		assert.same({x = 1, y = 2, z = 3, a = "bla"},
				core.deserialize(core.serialize(v)))
	end)
end)
