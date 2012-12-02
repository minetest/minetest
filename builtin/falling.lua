-- Minetest: builtin/item.lua

--
-- Falling stuff
--

minetest.register_entity("__builtin:falling_node", {
	initial_properties = {
		physical = true,
		collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
		visual = "wielditem",
		textures = {},
		visual_size = {x=0.667, y=0.667},
	},

	nodename = "",

	set_node = function(self, nodename)
		self.nodename = nodename
		local stack = ItemStack(nodename)
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
			textures = {nodename},
		}
		self.object:set_properties(prop)
	end,

	get_staticdata = function(self)
		return self.nodename
	end,

	on_activate = function(self, staticdata)
		self.nodename = staticdata
		self.object:set_armor_groups({immortal=1})
		--self.object:setacceleration({x=0, y=-10, z=0})
		self:set_node(self.nodename)
	end,

	on_step = function(self, dtime)
		-- Set gravity
		self.object:setacceleration({x=0, y=-10, z=0})
		-- Turn to actual sand when collides to ground or just move
		local pos = self.object:getpos()
		local bcp = {x=pos.x, y=pos.y-0.7, z=pos.z} -- Position of bottom center point
		local bcn = minetest.env:get_node(bcp)
		-- Note: walkable is in the node definition, not in item groups
		if minetest.registered_nodes[bcn.name] and
				minetest.registered_nodes[bcn.name].walkable then
			local np = {x=bcp.x, y=bcp.y+1, z=bcp.z}
			-- Check what's here
			local n2 = minetest.env:get_node(np)
			-- If it's not air or liquid, remove node and replace it with
			-- it's drops
			if n2.name ~= "air" and (not minetest.registered_nodes[n2.name] or
					minetest.registered_nodes[n2.name].liquidtype == "none") then
				local drops = minetest.get_node_drops(n2.name, "")
				minetest.env:remove_node(np)
				-- Add dropped items
				local _, dropped_item
				for _, dropped_item in ipairs(drops) do
					minetest.env:add_item(np, dropped_item)
				end
				-- Run script hook
				local _, callback
				for _, callback in ipairs(minetest.registered_on_dignodes) do
					callback(np, n2, nil)
				end
			end
			-- Create node and remove entity
			minetest.env:add_node(np, {name=self.nodename})
			self.object:remove()
		else
			-- Do nothing
		end
	end
})

function spawn_falling_node(p, nodename)
	obj = minetest.env:add_entity(p, "__builtin:falling_node")
	obj:get_luaentity():set_node(nodename)
end

function drop_attached_node(p)
	local nn = minetest.env:get_node(p).name
	minetest.env:remove_node(p)
	for _,item in ipairs(minetest.get_node_drops(nn, "")) do
		local pos = {
			x = p.x + math.random(60)/60-0.3,
			y = p.y + math.random(60)/60-0.3,
			z = p.z + math.random(60)/60-0.3,
		}
		minetest.env:add_item(pos, item)
	end
end

function check_attached_node(p, n)
	local def = minetest.registered_nodes[n.name]
	local d = {x=0, y=0, z=0}
	if def.paramtype2 == "wallmounted" then
		if n.param2 == 0 then
			d.y = 1
		elseif n.param2 == 1 then
			d.y = -1
		elseif n.param2 == 2 then
			d.x = 1
		elseif n.param2 == 3 then
			d.x = -1
		elseif n.param2 == 4 then
			d.z = 1
		elseif n.param2 == 5 then
			d.z = -1
		end
	else
		d.y = -1
	end
	local p2 = {x=p.x+d.x, y=p.y+d.y, z=p.z+d.z}
	local nn = minetest.env:get_node(p2).name
	local def2 = minetest.registered_nodes[nn]
	if def2 and not def2.walkable then
		return false
	end
	return true
end

--
-- Some common functions
--

function nodeupdate_single(p)
	n = minetest.env:get_node(p)
	if minetest.get_node_group(n.name, "falling_node") ~= 0 then
		p_bottom = {x=p.x, y=p.y-1, z=p.z}
		n_bottom = minetest.env:get_node(p_bottom)
		-- Note: walkable is in the node definition, not in item groups
		if minetest.registered_nodes[n_bottom.name] and
				not minetest.registered_nodes[n_bottom.name].walkable then
			minetest.env:remove_node(p)
			spawn_falling_node(p, n.name)
			nodeupdate(p)
		end
	end
	
	if minetest.get_node_group(n.name, "attached_node") ~= 0 then
		if not check_attached_node(p, n) then
			drop_attached_node(p)
			nodeupdate(p)
		end
	end
end

function nodeupdate(p)
	-- Round p to prevent falling entities to get stuck
	p.x = math.floor(p.x+0.5)
	p.y = math.floor(p.y+0.5)
	p.z = math.floor(p.z+0.5)
	
	for x = -1,1 do
	for y = -1,1 do
	for z = -1,1 do
		p2 = {x=p.x+x, y=p.y+y, z=p.z+z}
		nodeupdate_single(p2)
	end
	end
	end
end

--
-- Global callbacks
--

function on_placenode(p, node)
	nodeupdate(p)
end
minetest.register_on_placenode(on_placenode)

function on_dignode(p, node)
	nodeupdate(p)
end
minetest.register_on_dignode(on_dignode)
