local function register_liquid(name, description, texture, props, sprops)
	local n_source = name .. "_source"
	local n_flowing = name .. "_flowing"
	local desc = {
		tiles = {
			{name = texture, backface_culling = false},
		},
		special_tiles = {
			{name = texture, backface_culling = false},
			{name = texture, backface_culling = true},
		},
		paramtype = "light",
		walkable = false,
		pointable = false,
		diggable = false,
		buildable_to = true,
		is_ground_content = false,
		drop = "",
		drowning = 1,
		liquid_alternative_flowing = n_flowing,
		liquid_alternative_source = n_source,
	}
	for k, v in pairs(props) do
		desc[k] = v
	end
	desc.description = description .. " (flowing)"
	desc.drawtype = "flowingliquid"
	desc.liquidtype = "flowing"
	minetest.register_node(n_flowing, desc)
	desc.description = description
	desc.drawtype = "liquid"
	desc.liquidtype = "source"
	minetest.register_node(n_source, desc)
end

register_liquid("land:water", "Water", "water.png", {
	use_texture_alpha = true,
	liquid_viscosity = 1,
	post_effect_color = {a = 64, r = 128, g = 128, b = 255},
})

register_liquid("land:river_water", "River water", "river_water.png", {
	alpha = 192,
	liquid_viscosity = 2,
	liquid_range = 4,
	post_effect_color = {a = 128, r = 128, g = 192, b = 255},
})

register_liquid("land:lava", "Lava", "lava.png", {
	damage_per_second = 6,
	light_source = 14,
	liquid_viscosity = 7,
	post_effect_color = {a = 192, r = 255, g = 128, b = 0},
})
