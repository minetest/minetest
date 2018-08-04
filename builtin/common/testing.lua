local base = ";" .. core.get_builtin_path() .. "common/"
package.path = package.path .. base .. "?.lua" .. base .. "?/init.lua"
assert       = require("luassert")

local total_acc = {
	successes = 0,
	failures = 0,
}

local state = nil
function describe(title, func)
	assert(not state)
	state = {
		title     = title,
		successes = 0,
		failures  = 0,
	}


	-- Run it
	func()

	total_acc.successes = total_acc.successes + state.successes
	total_acc.failures  = total_acc.failures  + state.failures

	core.log(state.failures == 0 and "info" or "error",
		("[%s] %d successes / %d failures"):format(title, state.successes, state.failures))

	state = nil
end

function it(text, func)
	assert(state)

	local good, errOrResult = pcall(func)
	if good then
		state.successes = state.successes + 1
	else
		if state.failures == 0 then
			core.log("error", state.title)
		end
		state.failures = state.failures + 1

		core.log("error", "- " .. text)
		core.log("error", errOrResult)
	end
end

function core.has_failed_tests()
	return total_acc.failures > 0
end
