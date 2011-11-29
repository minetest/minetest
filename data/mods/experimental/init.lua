--
-- Experimental things
--

-- An example furnace-thing implemented in Lua

minetest.register_node("luafurnace", {
	tile_images = {"lava.png", "furnace_side.png", "furnace_side.png",
		"furnace_side.png", "furnace_side.png", "furnace_front.png"},
	--inventory_image = "furnace_front.png",
	inventory_image = inventorycube("furnace_front.png"),
	paramtype = "facedir_simple",
	metadata_name = "generic",
	material = digprop_stonelike(3.0),
})

minetest.register_on_placenode(function(pos, newnode, placer)
	if newnode.name == "luafurnace" then
		print("get_meta");
		local meta = minetest.env:get_meta(pos)
		print("inventory_set_list");
		meta:inventory_set_list("fuel", {""})
		print("inventory_set_list");
		meta:inventory_set_list("src", {""})
		print("inventory_set_list");
		meta:inventory_set_list("dst", {"","","",""})
		print("set_inventory_draw_spec");
		meta:set_inventory_draw_spec(
			"invsize[8,9;]"
			.."list[current_name;fuel;2,3;1,1;]"
			.."list[current_name;src;2,1;1,1;]"
			.."list[current_name;dst;5,1;2,2;]"
			.."list[current_player;main;0,5;8,4;]"
		)
		
		local total_cooked = 0;
		print("set_string")
		meta:set_string("total_cooked", total_cooked)
		print("set_infotext");
		meta:set_infotext("Lua Furnace: total cooked: "..total_cooked)
	end
end)

function stackstring_take_item(stackstring)
	if stackstring == nil then
		return '', nil
	end
	local stacktype = nil
	stacktype = string.match(stackstring,
			'([%a%d]+Item[%a%d]*)')
	if stacktype == "NodeItem" or stacktype == "CraftItem" then
		local itemtype = nil
		local itemname = nil
		local itemcount = nil
		itemtype, itemname, itemcount = string.match(stackstring,
				'([%a%d]+Item[%a%d]*) "([^"]*)" (%d+)')
		itemcount = tonumber(itemcount)
		if itemcount == 0 then
			return '', nil
		elseif itemcount == 1 then
			return '', {type=itemtype, name=itemname}
		else
			return itemtype.." \""..itemname.."\" "..(itemcount-1),
					{type=itemtype, name=itemname}
		end
	elseif stacktype == "ToolItem" then
		local itemtype = nil
		local itemname = nil
		local itemwear = nil
		itemtype, itemname, itemwear = string.match(stackstring,
				'([%a%d]+Item[%a%d]*) "([^"]*)" (%d+)')
		itemwear = tonumber(itemwear)
		return '', {type=itemtype, name=itemname, wear=itemwear}
	end
end

function stackstring_put_item(stackstring, item)
	if item == nil then
		return stackstring, false
	end
	stackstring = stackstring or ''
	local stacktype = nil
	stacktype = string.match(stackstring,
			'([%a%d]+Item[%a%d]*)')
	stacktype = stacktype or ''
	if stacktype ~= '' and stacktype ~= item.type then
		return stackstring, false
	end
	if item.type == "NodeItem" or item.type == "CraftItem" then
		local itemtype = nil
		local itemname = nil
		local itemcount = nil
		itemtype, itemname, itemcount = string.match(stackstring,
				'([%a%d]+Item[%a%d]*) "([^"]*)" (%d+)')
		itemtype = itemtype or item.type
		itemname = itemname or item.name
		if itemcount == nil then
			itemcount = 0
		end
		itemcount = itemcount + 1
		return itemtype.." \""..itemname.."\" "..itemcount, true
	elseif item.type == "ToolItem" then
		if stacktype ~= nil then
			return stackstring, false
		end
		local itemtype = nil
		local itemname = nil
		local itemwear = nil
		itemtype, itemname, itemwear = string.match(stackstring,
				'([%a%d]+Item[%a%d]*) "([^"]*)" (%d+)')
		itemwear = tonumber(itemwear)
		return itemtype.." \""..itemname.."\" "..itemwear, true
	end
	return stackstring, false
end

function stackstring_put_stackstring(stackstring, src)
	while src ~= '' do
		--print("src="..dump(src))
		src, item = stackstring_take_item(src)
		--print("src="..dump(src).." item="..dump(item))
		local success
		stackstring, success = stackstring_put_item(stackstring, item)
		if not success then
			return stackstring, false
		end
	end
	return stackstring, true
