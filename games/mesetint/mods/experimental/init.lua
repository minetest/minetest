--
-- Experimental things
--

-- For testing random stuff

experimental = {}

timers_to_add = {}
timers = {}
minetest.register_globalstep(function(dtime)
  for indes, timer in ipairs(timers_to_add) do
    table.insert(timers, timer)
  end
  timers_to_add = {}
  for index, timer in ipairs(timers) do
    timer.time = timer.time - dtime
    if timer.time <= 0 then
      timer.func()
      timers[index] = nil
    end
  end
end)

after = function(time, func)
  table.insert(timers_to_add, {time=time, func=func})
end

--[[
stepsound = -1
function test_sound()
	print("test_sound")
	stepsound = minetest.sound_play("default_grass_footstep", {gain=1.0})
	after(2.0, test_sound)
	--after(0.1, test_sound_stop)
end
function test_sound_stop()
	print("test_sound_stop")
	minetest.sound_stop(stepsound)
	after(2.0, test_sound)
end
test_sound()
--]]

function on_step(dtime)
	-- print("experimental on_step")
	--[[
	objs = minetest.env:get_objects_inside_radius({x=0,y=0,z=0}, 10)
	for k, obj in pairs(objs) do
		name = obj:get_player_name()
		if name then
			print(name.." at "..dump(obj:getpos()))
			print(name.." dir: "..dump(obj:get_look_dir()))
			print(name.." pitch: "..dump(obj:get_look_pitch()))
			print(name.." yaw: "..dump(obj:get_look_yaw()))
		else
			print("Some object at "..dump(obj:getpos()))
		end
	end
	--]]
	--[[
	if experimental.t1 == nil then
		experimental.t1 = 0
	end
	experimental.t1 = experimental.t1 + dtime
	if experimental.t1 >= 2 then
		experimental.t1 = experimental.t1 - 2
		minetest.log("time of day is "..minetest.env:get_timeofday())
		if experimental.day then
			minetest.log("forcing day->night")
			experimental.day = false
			minetest.env:set_timeofday(0.0)
		else
			minetest.log("forcing night->day")
			experimental.day = true
			minetest.env:set_timeofday(0.5)
		end
		minetest.log("time of day is "..minetest.env:get_timeofday())
	end
	--]]
end
minetest.register_globalstep(on_step)

-- An example furnace-thing implemented in Lua

