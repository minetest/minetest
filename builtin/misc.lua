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
		if obj:get_player_name() then
			table.insert(list, obj)
		end
	end
	return list
end

function minetest.hash_node_position(pos)
	return (pos.z+32768)*65536*65536 + (pos.y+32768)*65536 + pos.x+32768
end

function minetest.get_node_group(name, group)
	if not minetest.registered_nodes[name] or not
			minetest.registered_nodes[name].groups[group] then
		return 0
	end
	return minetest.registered_nodes[name].groups[group]
end

