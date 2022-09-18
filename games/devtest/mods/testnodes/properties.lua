-- Test Nodes: Node property tests

local S = minetest.get_translator("testnodes")

-- Is supposed to fall when it doesn't rest on solid ground
minetest.register_node("testnodes:falling", {
	description = S("Falling Node"),
	tiles = {
		"testnodes_node.png",
		"testnodes_node.png",
		"testnodes_node_falling.png",
	},
	groups = { falling_node = 1, dig_immediate = 3 },
})

minetest.register_node("testnodes:falling_facedir", {
	description = S("Falling Facedir Node"),
	tiles = {
		"testnodes_1.png",
		"testnodes_2.png",
		"testnodes_3.png",
		"testnodes_4.png",
		"testnodes_5.png",
		"testnodes_6.png",
	},
	paramtype2 = "facedir",
	groups = { falling_node = 1, dig_immediate = 3 },
})

-- Same as falling node, but will stop falling on top of liquids
minetest.register_node("testnodes:falling_float", {
	description = S("Falling+Floating Node"),
	groups = { falling_node = 1, float = 1, dig_immediate = 3 },


	tiles = {
		"testnodes_node.png",
		"testnodes_node.png",
		"testnodes_node_falling.png",
	},
	color = "cyan",
})

-- This node attaches to the floor and drops as item
-- when the floor is gone.
minetest.register_node("testnodes:attached", {
	description = S("Floor-Attached Node"),
	tiles = {
		"testnodes_attached_top.png",
		"testnodes_attached_bottom.png",
		"testnodes_attached_side.png",
	},
	groups = { attached_node = 1, dig_immediate = 3 },
})

-- This node attaches to the side of a node and drops as item
-- when the node it attaches to is gone.
minetest.register_node("testnodes:attached_wallmounted", {
	description = S("Wallmounted Attached Node"),
	paramtype2 = "wallmounted",
	tiles = {
		"testnodes_attachedw_top.png",
		"testnodes_attachedw_bottom.png",
		"testnodes_attachedw_side.png",
	},
	groups = { attached_node = 1, dig_immediate = 3 },
})

-- Jump disabled
minetest.register_node("testnodes:nojump", {
	description = S("Non-jumping Node"),
	groups = {disable_jump=1, dig_immediate=3},
	tiles = {"testnodes_nojump_top.png", "testnodes_nojump_side.png"},
})

-- Jump disabled plant
minetest.register_node("testnodes:nojump_walkable", {
	description = S("Non-jumping Plant Node"),
	drawtype = "plantlike",
	groups = {disable_jump=1, dig_immediate=3},
	walkable = false,
	tiles = {"testnodes_nojump_top.png"},
})

-- Climbable up and down with jump and sneak keys
minetest.register_node("testnodes:climbable", {
	description = S("Climbable Node"),
	climbable = true,
	walkable = false,


	paramtype = "light",
	sunlight_propagates = true,
	is_ground_content = false,
	tiles ={"testnodes_climbable_side.png"},
	drawtype = "glasslike",
	groups = {dig_immediate=3},
})

-- Climbable only downwards with sneak key
minetest.register_node("testnodes:climbable_nojump", {
	description = S("Downwards-climbable Node"),
	climbable = true,
	walkable = false,

	groups = {disable_jump=1, dig_immediate=3},
	drawtype = "glasslike",
	tiles ={"testnodes_climbable_nojump_side.png"},
	paramtype = "light",
	sunlight_propagates = true,
})