end

function test_stack()
	local stack
	local item
	local success

	stack, item = stackstring_take_item('NodeItem "TNT" 3')
	assert(stack == 'NodeItem "TNT" 2')
	assert(item.type == 'NodeItem')
	assert(item.name == 'TNT')

	stack, item = stackstring_take_item('CraftItem "with spaces" 2')
	assert(stack == 'CraftItem "with spaces" 1')
	assert(item.type == 'CraftItem')
	assert(item.name == 'with spaces')

	stack, item = stackstring_take_item('CraftItem "with spaces" 1')
	assert(stack == '')
	assert(item.type == 'CraftItem')
	assert(item.name == 'with spaces')

	stack, item = stackstring_take_item('CraftItem "s8df2kj3" 0')
	assert(stack == '')
	assert(item == nil)

	stack, item = stackstring_take_item('ToolItem "With Spaces" 32487')
	assert(stack == '')
	assert(item.type == 'ToolItem')
	assert(item.name == 'With Spaces')
	assert(item.wear == 32487)

	stack, success = stackstring_put_item('NodeItem "With Spaces" 40',
			{type='NodeItem', name='With Spaces'})
	assert(stack == 'NodeItem "With Spaces" 41')
	assert(success == true)

	stack, success = stackstring_put_item('CraftItem "With Spaces" 40',
			{type='CraftItem', name='With Spaces'})
	assert(stack == 'CraftItem "With Spaces" 41')
	assert(success == true)

	stack, success = stackstring_put_item('ToolItem "With Spaces" 32487',
			{type='ToolItem', name='With Spaces'})
	assert(stack == 'ToolItem "With Spaces" 32487')
	assert(success == false)

	stack, success = stackstring_put_item('NodeItem "With Spaces" 40',
			{type='ToolItem', name='With Spaces'})
	assert(stack == 'NodeItem "With Spaces" 40')
	assert(success == false)
	
	assert(stackstring_put_stackstring('NodeItem "With Spaces" 2',
			'NodeItem "With Spaces" 1') == 'NodeItem "With Spaces" 3')
end
test_stack()

