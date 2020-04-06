local S = minetest.get_translator("testtools")
local F = minetest.formspec_escape

-- TODO: Add a Node Metadata tool

local function check_priv(player, priv)
	-- Tools don't need any privs in the test game
	return true
end

-- Param 2 Tool: Set param2 value of tools
-- Punch: +1
-- Punch+Shift:	+8
-- Place: -1
-- Place+Shift:	-8
minetest.register_tool("testtools:param2tool", {
	description = S("Param2 Tool"),
	inventory_image = "testtools_param2tool.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		local pos = minetest.get_pointed_thing_position(pointed_thing)
		if pointed_thing.type ~= "node" or (not pos) then
			return
		end
		local add = 1
		if user then
			local ctrl = user:get_player_control()
			if ctrl.sneak then
				add = 8
			end
		end
		local node = minetest.get_node(pos)
		node.param2 = node.param2 + add
		minetest.swap_node(pos, node)
	end,
	on_place = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		local pos = minetest.get_pointed_thing_position(pointed_thing)
		if pointed_thing.type ~= "node" or (not pos) then
			return
		end
		local add = -1
		if user then
			local ctrl = user:get_player_control()
			if ctrl.sneak then
				add = -8
			end
		end
		local node = minetest.get_node(pos)
		node.param2 = node.param2 + add
		minetest.swap_node(pos, node)
	end,
})

minetest.register_tool("testtools:node_setter", {
	description = S("Node Setter"),
	inventory_image = "testtools_node_setter.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		local pos = minetest.get_pointed_thing_position(pointed_thing)
		if pointed_thing.type == "nothing" then
			local meta = itemstack:get_meta()
			meta:set_string("node", "air")
			meta:set_int("node_param2", 0)
			if user and user:is_player() then
				minetest.chat_send_player(user:get_player_name(), S("Now placing: @1 (param2=@2)", "air", 0))
			end
			return itemstack
		elseif pointed_thing.type ~= "node" or (not pos) then
			return
		end
		local node = minetest.get_node(pos)
		local meta = itemstack:get_meta()
		meta:set_string("node", node.name)
		meta:set_int("node_param2", node.param2)
		if user and user:is_player() then
			minetest.chat_send_player(user:get_player_name(), S("Now placing: @1 (param2=@2)", node.name, node.param2))
		end
		return itemstack
	end,
	on_secondary_use = function(itemstack, user, pointed_thing)
		local meta = itemstack:get_meta()
		local nodename = meta:get_string("node") or ""
		local param2 = meta:get_int("node_param2") or 0

		minetest.show_formspec(user:get_player_name(), "testtools:node_setter",
			"size[4,4]"..
			"field[0.5,1;3,1;nodename;"..F(S("Node name (itemstring):"))..";"..F(nodename).."]"..
			"field[0.5,2;3,1;param2;"..F(S("param2:"))..";"..F(tostring(param2)).."]"..
			"button_exit[0.5,3;3,1;submit;"..F(S("Submit")).."]"
		)
	end,
	on_place = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		local pos = minetest.get_pointed_thing_position(pointed_thing)
		local meta = itemstack:get_meta()
		local nodename = meta:get_string("node")
		if nodename == "" and user and user:is_player() then
			minetest.chat_send_player(user:get_player_name(), S("Punch a node first!"))
			return
		end
		local param2 = meta:get_int("node_param2")
		if not param2 then
			param2 = 0
		end
		local node = { name = nodename, param2 = param2 }
		if not minetest.registered_nodes[nodename] then
			minetest.chat_send_player(user:get_player_name(), S("Cannot set unknown node: @1", nodename))
			return
		end
		minetest.set_node(pos, node)
	end,
})

minetest.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "testtools:node_setter" then
		local playername = player:get_player_name()
		local witem = player:get_wielded_item()
		if witem:get_name() == "testtools:node_setter" then
			if fields.nodename and fields.param2 then
				local param2 = tonumber(fields.param2)
				if not param2 then
					return
				end
				local meta = witem:get_meta()
				meta:set_string("node", fields.nodename)
				meta:set_int("node_param2", param2)
				player:set_wielded_item(witem)
			end
		end
	end
