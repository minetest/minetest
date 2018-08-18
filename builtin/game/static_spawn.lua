-- Minetest: builtin/static_spawn.lua

local static_spawnpoint_string = core.settings:get("static_spawnpoint")
if static_spawnpoint_string and
		static_spawnpoint_string ~= "" and
		not core.setting_get_pos("static_spawnpoint") then
	error('The static_spawnpoint setting is invalid: "' ..
			static_spawnpoint_string .. '"')
end

local function check_walkable(player)
	-- Because delayed, check if player still present
	local playername = player:get_player_name()
	if not playername then
		return
	end

	local ppos = vector.round(player:get_pos())
	print("Checking if you are out of your tree")

	for move = 1, 16 do
		local spawn_y = minetest.get_spawn_level(ppos.x, ppos.z)
		if spawn_y then
			-- 'Suitable spawn point'
			ppos.y = spawn_y
			local nodel = minetest.get_node(ppos)
			local defl = minetest.registered_nodes[nodel.name]
			ppos.y = ppos.y + 1
			local nodeu = minetest.get_node(ppos)
			local defu = minetest.registered_nodes[nodeu.name]
			if defl and defu and (not defl.walkable and not defu.walkable) then
				if move == 1 then
					print("You are out of your tree. Z " .. ppos.z)
					return
				end
				ppos.y = ppos.y - 1
				print("Found space. Moving you out of your tree. Z " .. ppos.z)
				player:set_pos(ppos)
				return
			end
		end

		print("You are in your tree. Z " .. ppos.z)
		print("Moving check Northwards")
		ppos.z = ppos.z + 1 -- TODO use a spiral search
	end
end

local function check_for_walkable(player)
	-- Because delayed, check if player still present
	local playername = player:get_player_name()
	if not playername then
		return
	end

	local ppos = player:get_pos()
	-- Check for unloaded area or loaded ignore
	local nodep = minetest.get_node_or_nil(ppos)
	if not nodep or nodep.name == "ignore" then
		-- Wait for mapgen or world loading
		minetest.after(0.5, check_for_walkable, player)
		print("Waiting for world generation or loading")
		return
	end

	print("World loaded or generated. Waiting a little more")
	minetest.after(1, check_walkable, player)
end

local function put_player_in_spawn(player_obj)
	local static_spawnpoint = core.setting_get_pos("static_spawnpoint")
	if not static_spawnpoint then
		-- Move player out of decoration if necessary
		minetest.after(0.5, check_for_walkable, player_obj)
		return false
	end
	core.log("action", "Moving " .. player_obj:get_player_name() ..
		" to static spawnpoint at " .. core.pos_to_string(static_spawnpoint))
	player_obj:set_pos(static_spawnpoint)
	return true
end

core.register_on_newplayer(put_player_in_spawn)
core.register_on_respawnplayer(put_player_in_spawn)
