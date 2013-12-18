local tbl = engine or minetest

tbl.async_jobs = {}

if engine ~= nil then
	function tbl.async_event_handler(jobid, serialized_retval)
		local retval = nil
		if serialized_retval ~= "ERROR" then
			retval= marshal.decode(serialized_retval)
		else
			tbl.log("error","Error fetching async result")
		end

		assert(type(tbl.async_jobs[jobid]) == "function")
		tbl.async_jobs[jobid](retval)
		tbl.async_jobs[jobid] = nil
	end
else

	minetest.register_globalstep(
		function(dtime)
			local list = tbl.get_finished_jobs()

			for i=1,#list,1 do
				local retval = marshal.decode(list[i].retval)

				assert(type(tbl.async_jobs[jobid]) == "function")
				tbl.async_jobs[list[i].jobid](retval)
				tbl.async_jobs[list[i].jobid] = nil
			end
		end)
end

function tbl.handle_async(fct, parameters, callback)

	--serialize fct
	local serialized_fct = marshal.encode(fct)

	assert(marshal.decode(serialized_fct) ~= nil)

	--serialize parameters
	local serialized_params = marshal.encode(parameters)

	if serialized_fct == nil or
		serialized_params == nil or
		serialized_fct:len() == 0 or
		serialized_params:len() == 0 then
		return false
	end

	local jobid = tbl.do_async_callback(	serialized_fct,
											serialized_fct:len(),
											serialized_params,
											serialized_params:len())

	tbl.async_jobs[jobid] = callback

	return true
end
