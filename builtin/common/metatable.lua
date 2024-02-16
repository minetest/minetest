-- Registered metatables, used in C++ for deserialization
local known_metatables = {}
function core.register_async_metatable(name, mt)
	if type(name) ~= "string" then
		return error(("attempt to use %s value as metatable name"):format(type(name)))
	elseif type(mt) ~= "table" then
		return error(("attempt to register a %s value as metatable"):format(type(mt)))
	elseif known_metatables[name] ~= nil and known_metatables[name] ~= mt then
		return error(("attempt to override metatable %s"):format(name))
	end
	known_metatables[name] = mt
	known_metatables[mt] = name
end
core.known_metatables = known_metatables
core.register_async_metatable("__builtin:vector", vector.metatable)
