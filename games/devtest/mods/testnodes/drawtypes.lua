--[[ Drawtype Test: This file tests out and provides examples for
all drawtypes in Minetest. It is attempted to keep the node
definitions as simple and minimal as possible to keep
side-effects to a minimum.

How to read the node definitions:
There are two parts which are separated by 2 newlines:
The first part contains the things that are more or less essential
for defining the drawtype (except description, which is
at the top for readability).
The second part (after the 2 newlines) contains stuff that are
unrelated to the drawtype, stuff that is mostly there to make
testing this node easier and more convenient.
]]

local S = minetest.get_translator("testnodes")

-- A regular cube
minetest.register_node("testnodes:normal", {
	description = S("Normal Drawtype Test Node"),
	drawtype = "normal",
	tiles = { "testnodes_normal.png" },

	groups = { dig_immediate = 3 },
})

-- Standard glasslike node
minetest.register_node("testnodes:glasslike", {
	description = S("Glasslike Drawtype Test Node"),
	drawtype = "glasslike",
	paramtype = "light",
	tiles = { "testnodes_glasslike.png" },

	groups = { dig_immediate = 3 },
})

-- Glasslike framed with the two textures (normal and "detail")
minetest.register_node("testnodes:glasslike_framed", {
	description = S("Glasslike Framed Drawtype Test Node"),
	drawtype = "glasslike_framed",
	paramtype = "light",
	tiles = {
		"testnodes_glasslike_framed.png",
		"testnodes_glasslike_detail.png",
	},


	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

-- Like the one above, but without the "detail" texture (texture 2).
-- This node was added to see how the engine behaves when the "detail" texture
-- is missing.
minetest.register_node("testnodes:glasslike_framed_no_detail", {
	description = S("Glasslike Framed without Detail Drawtype Test Node"),
	drawtype = "glasslike_framed",
	paramtype = "light",
	tiles = { "testnodes_glasslike_framed2.png" },


	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})


minetest.register_node("testnodes:glasslike_framed_optional", {
	description = S("Glasslike Framed Optional Drawtype Test Node"),
	drawtype = "glasslike_framed_optional",
	paramtype = "light",
	tiles = {
		"testnodes_glasslike_framed_optional.png",
		"testnodes_glasslike_detail.png",
	},


	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})



minetest.register_node("testnodes:allfaces", {
	description = S("Allfaces Drawtype Test Node"),
	drawtype = "allfaces",
	paramtype = "light",
	tiles = { "testnodes_allfaces.png" },

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:allfaces_optional", {
	description = S("Allfaces Optional Drawtype Test Node"),
	drawtype = "allfaces_optional",
	paramtype = "light",
	tiles = { "testnodes_allfaces_optional.png" },

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:allfaces_optional_waving", {
	description = S("Waving Allfaces Optional Drawtype Test Node"),
	drawtype = "allfaces_optional",
	paramtype = "light",
	tiles = { "testnodes_allfaces_optional.png^[brighten" },
	waving = 2,

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:firelike", {
	description = S("Firelike Drawtype Test Node"),
	drawtype = "firelike",
	paramtype = "light",
	tiles = { "testnodes_firelike.png" },


	walkable = false,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:fencelike", {
	description = S("Fencelike Drawtype Test Node"),
	drawtype = "fencelike",
	paramtype = "light",
	tiles = { "testnodes_fencelike.png" },

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:torchlike", {
	description = S("Floor Torchlike Drawtype Test Node"),
	drawtype = "torchlike",
	paramtype = "light",
	tiles = { "testnodes_torchlike_floor.png^[colorize:#FF0000:64" },


	walkable = false,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:torchlike_wallmounted", {
	description = S("Wallmounted Torchlike Drawtype Test Node"),
	drawtype = "torchlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	tiles = {
		"testnodes_torchlike_floor.png",
		"testnodes_torchlike_ceiling.png",
		"testnodes_torchlike_wall.png",
	},


	walkable = false,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:signlike", {
	description = S("Floor Signlike Drawtype Test Node"),
	drawtype = "signlike",
	paramtype = "light",
	tiles = { "testnodes_signlike.png^[colorize:#FF0000:64" },


	walkable = false,
	groups = { dig_immediate = 3 },
	sunlight_propagates = true,
})


minetest.register_node("testnodes:signlike_wallmounted", {
	description = S("Wallmounted Signlike Drawtype Test Node"),
	drawtype = "signlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	tiles = { "testnodes_signlike.png" },


	walkable = false,
	groups = { dig_immediate = 3 },
	sunlight_propagates = true,
})

minetest.register_node("testnodes:plantlike", {
	description = S("Plantlike Drawtype Test Node"),
	drawtype = "plantlike",
	paramtype = "light",
	tiles = { "testnodes_plantlike.png" },


	walkable = false,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:plantlike_waving", {
	description = S("Waving Plantlike Drawtype Test Node"),
	drawtype = "plantlike",
	paramtype = "light",
	tiles = { "testnodes_plantlike_waving.png" },
	waving = 1,


	walkable = false,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:plantlike_wallmounted", {
	description = S("Wallmounted Plantlike Drawtype Test Node"),
	drawtype = "plantlike",
	paramtype = "light",
	paramtype2 = "wallmounted",
	tiles = { "testnodes_plantlike_wallmounted.png" },
	leveled = 1,


	walkable = false,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})


-- param2 will rotate
local function rotate_on_rightclick(pos, node, clicker)
	local def = minetest.registered_nodes[node.name]
	local aux1 = clicker:get_player_control().aux1

	local deg, deg_max
	local color, color_mult = 0, 0
	if def.paramtype2 == "degrotate" then
		deg = node.param2
		deg_max = 240
	elseif def.paramtype2 == "colordegrotate" then
		-- MSB [3x color, 5x rotation] LSB
		deg = node.param2 % 2^5
		deg_max = 24
		color_mult = 2^5
		color = math.floor(node.param2 / color_mult)
	end

	deg = (deg + (aux1 and 10 or 1)) % deg_max
	node.param2 = color * color_mult + deg
	minetest.swap_node(pos, node)
	minetest.chat_send_player(clicker:get_player_name(),
		"Rotation is now " .. deg .. " / " .. deg_max)
end

minetest.register_node("testnodes:plantlike_degrotate", {
	description = S("Degrotate Plantlike Drawtype Test Node"),
	drawtype = "plantlike",
	paramtype = "light",
	paramtype2 = "degrotate",
	tiles = { "testnodes_plantlike_degrotate.png" },

	on_rightclick = rotate_on_rightclick,
	place_param2 = 7,
	walkable = false,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:mesh_degrotate", {
	description = S("Degrotate Mesh Drawtype Test Node"),
	drawtype = "mesh",
	paramtype = "light",
	paramtype2 = "degrotate",
	mesh = "testnodes_ocorner.obj",
	tiles = { "testnodes_mesh_stripes2.png" },

	on_rightclick = rotate_on_rightclick,
	place_param2 = 10, -- 15°
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:mesh_colordegrotate", {
	description = S("Color Degrotate Mesh Drawtype Test Node"),
	drawtype = "mesh",
	paramtype = "light",
	paramtype2 = "colordegrotate",
	palette = "testnodes_palette_facedir.png",
	mesh = "testnodes_ocorner.obj",
	tiles = { "testnodes_mesh_stripes3.png" },

	on_rightclick = rotate_on_rightclick,
	-- color index 1, 1 step (=15°) rotated
	place_param2 = 1 * 2^5 + 1,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

-- param2 will change height
minetest.register_node("testnodes:plantlike_leveled", {
	description = S("Leveled Plantlike Drawtype Test Node"),
	drawtype = "plantlike",
	paramtype = "light",
	paramtype2 = "leveled",
	tiles = {
		{ name = "testnodes_plantlike_leveled.png", tileable_vertical = true },
	},


	-- We set a default param2 here only for convenience, to make the "plant" visible after placement
	place_param2 = 8,
	walkable = false,
	sunlight_propagates = true,
	groups = { dig_immediate = 3 },
})

-- param2 changes shape
minetest.register_node("testnodes:plantlike_meshoptions", {
	description = S("Meshoptions Plantlike Drawtype Test Node"),
	drawtype = "plantlike",
	paramtype = "light",
	paramtype2 = "meshoptions",
	tiles = { "testnodes_plantlike_meshoptions.png" },


	walkable = false,
	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:plantlike_rooted", {
	description = S("Rooted Plantlike Drawtype Test Node"),
	drawtype = "plantlike_rooted",
	paramtype = "light",
	tiles = { "testnodes_plantlike_rooted_base.png" },
	special_tiles = { "testnodes_plantlike_rooted.png" },

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:plantlike_rooted_wallmounted", {
	description = S("Wallmounted Rooted Plantlike Drawtype Test Node"),
	drawtype = "plantlike_rooted",
	paramtype = "light",
	paramtype2 = "wallmounted",
	tiles = {
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base_side_wallmounted.png" },
	special_tiles = { "testnodes_plantlike_rooted_wallmounted.png" },

	groups = { dig_immediate = 3 },
})

minetest.register_node("testnodes:plantlike_rooted_waving", {
	description = S("Waving Rooted Plantlike Drawtype Test Node"),
	drawtype = "plantlike_rooted",
	paramtype = "light",
	tiles = {
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base_side_waving.png",
	},
	special_tiles = { "testnodes_plantlike_rooted_waving.png" },
	waving = 1,

	groups = { dig_immediate = 3 },
})

-- param2 changes height
minetest.register_node("testnodes:plantlike_rooted_leveled", {
	description = S("Leveled Rooted Plantlike Drawtype Test Node"),
	drawtype = "plantlike_rooted",
	paramtype = "light",
	paramtype2 = "leveled",
	tiles = {
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base_side_leveled.png",
	},
	special_tiles = {
		{ name = "testnodes_plantlike_rooted_leveled.png", tileable_vertical = true },
	},


	-- We set a default param2 here only for convenience, to make the "plant" visible after placement
	place_param2 = 8,
	groups = { dig_immediate = 3 },
})

-- param2 changes shape
minetest.register_node("testnodes:plantlike_rooted_meshoptions", {
	description = S("Meshoptions Rooted Plantlike Drawtype Test Node"),
	drawtype = "plantlike_rooted",
	paramtype = "light",
	paramtype2 = "meshoptions",
	tiles = {
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base_side_meshoptions.png",
	},
	special_tiles = {
		"testnodes_plantlike_rooted_meshoptions.png",
	},

	groups = { dig_immediate = 3 },
})

-- param2 changes rotation
minetest.register_node("testnodes:plantlike_rooted_degrotate", {
	description = S("Degrotate Rooted Plantlike Drawtype Test Node"),
	drawtype = "plantlike_rooted",
	paramtype = "light",
	paramtype2 = "degrotate",
	tiles = {
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base.png",
		"testnodes_plantlike_rooted_base_side_degrotate.png",
	},
	special_tiles = {
		"testnodes_plantlike_rooted_degrotate.png",
	},

	groups = { dig_immediate = 3 },
})

-- Demonstrative liquid nodes, source and flowing form.
-- DRAWTYPE ONLY, NO LIQUID PHYSICS!
-- Liquid ranges 0 to 8
for r = 0, 8 do
	minetest.register_node("testnodes:liquid_"..r, {
		description = S("Source Liquid Drawtype Test Node, Range @1", r),
		drawtype = "liquid",
		paramtype = "light",
		tiles = {
			"testnodes_liquidsource_r"..r..".png^[colorize:#FFFFFF:100",
		},
		special_tiles = {
			{name="testnodes_liquidsource_r"..r..".png^[colorize:#FFFFFF:100", backface_culling=false},
			{name="testnodes_liquidsource_r"..r..".png^[colorize:#FFFFFF:100", backface_culling=true},
		},
		use_texture_alpha = "blend",


		walkable = false,
		liquid_range = r,
		liquid_viscosity = 0,
		liquid_alternative_flowing = "testnodes:liquid_flowing_"..r,
		liquid_alternative_source = "testnodes:liquid_"..r,
		groups = { dig_immediate = 3 },
	})
	minetest.register_node("testnodes:liquid_flowing_"..r, {
		description = S("Flowing Liquid Drawtype Test Node, Range @1", r),
		drawtype = "flowingliquid",
		paramtype = "light",
		paramtype2 = "flowingliquid",
		tiles = {
			"testnodes_liquidflowing_r"..r..".png^[colorize:#FFFFFF:100",
		},
		special_tiles = {
			{name="testnodes_liquidflowing_r"..r..".png^[colorize:#FFFFFF:100", backface_culling=false},
			{name="testnodes_liquidflowing_r"..r..".png^[colorize:#FFFFFF:100", backface_culling=false},
		},
		use_texture_alpha = "blend",


		walkable = false,
		liquid_range = r,
		liquid_viscosity = 0,
		liquid_alternative_flowing = "testnodes:liquid_flowing_"..r,
		liquid_alternative_source = "testnodes:liquid_"..r,
		groups = { dig_immediate = 3 },
	})

end

-- Waving liquid test (drawtype only)
minetest.register_node("testnodes:liquid_waving", {
	description = S("Waving Source Liquid Drawtype Test Node"),
	drawtype = "liquid",
	paramtype = "light",
	tiles = {
		"testnodes_liquidsource.png^[colorize:#0000FF:127",
	},
	special_tiles = {
		{name="testnodes_liquidsource.png^[colorize:#0000FF:127", backface_culling=false},
		{name="testnodes_liquidsource.png^[colorize:#0000FF:127", backface_culling=true},
	},
	use_texture_alpha = "blend",
	waving = 3,


	walkable = false,
	liquid_range = 1,
	liquid_viscosity = 0,
	liquid_alternative_flowing = "testnodes:liquid_flowing_waving",
	liquid_alternative_source = "testnodes:liquid_waving",
	groups = { dig_immediate = 3 },
})
minetest.register_node("testnodes:liquid_flowing_waving", {
	description = S("Waving Flowing Liquid Drawtype Test Node"),
	drawtype = "flowingliquid",
	paramtype = "light",
	paramtype2 = "flowingliquid",
	tiles = {
		"testnodes_liquidflowing.png^[colorize:#0000FF:127",
	},
	special_tiles = {
		{name="testnodes_liquidflowing.png^[colorize:#0000FF:127", backface_culling=false},
		{name="testnodes_liquidflowing.png^[colorize:#0000FF:127", backface_culling=false},
	},
	use_texture_alpha = "blend",
	waving = 3,


	walkable = false,
	liquid_range = 1,
	liquid_viscosity = 0,
	liquid_alternative_flowing = "testnodes:liquid_flowing_waving",
	liquid_alternative_source = "testnodes:liquid_waving",
	groups = { dig_immediate = 3 },
})

-- Invisible node
minetest.register_node("testnodes:airlike", {
	description = S("Airlike Drawtype Test Node"),
	drawtype = "airlike",
	paramtype = "light",


	walkable = false,
	groups = { dig_immediate = 3 },
	sunlight_propagates = true,
})

-- param2 changes liquid height
minetest.register_node("testnodes:glassliquid", {
	description = S("Glasslike Liquid Level Drawtype Test Node"),
	drawtype = "glasslike_framed",
	paramtype = "light",
	paramtype2 = "glasslikeliquidlevel",
	tiles = {
		"testnodes_glasslikeliquid.png",
	},
	special_tiles = {
		"testnodes_liquid.png",
	},

	groups = { dig_immediate = 3 },
})

-- Adding many raillike examples, primarily to demonstrate the behavior of
-- "raillike groups". Nodes of the same type (rail, groupless, line, street)
-- should connect to nodes of the same "rail type" (=same shape, different
-- color) only.
local rails = {
	{ "rail", {"testnodes_rail_straight.png", "testnodes_rail_curved.png", "testnodes_rail_t_junction.png", "testnodes_rail_crossing.png"} },
	{ "line", {"testnodes_line_straight.png", "testnodes_line_curved.png", "testnodes_line_t_junction.png", "testnodes_line_crossing.png"}, },
	{ "street", {"testnodes_street_straight.png", "testnodes_street_curved.png", "testnodes_street_t_junction.png", "testnodes_street_crossing.png"}, },
	-- the "groupless" nodes are nodes in which the "connect_to_raillike" group is not set
	{ "groupless", {"testnodes_rail2_straight.png", "testnodes_rail2_curved.png", "testnodes_rail2_t_junction.png", "testnodes_rail2_crossing.png"} },
}
local colors = { "", "cyan", "red" }

for r=1, #rails do
	local id = rails[r][1]
	local tiles = rails[r][2]
	local raillike_group
	if id ~= "groupless" then
		raillike_group = minetest.raillike_group(id)
	end
	for c=1, #colors do
		local color
		if colors[c] ~= "" then
			color = colors[c]
		end
		minetest.register_node("testnodes:raillike_"..id..c, {
			description = S("Raillike Drawtype Test Node: @1 @2", id, c),
			drawtype = "raillike",
			paramtype = "light",
			tiles = tiles,
			groups = { connect_to_raillike = raillike_group, dig_immediate = 3 },


			color = color,
			selection_box = {
				type = "fixed",
				fixed = {{-0.5,  -0.5,  -0.5, 0.5, -0.4, 0.5}},
			},
			sunlight_propagates = true,
			walkable = false,
		})
	end
end



-- Add visual_scale variants of previous nodes for half and double size
local scale = function(subname, desc_double, desc_half)
	local original = "testnodes:"..subname
	local def = table.copy(minetest.registered_items[original])
	def.visual_scale = 2.0
	def.description = desc_double
	minetest.register_node("testnodes:"..subname.."_double", def)
	def = table.copy(minetest.registered_items[original])
	def.visual_scale = 0.5
	def.description = desc_half
	minetest.register_node("testnodes:"..subname.."_half", def)
end

scale("allfaces",
	S("Double-sized Allfaces Drawtype Test Node"),
	S("Half-sized Allfaces Drawtype Test Node"))
scale("allfaces_optional",
	S("Double-sized Allfaces Optional Drawtype Test Node"),
	S("Half-sized Allfaces Optional Drawtype Test Node"))
scale("allfaces_optional_waving",
	S("Double-sized Waving Allfaces Optional Drawtype Test Node"),
	S("Half-sized Waving Allfaces Optional Drawtype Test Node"))
scale("plantlike",
	S("Double-sized Plantlike Drawtype Test Node"),
	S("Half-sized Plantlike Drawtype Test Node"))
scale("plantlike_wallmounted",
	S("Double-sized Wallmounted Plantlike Drawtype Test Node"),
	S("Half-sized Wallmounted Plantlike Drawtype Test Node"))
scale("torchlike_wallmounted",
	S("Double-sized Wallmounted Torchlike Drawtype Test Node"),
	S("Half-sized Wallmounted Torchlike Drawtype Test Node"))
scale("signlike_wallmounted",
	S("Double-sized Wallmounted Signlike Drawtype Test Node"),
	S("Half-sized Wallmounted Signlike Drawtype Test Node"))
scale("firelike",
	S("Double-sized Firelike Drawtype Test Node"),
	S("Half-sized Firelike Drawtype Test Node"))
