minetest.register_chatcommand("test_inv", {
	params = "",
	description = "Test: Modify player's inventory formspec",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		player:set_inventory_formspec(
			"size[13,7.5]"..
			"image[6,0.6;1,2;player.png]"..
			"list[current_player;main;5,3.5;8,4;]"..
			"list[current_player;craft;8,0;3,3;]"..
			"list[current_player;craftpreview;12,1;1,1;]"..
			"list[detached:test_inventory;main;0,0;4,6;0]"..
			"button[0.5,7;2,1;button1;Button 1]"..
			"button_exit[2.5,7;2,1;button2;Exit Button]")
		return true, "Done."
	end,
})

minetest.register_chatcommand("test_bulk_set_node", {
	params = "",
	description = "Test: Bulk-set 9×9×9 stone nodes",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local pos_list = {}
		local ppos = player:get_pos()
		local i = 1
		for x=2,10 do
			for y=2,10 do
				for z=2,10 do
					pos_list[i] = {x=ppos.x + x,y = ppos.y + y,z = ppos.z + z}
					i = i + 1
				end
			end
		end
		minetest.bulk_set_node(pos_list, {name = "mapgen_stone"})
		return true, "Done."
	end,
})

minetest.register_chatcommand("bench_bulk_set_node", {
	params = "",
	description = "Benchmark: Bulk-set 99×99×99 stone nodes",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local pos_list = {}
		local ppos = player:get_pos()
		local i = 1
		for x=2,100 do
			for y=2,100 do
				for z=2,100 do
					pos_list[i] = {x=ppos.x + x,y = ppos.y + y,z = ppos.z + z}
					i = i + 1
				end
			end
		end

		minetest.chat_send_player(name, "Benchmarking minetest.bulk_set_node. Warming up ...");

		-- warm up with stone to prevent having different callbacks
		-- due to different node topology
		minetest.bulk_set_node(pos_list, {name = "mapgen_stone"})

		minetest.chat_send_player(name, "Warming up finished, now benchmarking ...");

		local start_time = minetest.get_us_time()
		for i=1,#pos_list do
			minetest.set_node(pos_list[i], {name = "mapgen_stone"})
		end
		local middle_time = minetest.get_us_time()
		minetest.bulk_set_node(pos_list, {name = "mapgen_stone"})
		local end_time = minetest.get_us_time()
		local msg = string.format("Benchmark results: minetest.set_node loop: %.2f ms; minetest.bulk_set_node: %.2f ms",
			((middle_time - start_time)) / 1000,
			((end_time - middle_time)) / 1000
		)
		return true, msg
	end,
})

local function advance_pos(pos, start_pos, advance_z)
	if advance_z then
		pos.z = pos.z + 2
		pos.x = start_pos.x
	else
		pos.x = pos.x + 2
	end
	if pos.x > 30900 or pos.x - start_pos.x > 46 then
		pos.x = start_pos.x
		pos.z = pos.z + 2
	end
	if pos.z > 30900 then
		-- We ran out of space! Aborting
		aborted = true
		return false
	end
	return pos
end

local function place_nodes(param)
	local nodes = param.nodes
	local name = param.name
	local pos = param.pos
	local start_pos = param.start_pos
	table.sort(nodes)
	minetest.chat_send_player(name, "Placing nodes …")
	local nodes_placed = 0
	local aborted = false
	for n=1, #nodes do
		local itemstring = nodes[n]
		local def = minetest.registered_nodes[itemstring]
		local p2_max = 0
		if param.param ~= "no_param2" then
			-- Also test the param2 values of the nodes
			-- ... but we only use permissible param2 values
			if def.paramtype2 == "wallmounted" then
				p2_max = 5
			elseif def.paramtype2 == "facedir" then
				p2_max = 23
			elseif def.paramtype2 == "glasslikeliquidlevel" then
				p2_max = 63
			elseif def.paramtype2 == "meshoptions" and def.drawtype == "plantlike" then
				p2_max = 63
			elseif def.paramtype2 == "leveled" then
				p2_max = 127
			elseif def.paramtype2 == "degrotate" and (def.drawtype == "plantlike" or def.drawtype == "mesh") then
				p2_max = 239
			elseif def.paramtype2 == "colorfacedir" or
				def.paramtype2 == "colorwallmounted" or
				def.paramtype2 == "colordegrotate" or
				def.paramtype2 == "color" then
				p2_max = 255
			end
		end
		for p2 = 0, p2_max do
			-- Skip undefined param2 values
			if not ((def.paramtype2 == "meshoptions" and p2 % 8 > 4) or
					(def.paramtype2 == "colorwallmounted" and p2 % 8 > 5) or
					((def.paramtype2 == "colorfacedir" or def.paramtype2 == "colordegrotate")
					and p2 % 32 > 23)) then

				minetest.set_node(pos, { name = itemstring, param2 = p2 })
				nodes_placed = nodes_placed + 1
				pos = advance_pos(pos, start_pos)
				if not pos then
					aborted = true
					break
				end
			end
		end
		if aborted then
			break
		end
	end
	if aborted then
		minetest.chat_send_player(name, "Not all nodes could be placed, please move further away from the world boundary. Nodes placed: "..nodes_placed)
	end
	minetest.chat_send_player(name, "Nodes placed: "..nodes_placed..".")
end

local function after_emerge(blockpos, action, calls_remaining, param)
	if calls_remaining == 0 then
		place_nodes(param)
	end
end

minetest.register_chatcommand("test_place_nodes", {
	params = "[ no_param2 ]",
	description = "Test: Place all non-experimental nodes and optionally their permissible param2 variants",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local pos = vector.floor(player:get_pos())
		pos.x = math.ceil(pos.x + 3)
		pos.z = math.ceil(pos.z + 3)
		pos.y = math.ceil(pos.y + 1)
		local start_pos = table.copy(pos)
		if pos.x > 30800 then
			return false, "Too close to world boundary (+X). Please move to X < 30800."
		end
		if pos.z > 30800 then
			return false, "Too close to world boundary (+Z). Please move to Z < 30800."
		end

		local aborted = false
		local nodes = {}
		local emerge_estimate = 0
		for itemstring, def in pairs(minetest.registered_nodes) do
			if itemstring ~= "ignore" and string.sub(itemstring, 1, 13) ~= "experimental:" then
				table.insert(nodes, itemstring)
				if def.paramtype2 == 0 then
					emerge_estimate = emerge_estimate + 1
				else
					emerge_estimate = emerge_estimate + 255
				end
			end
		end
		-- Emerge area to make sure that all nodes are being placed.
		-- Note we will emerge much more than we need to (overestimation),
		-- the estimation code could be improved performance-wise …
		local length = 16 + math.ceil(emerge_estimate / 24) * 2
		minetest.emerge_area(start_pos,
			{ x = start_pos.x + 46, y = start_pos.y, z = start_pos.z + length },
			after_emerge, { nodes = nodes, name = name, pos = pos, start_pos = start_pos, param = param })
		return true, "Emerging area …"
	end,
})

core.register_on_chatcommand(function(name, command, params)
	minetest.log("action", "caught command '"..command.."', issued by '"..name.."'. Parameters: '"..params.."'")
end)