end)

minetest.register_tool("testtools:remover", {
	description = S("Remover"),
	inventory_image = "testtools_remover.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		local pos = minetest.get_pointed_thing_position(pointed_thing)
		if pointed_thing.type == "node" and pos ~= nil then
			minetest.remove_node(pos)
		elseif pointed_thing.type == "object" then
			local obj = pointed_thing.ref
			if not obj:is_player() then
				obj:remove()
			end
		end
	end,
})

minetest.register_tool("testtools:falling_node_tool", {
	description = S("Falling Node Tool"),
	inventory_image = "testtools_falling_node_tool.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_place = function(itemstack, user, pointed_thing)
		-- Teleport node 1-2 units upwards (if possible) and make it fall
		if not check_priv(user) then
			return
		end
		local pos = minetest.get_pointed_thing_position(pointed_thing)
		if pointed_thing.type ~= "node" or (not pos) then
			return
		end
		local ok = false
		local highest
		for i=1,2 do
			local above = {x=pos.x,y=pos.y+i,z=pos.z}
			local n2 = minetest.get_node(above)
			local def2 = minetest.registered_nodes[n2.name]
			if def2 and (not def2.walkable) then
				highest = above
			else
				break
			end
		end
		if highest then
			local node = minetest.get_node(pos)
			local metatable = minetest.get_meta(pos):to_table()
			minetest.remove_node(pos)
			minetest.set_node(highest, node)
			local meta_highest = minetest.get_meta(highest)
			meta_highest:from_table(metatable)
			ok = minetest.spawn_falling_node(highest)
		else
			ok = minetest.spawn_falling_node(pos)
		end
		if not ok and user and user:is_player() then
			minetest.chat_send_player(user:get_player_name(), S("Falling node could not be spawned!"))
		end
	end,
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		local pos = minetest.get_pointed_thing_position(pointed_thing)
		if pointed_thing.type ~= "node" or (not pos) then
			return
		end
		local ok = minetest.spawn_falling_node(pos)
		if not ok and user and user:is_player() then
			minetest.chat_send_player(user:get_player_name(), S("Falling node could not be spawned!"))
		end
	end,
})

minetest.register_tool("testtools:rotator", {
	description = S("Entity Rotator"),
	inventory_image = "testtools_entity_rotator.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		if pointed_thing.type ~= "object" then
			return
		end
		local obj = pointed_thing.ref
		if obj:is_player() then
			-- No player rotation
			return
		else
			local axis = "y"
			if user and user:is_player() then
				local ctrl = user:get_player_control()
				if ctrl.sneak then
					axis = "x"
				elseif ctrl.aux1 then
					axis = "z"
				end
			end
			local rot = obj:get_rotation()
			rot[axis] = rot[axis] + math.pi/8
			if rot[axis] > math.pi*2 then
				rot[axis] = rot[axis] - math.pi*2
			end
			obj:set_rotation(rot)
		end
	end,
})

local mover_config = function(itemstack, user, pointed_thing)
	if not check_priv(user) then
		return
	end
	if not (user and user:is_player()) then
		return
	end
	local name = user:get_player_name()
	local ctrl = user:get_player_control()
	local meta = itemstack:get_meta()
	local dist = 1.0
	if meta:contains("distance") then
		dist = meta:get_int("distance")
	end
	if ctrl.sneak then
		dist = dist - 1
	else
		dist = dist + 1
	end
	meta:set_int("distance", dist)
	minetest.chat_send_player(user:get_player_name(), S("distance=@1/10", dist*2))
	return itemstack
end

