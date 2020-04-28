-- Test Nodes: Light test

local S = minetest.get_translator("testnodes")

-- All possible light levels
for i=1, minetest.LIGHT_MAX do
	minetest.register_node("testnodes:light"..i, {
		description = S("Light Source (@1)", i),
		paramtype = "light",
		light_source = i,


		tiles ={"testnodes_light_"..i..".png"},
		drawtype = "glasslike",
		walkable = false,
		sunlight_propagates = true,
		is_ground_content = false,
		groups = {dig_immediate=3},
	})
end

-- Lets light through, but not sunlight, leading to a
-- reduction in light level when light passes through
minetest.register_node("testnodes:sunlight_filter", {
	description = S("Sunlight Filter"),
	paramtype = "light",


	drawtype = "glasslike",
	tiles = {
		"testnodes_sunlight_filter.png",
	},
	groups = { dig_immediate = 3 },
})

-- Lets light and sunlight through without obstruction
minetest.register_node("testnodes:sunlight_propagator", {
	description = S("Sunlight Propagator"),
	paramtype = "light",
	sunlight_propagates = true,


	drawtype = "glasslike",
	tiles = {
		"testnodes_sunlight_filter.png^[brighten",
	},
	groups = { dig_immediate = 3 },
})
