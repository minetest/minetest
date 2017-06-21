local jobs
local time = 0.0

core.register_globalstep(function(dtime)
	time = time + dtime

	if not jobs then
		return
	end

	local to_execute = {}
	while time >= jobs.expire do
		to_execute[#to_execute+1] = jobs.data
		jobs = jobs.children
		if not jobs then
			break
		end
		local from = #jobs
		local to = 1
		while from > to do
			to = to-1
			local a = jobs[from]
			local b = jobs[from-1]
			if a.expire > b.expire then
				b,a = a,b
			end
			a.children[#a.children+1] = b
			jobs[to] = a
			from = from - 2
		end
		jobs = jobs[to]
	end
	for i = 1,#to_execute do
		core.set_last_run_mod(to_execute[i].mod_origin)
		to_execute[i].func(unpack(to_execute[i].arg))
	end
end)

function core.after(after, func, ...)
	assert(tonumber(after) and type(func) == "function",
		"Invalid core.after invocation")
	local job = {
		expire = time + after,
		children = {},
		data = {
			func = func,
			arg = {...},
			mod_origin = core.get_last_run_mod(),
		}
	}
	if jobs then
		if jobs.expire > job.expire then
			jobs,job = job,jobs
		end
		jobs.children[#jobs.children+1] = job
	else
		jobs = job
	end
end
