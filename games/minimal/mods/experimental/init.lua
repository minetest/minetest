--
-- Experimental things
--

-- For testing random stuff

experimental = {}

function experimental.print_to_everything(msg)
	minetest.log("action", msg)
	minetest.chat_send_all(msg)
end

--[[
experimental.player_visual_index = 0
function switch_player_visual()
	for _, obj in pairs(minetest.get_connected_players()) do
		if experimental.player_visual_index == 0 then
			obj:set_properties({visual="upright_sprite"})
		else
			obj:set_properties({visual="cube"})
		end
	end
	experimental.player_visual_index = (experimental.player_visual_index + 1) % 2
	minetest.after(1.0, switch_player_visual)
end
minetest.after(1.0, switch_player_visual)
]]

minetest.register_node("experimental:soundblock", {
	tile_images = {"unknown_node.png", "default_tnt_bottom.png",
			"default_tnt_side.png", "default_tnt_side.png",
			"default_tnt_side.png", "default_tnt_side.png"},
	inventory_image = minetest.inventorycube("unknown_node.png",
			"default_tnt_side.png", "default_tnt_side.png"),
	groups = {dig_immediate=3},
})

minetest.register_alias("sb", "experimental:soundblock")

minetest.register_abm({
	nodenames = {"experimental:soundblock"},
	interval = 1,
	chance = 1,
	action = function(p0, node, _, _)
		minetest.sound_play("default_grass_footstep", {pos=p0, gain=0.5})
	end,
})

--[[
stepsound = -1
function test_sound()
	print("test_sound")
	stepsound = minetest.sound_play("default_grass_footstep", {gain=1.0})
	minetest.after(2.0, test_sound)
	--minetest.after(0.1, test_sound_stop)
end
function test_sound_stop()
	print("test_sound_stop")
	minetest.sound_stop(stepsound)
	minetest.after(2.0, test_sound)
end
test_sound()
--]]

function on_step(dtime)
	-- print("experimental on_step")
	--[[
	objs = minetest.get_objects_inside_radius({x=0,y=0,z=0}, 10)
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
		minetest.log("time of day is "..minetest.get_timeofday())
		if experimental.day then
			minetest.log("forcing day->night")
			experimental.day = false
			minetest.set_timeofday(0.0)
		else
			minetest.log("forcing night->day")
			experimental.day = true
			minetest.set_timeofday(0.5)
		end
		minetest.log("time of day is "..minetest.get_timeofday())
	end
	--]]
end
minetest.register_globalstep(on_step)

--
-- Random stuff
--

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
		minetest.remove_node(p)
		minetest.add_entity(p, "experimental:tnt")
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
	initial_properties = {
		hp_max = 20,
		physical = false,
		collisionbox = {-0.4,-0.4,-0.4, 0.4,0.4,0.4},
		visual = "sprite",
		visual_size = {x=1, y=1},
		textures = {"experimental_dummyball.png"},
		spritediv = {x=1, y=3},
		initial_sprite_basepos = {x=0, y=0},
	},

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
		count = tonumber(message:sub(#cmd+1)) or 1
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to spawn (give)")
			return true -- Handled chat message
		end
		if not minetest.get_player_privs(name)["interact"] then
			minetest.chat_send_player(name, "you don't have permission to interact")
			return true -- Handled chat message
		end
		if count >= 2 and not minetest.get_player_privs(name)["server"] then
			minetest.chat_send_player(name, "you don't have " ..
					"permission to spawn multiple " ..
					"dummyballs (server)")
			return true -- Handled chat message
		end
		local player = minetest.get_player_by_name(name)
		if player == nil then
			print("Unable to spawn entity, player is nil")
			return true -- Handled chat message
		end
		local entityname = "experimental:dummyball"
		local p = player:getpos()
		p.y = p.y + 1
		for i = 1,count do
			minetest.add_entity(p, entityname)
		end
		minetest.chat_send_player(name, '"'..entityname
				..'" spawned '..tostring(count)..' time(s).');
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
	--minetest.add_node(cp, {name="sand"})
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
		minetest.add_node(pos, {name="papyrus"})
	end,
})]]

