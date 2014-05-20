
core.log("info", "Initializing Asynchronous environment")

function core.job_processor(serialized_func, serialized_param)
	local func = loadstring(serialized_func)
	local param = core.deserialize(serialized_param)
	local retval = nil

	if type(func) == "function" then
		retval = core.serialize(func(param))
	else
		core.log("error", "ASYNC WORKER: Unable to deserialize function")
	end

	return retval or core.serialize(nil)
end

