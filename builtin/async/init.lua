
if core.get_cache_path == nil then -- TODO

core.log("info", "Initializing Asynchronous environment for game")

function core.job_processor(func, params)
	print('will call ' .. tostring(func) .. ' with ' .. #params .. ' args')

	local retval = {func(unpack(params))}

	print('returned ' .. #retval .. ' values')

	return retval
end

else

core.log("info", "Initializing Asynchronous environment")

function core.job_processor(func, serialized_param)
	local param = core.deserialize(serialized_param)

	local retval = core.serialize(func(param))

	return retval or core.serialize(nil)
end

end
