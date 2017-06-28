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
		if not jobs[1] then
			jobs = nil
			break
		end
		local root = jobs[#jobs]
		for i = #jobs-1, 1, -1 do
			local root2 = jobs[i]
			if root.expire > root2.expire then
				root,root2 = root2,root
			end
			root.children[#root.children+1] = root2
		end
		jobs = root
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
