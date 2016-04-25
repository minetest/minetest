
-- mob spawner

local spawner_default = "mobs:pumba 10 15 0 0"

minetest.register_node("mobs:spawner", {
	tiles = {"mob_spawner.png"},
	drawtype = "glasslike",
	paramtype = "light",
	walkable = true,
	description = "Mob Spawner",
	groups = {cracky = 1},

	on_construct = function(pos)

		local meta = minetest.get_meta(pos)

		-- text entry formspec
		meta:set_string("formspec", "field[text;Mob MinLight MaxLight Amount PlayerDist;${command}]")
		meta:set_string("infotext", "Spawner Not Active (enter settings)")
		meta:set_string("command", spawner_default)
	end,

	on_right_click = function(pos, placer)

		if minetest.is_protected(pos, placer:get_player_name()) then
			return
		end
	end,

	on_receive_fields = function(pos, formname, fields, sender)

		if not fields.text or fields.text == "" then
			return
		end

		local meta = minetest.get_meta(pos)
		local comm = fields.text:split(" ")
		local name = sender:get_player_name()

		if minetest.is_protected(pos, name) then
			minetest.record_protection_violation(pos, name)
			return
		end

		local mob = comm[1] -- mob to spawn
		local mlig = tonumber(comm[2]) -- min light
		local xlig = tonumber(comm[3]) -- max light
		local num = tonumber(comm[4]) -- total mobs in area
		local pla = tonumber(comm[5]) -- player distance (0 to disable)
		local yof = tonumber(comm[6]) -- Y offset to spawn mob

		if mob and mob ~= "" and mobs.spawning_mobs[mob] == true
		and num and num >= 0 and num <= 10
		and mlig and mlig >= 0 and mlig <= 15
		and xlig and xlig >= 0 and xlig <= 15
		and pla and pla >=0 and pla <= 20
		and yof > -10 and yof < 10 then

			meta:set_string("command", fields.text)
			meta:set_string("infotext", "Spawner Active (" .. mob .. ")")

		else
			minetest.chat_send_player(name, "Mob Spawner settings failed!")
			minetest.chat_send_player(name, "> name min_light[0-14] max_light[0-14] max_mobs_in_area[0 to disable] distance[1-20] y_offset[-10 to 10]")
		end
	end,
})

-- spawner abm
minetest.register_abm({
	nodenames = {"mobs:spawner"},
	interval = 10,
	chance = 4,
	catch_up = false,

	action = function(pos, node, active_object_count, active_object_count_wider)

		-- check objects inside 9x9 area around spawner
		local objs = minetest.get_objects_inside_radius(pos, 9)

		-- get meta and command
		local meta = minetest.get_meta(pos)
		local comm = meta:get_string("command"):split(" ")

		-- get settings from command
		local mob = comm[1]
		local mlig = tonumber(comm[2])
		local xlig = tonumber(comm[3])
		local num = tonumber(comm[4])
		local pla = tonumber(comm[5]) or 0
		local yof = tonumber(comm[6]) or 0

		-- if amount is 0 then do nothing
		if num == 0 then
			return
		end

		local count = 0
		local ent = nil

		-- count mob objects of same type in area
		for k, obj in pairs(objs) do

			ent = obj:get_luaentity()

			if ent and ent.name == mob then
				count = count + 1
			end
		end

		-- is there too many of same type?
		if count >= num then
			return
		end

		-- spawn mob if player detected and in range
		if pla > 0 then

			local in_range = 0
			local objs = minetest.get_objects_inside_radius(pos, pla)

			for _,oir in pairs(objs) do

				if oir:is_player() then

					in_range = 1

					break
				end
			end

			-- player not found
			if in_range == 0 then
				return
			end
		end

		-- find air blocks within 5 nodes of spawner
		local air = minetest.find_nodes_in_area(
			{x = pos.x - 5, y = pos.y + yof, z = pos.z - 5},
			{x = pos.x + 5, y = pos.y + yof, z = pos.z + 5},
			{"air"})

		-- spawn in random air block
		if air and #air > 0 then

			local pos2 = air[math.random(#air)]
			local lig = minetest.get_node_light(pos2) or 0

			pos2.y = pos2.y + 0.5

			-- only if light levels are within range
			if lig >= mlig and lig <= xlig then
				minetest.add_entity(pos2, mob)
			end
		end

	end
})
