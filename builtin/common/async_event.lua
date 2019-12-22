
minetest.async_jobs = {}

local function handle_job(jobid, serialized_retval)
	local retval = minetest.deserialize(serialized_retval)
	assert(type(minetest.async_jobs[jobid]) == "function")
	minetest.async_jobs[jobid](retval)
	minetest.async_jobs[jobid] = nil
end

if minetest.register_globalstep then
	minetest.register_globalstep(function(dtime)
		for i, job in ipairs(minetest.get_finished_jobs()) do
			handle_job(job.jobid, job.retval)
		end
	end)
else
	minetest.async_event_handler = handle_job
end

function minetest.handle_async(func, parameter, callback)
	-- Serialize function
	local serialized_func = string.dump(func)

	assert(serialized_func ~= nil)

	-- Serialize parameters
	local serialized_param = minetest.serialize(parameter)

	if serialized_param == nil then
		return false
	end

	local jobid = minetest.do_async_callback(serialized_func, serialized_param)

	minetest.async_jobs[jobid] = callback

	return true
end

