-- HYDRO_TURBINE

minetest.register_node("mesecons_hydroturbine:hydro_turbine_off", {
	tile_images = {"jeija_hydro_turbine_off.png", "jeija_hydro_turbine_off.png", "jeija_hydro_turbine_off.png", "jeija_hydro_turbine_off.png", "jeija_hydro_turbine_off.png", "jeija_hydro_turbine_off.png"},
	groups = {dig_immediate=2},
    	description="Water Turbine",
})

minetest.register_node("mesecons_hydroturbine:hydro_turbine_on", {
	tile_images = {"jeija_hydro_turbine_on.png", "jeija_hydro_turbine_on.png", "jeija_hydro_turbine_on.png", "jeija_hydro_turbine_on.png", "jeija_hydro_turbine_on.png", "jeija_hydro_turbine_on.png"},
	drop = '"mesecons_hydroturbine:hydro_turbine_off" 1',
	groups = {dig_immediate=2},
    	description="Water Turbine",
})


minetest.register_abm({
nodenames = {"mesecons_hydroturbine:hydro_turbine_off"},
	interval = 1,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local waterpos={x=pos.x, y=pos.y+1, z=pos.z}
		if minetest.env:get_node(waterpos).name=="default:water_flowing" then
			--minetest.env:remove_node(pos)
			minetest.env:add_node(pos, {name="mesecons_hydroturbine:hydro_turbine_on"})
			nodeupdate(pos)
			mesecon:receptor_on(pos)
		end
	end,
})

minetest.register_abm({
nodenames = {"mesecons_hydroturbine:hydro_turbine_on"},
	interval = 1,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local waterpos={x=pos.x, y=pos.y+1, z=pos.z}
		if minetest.env:get_node(waterpos).name~="default:water_flowing" then
			--minetest.env:remove_node(pos)
			minetest.env:add_node(pos, {name="mesecons_hydroturbine:hydro_turbine_off"})
			nodeupdate(pos)
			mesecon:receptor_off(pos)
		end
	end,
})

mesecon:add_receptor_node("mesecons_hydroturbine:hydro_turbine_on")
mesecon:add_receptor_node_off("mesecons_hydroturbine:hydro_turbine_off")

minetest.register_craft({
	output = '"mesecons_hydroturbine:hydro_turbine_off" 2',
	recipe = {
	{'','"default:stick"', ''},
	{'"default:stick"', '"default:steel_ingot"', '"default:stick"'},
	{'','"default:stick"', ''},
	}
})

