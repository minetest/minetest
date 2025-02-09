unittests.register("test_get_version", function()
	local version = core.get_version()
	assert(type(version) == "table")
	assert(type(version.project) == "string")
	assert(type(version.string) == "string")
	assert(type(version.proto_min) == "number")
	assert(type(version.proto_max) == "number")
	assert(version.proto_max >= version.proto_min)
	assert(type(version.is_dev) == "boolean")
	if version.is_dev then
		assert(type(version.hash) == "string")
	else
		assert(version.hash == nil)
	end
end)

unittests.register("test_protocol_versions", function(player)
	local info = core.get_player_information(player:get_player_name())
	-- Newest protocol version should be in mapping of protocol versions
	assert(table.key_value_swap(core.protocol_versions)[info.protocol_version])
end, {player = true})
