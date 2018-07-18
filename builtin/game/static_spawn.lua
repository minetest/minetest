-- Minetest: builtin/static_spawn.lua

local static_spawnpoint_string = core.settings:get("static_spawnpoint")
if static_spawnpoint_string and
		static_spawnpoint_string ~= "" and
		not core.setting_get_pos("static_spawnpoint") then
	error('The static_spawnpoint setting is invalid: "' ..
			static_spawnpoint_string .. '"')
end

local function test_walkability(playername)
	local player = minetest.get_player_by_name(playername)
	if not player then
		return
	end
	local pos = vector.round(player:get_pos())
	local node = minetest.get_node_or_nil(pos)
	if not node then
		-- wait for mapgen
		minetest.after(0.5, test_walkability, playername)
		return
	end
	local def = minetest.registered_nodes[node.name]
	if def and not def.walkable then
		-- player is in air, water, plant or something similar
		return
	end
	for off = 1, 80 do
		pos.y = pos.y+1
		local node = minetest.get_node_or_nil(pos)
		if not node then
			-- teleport player there and wait for mapgen again
			core.log("info", "Moving " .. playername .. " " .. off ..
				" nodes up due to spawn in solid nodes, waiting for mapgen.")
			player:set_pos(pos)
			minetest.after(0.5, test_walkability, playername)
			return
		end
		local def = minetest.registered_nodes[node.name]
		if def and not def.walkable then
			-- found air, water, plant or something similar
			core.log("info", "Moving " .. playername .. " " .. off ..
				" nodes up due to spawn in solid nodes.")
			player:set_pos(pos)
			return
		end
	end
	-- only solid nodes found, test the next 80 nodes again in the next step
	pos.y = pos.y+1
	core.log("info", "Moving " .. playername .. " 81 " ..
		" nodes up due to spawn in solid nodes, 80 nodes exceeded.")
	player:set_pos(pos)
	minetest.after(0, test_walkability, playername)
end

local function put_player_in_spawn(player_obj)
	local playername = player_obj:get_player_name()
	minetest.after(0.5, test_walkability, playername)
	local static_spawnpoint = core.setting_get_pos("static_spawnpoint")
	if not static_spawnpoint then
		return false
	end
	core.log("action", "Moving " .. playername ..
		" to static spawnpoint at " .. core.pos_to_string(static_spawnpoint))
	player_obj:set_pos(static_spawnpoint)
	return true
end

core.register_on_newplayer(put_player_in_spawn)
core.register_on_respawnplayer(put_player_in_spawn)
