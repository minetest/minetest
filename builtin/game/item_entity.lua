-- Minetest: builtin/item_entity.lua

function core.spawn_item(pos, item)
	-- Take item in any format
	local stack = ItemStack(item)
	local obj = core.add_entity(pos, "__builtin:item")
	obj:get_luaentity():set_item(stack)
	return obj
end

-- If item_entity_ttl is not set, enity will have default life time 
-- Setting it to -1 disables the feature

local time_to_live = tonumber(core.setting_get("item_entity_ttl"))
if not time_to_live then
	time_to_live = 900
end

core.register_entity(":__builtin:item", {

	hp_max = 1,
	physical = true,
	collide_with_objects = false,
	collisionbox = {-0.24, -0.24, -0.24, 0.24, 0.24, 0.24},
	visual = "wielditem",
	visual_size = {x = 0.3, y = 0.3},
	textures = {""},
	spritediv = {x = 1, y = 1},
	initial_sprite_basepos = {x = 0, y = 0},
	is_visible = false,

	itemstring = '',
	physical_state = true,
	age = 0,

	set_item = function(self, stack)
		if type(stack) == "string" then
			stack = ItemStack(stack)
		end
		local itemname = stack:get_name()
		--print("DROP! itemname: "..dump(itemname))
		if not (itemname and (itemname ~= "")) then
			-- Only used for visual.
			itemname = "unknown"
			print("UNKNOWN!")
		end
		self.itemstack = stack
		local s = math.min(0.3, 0.15 + 0.15 * (stack:get_count() / stack:get_stack_max()))
		local c = 0.8 * s
		self.object:set_properties({
			is_visible = true,
			visual = "wielditem",
			textures = {itemname},
			visual_size = {x = s, y = s},
			collisionbox = {-c, -c, -c, c, c, c},
			automatic_rotate = math.pi * 0.2,
		})
		return true
	end,

	get_staticdata = function(self)
		return core.serialize({
			item = self.itemstack and self.itemstack:to_table(),
			always_collect = self.always_collect,
			age = self.age
		})
	end,

	on_activate = function(self, staticdata, dtime_s)
		if staticdata and (staticdata ~= "") then
			local itemstack
			if staticdata:sub(1, 6) == "return" then
				local data = core.deserialize(staticdata)
				if data and type(data) == "table" then
					itemstack = ItemStack(data.item or data.itemstring)
					self.always_collect = data.always_collect
					if data.age then 
						self.age = data.age + dtime_s
					else
						self.age = dtime_s
					end
				end
			else
				self.itemstring = staticdata
			end
			self:set_item(itemstack)
		end
		self.object:set_armor_groups({immortal = 1})
		self.object:setvelocity({x = 0, y = 2, z = 0})
		self.object:setacceleration({x = 0, y = -10, z = 0})
	end,

	join_stacks = function(self, ent, own_stack, ent_stack)
		local own_count = own_stack:get_count()
		local ent_count = ent_stack:get_count()
		local own_max = own_stack:get_stack_max()
		local total = own_count + ent_count
		local left = total - own_max
		local s, c
		if left > 0 then
			ent.itemstack:set_count(left)
			ent.itemstack = ent_stack
			local ent_max = ent_stack:get_stack_max()
			s = math.min(0.3, 0.15 + 0.15 * (left / ent_max))
			c = 0.8 * s
			ent.object:set_properties({
				visual_size = {x = s, y = s},
				collisionbox = {-c, -c, -c, c, c, c}
			})
		else
			ent.itemstack = nil
			ent.object:remove()
		end
		own_count = math.min(own_max, total)
		self.itemstack:set_count(own_count)
		local pos = self.object:getpos()
		pos.y = pos.y + (total - own_count) / own_max * 0.15
		self.object:setpos(pos)
		s = math.min(0.3, 0.15 + 0.15 * (own_count / own_max))
		c = 0.8 * s
		self.object:set_properties({
			visual_size = {x = s, y = s},
			collisionbox = {-c, -c, -c, c, c, c}
		})
	end,

	on_step = function(self, dtime)
		self.age = self.age + dtime
		if time_to_live > 0 and self.age > time_to_live then
			self.itemstack = nil
			self.object:remove()
			return
		end
		local p = self.object:getpos()
		p.y = p.y - 0.3
		local nn = core.get_node(p).name
		local v = self.object:getvelocity()
		local nndef = core.registered_nodes[nn]
		-- If node is not registered or node is walkably solid
		-- and resting on nodebox
		if ((not nndef) or nndef.walkable) and v.y == 0 then
			if self.physical_state then
				p.y = p.y + 0.3
				local own_stack = self.itemstack
				for _, object in ipairs(core.get_objects_inside_radius(p, 0.8)) do
					if object ~= self.object then
						local ent = object:get_luaentity()
						if ent and ent.name == "__builtin:item"
								and not ent.physical_state then
							local ent_stack = ent.itemstack
							if ent_stack
									and (own_stack:get_name() == ent_stack:get_name())
									and ent_stack:get_free_space() > 0 then 
								self:join_stacks(ent, own_stack, ent_stack)
							end
						end
					else
						print "IGNORE SELF"
					end
				end
				self.object:setvelocity({x = 0, y = 0, z = 0})
				self.object:setacceleration({x = 0, y = 0, z = 0})
				self.physical_state = false
				self.object:set_properties({physical = false})
			end
		elseif not self.physical_state then
			self.object:setvelocity({x = 0, y = 0, z = 0})
			self.object:setacceleration({x = 0, y = -10, z = 0})
			self.physical_state = true
			self.object:set_properties({physical = true})
		end
	end,

	on_punch = function(self, hitter)
		if self.itemstack then
			local left = hitter:get_inventory():add_item("main", self.itemstack)
			if not left:is_empty() then
				self.itemstack = left
				return
			end
		end
		self.itemstack = nil
		self.object:remove()
	end,
})