minetest.register_tool("testtools:object_mover", {
	description = S("Object Mover"),
	inventory_image = "testtools_object_mover.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_place = mover_config,
	on_secondary_use = mover_config,
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		if pointed_thing.type ~= "object" then
			return
		end
		local obj = pointed_thing.ref
		if not (user and user:is_player()) then
			return
		end
		local yaw = user:get_look_horizontal()
		local dir = minetest.yaw_to_dir(yaw)
		local pos = obj:get_pos()
		local pitch = user:get_look_vertical()
		if pitch > 0.25 * math.pi then
			dir.y = -1
			dir.x = 0
			dir.z = 0
		elseif pitch < -0.25 * math.pi then
			dir.y = 1
			dir.x = 0
			dir.z = 0
		end
		local ctrl = user:get_player_control()
		if ctrl.sneak then
			dir = vector.multiply(dir, -1)
		end
		local meta = itemstack:get_meta()
		if meta:contains("distance") then
			local dist = meta:get_int("distance")
			dir = vector.multiply(dir, dist*0.2)
		end
		pos = vector.add(pos, dir)
		obj:set_pos(pos)
	end,
})



minetest.register_tool("testtools:entity_scaler", {
	description = S("Entity Visual Scaler"),
	inventory_image = "testtools_entity_scaler.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		if pointed_thing.type ~= "object" then
			return
		end
		local obj = pointed_thing.ref
		if obj:is_player() then
			-- No player scaling
			return
		else
			local diff = 0.1
			if user and user:is_player() then
				local ctrl = user:get_player_control()
				if ctrl.sneak then
					diff = -0.1
				end
			end
			local prop = obj:get_properties()
			if not prop.visual_size then
				prop.visual_size = { x=1, y=1, z=1 }
			else
				prop.visual_size = { x=prop.visual_size.x+diff, y=prop.visual_size.y+diff, z=prop.visual_size.z+diff }
				if prop.visual_size.x <= 0.1 then
					prop.visual_size.x = 0.1
				end
				if prop.visual_size.y <= 0.1 then
					prop.visual_size.y = 0.1
				end
				if prop.visual_size.z <= 0.1 then
					prop.visual_size.z = 0.1
				end
			end
			obj:set_properties(prop)
		end
	end,
})

local selections = {}
local entity_list
local function get_entity_list()
	if entity_list then
		return entity_list
	end
	local ents = minetest.registered_entities
	local list = {}
	for k,_ in pairs(ents) do
		table.insert(list, k)
	end
	table.sort(list)
	entity_list = list
	return entity_list
end
minetest.register_tool("testtools:entity_spawner", {
	description = S("Entity Spawner"),
	inventory_image = "testtools_entity_spawner.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_place = function(itemstack, user, pointed_thing)
		local name = user:get_player_name()
		if selections[name] and pointed_thing.type == "node" then
			local pos = pointed_thing.above
			minetest.add_entity(pos, get_entity_list()[selections[name]])
		end
	end,
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		if pointed_thing.type == "object" then
			return
		end
		if user and user:is_player() then
			local list = table.concat(get_entity_list(), ",")
			local name = user:get_player_name()
			local sel = selections[name] or ""
			minetest.show_formspec(name, "testtools:entity_list",
				"size[9,9]"..
				"textlist[0,0;9,8;entity_list;"..list..";"..sel..";false]"..
				"button[0,8;4,1;spawn;Spawn entity]"
			)
		end
	end,
})

local function prop_to_string(property, use_quotes)
	if type(property) == "string" then
		if use_quotes then
			return "\"" .. property .. "\""
		else
			return property
		end
	elseif type(property) == "table" then
		return tostring(dump(property)):gsub("\n", "")
	else
		return tostring(property)
	end
end

local property_formspec_data = {}
local property_formspec_index = {}
local selected_objects = {}
local function get_object_properties_form(obj, playername)
	if not playername then return "" end
	local props = obj:get_properties()
	local str = ""
	property_formspec_data[playername] = {}
	local proplist = {}
	for k,_ in pairs(props) do
		table.insert(proplist, k)
	end
	table.sort(proplist)
	for p=1, #proplist do
		local k = proplist[p]
		local v = props[k]
		local newline = ""
		newline = k .. " = "
		newline = newline .. prop_to_string(v, true)
		str = str .. F(newline)
		if p < #proplist then
			str = str .. ","
		end
		table.insert(property_formspec_data[playername], k)
	end
	return str
