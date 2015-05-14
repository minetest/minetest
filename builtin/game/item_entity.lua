-- Minetest: builtin/item_entity.lua

function core.spawn_item(pos, item)
	-- Take item in any format
	local stack = ItemStack(item)
	local obj = core.add_entity(pos, "__builtin:item")
	obj:get_luaentity():set_item(stack:to_string())
	return obj
end

-- If item_entity_ttl is not set, enity will have default life time 
-- Setting it to -1 disables the feature

local time_to_live = tonumber(core.setting_get("item_entity_ttl"))
if not time_to_live then
	time_to_live = 900
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

	itemstring = '',
	physical_state = true,
	age = 0,

	set_item = function(self, itemstring)
		self.itemstring = itemstring
		local stack = ItemStack(itemstring)
		local count = stack:get_count()
		local max_count = stack:get_stack_max()
		if count > max_count then
			count = max_count
			self.itemstring = stack:get_name().." "..max_count
		end
		local s = 0.2 + 0.1 * (count / max_count)
		local c = s
		local itemtable = stack:to_table()
		local itemname = nil
		if itemtable then
			itemname = stack:to_table().name
		end
		local item_texture = nil
		local item_type = ""
		if core.registered_items[itemname] then
			item_texture = core.registered_items[itemname].inventory_image
			item_type = core.registered_items[itemname].type
		end
		local prop = {
			is_visible = true,
			visual = "wielditem",
			textures = {itemname},
			visual_size = {x = s, y = s},
			collisionbox = {-c, -c, -c, c, c, c},
			automatic_rotate = math.pi * 0.5,
		}
		self.object:set_properties(prop)
	end,

	get_staticdata = function(self)
		return core.serialize({
			itemstring = self.itemstring,
			always_collect = self.always_collect,
			age = self.age
		})
	end,

	on_activate = function(self, staticdata, dtime_s)
		if string.sub(staticdata, 1, string.len("return")) == "return" then
			local data = core.deserialize(staticdata)
			if data and type(data) == "table" then
				self.itemstring = data.itemstring
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
		self.object:set_armor_groups({immortal = 1})
		self.object:setvelocity({x = 0, y = 2, z = 0})
		self.object:setacceleration({x = 0, y = -10, z = 0})
		self:set_item(self.itemstring)
	end,

	try_merge_with = function(self, own_stack, object, obj)
		local stack = ItemStack(obj.itemstring)
		if own_stack:get_name() == stack:get_name() and stack:get_free_space() > 0 then
			local overflow = false
			local count = stack:get_count() + own_stack:get_count()
			local max_count = stack:get_stack_max()
			if count > max_count then
				overflow = true
				count = count - max_count
			else
				self.itemstring = ''
			end
			local pos = object:getpos()
			pos.y = pos.y + (count - stack:get_count()) / max_count * 0.15
			object:moveto(pos, false)
			local s, c
			local max_count = stack:get_stack_max()
			local name = stack:get_name()
			if not overflow then
				obj.itemstring = name .. " " .. count
				s = 0.2 + 0.1 * (count / max_count)
				c = s
				object:set_properties({
					visual_size = {x = s, y = s},
					collisionbox = {-c, -c, -c, c, c, c}
				})
				self.object:remove()
				-- merging succeeded
				return true
			else
				s = 0.4
				c = 0.3
				object:set_properties({
					visual_size = {x = s, y = s},
					collisionbox = {-c, -c, -c, c, c, c}
				})
				obj.itemstring = name .. " " .. max_count
				s = 0.2 + 0.1 * (count / max_count)
				c = s
				self.object:set_properties({
					visual_size = {x = s, y = s},
					collisionbox = {-c, -c, -c, c, c, c}
				})
				self.itemstring = name .. " " .. count
			end
		end
		-- merging didn't succeed
		return false
	end,

	on_step = function(self, dtime)
		self.age = self.age + dtime
		if time_to_live > 0 and self.age > time_to_live then
			self.itemstring = ''
			self.object:remove()
			return
		end
		local p = self.object:getpos()
		p.y = p.y - 0.5
		local node = core.get_node_or_nil(p)
		local in_unloaded = (node == nil)
		if in_unloaded then
			-- Don't infinetly fall into unloaded map
			self.object:setvelocity({x = 0, y = 0, z = 0})
			self.object:setacceleration({x = 0, y = 0, z = 0})
			self.physical_state = false
			self.object:set_properties({physical = false})
			return
		end
		local nn = node.name
		-- If node is not registered or node is walkably solid and resting on nodebox
		local v = self.object:getvelocity()
		if not core.registered_nodes[nn] or core.registered_nodes[nn].walkable and v.y == 0 then
			if self.physical_state then
				local own_stack = ItemStack(self.object:get_luaentity().itemstring)
				-- Merge with close entities of the same item
				for _, object in ipairs(core.get_objects_inside_radius(p, 0.8)) do
					local obj = object:get_luaentity()
					if obj and obj.name == "__builtin:item"
							and obj.physical_state == false then
						if self:try_merge_with(own_stack, object, obj) then
							return
						end
					end
				end
				self.object:setvelocity({x = 0, y = 0, z = 0})
				self.object:setacceleration({x = 0, y = 0, z = 0})
				self.physical_state = false
				self.object:set_properties({physical = false})
			end
		else
			if not self.physical_state then
				self.object:setvelocity({x = 0, y = 0, z = 0})
				self.object:setacceleration({x = 0, y = -10, z = 0})
				self.physical_state = true
				self.object:set_properties({physical = true})
			end
		end
	end,

	on_punch = function(self, hitter)
		if self.itemstring ~= '' then
			local left = hitter:get_inventory():add_item("main", self.itemstring)
			if not left:is_empty() then
				self.itemstring = left:to_string()
				return
			end
		end
		self.itemstring = ''
		self.object:remove()
	end,
})
