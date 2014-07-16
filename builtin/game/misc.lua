-- Minetest: builtin/misc.lua

--
-- Misc. API functions
--

core.timers_to_add = {}
core.timers = {}
core.register_globalstep(function(dtime)
	for _, timer in ipairs(core.timers_to_add) do
		table.insert(core.timers, timer)
	end
	core.timers_to_add = {}
	local index = 1
	while index <= #core.timers do
		local timer = core.timers[index]
		timer.time = timer.time - dtime
		if timer.time <= 0 then
			timer.func(unpack(timer.args or {}))
			table.remove(core.timers,index)
		else
			index = index + 1
		end
	end
end)

function core.after(time, func, ...)
	assert(tonumber(time) and type(func) == "function",
			"Invalid core.after invocation")
	table.insert(core.timers_to_add, {time=time, func=func, args={...}})
end

function core.check_player_privs(name, privs)
	local player_privs = core.get_player_privs(name)
	local missing_privileges = {}
	for priv, val in pairs(privs) do
		if val then
			if not player_privs[priv] then
				table.insert(missing_privileges, priv)
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
			table.insert(temp_table, value)
		end
	end
	return temp_table
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

function core.string_to_pos(value)
	local p = {}
	p.x, p.y, p.z = string.match(value, "^([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
	if p.x and p.y and p.z then
		p.x = tonumber(p.x)
		p.y = tonumber(p.y)
		p.z = tonumber(p.z)
		return p
	end
	local p = {}
	p.x, p.y, p.z = string.match(value, "^%( *([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+) *%)$")
	if p.x and p.y and p.z then
		p.x = tonumber(p.x)
		p.y = tonumber(p.y)
		p.z = tonumber(p.z)
		return p
	end
	return nil
end

assert(core.string_to_pos("10.0, 5, -2").x == 10)
assert(core.string_to_pos("( 10.0, 5, -2)").z == -2)
assert(core.string_to_pos("asd, 5, -2)") == nil)

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

