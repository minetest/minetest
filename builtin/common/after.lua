local jobs = {}
local time = 0.0

core.register_globalstep(function(dtime)
	time = time + dtime

	if #jobs < 1 then
		return
	end

	-- Iterate backwards so that we miss any new timers added by
	-- a timer callback, and so that we don't skip the next timer
	-- in the list if we remove one.
	for i = #jobs, 1, -1 do
		local job = jobs[i]
		if time >= job.expire then
			core.set_last_run_mod(job.mod_origin)
			job.func(unpack(job.arg))
			table.remove(jobs, i)
		end
	end
end)

function core.after(after, func, ...)
	assert(tonumber(after) and type(func) == "function",
		"Invalid core.after invocation")
	jobs[#jobs + 1] = {
		func = func,
		expire = time + after,
		arg = {...},
		mod_origin = core.get_last_run_mod()
	}
end
