local jobs = {}
local time = 0.0
local time_next = math.huge

core.register_globalstep(function(dtime)
	time = time + dtime

	if time < time_next then
		return
	end

	time_next = math.huge

	-- Iterate backwards so that we miss any new timers added by
	-- a timer callback, and so that we don't skip the next timer
	-- in the list if we remove one.
	for i = #jobs, 1, -1 do
		local job = jobs[i]
		if time >= job.expire then
			core.set_last_run_mod(job.mod_origin)
			job.func(unpack(job.arg))
			jobs[i] = jobs[#jobs]
			jobs[#jobs] = nil
		else
			time_next = math.min(time_next, job.expire)
		end
	end
end)

function core.after(after, func, ...)
	assert(tonumber(after) and type(func) == "function",
		"Invalid core.after invocation")
	local expire = time + after
	jobs[#jobs + 1] = {
		func = func,
		expire = expire,
		arg = {...},
		mod_origin = core.get_last_run_mod()
	}
	time_next = math.min(time_next, expire)
end
