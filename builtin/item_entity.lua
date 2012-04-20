-- Minetest: builtin/item_entity.lua

function minetest.spawn_item(pos, item)
	-- Take item in any format
	local stack = ItemStack(item)
	local obj = minetest.env:add_entity(pos, "__builtin:item")
	obj:get_luaentity():set_item(stack:to_string())
	return obj
end

minetest.register_entity("__builtin:item", {
	initial_properties = {
		hp_max = 1,
		physical = true,
		collisionbox = {-0.17,-0.17,-0.17, 0.17,0.17,0.17},
		visual = "sprite",
		visual_size = {x=0.5, y=0.5},
		textures = {""},
		spritediv = {x=1, y=1},
		initial_sprite_basepos = {x=0, y=0},
		is_visible = false,
	},
	
	itemstring = '',
	physical_state = true,
	dontbugme = true,
	outercircle = 2.0,
	innercircle = 0.5,
	gravity = true,
	whocaresaboutnodes = false,
	lastplayer = false,

	set_item = function(self, itemstring)
		self.itemstring = itemstring
		local stack = ItemStack(itemstring)
		local itemtable = stack:to_table()
		local itemname = nil
		if itemtable then
			itemname = stack:to_table().name
		end
		local item_texture = nil
		local item_type = ""
		if minetest.registered_items[itemname] then
			item_texture = minetest.registered_items[itemname].inventory_image
			item_type = minetest.registered_items[itemname].type
		end
		prop = {
			is_visible = true,
			visual = "sprite",
			textures = {"unknown_item.png"}
		}
		prop.visual = "wielditem"
		prop.textures = {itemname}
		prop.visual_size = {x=0.20, y=0.20}
		prop.automatic_rotate = math.pi * 0.25
		self.object:set_properties(prop)
	end,
	get_staticdata = function(self)
		return self.itemstring
	end,

	on_activate = function(self, staticdata)
		self.itemstring = staticdata
		self.object:set_armor_groups({immortal=1})
		self.object:setvelocity({x=math.random(-1.5,1.5), y=0, z=math.random(-1.5,1.5)})
		self.object:setacceleration({x=0, y=-10, z=0})
		self:set_item(self.itemstring)
	end,

	on_step = function(self, dtime)
		local p = self.object:getpos()
		p.y = p.y - 0.3
		local nn = minetest.env:get_node(p).name
		if minetest.registered_nodes[nn].walkable and self.whocaresaboutnodes == false then
			if self.physical_state then
				self.object:setvelocity({x=0, y=0, z=0})
				self.object:setacceleration({x=0, y=0, z=0})
				self.physical_state = false
				self.object:set_properties({
					physical = false
				})
				self.dontbugme = false
			end
		else
			if not self.physical_state and self.gravity == true then
				self.object:setvelocity({x=0, y=0, z=0})
				self.object:setacceleration({x=0, y=-10, z=0})
				self.physical_state = true
				self.object:set_properties({
					physical = true
				})
				self.dontbugme = true
			end
		end
		local pos = p
		local objs = minetest.env:get_objects_inside_radius(pos, self.innercircle)
		local objs2 = minetest.env:get_objects_inside_radius(pos, self.outercircle)
		for k, obj in pairs(objs) do
			local objpos=obj:getpos()
			if objpos.y >= pos.y-1.5 and objpos.y <= pos.y+.5 then
				if obj:get_player_name() ~= nil then
					if self.itemstring ~= '' then
						obj:get_inventory():add_item("main", self.itemstring)
					end
					self.object:remove()
				end
			end
		end
		local playerfound = false
		for k, obj in pairs(objs2) do
			local objpos=obj:getpos()
			if obj:get_player_name() ~= nil and objpos.y >= pos.y-1.25 and objpos.y <= pos.y+.25 then
				playerfound = true
				if self.dontbugme == false then
					self.lastplayer = true
					local fx = objpos.x - pos.x
					local fy = objpos.y - pos.y
					local fz = objpos.z - pos.z
					self.gravity = false
					self.physical_state = false
					self.object:set_properties({
						physical = false
					})
					self.object:setvelocity({x=fx * 5, y=fy * 5, z=fz * 5})
					self.dontbugme = true
					self.whocaresaboutnodes = true
				end
			end
		end
		if playerfound == false then
			if self.lastplayer == true then
				self.lastplayer = false
				self.object:setvelocity({x=0, y=0, z=0})
				self.object:setacceleration({x=0, y=-10, z=0})
			end
			self.gravity = true
			self.physical_state = true
			self.object:set_properties({
				physical = true
			})
			self.whocaresaboutnodes = false
		end
	end,

	on_punch = function(self, hitter)
		if self.itemstring ~= '' then
			hitter:get_inventory():add_item("main", self.itemstring)
		end
		self.object:remove()
	end,
})

