local tbl = engine or minetest

local SCRIPTDIR = SCRIPTDIR or tbl.get_scriptdir()
minetest = tbl
dofile(SCRIPTDIR .. DIR_DELIM .. "serialize.lua")

tbl.async_jobs = {}

local function handle_job(jobid, serialized_retval)
	local retval = tbl.deserialize(serialized_retval)
	assert(type(tbl.async_jobs[jobid]) == "function")
	tbl.async_jobs[jobid](retval)
	tbl.async_jobs[jobid] = nil
end

if engine ~= nil then
	tbl.async_event_handler = handle_job
else
	minetest.register_globalstep(function(dtime)
		for i, job in ipairs(tbl.get_finished_jobs()) do
			handle_job(job.jobid, job.retval)
		end
	end)
end

function tbl.handle_async(func, parameter, callback)
	-- Serialize function
	local serialized_func = string.dump(func)

	assert(serialized_func ~= nil)

	-- Serialize parameters
	local serialized_param = tbl.serialize(parameter)

	if serialized_param == nil then
		return false
	end

	local jobid = tbl.do_async_callback(serialized_func, serialized_param)

	tbl.async_jobs[jobid] = callback

	return true
end

