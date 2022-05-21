-- Minetest: builtin/misc_s.lua
-- The distinction of what goes here is a bit tricky, basically it's everything
-- that does not (directly or indirectly) need access to ServerEnvironment,
-- Server or writable access to IGameDef on the engine side.
-- (The '_s' stands for standalone.)

--
-- Misc. API functions
--

function core.hash_node_position(pos)
	return (pos.z + 32768) * 65536 * 65536
		 + (pos.y + 32768) * 65536
		 +  pos.x + 32768
end


function core.get_position_from_hash(hash)
	local x = (hash % 65536) - 32768
	hash  = math.floor(hash / 65536)
	local y = (hash % 65536) - 32768
	hash  = math.floor(hash / 65536)
	local z = (hash % 65536) - 32768
	return vector.new(x, y, z)
end


function core.get_item_group(name, group)
	if not core.registered_items[name] or not
			core.registered_items[name].groups[group] then
		return 0
	end
	return core.registered_items[name].groups[group]
end


function core.get_node_group(name, group)
	core.log("deprecated", "Deprecated usage of get_node_group, use get_item_group instead")
	return core.get_item_group(name, group)
end


function core.setting_get_pos(name)
	local value = core.settings:get(name)
	if not value then
		return nil
	end
	return core.string_to_pos(value)
end


-- See l_env.cpp for the other functions
function core.get_artificial_light(param1)
	return math.floor(param1 / 16)
end

-- PNG encoder safety wrapper

local o_encode_png = core.encode_png
function core.encode_png(width, height, data, compression)
	if type(width) ~= "number" then
		error("Incorrect type for 'width', expected number, got " .. type(width))
	end
	if type(height) ~= "number" then
		error("Incorrect type for 'height', expected number, got " .. type(height))
	end

	local expected_byte_count = width * height * 4

	if type(data) ~= "table" and type(data) ~= "string" then
		error("Incorrect type for 'data', expected table or string, got " .. type(data))
	end

	local data_length = type(data) == "table" and #data * 4 or string.len(data)

	if data_length ~= expected_byte_count then
		error(string.format(
			"Incorrect length of 'data', width and height imply %d bytes but %d were provided",
			expected_byte_count,
			data_length
		))
	end

	if type(data) == "table" then
		local dataBuf = {}
		for i = 1, #data do
			dataBuf[i] = core.colorspec_to_bytes(data[i])
		end
		data = table.concat(dataBuf)
	end

	return o_encode_png(width, height, data, compression or 6)
end
