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

	if d <= 7 then

	local mod = "^[colorize:#000000:127"
	minetest.register_node("testnodes:vliquid_"..d, {
		description = "Test Liquid Source, Viscosity/Resistance "..d,
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
		description = "Flowing Test Liquid, Viscosity/Resistance "..d,
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

	mod = "^[colorize:#000000:192"
	local v = 4
	minetest.register_node("testnodes:vrliquid_"..d, {
		description = "Test Liquid Source, Viscosity "..v..", Resistance "..d,
		drawtype = "liquid",
		tiles = {"testnodes_liquidsource_r"..d..".png"..mod},
		special_tiles = {
			{name = "testnodes_liquidsource_r"..d..".png"..mod, backface_culling = false},
			{name = "testnodes_liquidsource_r"..d..".png"..mod, backface_culling = true},
		},
		use_texture_alpha = "blend",
		paramtype = "light",
		walkable = false,
		pointable = false,
		diggable = false,
		buildable_to = true,
		is_ground_content = false,
		liquidtype = "source",
		liquid_alternative_flowing = "testnodes:vrliquid_flowing_"..d,
		liquid_alternative_source = "testnodes:vrliquid_"..d,
		liquid_viscosity = v,
		move_resistance = d,
	})

	minetest.register_node("testnodes:vrliquid_flowing_"..d, {
		description = "Flowing Test Liquid, Viscosity "..v..", Resistance "..d,
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
		pointable = false,
		diggable = false,
		buildable_to = true,
		is_ground_content = false,
		liquidtype = "flowing",
		liquid_alternative_flowing = "testnodes:vrliquid_flowing_"..d,
		liquid_alternative_source = "testnodes:vrliquid_"..d,
		liquid_viscosity = v,
		move_resistance = d,
	})

	end

end
