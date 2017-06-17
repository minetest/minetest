core.registered_anticheat_objects = {}

local current_anticheat_objects = {}

function core.register_anticheat_object(name, def)
	if core.registered_anticheat_objects[name] then
		core.log("warning", "Couldn't register anticheat object "..name..": anticheat object with same name already exists.")
	end

	if def.enable then
		core.log("warning", "Anticheat object "..name.." definition contains an enable method. Use on_enable instead. Overwriting.")
	end
	def.enable = function(self)
		if self.on_enable then
			if self:on_enable() ~= false then
				self.enabled = true
			end
		else
			self.enabled = true
		end
	end

	if def.disable then
		core.log("warning", "Anticheat object "..name.." definition contains a disable method. Use on_disable instead. Overwriting.")
	end
	def.disable = function(self)
		if self.on_disable then
			if self:on_disable() ~= false then
				self.enabled = false
			end
		else
			self.enabled = false
		end
	end

	core.registered_anticheat_objects[name] = def
end

function core.get_anticheat_object(player, type)
	if type(player) == "string" then
		player = minetest.get_player_by_name(player)
	end

	local player_name = player:get_player_name()
	if not current_anticheat_objects[player_name] then
		current_anticheat_objects[player_name] = {}
	end
	if not current_anticheat_objects[player_name][type] then
		if not core.registered_anticheat_objects[type] then
			return nil
		end

		-- Create anticheat object
		local obj = {}
		setmetatable(obj, {__index = core.registered_anticheat_objects[type]})
		obj.player = player
		--obj.enabled = core.settings:get_bool("enable_anticheat") and not core.is_singleplayer()
		-- DEBUG:
		obj.enabled = core.settings:get_bool("enable_anticheat")
		if obj.init then
			obj:init()
		end
		current_anticheat_objects[player_name][type] = obj
	end

	return current_anticheat_objects[player_name][type]
end

function core.run_anticheat_callbacks(player, type, params)
	assert(core.registered_anticheat_objects[type])
	if not params then
		params = {}
	end
	params.type = type
	return core.run_callbacks(core.registered_on_cheats, 0, player, params)
end

core.register_on_leaveplayer(function(player)
	local player_name = player:get_player_name()
	if not current_anticheat_objects[player_name] then
		return
	end
	for type, obj in pairs(current_anticheat_objects[player_name]) do
		if obj.deinit then
			obj:deinit()
		end
	end
	current_anticheat_objects[player_name] = nil
end)

-- Builtin anticheat definitions. These anticheat objects are called from the C++ engine

core.register_anticheat_object("interacted_too_far", {
	check = function(self, node_pos)
		if not self.enabled then
			return false
		end

		local distance = vector.distance(self.player:get_pos(), node_pos)
		local max_distance = self:get_max_distance()

		if distance > max_distance then
			core.log("action", "Player " .. self.player:get_player_name()
						.. " tried to access " .. core.pos_to_string(node_pos)
						.. " from too far: "
						.. "d=" .. distance .. ", max_d=" .. max_distance
						.. ". ignoring.")
			core.run_anticheat_callbacks(self.player, "interacted_too_far", {node_pos = node_pos, distance = distance, max_distance = max_distance})
			return true
		else
			return false
		end
	end,

	set_max_distance = function(self, max_distance)
		self.max_distance = max_distance
	end,

	get_max_distance = function(self)
		if self.max_distance then
			return self.max_distance
		end

		local item_range = minetest.registered_items[self.player:get_wielded_item():get_name()].range or 4.0
		if item_range >= 0 then
			return item_range
		end

		local hand_range = minetest.registered_items[":"].range or 4.0
		if hand_range >= 0 then
			return hand_range
		end

		return 4.0
	end
})

core.register_anticheat_object("finished_unknown_dig", {
	check = function(self, started_pos, completed_pos)
		if not self.enabled then
			return false
		end

		if not vector.equals(started_pos, completed_pos) then
			core.log("info", "Server: NoCheat: " .. self.player:get_player_name()
					.. " started digging "
					.. core.pos_to_string(started_pos) .. " and completed digging "
					.. core.pos_to_string(completed_pos) .. "; not digging.")
			core.run_anticheat_callbacks(self.player, "finished_unknown_dig", {started_pos = started_pos, completed_pos = completed_pos})
			return true
		else
			return false
		end
	end
})

