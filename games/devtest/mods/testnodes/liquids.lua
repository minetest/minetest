-- Add liquids for ranges and viscosity levels 0-8

for d=0, 8 do
	minetest.register_node("testnodes:rliquid_"..d, {
		description = "Test Liquid Source, Range "..d,
		drawtype = "liquid",
		tiles = {"testnodes_liquidsource_r"..d..".png"},
		special_tiles = {
			{name = "testnodes_liquidsource_r"..d..".png", backface_culling = false},
			{name = "testnodes_liquidsource_r"..d..".png", backface_culling = true},
		},
		use_texture_alpha = "blend",
		paramtype = "light",
		walkable = false,
		buildable_to = true,
		is_ground_content = false,
		liquidtype = "source",
		liquid_alternative_flowing = "testnodes:rliquid_flowing_"..d,
		liquid_alternative_source = "testnodes:rliquid_"..d,
		liquid_range = d,
	})

	minetest.register_node("testnodes:rliquid_flowing_"..d, {
		description = "Flowing Test Liquid, Range "..d,
		drawtype = "flowingliquid",
		tiles = {"testnodes_liquidflowing_r"..d..".png"},
		special_tiles = {
			{name = "testnodes_liquidflowing_r"..d..".png", backface_culling = false},
			{name = "testnodes_liquidflowing_r"..d..".png", backface_culling = false},
		},
		use_texture_alpha = "blend",
		paramtype = "light",
		paramtype2 = "flowingliquid",
		walkable = false,
		buildable_to = true,
		is_ground_content = false,
		liquidtype = "flowing",
		liquid_alternative_flowing = "testnodes:rliquid_flowing_"..d,
		liquid_alternative_source = "testnodes:rliquid_"..d,
		liquid_range = d,
	})

	local mod = "^[colorize:#000000:127"
	minetest.register_node("testnodes:vliquid_"..d, {
		description = "Test Liquid Source, Viscosity "..d,
		drawtype = "liquid",
		tiles = {"testnodes_liquidsource_r"..d..".png"..mod},
		special_tiles = {
			{name = "testnodes_liquidsource_r"..d..".png"..mod, backface_culling = false},
			{name = "testnodes_liquidsource_r"..d..".png"..mod, backface_culling = true},
		},
		use_texture_alpha = "blend",
		paramtype = "light",
		walkable = false,
		buildable_to = true,
		is_ground_content = false,
		liquidtype = "source",
		liquid_alternative_flowing = "testnodes:vliquid_flowing_"..d,
		liquid_alternative_source = "testnodes:vliquid_"..d,
		liquid_viscosity = d,
	})

	minetest.register_node("testnodes:vliquid_flowing_"..d, {
		description = "Flowing Test Liquid, Viscosity "..d,
		drawtype = "flowingliquid",
		tiles = {"testnodes_liquidflowing_r"..d..".png"..mod},
		special_tiles = {
			{name = "testnodes_liquidflowing_r"..d..".png"..mod, backface_culling = false},
			{name = "testnodes_liquidflowing_r"..d..".png"..mod, backface_culling = false},
		},
		use_texture_alpha = "blend",
		paramtype = "light",
		paramtype2 = "flowingliquid",
		walkable = false,
		buildable_to = true,
		is_ground_content = false,
		liquidtype = "flowing",
		liquid_alternative_flowing = "testnodes:vliquid_flowing_"..d,
		liquid_alternative_source = "testnodes:vliquid_"..d,
		liquid_viscosity = d,
	})

end
