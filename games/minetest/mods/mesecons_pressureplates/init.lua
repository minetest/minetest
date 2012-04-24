-- PRESSURE PLATE WOOD

minetest.register_node("mesecons_pressureplates:pressure_plate_wood_off", {
	drawtype = "raillike",
	tile_images = {"jeija_pressure_plate_wood_off.png"},
	inventory_image = "jeija_pressure_plate_wood_off.png",
	wield_image = "jeija_pressure_plate_wood_off.png",
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
	},
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=3},
    	description="Wood Pressure Plate",
})

minetest.register_node("mesecons_pressureplates:pressure_plate_wood_on", {
	drawtype = "raillike",
	tile_images = {"jeija_pressure_plate_wood_on.png"},
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
	},
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=3},
	drop='"mesecons_pressureplates:pressure_plate_wood_off" 1',
})

minetest.register_craft({
	output = '"mesecons_pressureplates:pressure_plate_wood_off" 1',
	recipe = {
		{'"default:wood"', '"default:wood"'},
	}
})

minetest.register_abm(
	{nodenames = {"mesecons_pressureplates:pressure_plate_wood_off"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local objs = minetest.env:get_objects_inside_radius(pos, 1)
		for k, obj in pairs(objs) do
			local objpos=obj:getpos()
			if objpos.y>pos.y-1 and objpos.y<pos.y then
				minetest.env:add_node(pos, {name="mesecons_pressureplates:pressure_plate_wood_on"})
				mesecon:receptor_on(pos, mesecon:get_rules("pressureplate"))
			end
		end	
	end,
})

minetest.register_abm(
	{nodenames = {"mesecons_pressureplates:pressure_plate_wood_on"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local objs = minetest.env:get_objects_inside_radius(pos, 1)
		if objs[1]==nil then
			minetest.env:add_node(pos, {name="mesecons_pressureplates:pressure_plate_wood_off"})
			mesecon:receptor_off(pos, mesecon:get_rules("pressureplate"))
		end
	end,
})

minetest.register_on_dignode(
	function(pos, oldnode, digger)
		if oldnode.name == "mesecons_pressureplates:pressure_plate_wood_on" then
			mesecon:receptor_off(pos, mesecon:get_rules("pressureplate"))
		end	
	end
)

mesecon:add_receptor_node("mesecons_pressureplates:pressure_plate_wood_on")
mesecon:add_receptor_node_off("mesecons_pressureplates:pressure_plate_wood_off")

-- PRESSURE PLATE STONE

minetest.register_node("mesecons_pressureplates:pressure_plate_stone_off", {
	drawtype = "raillike",
	tile_images = {"jeija_pressure_plate_stone_off.png"},
	inventory_image = "jeija_pressure_plate_stone_off.png",
	wield_image = "jeija_pressure_plate_stone_off.png",
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
	},
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=3},
    	description="Stone Pressure Plate",
})

minetest.register_node("mesecons_pressureplates:pressure_plate_stone_on", {
	drawtype = "raillike",
	tile_images = {"jeija_pressure_plate_stone_on.png"},
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
	},
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=3},
	drop='"mesecons_pressureplates:pressure_plate_stone_off" 1',
})

minetest.register_craft({
	output = '"mesecons_pressureplates:pressure_plate_stone_off" 1',
	recipe = {
		{'"default:cobble"', '"default:cobble"'},
	}
})

minetest.register_abm(
	{nodenames = {"mesecons_pressureplates:pressure_plate_stone_off"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local objs = minetest.env:get_objects_inside_radius(pos, 1)
		for k, obj in pairs(objs) do
			local objpos=obj:getpos()
			if objpos.y>pos.y-1 and objpos.y<pos.y then
				minetest.env:add_node(pos, {name="mesecons_pressureplates:pressure_plate_stone_on"})
				mesecon:receptor_on(pos, mesecon:get_rules("pressureplate"))
			end
		end	
	end,
})

minetest.register_abm(
	{nodenames = {"mesecons_pressureplates:pressure_plate_stone_on"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local objs = minetest.env:get_objects_inside_radius(pos, 1)
		if objs[1]==nil then
			minetest.env:add_node(pos, {name="mesecons_pressureplates:pressure_plate_stone_off"})
			mesecon:receptor_off(pos, mesecon:get_rules("pressureplate"))
		end
	end,
})

minetest.register_on_dignode(
	function(pos, oldnode, digger)
		if oldnode.name == "mesecons_pressureplates:pressure_plate_stone_on" then
			mesecon:receptor_off(pos, mesecon:get_rules("pressureplate"))
		end	
	end
)

mesecon:add_receptor_node("mesecons_pressureplates:pressure_plate_stone_on")
mesecon:add_receptor_node_off("mesecons_pressureplates:pressure_plate_stone_off")

mesecon:add_rules("pressureplate", 
{{x=0,  y=1,  z=-1},
{x=0,  y=0,  z=-1},
{x=0,  y=-1, z=-1},
{x=0,  y=1,  z=1},
{x=0,  y=-1, z=1},
{x=0,  y=0,  z=1},
{x=1,  y=0,  z=0},
{x=1,  y=1,  z=0},
{x=1,  y=-1, z=0},
{x=-1, y=1,  z=0},
{x=-1, y=-1, z=0},
{x=-1, y=0,  z=0},
{x=0, y=-1,  z=0},
{x=0, y=-2,  z=0},
{x=0, y=1,  z=0}})
