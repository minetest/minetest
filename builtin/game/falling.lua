-- Minetest: builtin/item.lua

--
-- Falling stuff
--

core.register_entity(":__builtin:falling_node", {
	initial_properties = {
		visual = "wielditem",
		visual_size = {x = 0.667, y = 0.667},
		textures = {},
		physical = true,
		is_visible = false,
		collide_with_objects = false,
		collisionbox = {-0.5, -0.5, -0.5, 0.5, 0.5, 0.5},
	},

	node = {},

	set_node = function(self, node)
		self.node = node
		self.object:set_properties({
			is_visible = true,
			textures = {node.name},
		})
	end,

	get_staticdata = function(self)
		return core.serialize(self.node)
	end,

	on_activate = function(self, staticdata)
		self.object:set_armor_groups({immortal = 1})
		
		local node = core.deserialize(staticdata)
		if node then
			self:set_node(node)
		elseif staticdata ~= "" then
			self:set_node({name = staticdata})
		end
	end,

	on_step = function(self, dtime)
		 -- Set gravity
		local acceleration = self.object:getacceleration()
		if not vector.equals(acceleration, {x = 0, y = -10, z = 0}) then
			self.object:setacceleration({x = 0, y = -10, z = 0})
		end
		-- Turn to actual sand when collides to ground or just move
		local pos = self.object:getpos()
		local bcp = {x = pos.x, y = pos.y - 0.7, z = pos.z} -- Position of bottom center point
		local bcn = core.get_node(bcp)
		local bcd = core.registered_nodes[bcn.name]
		-- Note: walkable is in the node definition, not in item groups
		if not bcd or
				(bcd.walkable or
				(core.get_item_group(self.node.name, "float") ~= 0 and
				bcd.liquidtype ~= "none")) then
			if bcd and bcd.leveled and
					bcn.name == self.node.name then
				local addlevel = self.node.level
				if not addlevel or addlevel <= 0 then
					addlevel = bcd.leveled
				end
				if core.add_node_level(bcp, addlevel) == 0 then
					self.object:remove()
					return
				end
			elseif bcd and bcd.buildable_to and
					(core.get_item_group(self.node.name, "float") == 0 or
					bcd.liquidtype == "none") then
				core.remove_node(bcp)
				return
			end
			local np = {x = bcp.x, y = bcp.y + 1, z = bcp.z}
			-- Check what's here
			local n2 = core.get_node(np)
			-- If it's not air or liquid, remove node and replace it with
			-- it's drops
			if n2.name ~= "air" and (not core.registered_nodes[n2.name] or
					core.registered_nodes[n2.name].liquidtype == "none") then
				core.remove_node(np)
				if core.registered_nodes[n2.name].buildable_to == false then
					-- Add dropped items
					local drops = core.get_node_drops(n2.name, "")
					for _, dropped_item in ipairs(drops) do
						core.add_item(np, dropped_item)
					end
				end
				-- Run script hook
				for _, callback in ipairs(core.registered_on_dignodes) do
					callback(np, n2)
				end
			end
			-- Create node and remove entity
			if core.registered_nodes[self.node.name] then
				core.add_node(np, self.node)
			end
			self.object:remove()
			nodeupdate(np)
			return
		end
		local vel = self.object:getvelocity()
		if vector.equals(vel, {x = 0, y = 0, z = 0}) then
			local npos = self.object:getpos()
			self.object:setpos(vector.round(npos))
		end
	end
})

function spawn_falling_node(p, node)
	local obj = core.add_entity(p, "__builtin:falling_node")
	obj:get_luaentity():set_node(node)
end

function drop_attached_node(p)
	local nn = core.get_node(p).name
	core.remove_node(p)
	for _, item in ipairs(core.get_node_drops(nn, "")) do
		local pos = {
			x = p.x + math.random()/2 - 0.25,
			y = p.y + math.random()/2 - 0.25,
			z = p.z + math.random()/2 - 0.25,
		}
		core.add_item(pos, item)
	end
end

