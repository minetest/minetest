engine.log("info","Initializing Asynchronous environment")

dofile(SCRIPTDIR .. DIR_DELIM .. "misc_helpers.lua")

function engine.job_processor(serialized_function, serialized_data)

	local fct = marshal.decode(serialized_function)
	local params = marshal.decode(serialized_data)
	local retval = marshal.encode(nil)

	if fct ~= nil and type(fct) == "function" then
		local result = fct(params)
		retval = marshal.encode(result)
	else
		engine.log("error","ASYNC WORKER: unable to deserialize function")
	end

	return retval,retval:len()
end