core.register_anticheat_object("dug_unbreakable", {
	check = function(self, node_pos)
		if not self.enabled then
			return false
		end

		local groups = core.registered_items[core.get_node(node_pos).name].groups
		local tool_capabilities = self.player:get_wielded_item():get_tool_capabilities()

		if not core.get_dig_params(groups, tool_capabilities).diggable then
			-- Not diggable? Try hand
			tool_capabilities = core.registered_items[":"].tool_capabilities
			if not core.get_dig_params(groups, tool_capabilities).diggable then
				-- Ignore dig
				core.log("info", "Server: NoCheat: " .. self.player:get_player_name()
						.. " completed digging " .. core.pos_to_string(node_pos)
						.. "; which is not diggable with tool. not digging.")
				core.run_anticheat_callbacks(self.player, "dug_unbreakable", {node_pos = node_pos, groups = groups})
				return true
			end
		end

		return false
	end
})

core.register_anticheat_object("dug_too_fast", {
	check = function(self, node_pos, nocheat_time, grab_lag_pool)
		if not self.enabled then
			return false
		end

		local groups = core.registered_items[core.get_node(node_pos).name].groups
		local tool_capabilities = self.player:get_wielded_item():get_tool_capabilities()
		local dtime = core.get_dig_params(groups, tool_capabilities).time

		if dtime > 2.0 and nocheat_time * 1.2 > dtime then
			-- All is good, but grab time from pool; don't care if
			-- it's actually available
			grab_lag_pool(dtime, self.player:get_player_name(), "dig")
			return false
		elseif grab_lag_pool(dtime, self.player:get_player_name(), "dig") then
			return false
		else
			core.log("info", "Server: NoCheat: " .. self.player:get_player_name()
							.. " completed digging " .. core.pos_to_string(node_pos)
							.. "too fast; not digging.")
			core.run_anticheat_callbacks(self.player, "dug_too_fast", {node_pos = node_pos,
				dtime = dtime, nocheat_time = nocheat_time})
			return true
		end
	end
})

core.register_anticheat_object("moved_too_fast", {
	check = function(self, pos, max_lag_estimate, time_from_last_teleport, last_good_position, grab_lag_pool)
		if not self.enabled or self.player:get_attach() then
			return false
		end

		--[[
			Check player movements

			NOTE: Actually the server should handle player physics like the
			client does and compare player's position to what is calculated
			on our side. This is required when eg. players fly due to an
			explosion. Altough a node-based alternative might be possible
			too, and much more lightweight.
		]]

		local player_max_speed = self:get_max_speed()
		-- Tolerance. The lag pool does this a bit.
		--player_max_speed = player_max_speed * 2.5

		local diff = vector.subtract(pos, last_good_position)
		local d_vert = diff.y
		diff.y = 0
		local d_horiz = vector.length(diff)
		local required_time = d_horiz / player_max_speed

		if d_vert > 0 and d_vert / player_max_speed > required_time then
			required_time = d_vert / player_max_speed -- Moving upwards
		end

		if grab_lag_pool(required_time, self.player:get_player_name(), "move") then
			return false
		else
			local LAG_POOL_MIN = 5.0
			local lag_pool_max = math.max(max_lag_estimate * 2.0, LAG_POOL_MIN)
			if time_from_last_teleport > lag_pool_max then
				minetest.log("action", "Player " .. self.player:get_player_name()
						.. " moved too fast; resetting position")
				core.run_anticheat_callbacks(self.player, "moved_too_fast", {player_pos = pos,
					player_max_speed = player_max_speed})
				return true
			end
		end
	end,

	set_max_speed = function(self, max_speed)
		self.max_speed = max_speed
	end,

	get_max_speed = function(self)
		if self.max_speed then
			return self.max_speed
		end

		if core.check_player_privs(self.player, "fast") then
			return core.settings:get("movement_speed_fast") * self.player:get_physics_override().speed
		else
			return core.settings:get("movement_speed_walk") * self.player:get_physics_override().speed
		end
	end
})

core.register_anticheat_object("interacted_while_dead", {
	check = function(self)
		if not self.enabled then
			return false
		end

		if self.player:get_hp() == 0 then
			minetest.log("action", "Server: NoCheat: " .. self.player:get_player_name()
					.. " tried to interact while dead; ignoring.")
			core.run_anticheat_callbacks(self.player, "interacted_while_dead", {})
			return true
		else
			return false
		end
	end
})