function check_attached_node(p, n)
	local def = core.registered_nodes[n.name]
	local d = {x = 0, y = 0, z = 0}
	if def.paramtype2 == "wallmounted" then
		d = core.wallmounted_to_dir(n.param2)
	else
		d.y = -1
	end
	local p2 = vector.add(p, d)
	local nn = core.get_node(p2).name
	local def2 = core.registered_nodes[nn]
	if def2 and not def2.walkable then
		return false
	end
	return true
end

--
-- Some common functions
--

function nodeupdate_single(p)
	local n = core.get_node(p)
	if core.get_item_group(n.name, "falling_node") ~= 0 then
		local p_bottom = {x = p.x, y = p.y - 1, z = p.z}
		local n_bottom = core.get_node(p_bottom)
		-- Note: walkable is in the node definition, not in item groups
		if core.registered_nodes[n_bottom.name] and
				(core.get_item_group(n.name, "float") == 0 or
					core.registered_nodes[n_bottom.name].liquidtype == "none") and
				(n.name ~= n_bottom.name or (core.registered_nodes[n_bottom.name].leveled and
					core.get_node_level(p_bottom) < core.get_node_max_level(p_bottom))) and
				(not core.registered_nodes[n_bottom.name].walkable or
					core.registered_nodes[n_bottom.name].buildable_to) then
			n.level = core.get_node_level(p)
			core.remove_node(p)
			spawn_falling_node(p, n)
			return true
		end
	end

	if core.get_item_group(n.name, "attached_node") ~= 0 then
		if not check_attached_node(p, n) then
			drop_attached_node(p)
			return true
		end
	end

	return false
end

-- This table is specifically ordered.
-- We don't walk diagonals, only our direct neighbors, and self.
-- Down first as likely case, but always before self. The same with sides.
-- Up must come last, so that things above self will also fall all at once.
local nodeupdate_neighbors = {
	{x = -1, y = -1, z = 0},
	{x = 1, y = -1, z = 0},
	{x = 0, y = -1, z = -1},
	{x = 0, y = -1, z = 1},
	{x = 0, y = -1, z = 0},
	{x = -1, y = 0, z = 0},
	{x = 1, y = 0, z = 0},
	{x = 0, y = 0, z = 1},
	{x = 0, y = 0, z = -1},
	{x = 0, y = 0, z = 0},
	{x = 0, y = 1, z = 0},
}

function nodeupdate(p)
	-- Round p to prevent falling entities to get stuck.
	p = vector.round(p)

	-- We make a stack, and manually maintain size for performance.
	-- Stored in the stack, we will maintain tables with pos, and
	-- last neighbor visited. This way, when we get back to each
	-- node, we know which directions we have already walked, and
	-- which direction is the next to walk.
	local s = {}
	local n = 0
	-- The neighbor order we will visit from our table.
	local v = 1

	while true do
		-- Push current pos onto the stack.
		n = n + 1
		s[n] = {p = p, v = v}
		-- Select next node from neighbor list.
		p = vector.add(p, nodeupdate_neighbors[v])
		-- Now we check out the node. If it is in need of an update,
		-- it will let us know in the return value (true = updated).
		if not nodeupdate_single(p) then
			-- If we don't need to "recurse" (walk) to it then pop
			-- our previous pos off the stack and continue from there,
			-- with the v value we were at when we last were at that
			-- node
			repeat
				local pop = s[n]
				p = pop.p
				v = pop.v
				s[n] = nil
				n = n - 1
				-- If there's nothing left on the stack, and no
				-- more sides to walk to, we're done and can exit
				if n == 0 and v == 11 then
					return
				end
			until v < 11
			-- The next round walk the next neighbor in list.
			v = v + 1
		else
			-- If we did need to walk the neighbor, then
			-- start walking it from the walk order start (1),
			-- and not the order we just pushed up the stack.
			v = 1
		end
	end
end

--
-- Global callbacks
--

function on_placenode(p, node)
	nodeupdate(p)
end
core.register_on_placenode(on_placenode)

function on_dignode(p, node)
	nodeupdate(p)
end
core.register_on_dignode(on_dignode)