--[[
minetest.register_node("experimental:luafurnace", {
	tile_images = {"default_lava.png", "default_furnace_side.png",
		"default_furnace_side.png", "default_furnace_side.png",
		"default_furnace_side.png", "default_furnace_front.png"},
	--inventory_image = "furnace_front.png",
	inventory_image = minetest.inventorycube("default_furnace_front.png"),
	paramtype = "facedir_simple",
	metadata_name = "generic",
	material = minetest.digprop_stonelike(3.0),
})

minetest.register_on_placenode(function(pos, newnode, placer)
	if newnode.name == "experimental:luafurnace" then
		local meta = minetest.env:get_meta(pos)
		meta:inventory_set_list("fuel", {""})
		meta:inventory_set_list("src", {""})
		meta:inventory_set_list("dst", {"","","",""})
		meta:set_inventory_draw_spec(
			"invsize[8,9;]"
			.."list[current_name;fuel;2,3;1,1;]"
			.."list[current_name;src;2,1;1,1;]"
			.."list[current_name;dst;5,1;2,2;]"
			.."list[current_player;main;0,5;8,4;]"
		)
		
		local total_cooked = 0;
		meta:set_string("total_cooked", total_cooked)
		meta:set_infotext("Lua Furnace: total cooked: "..total_cooked)
	end
end)

minetest.register_abm({
	nodenames = {"experimental:luafurnace"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local meta = minetest.env:get_meta(pos)
		for i, name in ipairs({
				"fuel_totaltime",
				"fuel_time",
				"src_totaltime",
				"src_time"
		}) do
			if not meta:get_string(name) then
				meta:set_string(name, 0)
			end
		end

		local inv = meta:get_inventory()
		
		local fuelitem = inv:get_stack("fuel", 1):peek_item()
		local srcitem = inv:get_stack("src", 1):peek_item()
		--print("fuelitem="..dump(fuelitem))
		--print("srcitem="..dump(srcitem))
		
		local was_active = false

		local src_cooktime = -1
		local result_stackstring = nil
		
		if srcitem then
			local prop = get_item_definition(srcitem)
			if prop and prop.cookresult_itemstring ~= "" then
				result_stackstring = prop.cookresult_itemstring
				src_cooktime = prop.furnace_cooktime or 3
			end
		end

		print("src_cooktime="..dump(src_cooktime))
		print("result_stackstring="..dump(result_stackstring))

		if tonumber(meta:get_string("fuel_time")) < tonumber(meta:get_string("fuel_totaltime")) then
			was_active = true
			meta:set_string("fuel_time", tonumber(meta:get_string("fuel_time")) + 1)
			meta:set_string("src_time", tonumber(meta:get_string("src_time")) + 1)
			--print("result_stackstring="..dump(result_stackstring))
			--print('tonumber(meta:get_string("src_time"))='..dump(tonumber(meta:get_string("src_time"))))
			--print("src_cooktime="..dump(src_cooktime))
			if result_stackstring and tonumber(meta:get_string("src_time")) >= src_cooktime and src_cooktime >= 0 then
				-- Put result in "dst" list
				success = inv:autoinsert_stackstring("dst", result_stackstring)
				if not success then
					print("Could not autoinsert '"..result_stackstring.."'")
				end
				-- If succeeded, take stuff from "src" list
				if success then
					srcstack = inv:get_stack("src", 1)
					srcstack:take_item()
					inv:set_stack("src", 1, srcstack)
				end
				meta:set_string("src_time", 0)
			end
		end
		
		if tonumber(meta:get_string("fuel_time")) < tonumber(meta:get_string("fuel_totaltime")) then
			meta:set_infotext("Furnace active: "..(tonumber(meta:get_string("fuel_time"))/tonumber(meta:get_string("fuel_totaltime"))*100).."%")
			return
		end

		local srcitem = inv:get_stack("src", 1):peek_item()

		local src_cooktime = 0
		local result_stackstring = nil
		
		if srcitem then
			local prop = get_item_definition(srcitem)
			if prop and prop.cookresult_itemstring ~= "" then
				result_stackstring = prop.cookresult_itemstring
				src_cooktime = prop.furnace_cooktime or 3
			end
		end

		local fuelitem = inv:get_stack("fuel", 1):peek_item()

		if not result_stackstring or not fuelitem then
			if was_active then
				meta:set_infotext("Furnace is empty")
			end
			return
		end

		local burntime = -1
		if fuelitem then
			local prop = get_item_definition(fuelitem)
			if prop then
				burntime = prop.furnace_burntime or -1
			end
		end

		if burntime <= 0 then
			meta:set_infotext("Furnace out of fuel")
			return
		end

		meta:set_string("fuel_totaltime", burntime)
		meta:set_string("fuel_time", 0)
		
		local stack = inv:get_stack("fuel", 1)
		stack:take_item()
		inv:set_stack("fuel", 1, stack)
	end,
})
minetest.register_abm({
	nodenames = {"experimental:luafurnace"},
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
		if fuelitem.type == "node" then
			local prop = minetest.registered_nodes[fuelitem.name]
			if prop == nil then return end
			if prop.furnace_burntime < 0 then return end
		else
			return
		end
		local resultstack = nil
		if srcitem.type == "node" then
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
	output = 'node "experimental:luafurnace" 1',
	recipe = {
		{'node "default:cobble"', 'node "default:cobble"', 'node "default:cobble"'},
		{'node "default:cobble"', 'node "default:steel_ingot"', 'node "default:cobble"'},
		{'node "default:cobble"', 'node "default:cobble"', 'node "default:cobble"'},
	}
})
--]]

--
-- Random stuff
--

--[[
minetest.register_tool("experimental:horribletool", {
	image = "default_lava.png",
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

--
-- TNT (not functional)
--

minetest.register_craft({
	output = 'experimental:tnt',
	recipe = {
		{'default:wood'},
		{'default:coal_lump'},
		{'default:wood'}
	}
})

minetest.register_node("experimental:tnt", {
	tile_images = {"default_tnt_top.png", "default_tnt_bottom.png",
			"default_tnt_side.png", "default_tnt_side.png",
			"default_tnt_side.png", "default_tnt_side.png"},
	inventory_image = minetest.inventorycube("default_tnt_top.png",
			"default_tnt_side.png", "default_tnt_side.png"),
	drop = '', -- Get nothing
	material = {
		diggability = "not",
	},
})

minetest.register_on_punchnode(function(p, node)
	if node.name == "experimental:tnt" then
		minetest.env:remove_node(p)
		minetest.env:add_entity(p, "experimental:tnt")
		nodeupdate(p)
	end
end)

local TNT = {
	-- Static definition
	physical = true, -- Collides with things
	-- weight = 5,
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	textures = {"default_tnt_top.png", "default_tnt_bottom.png",
			"default_tnt_side.png", "default_tnt_side.png",
			"default_tnt_side.png", "default_tnt_side.png"},
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
	self.object:set_armor_groups({immortal=1})
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
		hitter:get_inventory():add_item("main", "experimental:tnt")
		--hitter:set_hp(hitter:get_hp() - 1)
	end
end

-- Called when object is right-clicked
function TNT:on_rightclick(clicker)
	--pos = self.object:getpos()
	--pos = {x=pos.x, y=pos.y+0.1, z=pos.z}
	--self.object:moveto(pos, false)
end

--print("TNT dump: "..dump(TNT))
--print("Registering TNT");
minetest.register_entity("experimental:tnt", TNT)

-- Add TNT's old name also
minetest.register_alias("TNT", "experimental:tnt")

--
-- The dummyball!
--

minetest.register_entity("experimental:dummyball", {
	-- Static definition
	hp_max = 20,
	physical = false,
	collisionbox = {-0.4,-0.4,-0.4, 0.4,0.4,0.4},
	visual = "sprite",
	visual_size = {x=1, y=1},
	textures = {"experimental_dummyball.png"},
	spritediv = {x=1, y=3},
	initial_sprite_basepos = {x=0, y=0},
	-- Dynamic variables
	phase = 0,
	phasetimer = 0,

	on_activate = function(self, staticdata)
		minetest.log("Dummyball activated!")
	end,

	on_step = function(self, dtime)
		self.phasetimer = self.phasetimer + dtime
		if self.phasetimer > 2.0 then
			self.phasetimer = self.phasetimer - 2.0
			self.phase = self.phase + 1
			if self.phase >= 3 then
				self.phase = 0
			end
			self.object:setsprite({x=0, y=self.phase})
			phasearmor = {
				[0]={cracky=3},
				[1]={crumbly=3},
				[2]={fleshy=3}
			}
			self.object:set_armor_groups(phasearmor[self.phase])
		end
	end,

	on_punch = function(self, hitter)
	end,
})

minetest.register_on_chat_message(function(name, message)
	local cmd = "/dummyball"
	if message:sub(0, #cmd) == cmd then
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to spawn (give)")
			return true -- Handled chat message
		end
		if not minetest.get_player_privs(name)["interact"] then
			minetest.chat_send_player(name, "you don't have permission to interact")
			return true -- Handled chat message
		end
		local player = minetest.env:get_player_by_name(name)
		if player == nil then
			print("Unable to spawn entity, player is nil")
			return true -- Handled chat message
		end
		local entityname = "experimental:dummyball"
		local p = player:getpos()
		p.y = p.y + 1
		minetest.env:add_entity(p, entityname)
		minetest.chat_send_player(name, '"'..entityname
				..'" spawned.');
		return true -- Handled chat message
	end
end)

--
-- A test entity for testing animated and yaw-modulated sprites
--

minetest.register_entity("experimental:testentity", {
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
		hitter:add_to_inventory('craft testobject1 1')
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
--print("setting max_users = " .. dump(minetest.setting_get("max_users")))
--print("setting asdf = " .. dump(minetest.setting_get("asdf")))

minetest.register_on_chat_message(function(name, message)
	--[[print("on_chat_message: name="..dump(name).." message="..dump(message))
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
	end]]
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

minetest.log("experimental modname="..dump(minetest.get_current_modname()))
minetest.log("experimental modpath="..dump(minetest.get_modpath("experimental")))
minetest.log("experimental worldpath="..dump(minetest.get_worldpath()))

-- END
