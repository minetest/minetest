
minetest.log("info", "Initializing Asynchronous environment")

function minetest.job_processor(serialized_func, serialized_param)
	local func = loadstring(serialized_func)
	local param = minetest.deserialize(serialized_param)
	local retval = nil

	if type(func) == "function" then
		retval = minetest.serialize(func(param))
	else
		minetest.log("error", "ASYNC WORKER: Unable to deserialize function")
	end

	return retval or minetest.serialize(nil)
end