-- Replace texts of alls signs with "foo" every 10 seconds
--[[minetest.register_abm({
	nodenames = {"sign_wall"},
	interval = 10.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		print("ABM: Sign text changed")
		local meta = minetest.get_meta(pos)
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
        n = minetest.get_node(pos)
        print(dump(n))
        if n.name ~= "air" then
            return
        end

        pos.y = pos.y + 2
        ncpos = pos
        nctime = os.clock()
        minetest.add_node(ncpos, {name="nyancat"})
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
                minetest.add_node(ncold[#ncold], {name="air"})
                table.remove(ncold, #ncold)
            end
            minetest.add_node(p0, {name="nyancat_rainbow"})
            minetest.add_node(p1, {name="nyancat", param1=s0[4]})
            minetest.add_node(p2, {name="air"})
            ncpos = p1
        end
    end,
})--]]

minetest.register_node("experimental:tester_node_1", {
	description = "Tester Node 1 (construct/destruct/timer)",
	tile_images = {"wieldhand.png"},
	groups = {oddly_breakable_by_hand=2},
	sounds = default.node_sound_wood_defaults(),
	-- This was known to cause a bug in minetest.item_place_node() when used
	-- via minetest.place_node(), causing a placer with no position
  	paramtype2 = "facedir",

	on_construct = function(pos)
		experimental.print_to_everything("experimental:tester_node_1:on_construct("..minetest.pos_to_string(pos)..")")
		local meta = minetest.get_meta(pos)
		meta:set_string("mine", "test")
		local timer = minetest.get_node_timer(pos)
		timer:start(4, 3)
	end,

    after_place_node = function(pos, placer)
		experimental.print_to_everything("experimental:tester_node_1:after_place_node("..minetest.pos_to_string(pos)..")")
		local meta = minetest.get_meta(pos)
		if meta:get_string("mine") == "test" then
			experimental.print_to_everything("correct metadata found")
		else
			experimental.print_to_everything("incorrect metadata found")
		end
	end,
       
	on_destruct = function(pos)
		experimental.print_to_everything("experimental:tester_node_1:on_destruct("..minetest.pos_to_string(pos)..")")
	end,
 
	after_destruct = function(pos)
		experimental.print_to_everything("experimental:tester_node_1:after_destruct("..minetest.pos_to_string(pos)..")")
	end,
 
	after_dig_node = function(pos, oldnode, oldmetadata, digger)
		experimental.print_to_everything("experimental:tester_node_1:after_dig_node("..minetest.pos_to_string(pos)..")")
	end,

	on_timer = function(pos, elapsed)
		experimental.print_to_everything("on_timer(): elapsed="..dump(elapsed))
		return true
	end,
})

minetest.register_craftitem("experimental:tester_tool_1", {
	description = "Tester Tool 1",
	inventory_image = "experimental_tester_tool_1.png",
    on_use = function(itemstack, user, pointed_thing)
		--print(dump(pointed_thing))
		if pointed_thing.type == "node" then
			if minetest.get_node(pointed_thing.under).name == "experimental:tester_node_1" then
				local p = pointed_thing.under
				minetest.log("action", "Tester tool used at "..minetest.pos_to_string(p))
				minetest.dig_node(p)
			else
				local p = pointed_thing.above
				minetest.log("action", "Tester tool used at "..minetest.pos_to_string(p))
				minetest.place_node(p, {name="experimental:tester_node_1"})
			end
		end
	end,
})

minetest.register_craft({
	output = 'experimental:tester_tool_1',
	recipe = {
		{'group:crumbly'},
		{'group:crumbly'},
	}
})

--[[minetest.register_on_joinplayer(function(player)
	minetest.after(3, function()
		player:set_inventory_formspec("size[8,7.5]"..
			"image[1,0.6;1,2;player.png]"..
			"list[current_player;main;0,3.5;8,4;]"..
			"list[current_player;craft;3,0;3,3;]"..
			"list[current_player;craftpreview;7,1;1,1;]")
	end)
end)]]

-- Create a detached inventory
local inv = minetest.create_detached_inventory("test_inventory", {
	allow_move = function(inv, from_list, from_index, to_list, to_index, count, player)
		experimental.print_to_everything("allow move asked")
		return count -- Allow all
	end,
    allow_put = function(inv, listname, index, stack, player)
		experimental.print_to_everything("allow put asked")
		return 1 -- Allow only 1
	end,
    allow_take = function(inv, listname, index, stack, player)
		experimental.print_to_everything("allow take asked")
		return 4 -- Allow 4 at max
	end,
	on_move = function(inv, from_list, from_index, to_list, to_index, count, player)
		experimental.print_to_everything(player:get_player_name().." moved items")
	end,
    on_put = function(inv, listname, index, stack, player)
		experimental.print_to_everything(player:get_player_name().." put items")
	end,
    on_take = function(inv, listname, index, stack, player)
		experimental.print_to_everything(player:get_player_name().." took items")
	end,
})
inv:set_size("main", 4*6)
inv:add_item("main", "experimental:tester_tool_1")
inv:add_item("main", "experimental:tnt 5")

minetest.register_chatcommand("test1", {
	params = "",
	description = "Test 1: Modify player's inventory view",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return
		end
		player:set_inventory_formspec(
				"size[13,7.5]"..
				"image[6,0.6;1,2;player.png]"..
				"list[current_player;main;5,3.5;8,4;]"..
				"list[current_player;craft;8,0;3,3;]"..
				"list[current_player;craftpreview;12,1;1,1;]"..
				"list[detached:test_inventory;main;0,0;4,6;0]"..
				"button[0.5,7;2,1;button1;Button 1]"..
				"button_exit[2.5,7;2,1;button2;Exit Button]"
		)
		minetest.chat_send_player(name, "Done.");
	end,
})

minetest.register_on_player_receive_fields(function(player, formname, fields)
	experimental.print_to_everything("Inventory fields 1: player="..player:get_player_name()..", fields="..dump(fields))
end)
minetest.register_on_player_receive_fields(function(player, formname, fields)
	experimental.print_to_everything("Inventory fields 2: player="..player:get_player_name()..", fields="..dump(fields))
	return true -- Disable the first callback
end)
minetest.register_on_player_receive_fields(function(player, formname, fields)
	experimental.print_to_everything("Inventory fields 3: player="..player:get_player_name()..", fields="..dump(fields))
end)

minetest.log("experimental modname="..dump(minetest.get_current_modname()))
minetest.log("experimental modpath="..dump(minetest.get_modpath("experimental")))
minetest.log("experimental worldpath="..dump(minetest.get_worldpath()))

-- END
