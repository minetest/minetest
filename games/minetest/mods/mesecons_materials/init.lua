--GLUE
minetest.register_craftitem("mesecons_materials:glue", {
	image = "jeija_glue.png",
	on_place_on_ground = minetest.craftitem_place_item,
    	description="Glue",
})

minetest.register_craft({
	output = '"mesecons_materials:glue" 2',
	recipe = {
		{'"default:junglegrass"', '"default:junglegrass"'},
		{'"default:junglegrass"', '"default:junglegrass"'},
	}
})

-- IC
minetest.register_craftitem("mesecons_materials:ic", {
	image = "jeija_ic.png",
	on_place_on_ground = minetest.craftitem_place_item,
    	description="IC",
})

minetest.register_craft({
	output = 'craft "mesecons_materials:ic" 2',
	recipe = {
		{'mesecons_materials:silicon', 'mesecons_materials:silicon', 'mesecons:mesecon_off'},
		{'mesecons_materials:silicon', 'mesecons_materials:silicon', 'mesecons:mesecon_off'},
		{'mesecons:mesecon_off', 'mesecons:mesecon_off', ''},
	}
})

-- Silicon
minetest.register_craftitem("mesecons_materials:silicon", {
	image = "jeija_silicon.png",
	on_place_on_ground = minetest.craftitem_place_item,
    	description="Silicon",
})

minetest.register_craft({
	output = '"mesecons_materials:silicon" 4',
	recipe = {
		{'"default:sand"', '"default:sand"'},
		{'"default:sand"', '"default:steel_ingot"'},
	}
})
