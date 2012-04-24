--SHORT RANGE DETECTORS
minetest.register_node("mesecons_detector:object_detector_off", {
	tile_images = {"default_steel_block.png", "default_steel_block.png", "jeija_object_detector_off.png", "jeija_object_detector_off.png", "jeija_object_detector_off.png", "jeija_object_detector_off.png"},
	paramtype = "light",
	walkable = true,
	groups = {cracky=3},
	description="Player Detector",
})

minetest.register_node("mesecons_detector:object_detector_on", {
	tile_images = {"default_steel_block.png", "default_steel_block.png", "jeija_object_detector_on.png", "jeija_object_detector_on.png", "jeija_object_detector_on.png", "jeija_object_detector_on.png"},
	paramtype = "light",
	walkable = true,
	groups = {cracky=3},
	drop = '"mesecons_detector:object_detector_off" 1',
	description="Player Detector",
})

minetest.register_craft({
	output = '"mesecons_detector:object_detector_off" 1',
	recipe = {
		{"default:steelblock", '', "default:steelblock"},
		{"default:steelblock", "mesecons_materials:ic", "default:steelblock"},
		{"default:steelblock", "mesecons:mesecon_off", "default:steelblock"},
	}
})

minetest.register_abm(
	{nodenames = {"mesecons_detector:object_detector_off"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local objs = minetest.env:get_objects_inside_radius(pos, 6)
		for k, obj in pairs(objs) do
			if obj:get_entity_name()~="mesecons_pistons:piston_pusher_sticky" and obj:get_entity_name()~="mesecons_pistons:piston_pusher_normal" and obj:get_player_name()~=nil then -- Detected object is not piston pusher - will be changed if every entity has a type (like entity_type=mob)
				if minetest.env:get_node({x=pos.x, y=pos.y-1, z=pos.z}).name=="default:sign_wall" then
					if obj:get_player_name()~=minetest.env:get_meta({x=pos.x, y=pos.y-1, z=pos.z}):get_text() then
						return
					end
				end
				local objpos=obj:getpos()
				minetest.env:add_node(pos, {name="mesecons_detector:object_detector_on"})
				mesecon:receptor_on(pos, mesecon:get_rules("pressureplate"))
			end
		end	
	end,
})

minetest.register_abm(
	{nodenames = {"mesecons_detector:object_detector_on"},
	interval = 1.0,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local objs = minetest.env:get_objects_inside_radius(pos, 6)
		local objectfound=0
		for k, obj in pairs(objs) do
			if obj:get_entity_name()~="mesecons_pistons:piston_pusher_sticky" and obj:get_entity_name()~="mesecons_pistons:piston_pusher_normal" and obj~=nil 
			and obj:get_player_name()~=nil then
				if minetest.env:get_node({x=pos.x, y=pos.y-1, z=pos.z}).name=="default:sign_wall" then
					if minetest.env:get_meta({x=pos.x, y=pos.y-1, z=pos.z}):get_text() == obj:get_player_name() then
						objectfound=objectfound+1
					end
				else
-- Detected object is not piston pusher - will be changed if every entity has a type (like entity_type=mob)
					objectfound=objectfound + 1
				end
			end
		end	
		if objectfound==0 then
			minetest.env:add_node(pos, {name="mesecons_detector:object_detector_off"})
			mesecon:receptor_off(pos, mesecon:get_rules("pressureplate"))
		end
	end,
})

minetest.register_on_dignode(
	function(pos, oldnode, digger)
		if oldnode.name == "mesecons_detector:object_detector_on" then
			mesecon:receptor_off(pos, mesecon:get_rules("pressureplate"))
		end	
	end
)

mesecon:add_receptor_node("mesecons_detector:object_detector_on")
mesecon:add_receptor_node_off("mesecons_detector:object_detector_off")
