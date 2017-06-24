local jobs = {}
local time = 0.0
local next_expiry = math.huge

core.register_globalstep(function(dtime)
	time = time + dtime

	if time < next_expiry then
		return
	end
	next_expiry = math.huge

	-- Iterate backwards so that we miss any new timers added by
	-- a timer callback, and so that we don't skip the next timer
	-- in the list if we remove one.
	for i = #jobs, 1, -1 do
		local job = jobs[i]
		if time >= job.expire then
			core.set_last_run_mod(job.mod_origin)
			job.func(unpack(job.arg))
			table.remove(jobs, i)
		elseif next_expiry > job.expire then
			next_expiry = job.expire
		end
	end
end)

function core.after(after, func, ...)
	assert(tonumber(after) and type(func) == "function",
		"Invalid core.after invocation")
	local expire = time + after
	next_expiry = math.min(next_expiry, expire)
	jobs[#jobs + 1] = {
		func = func,
		expire = expire,
		arg = {...},
		mod_origin = core.get_last_run_mod()
	}
end
