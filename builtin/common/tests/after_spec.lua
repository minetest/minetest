_G.core = {}

dofile("builtin/common/vector.lua")
dofile("builtin/common/misc_helpers.lua")

function core.get_last_run_mod() return "*test*" end
function core.set_last_run_mod() end

local do_step
function core.register_globalstep(func)
	do_step = func
end

dofile("builtin/common/after.lua")

describe("after", function()
	it("executes callbacks when expected", function()
		local result = ""
		core.after(0, function()
			result = result .. "a"
		end)
		core.after(1, function()
			result = result .. "b"
		end)
		core.after(1, function()
			result = result .. "c"
		end)
		core.after(2, function()
			result = result .. "d"
		end)
		local cancel = core.after(2, function()
			result = result .. "e"
		end)
		do_step(0)
		assert.same(result, "a")

		do_step(1)
		assert.same(result, "abc")

		core.after(2, function()
			result = result .. "f"
		end)
		core.after(1, function()
			result = result .. "g"
		end)
		core.after(-1, function()
			result = result .. "h"
		end)
		cancel:cancel()
		do_step(1)
		assert.same(result, "abchdg")

		do_step(1)
		assert.same(result, "abchdgf")
	end)

	it("defers jobs with delay 0", function()
		local result = ""
		core.after(0, function()
			core.after(0, function()
				result = result .. "b"
			end)
			result = result .. "a"
		end)
		do_step(1)
		assert.same(result, "a")
		do_step(1)
		assert.same(result, "ab")
	end)

	it("passes arguments", function()
		core.after(0, function(...)
			assert.same(select("#", ...), 0)
		end)
		core.after(0, function(...)
			assert.same(select("#", ...), 4)
			assert.same((select(1, ...)), 1)
			assert.same((select(2, ...)), nil)
			assert.same((select(3, ...)), "a")
			assert.same((select(4, ...)), nil)
		end, 1, nil, "a", nil)
		do_step(0)
	end)

	it("rejects invalid arguments", function()
		assert.has.errors(function() core.after() end)
		assert.has.errors(function() core.after(nil, nil) end)
		assert.has.errors(function() core.after(0) end)
		assert.has.errors(function() core.after(0, nil) end)
		assert.has.errors(function() core.after(nil, function() end) end)
		assert.has.errors(function() core.after(0 / 0, function() end) end)
	end)
end)
