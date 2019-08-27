-- Minetest: builtin/item_entity.lua

function core.spawn_item(pos, item)
	-- Take item in any format
	local stack = ItemStack(item)
	local obj = core.add_entity(pos, "__builtin:item")
	-- Don't use obj if it couldn't be added to the map.
	if obj then
		obj:get_luaentity():set_item(stack:to_string())
	end
	return obj
end

-- If item_entity_ttl is not set, enity will have default life time
-- Setting it to -1 disables the feature

local time_to_live = tonumber(core.settings:get("item_entity_ttl")) or 900
local gravity = tonumber(core.settings:get("movement_gravity")) or 9.81

local function enable_physics(object, luaentity)
	if luaentity.physical_state == false then
		luaentity.physical_state = true
		object:set_properties({
			physical = true
		})
		object:set_velocity({x=0,y=0,z=0})
		object:set_acceleration({x=0,y=-gravity,z=0})
	end
end

local function disable_physics(object, luaentity)
	if luaentity.physical_state == true then
		luaentity.physical_state = false
		object:set_properties({
			physical = false
		})
		object:set_velocity({x=0,y=0,z=0})
		object:set_acceleration({x=0,y=0,z=0})
	end
end

core.register_entity(":__builtin:item", {
	initial_properties = {
		hp_max = 1,
		physical = true,
		collide_with_objects = false,
		collisionbox = {-0.3, -0.3, -0.3, 0.3, 0.3, 0.3},
		visual = "wielditem",
		visual_size = {x = 0.4, y = 0.4},
		textures = {""},
		spritediv = {x = 1, y = 1},
		initial_sprite_basepos = {x = 0, y = 0},
		is_visible = false,
	},

	itemstring = "",
	moving_state = true,
	slippery_state = false,
	physical_state = true,
	age = 0,
	-- Helper vars for pushing item out of solid nodes
	force_out = nil,
	force_out_start = nil,
	force_out_timer = 0,

	set_item = function(self, item)
		local stack = ItemStack(item or self.itemstring)
		self.itemstring = stack:to_string()
		if self.itemstring == "" then
			-- item not yet known
			return
		end

		-- Backwards compatibility: old clients use the texture
		-- to get the type of the item
		local itemname = stack:is_known() and stack:get_name() or "unknown"

		local max_count = stack:get_stack_max()
		local count = math.min(stack:get_count(), max_count)
		local size = 0.2 + 0.1 * (count / max_count) ^ (1 / 3)
		local coll_height = size * 0.75

		self.object:set_properties({
			is_visible = true,
			visual = "wielditem",
			textures = {itemname},
			visual_size = {x = size, y = size},
			collisionbox = {-size, -coll_height, -size,
				size, coll_height, size},
			selectionbox = {-size, -size, -size, size, size, size},
			automatic_rotate = math.pi * 0.5 * 0.2 / size,
			wield_item = self.itemstring,
		})

	end,

	get_staticdata = function(self)
		return core.serialize({
			itemstring = self.itemstring,
			age = self.age,
			dropped_by = self.dropped_by
		})
	end,

	on_activate = function(self, staticdata, dtime_s)
		if string.sub(staticdata, 1, string.len("return")) == "return" then
			local data = core.deserialize(staticdata)
			if data and type(data) == "table" then
				self.itemstring = data.itemstring
				self.age = (data.age or 0) + dtime_s
				self.dropped_by = data.dropped_by
			end
		else
			self.itemstring = staticdata
		end
		self.object:set_armor_groups({immortal = 1})
		self.object:set_velocity({x = 0, y = 2, z = 0})
		self.object:set_acceleration({x = 0, y = -gravity, z = 0})
		self:set_item()
	end,

	try_merge_with = function(self, own_stack, object, entity)
		if self.age == entity.age then
			-- Can not merge with itself
			return false
		end

		local stack = ItemStack(entity.itemstring)
		local name = stack:get_name()
		if own_stack:get_name() ~= name or
				own_stack:get_meta() ~= stack:get_meta() or
				own_stack:get_wear() ~= stack:get_wear() or
				own_stack:get_free_space() == 0 then
			-- Can not merge different or full stack
			return false
		end

		local count = own_stack:get_count()
		local total_count = stack:get_count() + count
		local max_count = stack:get_stack_max()

		if total_count > max_count then
			return false
		end
		-- Merge the remote stack into this one

		local pos = object:get_pos()
		pos.y = pos.y + ((total_count - count) / max_count) * 0.15
		self.object:move_to(pos)

		self.age = 0 -- Handle as new entity
		own_stack:set_count(total_count)
		self:set_item(own_stack)

		entity.itemstring = ""
		object:remove()
		return true
	end,

	on_step = function(self, dtime)
		self.age = self.age + dtime
		if time_to_live > 0 and self.age > time_to_live then
			self.itemstring = ""
			self.object:remove()
			return
		end

		local pos = self.object:get_pos()
		local node = core.get_node_or_nil({
			x = pos.x,
			y = pos.y + self.object:get_properties().collisionbox[2] - 0.05,
			z = pos.z
		})
		-- Delete in 'ignore' nodes
		if node and node.name == "ignore" then
			self.itemstring = ""
			self.object:remove()
			return
		end

		local node_this = core.get_node_or_nil(pos)
		local is_stuck = false
		if node_this then
			local sdef = minetest.registered_nodes[node_this.name]
			is_stuck = (sdef.walkable == nil or sdef.walkable == true)
				and (sdef.collision_box == nil or sdef.collision_box.type == "regular")
				and (sdef.node_box == nil or sdef.node_box.type == "regular")
		end

		-- Push item out when stuck inside solid node
		if is_stuck then
			local shootdir
			local cx = pos.x % 1
			local cz = pos.z % 1
			local order = {}

			-- First prepare the order in which the 4 sides are to be checked.
			-- 1st: closest
			-- 2nd: other direction
			-- 3rd and 4th: other axis
			local cxcz = function(o, cw, one, zero)
				if cw > 0 then
					table.insert(o, { [one]=1, y=0, [zero]=0 })
					table.insert(o, { [one]=-1, y=0, [zero]=0 })
				else
					table.insert(o, { [one]=-1, y=0, [zero]=0 })
					table.insert(o, { [one]=1, y=0, [zero]=0 })
				end
				return o
			end
			if math.abs(cx) > math.abs(cz) then
				order = cxcz(order, cx, "x", "z")
				order = cxcz(order, cz, "z", "x")
			else
				order = cxcz(order, cz, "z", "x")
				order = cxcz(order, cx, "x", "z")
			end

			-- Check which one of the 4 sides is free
			for o=1, #order do
				local nn = minetest.get_node(vector.add(pos, order[o])).name
				local def_side = minetest.registered_nodes[nn]
				if def_side and def_side.walkable == false and nn ~= "ignore" then
					shootdir = order[o]
					break
				end
			end
			-- If none of the 4 sides is free, shoot upwards
			if shootdir == nil then
				shootdir = { x=0, y=1, z=0 }
				local nn = minetest.get_node(vector.add(pos, shootdir)).name
				if nn == "ignore" then
					-- Do not push into ignore
					return
				end
			end

			-- Set new item moving speed accordingly
			local newv = vector.multiply(shootdir, 3)
			self.object:set_acceleration({x = 0, y = 0, z = 0})
			self.object:set_velocity(newv)

			disable_physics(self.object, self)

			if shootdir.y == 0 then
				self.force_out = newv
				self.force_out_start = vector.floor(pos)
				self.force_out_timer = 1
			end
			return
		end

		-- This code is run after the entity got a push from above “push away” code.
		-- It is responsible for making sure the entity is entirely outside the solid node
		-- (with its full collision box), not just its center.
		if self.force_out_timer > 0 then
			local c = self.object:get_properties().collisionbox
			local s = self.force_out_start
			local f = self.force_out
			local ok = false
			if f.x > 0 and (pos.x > (s.x + 0.5 + (c[4] - c[1])/2)) then
				ok = true
			elseif f.x < 0 and (pos.x < (s.x + 0.5 - (c[4] - c[1])/2)) then
				ok = true
			elseif f.z > 0 and (pos.z > (s.z + 0.5 + (c[6] - c[3])/2)) then
				ok = true
			elseif f.z < 0 and (pos.z < (s.z + 0.5 - (c[6] - c[3])/2)) then
				ok = true
			end
			-- Item was successfully forced out. No more pushing
			if ok then
				self.force_out_timer = -1
				self.force_out = nil
				enable_physics(self.object, self)
			else
				self.force_out_timer = self.force_out_timer - dtime
			end
			return
		elseif self.force_out or not self.physical_state then
			self.force_out = nil
			enable_physics(self.object, self)
			return
		end

		-- Slide on slippery nodes
		local vel = self.object:get_velocity()
		local def = node and core.registered_nodes[node.name]
		local is_moving = (def and not def.walkable) or
			vel.x ~= 0 or vel.y ~= 0 or vel.z ~= 0
		local is_slippery = false

		if def and def.walkable then
			local slippery = core.get_item_group(node.name, "slippery")
			is_slippery = slippery ~= 0
			if is_slippery and (math.abs(vel.x) > 0.2 or math.abs(vel.z) > 0.2) then
				-- Horizontal deceleration
				local slip_factor = 4.0 / (slippery + 4)
				self.object:set_acceleration({
					x = -vel.x * slip_factor,
					y = 0,
					z = -vel.z * slip_factor
				})
			elseif vel.y == 0 then
				is_moving = false
			end
		end

		if self.moving_state == is_moving and
				self.slippery_state == is_slippery then
			-- Do not update anything until the moving state changes
			return
		end

		self.moving_state = is_moving
		self.slippery_state = is_slippery

		if is_moving then
			self.object:set_acceleration({x = 0, y = -gravity, z = 0})
		else
			self.object:set_acceleration({x = 0, y = 0, z = 0})
			self.object:set_velocity({x = 0, y = 0, z = 0})
		end

		--Only collect items if not moving
		if is_moving then
			return
		end
		-- Collect the items around to merge with
		local own_stack = ItemStack(self.itemstring)
		if own_stack:get_free_space() == 0 then
			return
		end
		local objects = core.get_objects_inside_radius(pos, 1.0)
		for k, obj in pairs(objects) do
			local entity = obj:get_luaentity()
			if entity and entity.name == "__builtin:item" then
				if self:try_merge_with(own_stack, obj, entity) then
					own_stack = ItemStack(self.itemstring)
					if own_stack:get_free_space() == 0 then
						return
					end
				end
			end
		end
	end,

	on_punch = function(self, hitter)
		local inv = hitter:get_inventory()
		if inv and self.itemstring ~= "" then
			local left = inv:add_item("main", self.itemstring)
			if left and not left:is_empty() then
				self:set_item(left)
				return
			end
		end
		self.itemstring = ""
		self.object:remove()
	end,
})
