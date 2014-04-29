-- Minetest: builtin/misc.lua

--
-- Misc. API functions
--

minetest.timers_to_add = {}
minetest.timers = {}
minetest.register_globalstep(function(dtime)
	for _, timer in ipairs(minetest.timers_to_add) do
		table.insert(minetest.timers, timer)
	end
	minetest.timers_to_add = {}
	for index, timer in ipairs(minetest.timers) do
		timer.time = timer.time - dtime
		if timer.time <= 0 then
			timer.func(unpack(timer.args or {}))
			table.remove(minetest.timers,index)
		end
	end
end)

function minetest.after(time, func, ...)
	assert(tonumber(time) and type(func) == "function",
			"Invalid minetest.after invocation")
	table.insert(minetest.timers_to_add, {time=time, func=func, args={...}})
end

function minetest.check_player_privs(name, privs)
	local player_privs = minetest.get_player_privs(name)
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

minetest.register_on_joinplayer(function(player)
	player_list[player:get_player_name()] = player
end)

minetest.register_on_leaveplayer(function(player)
	player_list[player:get_player_name()] = nil
end)

function minetest.get_connected_players()
	local temp_table = {}
	for index, value in pairs(player_list) do
		if value:is_player_connected() then
			table.insert(temp_table, value)
		end
	end
	return temp_table
end

function minetest.hash_node_position(pos)
	return (pos.z+32768)*65536*65536 + (pos.y+32768)*65536 + pos.x+32768
end

function minetest.get_position_from_hash(hash)
	local pos = {}
	pos.x = (hash%65536) - 32768
	hash = math.floor(hash/65536)
	pos.y = (hash%65536) - 32768
	hash = math.floor(hash/65536)
	pos.z = (hash%65536) - 32768
	return pos
end

function minetest.get_item_group(name, group)
	if not minetest.registered_items[name] or not
			minetest.registered_items[name].groups[group] then
		return 0
	end
	return minetest.registered_items[name].groups[group]
end

function minetest.get_node_group(name, group)
	minetest.log("deprecated", "Deprecated usage of get_node_group, use get_item_group instead")
	return minetest.get_item_group(name, group)
end

function minetest.string_to_pos(value)
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

assert(minetest.string_to_pos("10.0, 5, -2").x == 10)
assert(minetest.string_to_pos("( 10.0, 5, -2)").z == -2)
assert(minetest.string_to_pos("asd, 5, -2)") == nil)

function minetest.setting_get_pos(name)
	local value = minetest.setting_get(name)
	if not value then
		return nil
	end
	return minetest.string_to_pos(value)
end

-- To be overriden by protection mods
function minetest.is_protected(pos, name)
	return false
end

function minetest.record_protection_violation(pos, name)
	for _, func in pairs(minetest.registered_on_protection_violation) do
		func(pos, name)
	end
end

