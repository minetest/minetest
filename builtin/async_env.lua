engine.log("info", "Initializing Asynchronous environment")
local tbl = engine or minetest

minetest = tbl
dofile(SCRIPTDIR .. DIR_DELIM .. "serialize.lua")
dofile(SCRIPTDIR .. DIR_DELIM .. "misc_helpers.lua")

function tbl.job_processor(serialized_func, serialized_param)
	local func = loadstring(serialized_func)
	local param = tbl.deserialize(serialized_param)
	local retval = nil

	if type(func) == "function" then
		retval = tbl.serialize(func(param))
	else
		tbl.log("error", "ASYNC WORKER: Unable to deserialize function")
	end

	return retval or tbl.serialize(nil)
end

