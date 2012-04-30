-- MESELAMPS
minetest.register_node("mesecons_lamp:lamp_on", {
	drawtype = "torchlike",
	tile_images = {"jeija_meselamp_on_ceiling_on.png", "jeija_meselamp_on_floor_on.png", "jeija_meselamp_on.png"},
	inventory_image = "jeija_meselamp_on_floor_on.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	legacy_wallmounted = true,
	paramtype2 = "wallmounted",
	light_source = LIGHT_MAX,
	selection_box = {
		--type = "wallmounted",
		--type = "fixed",
		fixed = {-0.38, -0.5, -0.1, 0.38, -0.2, 0.1},
	},
	groups = {dig_immediate=3},
	drop='"mesecons_lamp:lamp_off" 1',
})

minetest.register_node("mesecons_lamp:lamp_off", {
	drawtype = "torchlike",
	tile_images = {"jeija_meselamp_on_ceiling_off.png", "jeija_meselamp_on_floor_off.png", "jeija_meselamp_off.png"},
	inventory_image = "jeija_meselamp_on_floor_off.png",
	wield_image = "jeija_meselamp_on_ceiling_off.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	wall_mounted = false,
	selection_box = {
		--type = "fixed",
		fixed = {-0.38, -0.5, -0.1, 0.38, -0.2, 0.1},
	},
	groups = {dig_immediate=3},
    	description="Meselamp",
})

minetest.register_craft({
	output = '"mesecons_lamp:lamp_off" 1',
	recipe = {
		{'', '"default:glass"', ''},
		{'"mesecons:mesecon_off"', '"default:steel_ingot"', '"mesecons:mesecon_off"'},
		{'', '"default:glass"', ''},
	}
})

mesecon:register_on_signal_on(function(pos, node)
	if node.name == "mesecons_lamp:lamp_off" then
		minetest.env:add_node(pos, {name="mesecons_lamp:lamp_on"})
		nodeupdate(pos)
	end
end)

mesecon:register_on_signal_off(function(pos, node)
	if node.name == "mesecons_lamp:lamp_on" then
		minetest.env:add_node(pos, {name="mesecons_lamp:lamp_off"})
		nodeupdate(pos)
	end
end)