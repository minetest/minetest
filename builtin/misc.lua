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
			timer.func(timer.param)
			table.remove(minetest.timers,index)
		end
	end
end)

function minetest.after(time, func, param)
	table.insert(minetest.timers_to_add, {time=time, func=func, param=param})
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

function minetest.get_connected_players()
	-- This could be optimized a bit, but leave that for later
	local list = {}
	for _, obj in pairs(minetest.env:get_objects_inside_radius({x=0,y=0,z=0}, 1000000)) do
		if obj:is_player() then
			table.insert(list, obj)
		end
	end
	return list
end

function minetest.hash_node_position(pos)
	return (pos.z+32768)*65536*65536 + (pos.y+32768)*65536 + pos.x+32768
end

function minetest.get_item_group(name, group)
	if not minetest.registered_items[name] or not
			minetest.registered_items[name].groups[group] then
		return 0
	end
	return minetest.registered_items[name].groups[group]
end

function minetest.get_node_group(name, group)
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