end

local editor_formspec = function(playername, obj, value, sel)
	if not value then
		value = ""
	end
	if not sel then
		sel = "0"
	end
	local list = get_object_properties_form(obj, playername)
	local title
	if obj:is_player() then
		title = S("Object properties of player “@1”", obj:get_player_name())
	else
		local ent = obj:get_luaentity()
		title = S("Object properties of @1", ent.name)
	end
	minetest.show_formspec(playername, "testtools:object_editor",
		"size[9,9]"..
		"label[0,0;"..F(title).."]"..
		"textlist[0,0.5;9,7.5;object_props;"..list..";"..sel..";false]"..
		"field[0.2,8.75;6,1;value;"..F(S("Value"))..";"..F(value).."]"..
		"button[6,8.5;1,1;submit_boolean;"..F(S("Bool")).."]"..
		"button[7,8.5;1,1;submit_string;"..F(S("Str")).."]"..
		"button[8,8.5;1,1;submit_number;"..F(S("Num")).."]"
	)
end

-- TODO: Allow editing tables
minetest.register_tool("testtools:object_editor", {
	description = S("Object Property Editor"),
	inventory_image = "testtools_object_editor.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		if user and user:is_player() then
			local name = user:get_player_name()
			if pointed_thing.type == "object" then
				selected_objects[name] = pointed_thing.ref
				editor_formspec(name, pointed_thing.ref)
			-- Use on yourself if pointing nothing
			elseif pointed_thing.type == "nothing" then
				selected_objects[name] = user
				editor_formspec(name, user)
			end
		end
	end,
})

local ent_parent = {}
local ent_child = {}
local DEFAULT_ATTACH_OFFSET_Y = 11

local attacher_config = function(itemstack, user, pointed_thing)
	if not check_priv(user) then
		return
	end
	if not (user and user:is_player()) then
		return
	end
	if pointed_thing.type == "object" then
		return
	end
	local name = user:get_player_name()
	local ctrl = user:get_player_control()
	local meta = itemstack:get_meta()
	if ctrl.aux1 then
		local rot_x = meta:get_float("rot_x")
		if ctrl.sneak then
			rot_x = rot_x - math.pi/8
		else
			rot_x = rot_x + math.pi/8
		end
		if rot_x > 6.2 then
			rot_x = 0
		elseif rot_x < 0 then
			rot_x = math.pi * (15/8)
		end
		minetest.chat_send_player(name, S("rotation=@1", minetest.pos_to_string({x=rot_x,y=0,z=0})))
		meta:set_float("rot_x", rot_x)
	else
		local pos_y
		if meta:contains("pos_y") then
			pos_y = meta:get_int("pos_y")
		else
			pos_y = DEFAULT_ATTACH_OFFSET_Y
		end
		if ctrl.sneak then
			pos_y = pos_y - 1
		else
			pos_y = pos_y + 1
		end
		minetest.chat_send_player(name, S("position=@1", minetest.pos_to_string({x=0,y=pos_y,z=0})))
		meta:set_int("pos_y", pos_y)
	end
	return itemstack
end

