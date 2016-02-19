-- Minetest: builtin/misc.lua

--
-- Misc. API functions
--

local jobs = {}
local time = 0.0
local last = 0.0

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
	table.insert(jobs, {
		func = func,
		expire = time + after,
		arg = {...},
		mod_origin = core.get_last_run_mod()
	})
end

-- name can also be a player
function core.check_player_privs(name, ...)
	if not name then
		error("core.check_player_privs: missing player/name")
	end

	-- Check if we have been provided with a Player object.
	if type(name) ~= "string" then
		name = name:get_player_name()
	end

	local requested_privs = {...}
	local player_privs = core.get_player_privs(name)
	local missing_privileges,n = {},1

	if type(requested_privs[1]) == "table" then
		-- We were provided with a table like { privA = true, privB = true }.
		for priv, value in pairs(requested_privs[1]) do
			if value and not player_privs[priv] then
				missing_privileges[n] = priv
				n = n+1
			end
		end
	else
		-- Only a list, we can process it directly.
		for key, priv in pairs(requested_privs) do
			if not player_privs[priv] then
				missing_privileges[n] = priv
				n = n+1
			end
		end
	end
	if n == 1 then
		return true
	end
	return false, missing_privileges
end

local player_list = {}

core.register_on_joinplayer(function(player)
	player_list[player:get_player_name()] = player
end)

core.register_on_leaveplayer(function(player)
	player_list[player:get_player_name()] = nil
end)

-- works faster than get_connected_players but doesn't test if it's connected
function core.get_player_list()
	return player_list
end

function core.get_connected_players()
	local temp_table,n = {},1
	for index, value in pairs(player_list) do
		if value:is_player_connected() then
			temp_table[n] = value
			n = n+1
		end
	end
	return temp_table
end

-- Returns two position vectors representing a cubic box of `range` in each
-- direction centered around the player corresponding to `player_name`
function core.get_player_radius_area(player_name, range)
	if not player_name then
		error("get_player_radius_area: missing player name")
	end

	local player = core.get_player_by_name(player_name)
	if not player then
		return
	end

	local pos = player:getpos()
	if not range then
		return pos, pos
	end

	return vector.subtract(pos, range), vector.add(pos, range)
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
	return (core.registered_items[name] and
			core.registered_items[name].groups[group]) or
			0
end

function core.get_node_group(name, group)
	core.log("deprecated", "Deprecated usage of get_node_group by mod "..
		core.get_last_run_mod()..
		", use get_item_group instead")
	return core.get_item_group(name, group)
end

function core.setting_get_pos(name)
	local value = core.setting_get(name)
	return value and core.string_to_pos(value)
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
