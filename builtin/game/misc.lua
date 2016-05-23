-- Minetest: builtin/misc.lua

--
-- Misc. API functions
--

local jobs = {}
local time = 0.0
local last = core.get_us_time() / 1000000

core.register_globalstep(function(dtime)
	local new = core.get_us_time() / 1000000
	if new > last then
		time = time + (new - last)
	else
		-- Overflow, we may lose a little bit of time here but
		-- only 1 tick max, potentially running timers slightly
		-- too early.
		time = time + new
	end
	last = new

	if #jobs < 1 then
		return
	end

	-- Iterate backwards so that we miss any new timers added by
	-- a timer callback, and so that we don't skip the next timer
	-- in the list if we remove one.
	for i = #jobs, 1, -1 do
		local job = jobs[i]
		if time >= job.expire then
			core.set_last_run_mod(job.mod_origin)
			job.func(unpack(job.arg))
			table.remove(jobs, i)
		end
	end
end)

function core.after(after, func, ...)
	assert(tonumber(time) and type(func) == "function",
			"Invalid core.after invocation")
	jobs[#jobs + 1] = {
		func = func,
		expire = time + after,
		arg = {...},
		mod_origin = core.get_last_run_mod()
	}
end

function core.check_player_privs(player_or_name, ...)
	local name = player_or_name
	-- Check if we have been provided with a Player object.
	if type(name) ~= "string" then
		name = name:get_player_name()
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

local player_list = {}

core.register_on_joinplayer(function(player)
	player_list[player:get_player_name()] = player
end)

core.register_on_leaveplayer(function(player)
	player_list[player:get_player_name()] = nil
end)

function core.get_connected_players()
	local temp_table = {}
	for index, value in pairs(player_list) do
		if value:is_player_connected() then
			temp_table[#temp_table + 1] = value
		end
	end
	return temp_table
end

-- Returns two position vectors representing a box of `radius` in each
-- direction centered around the player corresponding to `player_name`
function core.get_player_radius_area(player_name, radius)
	local player = core.get_player_by_name(player_name)
	if player == nil then
		return nil
	end

	local p1 = player:getpos()
	local p2 = p1

	if radius then
		p1 = vector.subtract(p1, radius)
		p2 = vector.add(p2, radius)
	end

	return p1, p2
end

function core.hash_node_position(pos)
	return (pos.z+32768)*65536*65536 + (pos.y+32768)*65536 + pos.x+32768
end

function core.get_position_from_hash(hash)
	local pos = {}
	pos.x = (hash%65536) - 32768
	hash = math.floor(hash/65536)
	pos.y = (hash%65536) - 32768
	hash = math.floor(hash/65536)
	pos.z = (hash%65536) - 32768
	return pos
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
	local value = core.setting_get(name)
	if not value then
		return nil
	end
	return core.string_to_pos(value)
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