minetest.register_tool("testtools:object_attacher", {
	description = S("Object Attacher"),
	inventory_image = "testtools_object_attacher.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_place = attacher_config,
	on_secondary_use = attacher_config,
	on_use = function(itemstack, user, pointed_thing)
		if not check_priv(user) then
			return
		end
		if user and user:is_player() then
			local name = user:get_player_name()
			local selected_object
			if pointed_thing.type == "object" then
				selected_object = pointed_thing.ref
			elseif pointed_thing.type == "nothing" then
				selected_object = user
			else
				return
			end
			local ctrl = user:get_player_control()
			if ctrl.sneak then
				if selected_object:get_attach() then
					selected_object:set_detach()
					minetest.chat_send_player(name, S("Object detached!"))
				else
					minetest.chat_send_player(name, S("Object is not attached!"))
				end
				return
			end
			local parent = ent_parent[name]
			local child = ent_child[name]
			local ename = S("<unknown>")
			if not parent then
				parent = selected_object
				ent_parent[name] = parent
			elseif not child then
				child = selected_object
				ent_child[name] = child
			end
			local entity = selected_object:get_luaentity()
			if entity then
				ename = entity.name
			elseif selected_object:is_player() then
				ename = selected_object:get_player_name()
			end
			if selected_object == parent then
				minetest.chat_send_player(name, S("Parent object selected: @1", ename))
			elseif selected_object == child then
				minetest.chat_send_player(name, S("Child object selected: @1", ename))
			end
			if parent and child then
				if parent == child then
					minetest.chat_send_player(name, S("Can't attach an object to itself!"))
					ent_parent[name] = nil
					ent_child[name] = nil
					return
				end
				local meta = itemstack:get_meta()
				local y
				if meta:contains("pos_y") then
					y = meta:get_int("pos_y")
				else
					y = DEFAULT_ATTACH_OFFSET_Y
				end
				local rx = meta:get_float("rot_x") or 0
				local offset = {x=0,y=y,z=0}
				local angle = {x=rx,y=0,z=0}
				child:set_attach(parent, "", offset, angle)
				local check_parent = child:get_attach()
				if check_parent then
					minetest.chat_send_player(name, S("Object attached! position=@1, rotation=@2",
						minetest.pos_to_string(offset), minetest.pos_to_string(angle)))
				else
					minetest.chat_send_player(name, S("Attachment failed!"))
				end
				ent_parent[name] = nil
				ent_child[name] = nil
			end
		end
	end,
})

minetest.register_on_player_receive_fields(function(player, formname, fields)
	if not (player and player:is_player()) then
		return
	end
	if formname == "testtools:entity_list" then
		local name = player:get_player_name()
		if fields.entity_list then
			local expl = minetest.explode_textlist_event(fields.entity_list)
			if expl.type == "DCL" then
				local pos = vector.add(player:get_pos(), {x=0,y=1,z=0})
				selections[name] = expl.index
				if check_priv(player, "give") then
					minetest.add_entity(pos, get_entity_list()[expl.index])
				end
				return
			elseif expl.type == "CHG" then
				selections[name] = expl.index
				return
			end
		elseif fields.spawn and selections[name] then
			local pos = vector.add(player:get_pos(), {x=0,y=1,z=0})
			if check_priv(player, "give") then
				minetest.add_entity(pos, get_entity_list()[selections[name]])
			end
			return
		end
	elseif formname == "testtools:object_editor" then
		local name = player:get_player_name()
		if fields.object_props then
			local expl = minetest.explode_textlist_event(fields.object_props)
			if expl.type == "DCL" or expl.type == "CHG" then
				property_formspec_index[name] = expl.index

				local props = selected_objects[name]:get_properties()
				local keys = property_formspec_data[name]
				if (not property_formspec_index[name]) or (not props) then
					return
				end
				local key = keys[property_formspec_index[name]]
				editor_formspec(name, selected_objects[name], prop_to_string(props[key]), expl.index)
				return
			end
		end
		if fields.submit_boolean or fields.submit_string or fields.submit_number then
			local props = selected_objects[name]:get_properties()
			local keys = property_formspec_data[name]
			if (not property_formspec_index[name]) or (not props) then
				return
			end
			local key = keys[property_formspec_index[name]]
			if not key then
				return
			end
			if fields.submit_boolean then
				if fields.value == "true" then
					props[key] = true
				elseif fields.value == "false" then
					props[key] = false
				else
					return
				end
			elseif fields.submit_number then
				props[key] = tonumber(fields.value)
				if type(props[key]) ~= "number" then
					return
				end
			else
				props[key] = fields.value
			end
			selected_objects[name]:set_properties(props)
			editor_formspec(name, selected_objects[name], prop_to_string(props[key]))
			return
		end
	end
end)
