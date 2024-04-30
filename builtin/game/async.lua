
core.async_jobs = {}

function core.async_event_handler(jobid, retval)
	local callback = core.async_jobs[jobid]
	assert(type(callback) == "function")
	callback(unpack(retval, 1, retval.n))
	core.async_jobs[jobid] = nil
end

function core.handle_async(func, callback, ...)
	assert(type(func) == "function" and type(callback) == "function",
		"Invalid core.handle_async invocation")
	local args = {n = select("#", ...), ...}
	local mod_origin = core.get_last_run_mod()

	local job = core.do_async_callback(func, args, mod_origin)
	core.async_jobs[job:get_id()] = callback

	return job
end

if core.async_job_methods then
	function core.async_job_methods:cancel()
		local id = self:get_id()
		local cancelled = core.cancel_async_callback(id)
		if cancelled then
			core.async_jobs[id] = nil
		end
		return cancelled
	end

	core.async_job_methods = nil
end
