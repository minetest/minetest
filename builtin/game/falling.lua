-- Minetest: builtin/item.lua

local builtin_shared = ...

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
	meta = {},

	set_node = function(self, node, meta)
		self.node = node
		meta = meta or {}
		if type(meta.to_table) == "function" then
			meta = meta:to_table()
		end
		for _, list in pairs(meta.inventory or {}) do
			for i, stack in pairs(list) do
				if type(stack) == "userdata" then
					list[i] = stack:to_string()
				end
			end
		end
		self.meta = meta
		self.object:set_properties({
			is_visible = true,
			textures = {node.name},
		})
	end,

	get_staticdata = function(self)
		local ds = {
			node = self.node,
			meta = self.meta,
		}
		return core.serialize(ds)
	end,

	on_activate = function(self, staticdata)
		self.object:set_armor_groups({immortal = 1})

		local ds = core.deserialize(staticdata)
		if ds and ds.node then
			self:set_node(ds.node, ds.meta)
		elseif ds then
			self:set_node(ds)
		elseif staticdata ~= "" then
			self:set_node({name = staticdata})
		end
	end,

	on_step = function(self, dtime)
		-- Set gravity
		local acceleration = self.object:get_acceleration()
		if not vector.equals(acceleration, {x = 0, y = -10, z = 0}) then
			self.object:set_acceleration({x = 0, y = -10, z = 0})
		end
		-- Turn to actual node when colliding with ground, or continue to move
		local pos = self.object:get_pos()
		-- Position of bottom center point
		local bcp = {x = pos.x, y = pos.y - 0.7, z = pos.z}
		-- 'bcn' is nil for unloaded nodes
		local bcn = core.get_node_or_nil(bcp)
		-- Delete on contact with ignore at world edges
		if bcn and bcn.name == "ignore" then
			self.object:remove()
			return
		end
		local bcd = bcn and core.registered_nodes[bcn.name]
		if bcn and
				(not bcd or bcd.walkable or
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
			local nd = core.registered_nodes[n2.name]
			-- If it's not air or liquid, remove node and replace it with
			-- it's drops
			if n2.name ~= "air" and (not nd or nd.liquidtype == "none") then
				core.remove_node(np)
				if nd and nd.buildable_to == false then
					-- Add dropped items
					local drops = core.get_node_drops(n2, "")
					for _, dropped_item in pairs(drops) do
						core.add_item(np, dropped_item)
					end
				end
				-- Run script hook
				for _, callback in pairs(core.registered_on_dignodes) do
					callback(np, n2)
				end
			end
			-- Create node and remove entity
			local def = core.registered_nodes[self.node.name]
			if def then
				core.add_node(np, self.node)
				if self.meta then
					local meta = core.get_meta(np)
					meta:from_table(self.meta)
				end
				if def.sounds and def.sounds.place then
					core.sound_play(def.sounds.place, {pos = np})
				end
			end
			self.object:remove()
			core.check_for_falling(np)
			return
		end
		local vel = self.object:get_velocity()
		if vector.equals(vel, {x = 0, y = 0, z = 0}) then
			local npos = self.object:get_pos()
			self.object:set_pos(vector.round(npos))
		end
	end
})

local function convert_to_falling_node(pos, node)
	local obj = core.add_entity(pos, "__builtin:falling_node")
	if not obj then
		return false
	end
	node.level = core.get_node_level(pos)
	local meta = core.get_meta(pos)
	local metatable = meta and meta:to_table() or {}

	local def = core.registered_nodes[node.name]
	if def and def.sounds and def.sounds.fall then
		core.sound_play(def.sounds.fall, {pos = pos})
	end

	obj:get_luaentity():set_node(node, metatable)
	core.remove_node(pos)
	return true
end

function core.spawn_falling_node(pos)
	local node = core.get_node(pos)
	if node.name == "air" or node.name == "ignore" then
		return false
	end
	return convert_to_falling_node(pos, node)
end

local function drop_attached_node(p)
	local n = core.get_node(p)
	local drops = core.get_node_drops(n, "")
	local def = core.registered_items[n.name]
	if def and def.preserve_metadata then
		local oldmeta = core.get_meta(p):to_table().fields
		-- Copy pos and node because the callback can modify them.
		local pos_copy = {x=p.x, y=p.y, z=p.z}
		local node_copy = {name=n.name, param1=n.param1, param2=n.param2}
		local drop_stacks = {}
		for k, v in pairs(drops) do
			drop_stacks[k] = ItemStack(v)
		end
		drops = drop_stacks
		def.preserve_metadata(pos_copy, node_copy, oldmeta, drops)
	end
	if def and def.sounds and def.sounds.fall then
		core.sound_play(def.sounds.fall, {pos = p})
	end
	core.remove_node(p)
	for _, item in pairs(drops) do
		local pos = {
			x = p.x + math.random()/2 - 0.25,
			y = p.y + math.random()/2 - 0.25,
			z = p.z + math.random()/2 - 0.25,
		}
		core.add_item(pos, item)
	end
end

function builtin_shared.check_attached_node(p, n)
	local def = core.registered_nodes[n.name]
	local d = {x = 0, y = 0, z = 0}
	if def.paramtype2 == "wallmounted" or
			def.paramtype2 == "colorwallmounted" then
		-- The fallback vector here is in case 'wallmounted to dir' is nil due
		-- to voxelmanip placing a wallmounted node without resetting a
		-- pre-existing param2 value that is out-of-range for wallmounted.
		-- The fallback vector corresponds to param2 = 0.
		d = core.wallmounted_to_dir(n.param2) or {x = 0, y = 1, z = 0}
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

function core.check_single_for_falling(p)
	local n = core.get_node(p)
	if core.get_item_group(n.name, "falling_node") ~= 0 then
		local p_bottom = {x = p.x, y = p.y - 1, z = p.z}
		-- Only spawn falling node if node below is loaded
		local n_bottom = core.get_node_or_nil(p_bottom)
		local d_bottom = n_bottom and core.registered_nodes[n_bottom.name]
		if d_bottom and

				(core.get_item_group(n.name, "float") == 0 or
				d_bottom.liquidtype == "none") and

				(n.name ~= n_bottom.name or (d_bottom.leveled and
				core.get_node_level(p_bottom) <
				core.get_node_max_level(p_bottom))) and

				(not d_bottom.walkable or d_bottom.buildable_to) then
			convert_to_falling_node(p, n)
			return true
		end
	end

	if core.get_item_group(n.name, "attached_node") ~= 0 then
		if not builtin_shared.check_attached_node(p, n) then
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
local check_for_falling_neighbors = {
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

function core.check_for_falling(p)
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
		p = vector.add(p, check_for_falling_neighbors[v])
		-- Now we check out the node. If it is in need of an update,
		-- it will let us know in the return value (true = updated).
		if not core.check_single_for_falling(p) then
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

local function on_placenode(p, node)
	core.check_for_falling(p)
end
core.register_on_placenode(on_placenode)

local function on_dignode(p, node)
	core.check_for_falling(p)
end
core.register_on_dignode(on_dignode)

local function on_punchnode(p, node)
	core.check_for_falling(p)
end
core.register_on_punchnode(on_punchnode)
