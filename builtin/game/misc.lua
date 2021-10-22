-- Minetest: builtin/misc.lua

local S = core.get_translator("__builtin")

--
-- Misc. API functions
--

function core.check_player_privs(name, ...)
	if core.is_player(name) then
		name = name:get_player_name()
	elseif type(name) ~= "string" then
		error("core.check_player_privs expects a player or playername as " ..
			"argument.", 2)
	end

	local requested_privs = {...}
	local player_privs = core.get_player_privs(name)
	local missing_privileges = {}

	if type(requested_privs[1]) == "table" then
		-- We were provided with a table like { privA = true, privB = true }.
		for priv, value in pairs(requested_privs[1]) do
			if value and not player_privs[priv] then
				missing_privileges[#missing_privileges + 1] = priv
			end
		end
	else
		-- Only a list, we can process it directly.
		for key, priv in pairs(requested_privs) do
			if not player_privs[priv] then
				missing_privileges[#missing_privileges + 1] = priv
			end
		end
	end

	if #missing_privileges > 0 then
		return false, missing_privileges
	end

	return true, ""
end


function core.send_join_message(player_name)
	if not core.is_singleplayer() then
		core.chat_send_all("*** " .. S("@1 joined the game.", player_name))
	end
end


function core.send_leave_message(player_name, timed_out)
	local announcement = "*** " .. S("@1 left the game.", player_name)
	if timed_out then
		announcement = "*** " .. S("@1 left the game (timed out).", player_name)
	end
	core.chat_send_all(announcement)
end


core.register_on_joinplayer(function(player)
	local player_name = player:get_player_name()
	if not core.is_singleplayer() then
		local status = core.get_server_status(player_name, true)
		if status and status ~= "" then
			core.chat_send_player(player_name, status)
		end
	end
	core.send_join_message(player_name)
end)


core.register_on_leaveplayer(function(player, timed_out)
	local player_name = player:get_player_name()
	core.send_leave_message(player_name, timed_out)
end)


function core.is_player(player)
	-- a table being a player is also supported because it quacks sufficiently
	-- like a player if it has the is_player function
	local t = type(player)
	return (t == "userdata" or t == "table") and
		type(player.is_player) == "function" and player:is_player()
end


function core.player_exists(name)
	return core.get_auth_handler().get_auth(name) ~= nil
end


-- Returns two position vectors representing a box of `radius` in each
-- direction centered around the player corresponding to `player_name`

function core.get_player_radius_area(player_name, radius)
	local player = core.get_player_by_name(player_name)
	if player == nil then
		return nil
	end

	local p1 = player:get_pos()
	local p2 = p1

	if radius then
		p1 = vector.subtract(p1, radius)
		p2 = vector.add(p2, radius)
	end

	return p1, p2
end


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


-- To be overriden by protection mods

function core.is_protected(pos, name)
	return false
end


function core.record_protection_violation(pos, name)
	for _, func in pairs(core.registered_on_protection_violation) do
		func(pos, name)
	end
end

-- To be overridden by Creative mods

local creative_mode_cache = core.settings:get_bool("creative_mode")
function core.is_creative_enabled(name)
	return creative_mode_cache
end

-- Checks if specified volume intersects a protected volume

function core.is_area_protected(minp, maxp, player_name, interval)
	-- 'interval' is the largest allowed interval for the 3D lattice of checks.

	-- Compute the optimal float step 'd' for each axis so that all corners and
	-- borders are checked. 'd' will be smaller or equal to 'interval'.
	-- Subtracting 1e-4 ensures that the max co-ordinate will be reached by the
	-- for loop (which might otherwise not be the case due to rounding errors).

	-- Default to 4
	interval = interval or 4
	local d = {}

	for _, c in pairs({"x", "y", "z"}) do
		if minp[c] > maxp[c] then
			-- Repair positions: 'minp' > 'maxp'
			local tmp = maxp[c]
			maxp[c] = minp[c]
			minp[c] = tmp
		end

		if maxp[c] > minp[c] then
			d[c] = (maxp[c] - minp[c]) /
				math.ceil((maxp[c] - minp[c]) / interval) - 1e-4
		else
			d[c] = 1 -- Any value larger than 0 to avoid division by zero
		end
	end

	for zf = minp.z, maxp.z, d.z do
		local z = math.floor(zf + 0.5)
		for yf = minp.y, maxp.y, d.y do
			local y = math.floor(yf + 0.5)
			for xf = minp.x, maxp.x, d.x do
				local x = math.floor(xf + 0.5)
				local pos = vector.new(x, y, z)
				if core.is_protected(pos, player_name) then
					return pos
				end
			end
		end
	end
	return false
end


local raillike_ids = {}
local raillike_cur_id = 0
function core.raillike_group(name)
	local id = raillike_ids[name]
	if not id then
		raillike_cur_id = raillike_cur_id + 1
		raillike_ids[name] = raillike_cur_id
		id = raillike_cur_id
	end
	return id
end


-- HTTP callback interface

function core.http_add_fetch(httpenv)
	httpenv.fetch = function(req, callback)
		local handle = httpenv.fetch_async(req)

		local function update_http_status()
			local res = httpenv.fetch_async_get(handle)
			if res.completed then
				callback(res)
			else
				core.after(0, update_http_status)
			end
		end
		core.after(0, update_http_status)
	end

	return httpenv
end


function core.close_formspec(player_name, formname)
	return core.show_formspec(player_name, formname, "")
end


function core.cancel_shutdown_requests()
	core.request_shutdown("", false, -1)
end


-- Used for callback handling with dynamic_add_media
core.dynamic_media_callbacks = {}


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
