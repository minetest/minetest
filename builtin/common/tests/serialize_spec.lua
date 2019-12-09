_G.core = {}

_G.setfenv = function() end

dofile("builtin/common/serialize.lua")

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

	it("handles recursive structures", function()
		local test_in = { hello = "world" }
		test_in.foo = test_in

		local test_out = core.deserialize(core.serialize(test_in))
		assert.same(test_in, test_out)
	end)
end)
