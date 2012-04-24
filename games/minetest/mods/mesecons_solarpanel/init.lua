-- Solar Panel
minetest.register_node("mesecons_solarpanel:solar_panel", {
	drawtype = "raillike",
	tile_images = {"jeija_solar_panel.png"},
	inventory_image = "jeija_solar_panel.png",
	wield_image = "jeija_solar_panel.png",
	paramtype = "light",
	walkable = false,
	is_ground_content = true,
	selection_box = {
		type = "fixed",
	},
	furnace_burntime = 5,
	groups = {snappy=2},
    	description="Solar Panel",
})

minetest.register_craft({
	output = '"mesecons_solarpanel:solar_panel" 1',
	recipe = {
		{'"mesecons_materials:silicon"', '"mesecons_materials:silicon"'},
		{'"mesecons_materials:silicon"', '"mesecons_materials:silicon"'},
	}
})

minetest.register_abm(
	{nodenames = {"mesecons_solarpanel:solar_panel"},
	interval = 0.1,
	chance = 1,
	action = function(pos, node, active_object_count, active_object_count_wider)
		local light = minetest.env:get_node_light(pos, nil)
		if light == nil then light = 0 end
		if light >= 12 then
			mesecon:receptor_on(pos)
		else
			mesecon:receptor_off(pos)
		end
	end,
})