minetest.register_abm({
	nodenames = {"luafurnace"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local meta = minetest.env:get_meta(pos)
		local fuellist = meta:inventory_get_list("fuel")
		local srclist = meta:inventory_get_list("src")
		local dstlist = meta:inventory_get_list("dst")
		if fuellist == nil or srclist == nil or dstlist == nil then
			return
		end
		_, srcitem = stackstring_take_item(srclist[1])
		_, fuelitem = stackstring_take_item(fuellist[1])
		if not srcitem or not fuelitem then return end
		if fuelitem.type == "NodeItem" then
			local prop = minetest.registered_nodes[fuelitem.name]
			if prop == nil then return end
			if prop.furnace_burntime < 0 then return end
		else
			return
		end
		local resultstack = nil
		if srcitem.type == "NodeItem" then
			local prop = minetest.registered_nodes[srcitem.name]
			if prop == nil then return end
			if prop.cookresult_item == "" then return end
			resultstack = prop.cookresult_item
		else
			return
		end

		if resultstack == nil then
			return
		end

		dstlist[1], success = stackstring_put_stackstring(dstlist[1], resultstack)
		if not success then
			return
		end

		fuellist[1], _ = stackstring_take_item(fuellist[1])
		srclist[1], _ = stackstring_take_item(srclist[1])

		meta:inventory_set_list("fuel", fuellist)
		meta:inventory_set_list("src", srclist)
		meta:inventory_set_list("dst", dstlist)

		local total_cooked = meta:get_string("total_cooked")
		total_cooked = tonumber(total_cooked) + 1
		meta:set_string("total_cooked", total_cooked)
		meta:set_infotext("Lua Furnace: total cooked: "..total_cooked)
	end,
})

minetest.register_craft({
	output = 'NodeItem "luafurnace" 1',
	recipe = {
		{'NodeItem "cobble"', 'NodeItem "cobble"', 'NodeItem "cobble"'},
		{'NodeItem "cobble"', 'NodeItem "cobble"', 'NodeItem "cobble"'},
		{'NodeItem "cobble"', 'NodeItem "cobble"', 'NodeItem "cobble"'},
	}
})

--
-- Random stuff
--

--[[
minetest.register_tool("horribletool", {
	image = "lava.png",
	basetime = 2.0
	dt_weight = 0.2
	dt_crackiness = 0.2
	dt_crumbliness = 0.2
	dt_cuttability = 0.2
	basedurability = 50
	dd_weight = -5
	dd_crackiness = -5
	dd_crumbliness = -5
	dd_cuttability = -5
})
--]]

minetest.register_craft({
	output = 'NodeItem "somenode" 4',
	recipe = {
		{'CraftItem "Stick" 1'},
	}
})

minetest.register_node("somenode", {
	tile_images = {"lava.png", "mese.png", "stone.png", "grass.png", "cobble.png", "tree_top.png"},
	inventory_image = "treeprop.png",
	material = {
		diggability = "normal",
		weight = 0,
		crackiness = 0,
		crumbliness = 0,
		cuttability = 0,
		flammability = 0
	},
	metadata_name = "chest",
})

--
-- TNT (not functional)
--

minetest.register_craft({
	output = 'NodeItem "TNT" 4',
	recipe = {
		{'NodeItem "wood" 1'},
		{'CraftItem "lump_of_coal" 1'},
		{'NodeItem "wood" 1'}
	}
})

minetest.register_node("TNT", {
	tile_images = {"tnt_top.png", "tnt_bottom.png", "tnt_side.png", "tnt_side.png", "tnt_side.png", "tnt_side.png"},
	inventory_image = "tnt_side.png",
	dug_item = '', -- Get nothing
	material = {
		diggability = "not",
	},
})

local TNT = {
	-- Static definition
	physical = true, -- Collides with things
	-- weight = 5,
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	textures = {"tnt_top.png","tnt_bottom.png","tnt_side.png","tnt_side.png","tnt_side.png","tnt_side.png"},
	-- Initial value for our timer
	timer = 0,
	-- Number of punches required to defuse
	health = 1,
	blinktimer = 0,
	blinkstatus = true,
}

-- Called when a TNT object is created
function TNT:on_activate(staticdata)
	print("TNT:on_activate()")
	self.object:setvelocity({x=0, y=4, z=0})
	self.object:setacceleration({x=0, y=-10, z=0})
	self.object:settexturemod("^[brighten")
end

-- Called periodically
function TNT:on_step(dtime)
	--print("TNT:on_step()")
	self.timer = self.timer + dtime
	self.blinktimer = self.blinktimer + dtime
	if self.blinktimer > 0.5 then
		self.blinktimer = self.blinktimer - 0.5
		if self.blinkstatus then
			self.object:settexturemod("")
		else
			self.object:settexturemod("^[brighten")
		end
		self.blinkstatus = not self.blinkstatus
	end
end

-- Called when object is punched
function TNT:on_punch(hitter)
	print("TNT:on_punch()")
	self.health = self.health - 1
	if self.health <= 0 then
		self.object:remove()
		hitter:add_to_inventory("NodeItem TNT 1")
		hitter:set_hp(hitter:get_hp() - 1)
	end
end

-- Called when object is right-clicked
function TNT:on_rightclick(clicker)
	--pos = self.object:getpos()
	--pos = {x=pos.x, y=pos.y+0.1, z=pos.z}
	--self.object:moveto(pos, false)
end

print("TNT dump: "..dump(TNT))

print("Registering TNT");
minetest.register_entity("TNT", TNT)

--
-- A test entity for testing animated and yaw-modulated sprites
--

minetest.register_entity("testentity", {
	-- Static definition
	physical = true, -- Collides with things
	-- weight = 5,
	collisionbox = {-0.7,-1.35,-0.7, 0.7,1.0,0.7},
	--collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "sprite",
	visual_size = {x=2, y=3},
	textures = {"dungeon_master.png^[makealpha:128,0,0^[makealpha:128,128,0"},
	spritediv = {x=6, y=5},
	initial_sprite_basepos = {x=0, y=0},

	on_activate = function(self, staticdata)
		print("testentity.on_activate")
		self.object:setsprite({x=0,y=0}, 1, 0, true)
		--self.object:setsprite({x=0,y=0}, 4, 0.3, true)

		-- Set gravity
		self.object:setacceleration({x=0, y=-10, z=0})
		-- Jump a bit upwards
		self.object:setvelocity({x=0, y=10, z=0})
	end,

	on_punch = function(self, hitter)
		self.object:remove()
		hitter:add_to_inventory('CraftItem testobject1 1')
	end,
})

--
-- More random stuff
--

minetest.register_on_respawnplayer(function(player)
	print("on_respawnplayer")
	-- player:setpos({x=0, y=30, z=0})
	-- return true
end)

minetest.register_on_generated(function(minp, maxp)
	--print("on_generated: minp="..dump(minp).." maxp="..dump(maxp))
	--cp = {x=(minp.x+maxp.x)/2, y=(minp.y+maxp.y)/2, z=(minp.z+maxp.z)/2}
	--minetest.env:add_node(cp, {name="sand"})
end)

-- Example setting get
print("setting max_users = " .. dump(minetest.setting_get("max_users")))
print("setting asdf = " .. dump(minetest.setting_get("asdf")))

minetest.register_on_chat_message(function(name, message)
	print("on_chat_message: name="..dump(name).." message="..dump(message))
	local cmd = "/testcommand"
	if message:sub(0, #cmd) == cmd then
		print(cmd.." invoked")
		return true
	end
	local cmd = "/help"
	if message:sub(0, #cmd) == cmd then
		print("script-overridden help command")
		minetest.chat_send_all("script-overridden help command")
		return true
	end
end)

-- Grow papyrus on TNT every 10 seconds
--[[minetest.register_abm({
	nodenames = {"TNT"},
	interval = 10.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		print("TNT ABM action")
		pos.y = pos.y + 1
		minetest.env:add_node(pos, {name="papyrus"})
	end,
})]]

-- Replace texts of alls signs with "foo" every 10 seconds
--[[minetest.register_abm({
	nodenames = {"sign_wall"},
	interval = 10.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		print("ABM: Sign text changed")
		local meta = minetest.env:get_meta(pos)
		meta:set_text("foo")
	end,
})]]

--[[local ncpos = nil
local ncq = 1
local ncstuff = {
    {2, 1, 0, 3}, {3, 0, 1, 2}, {4, -1, 0, 1}, {5, -1, 0, 1}, {6, 0, -1, 0},
    {7, 0, -1, 0}, {8, 1, 0, 3}, {9, 1, 0, 3}, {10, 1, 0, 3}, {11, 0, 1, 2},
    {12, 0, 1, 2}, {13, 0, 1, 2}, {14, -1, 0, 1}, {15, -1, 0, 1}, {16, -1, 0, 1},
    {17, -1, 0, 1}, {18, 0, -1, 0}, {19, 0, -1, 0}, {20, 0, -1, 0}, {21, 0, -1, 0},
    {22, 1, 0, 3}, {23, 1, 0, 3}, {24, 1, 0, 3}, {25, 1, 0, 3}, {10, 0, 1, 2}
}
local ncold = {}
local nctime = nil

minetest.register_abm({
    nodenames = {"dirt_with_grass"},
    interval = 100000.0,
    chance = 1,
    action = function(pos, node, active_object_count, active_object_count_wider)
        if ncpos ~= nil then
            return
        end
       
        if pos.x % 16 ~= 8 or pos.z % 16 ~= 8 then
            return
        end
       
        pos.y = pos.y + 1
        n = minetest.env:get_node(pos)
        print(dump(n))
        if n.name ~= "air" then
            return
        end

        pos.y = pos.y + 2
        ncpos = pos
        nctime = os.clock()
        minetest.env:add_node(ncpos, {name="nyancat"})
    end
})

minetest.register_abm({
    nodenames = {"nyancat"},
    interval = 1.0,
    chance = 1,
    action = function(pos, node, active_object_count, active_object_count_wider)
        if ncpos == nil then
            return
        end
        if pos.x == ncpos.x and pos.y == ncpos.y and pos.z == ncpos.z then
            clock = os.clock()
            if clock - nctime < 0.1 then
                return
            end
            nctime = clock
           
            s0 = ncstuff[ncq]
            ncq = s0[1]
            s1 = ncstuff[ncq]
            p0 = pos
            p1 = {x = p0.x + s0[2], y = p0.y, z = p0.z + s0[3]}
            p2 = {x = p1.x + s1[2], y = p1.y, z = p1.z + s1[3]}
            table.insert(ncold, 1, p0)
            while #ncold >= 10 do
                minetest.env:add_node(ncold[#ncold], {name="air"})
                table.remove(ncold, #ncold)
            end
            minetest.env:add_node(p0, {name="nyancat_rainbow"})
            minetest.env:add_node(p1, {name="nyancat", param1=s0[4]})
            minetest.env:add_node(p2, {name="air"})
            ncpos = p1
        end
    end,
})--]]