-- A liquid in which you can't rise
minetest.register_node("testnodes:liquid_nojump", {
	description = S("Non-jumping Liquid Source Node"),
	liquidtype = "source",
	liquid_range = 1,
	liquid_viscosity = 0,
	liquid_alternative_flowing = "testnodes:liquidflowing_nojump",
	liquid_alternative_source = "testnodes:liquid_nojump",
	liquid_renewable = false,
	groups = {disable_jump=1, dig_immediate=3},
	walkable = false,

	drawtype = "liquid",
	tiles = {"testnodes_liquidsource.png^[colorize:#FF0000:127"},
	special_tiles = {
		{name = "testnodes_liquidsource.png^[colorize:#FF0000:127", backface_culling = false},
		{name = "testnodes_liquidsource.png^[colorize:#FF0000:127", backface_culling = true},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	pointable = false,
	liquids_pointable = true,
	buildable_to = true,
	is_ground_content = false,
	post_effect_color = {a = 70, r = 255, g = 0, b = 200},
})

-- A liquid in which you can't rise (flowing variant)
minetest.register_node("testnodes:liquidflowing_nojump", {
	description = S("Non-jumping Flowing Liquid Node"),
	liquidtype = "flowing",
	liquid_range = 1,
	liquid_viscosity = 0,
	liquid_alternative_flowing = "testnodes:liquidflowing_nojump",
	liquid_alternative_source = "testnodes:liquid_nojump",
	liquid_renewable = false,
	groups = {disable_jump=1, dig_immediate=3},
	walkable = false,


	drawtype = "flowingliquid",
	tiles = {"testnodes_liquidflowing.png^[colorize:#FF0000:127"},
	special_tiles = {
		{name = "testnodes_liquidflowing.png^[colorize:#FF0000:127", backface_culling = false},
		{name = "testnodes_liquidflowing.png^[colorize:#FF0000:127", backface_culling = false},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	paramtype2 = "flowingliquid",
	pointable = false,
	liquids_pointable = true,
	buildable_to = true,
	is_ground_content = false,
	post_effect_color = {a = 70, r = 255, g = 0, b = 200},
})

-- A liquid which doesn't have liquid movement physics (source variant)
minetest.register_node("testnodes:liquid_noswim", {
	description = S("No-swim Liquid Source Node"),
	liquidtype = "source",
	liquid_range = 1,
	liquid_viscosity = 0,
	liquid_alternative_flowing = "testnodes:liquidflowing_noswim",
	liquid_alternative_source = "testnodes:liquid_noswim",
	liquid_renewable = false,
	liquid_move_physics = false,
	groups = {dig_immediate=3},
	walkable = false,

	drawtype = "liquid",
	tiles = {"testnodes_liquidsource.png^[colorize:#FF00FF:127"},
	special_tiles = {
		{name = "testnodes_liquidsource.png^[colorize:#FF00FF:127", backface_culling = false},
		{name = "testnodes_liquidsource.png^[colorize:#FF00FF:127", backface_culling = true},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	pointable = false,
	liquids_pointable = true,
	buildable_to = true,
	is_ground_content = false,
	post_effect_color = {a = 70, r = 255, g = 200, b = 200},
})

-- A liquid which doen't have liquid movement physics (flowing variant)
minetest.register_node("testnodes:liquidflowing_noswim", {
	description = S("No-swim Flowing Liquid Node"),
	liquidtype = "flowing",
	liquid_range = 1,
	liquid_viscosity = 0,
	liquid_alternative_flowing = "testnodes:liquidflowing_noswim",
	liquid_alternative_source = "testnodes:liquid_noswim",
	liquid_renewable = false,
	liquid_move_physics = false,
	groups = {dig_immediate=3},
	walkable = false,


	drawtype = "flowingliquid",
	tiles = {"testnodes_liquidflowing.png^[colorize:#FF00FF:127"},
	special_tiles = {
		{name = "testnodes_liquidflowing.png^[colorize:#FF00FF:127", backface_culling = false},
		{name = "testnodes_liquidflowing.png^[colorize:#FF00FF:127", backface_culling = false},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	paramtype2 = "flowingliquid",
	pointable = false,
	liquids_pointable = true,
	buildable_to = true,
	is_ground_content = false,
	post_effect_color = {a = 70, r = 255, g = 200, b = 200},
})



-- Nodes that modify fall damage (various damage modifiers)
for i=-100, 100, 25 do
	if i ~= 0 then
		local subname, descnum
		if i < 0 then
			subname = "m"..math.abs(i)
			descnum = tostring(i)
		else
			subname = tostring(i)
			descnum = S("+@1", i)
		end
		local tex, color, desc
		if i > 0 then
			local val = math.floor((i/100)*255)
			tex = "testnodes_fall_damage_plus.png"
			color = { b=0, g=255-val, r=255, a=255 }
			desc = S("Fall Damage Node (+@1%)", i)
		else
			tex = "testnodes_fall_damage_minus.png"
			if i == -100 then
				color = { r=0, b=0, g=255, a=255 }
			else
				local val = math.floor((math.abs(i)/100)*255)
				color = { r=0, b=255, g=255-val, a=255 }
			end
			desc = S("Fall Damage Node (-@1%)", math.abs(i))
		end
		minetest.register_node("testnodes:damage"..subname, {
			description = desc,
			groups = {fall_damage_add_percent=i, dig_immediate=3},


			tiles = { tex },
			is_ground_content = false,
			color = color,
		})
	end
end

-- Bouncy nodes (various bounce levels)
for i=-140, 180, 20 do
	local val = math.floor(((i-20)/200)*255)
	minetest.register_node(("testnodes:bouncy"..i):gsub("-","NEG"), {
		description = S("Bouncy Node (@1%)", i),
		groups = {bouncy=i, dig_immediate=3},


		tiles ={"testnodes_bouncy.png"},
		is_ground_content = false,
		color = { r=255, g=255-val, b=val, a=255 },
	})
end

-- Slippery nodes (various slippery levels)
for i=1, 5 do
	minetest.register_node("testnodes:slippery"..i, {
		description = S("Slippery Node (@1)", i),
		tiles ={"testnodes_slippery.png"},
		is_ground_content = false,
		groups = {slippery=i, dig_immediate=3},
		color = { r=0, g=255, b=math.floor((i/5)*255), a=255 },
	})
end

-- Move resistance nodes (various resistance levels)
for r=0, 7 do
	if r > 0 then
		minetest.register_node("testnodes:move_resistance"..r, {
			description = S("Move-resistant Node (@1)", r),
			walkable = false,
			move_resistance = r,

			drawtype = "glasslike",
			paramtype = "light",
			sunlight_propagates = true,
			tiles = { "testnodes_move_resistance.png" },
			is_ground_content = false,
			groups = { dig_immediate = 3 },
			color = { b = 0, g = 255, r = math.floor((r/7)*255), a = 255 },
		})
	end

	minetest.register_node("testnodes:move_resistance_liquidlike"..r, {
		description = S("Move-resistant Node, liquidlike (@1)", r),
		walkable = false,
		move_resistance = r,
		liquid_move_physics = true,

		drawtype = "glasslike",
		paramtype = "light",
		sunlight_propagates = true,
		tiles = { "testnodes_move_resistance.png" },
		is_ground_content = false,
		groups = { dig_immediate = 3 },
		color = { b = 255, g = 0, r = math.floor((r/7)*255), a = 255 },
	})
end

minetest.register_node("testnodes:climbable_move_resistance_4", {
	description = S("Climbable Move-resistant Node (4)"),
	walkable = false,
	climbable = true,
	move_resistance = 4,

	drawtype = "glasslike",
	paramtype = "light",
	sunlight_propagates = true,
	tiles = {"testnodes_climbable_resistance_side.png"},
	is_ground_content = false,
	groups = { dig_immediate = 3 },
})

-- By placing something on the node, the node itself will be replaced
minetest.register_node("testnodes:buildable_to", {
	description = S("Replacable Node"),
	buildable_to = true,
	tiles = {"testnodes_buildable_to.png"},
	is_ground_content = false,
	groups = {dig_immediate=3},
})

-- Nodes that deal damage to players that are inside them.
-- Negative damage nodes should heal.
for d=-3,3 do
	if d ~= 0 then
		local sub, tile
		if d > 0 then
			sub = tostring(d)
			tile = "testnodes_damage.png"
		else
			sub = "m" .. tostring(math.abs(d))
			tile = "testnodes_damage_neg.png"
		end
		if math.abs(d) == 2 then
			tile = tile .. "^[colorize:#000000:70"
		elseif math.abs(d) == 3 then
			tile = tile .. "^[colorize:#000000:140"
		end
		minetest.register_node("testnodes:damage_"..sub, {
			description = S("Damage Node (@1 damage per second)", d),
			damage_per_second = d,


			walkable = false,
			is_ground_content = false,
			drawtype = "allfaces",
			paramtype = "light",
			sunlight_propagates = true,
			tiles = { tile },
			groups = {dig_immediate=3},
		})
	end
end

-- Causes drowning damage
minetest.register_node("testnodes:drowning_1", {
	description = S("Drowning Node (@1 damage)", 1),
	drowning = 1,


	walkable = false,
	is_ground_content = false,
	drawtype = "allfaces",
	paramtype = "light",
	sunlight_propagates = true,
	tiles = { "testnodes_drowning.png" },
	groups = {dig_immediate=3},
})

