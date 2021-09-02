_G.core = {}
dofile("builtin/common/common_buffers.lua")

describe("common buffer", function()
	it("default index is 1", function()
		assert.equals(core.get_common_buffer(), core.get_common_buffer(1))
	end)

	it("indices are distinct", function()
		assert.is_not.equals(core.get_common_buffer(1), core.get_common_buffer(2))
	end)

	it("indices must be numeric", function()
		assert.has_error(function() core.get_common_buffer(false) end)
		assert.has_error(function() core.get_common_buffer("1") end)
		assert.has_error(function() core.get_common_buffer(0/0) end)
		assert.has_no.error(function() core.get_common_buffer(1.1) end)
	end)
end)
