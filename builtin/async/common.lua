-- NOTE: This script is used by the main thread and async worker threads

-- Registered metatables, used in C++ for deserialization
core.registered_async_metatables = {}
function core.register_async_metatable(name, mt)
	if type(name) ~= "string" then
		return error(("attempt to use %s value as metatable name"):format(type(mt)))
	elseif type(mt) ~= "table" then
		return error(("attempt to register a %s value as metatable"):format(type(mt)))
	elseif core.registered_async_metatables[name] ~= nil then
		return error(("attempt to override metatable %s"):format(name))
	elseif core.registered_async_metatables[mt] ~= nil then
		return error("attempt to register metatable twice")
	end
	core.registered_async_metatables[name] = mt
	core.registered_async_metatables[mt] = name
end
core.register_async_metatable("__builtin:vector", vector.metatable)
