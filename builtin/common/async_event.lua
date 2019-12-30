core.async_jobs = {}

local function handle_job(jobid, serialized_retval)
	local retval = core.deserialize(serialized_retval)
	assert(type(core.async_jobs[jobid]) == "function")
	core.async_jobs[jobid](retval)
	core.async_jobs[jobid] = nil
end

if core.register_globalstep then
	core.register_globalstep(function()
		for _, job in ipairs(core.get_finished_jobs()) do
			handle_job(job.jobid, job.retval)
		end
	end)
else
	core.async_event_handler = handle_job
end

function core.handle_async(func, parameter, callback)
	-- Serialize function
	local serialized_func = string.dump(func)

	assert(serialized_func ~= nil)

	-- Serialize parameters
	local serialized_param = core.serialize(parameter)

	if serialized_param == nil then
		return false
	end

	local jobid = core.do_async_callback(serialized_func, serialized_param)

	core.async_jobs[jobid] = callback

	return true
end
