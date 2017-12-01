minetest.register_node("land:water_source", {
	description = "Water",
	use_texture_alpha = true,
	tiles = {"water.png"},
	groups = {},
})

function register_liquid(name, description, texture, props, sprops)
	local n_source = name .. "_source"
	local n_flowing = name .. "_flowing"
	desc = {
		tiles = {
			{name = texture, backface_culling = false},
		},
		special_tiles = {
			{name = texture, backface_culling = false},
			{name = texture, backface_culling = true},
		},
		use_texture_alpha = true,
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
	liquid_viscosity = 1,
	post_effect_color = {a = 64, r = 128, g = 128, b = 255},
})

register_liquid("land:river_water", "River water", "river_water.png", {
	liquid_viscosity = 2,
	liquid_range = 4,
	post_effect_color = {a = 128, r = 128, g = 192, b = 255},
})
