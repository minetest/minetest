-- Test Nodes: Node property tests

local S = minetest.get_translator("testnodes")

-- Is supposed to fall when it doesn't rest on solid ground
minetest.register_node("testnodes:falling", {
	description = S("Falling Node").."\n"..
		S("Falls down if no node below"),
	tiles = {
		"testnodes_node.png",
		"testnodes_node.png",
		"testnodes_node_falling.png",
	},
	groups = { falling_node = 1, dig_immediate = 3 },
})

minetest.register_node("testnodes:falling_facedir", {
	description = S("Falling Facedir Node").."\n"..
		S("Falls down if no node below").."\n"..
		S("param2 = facedir rotation"),
	tiles = {
		"testnodes_node_falling_1.png",
		"testnodes_node_falling_2.png",
		"testnodes_node_falling_3.png",
		"testnodes_node_falling_4.png",
		"testnodes_node_falling_5.png",
		"testnodes_node_falling_6.png",
	},
	paramtype2 = "facedir",
	groups = { falling_node = 1, dig_immediate = 3 },
})

-- Same as falling node, but will stop falling on top of liquids
minetest.register_node("testnodes:falling_float", {
	description = S("Falling+Floating Node").."\n"..
		S("Falls down if no node below, floats on liquids (liquidtype ~= \"none\")"),
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
	description = S("Floor-Attached Node").."\n"..
		S("Drops as item if no solid node below"),
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
	description = S("Wallmounted Attached Node").."\n"..
		S("Attaches to wall; drops as item if neighbor node is gone").."\n"..
		S("param2 = wallmounted rotation (0..5)"),
	paramtype2 = "wallmounted",
	tiles = {
		"testnodes_attachedw_top.png",
		"testnodes_attachedw_bottom.png",
		"testnodes_attachedw_side.png",
	},
	groups = { attached_node = 1, dig_immediate = 3 },
})

-- Wallmounted node that always attaches to the floor
minetest.register_node("testnodes:attached_wallmounted_floor", {
	description = S("Floor-Attached Wallmounted Node"),
	paramtype2 = "wallmounted",
	tiles = {
		"testnodes_attached_top.png",
		"testnodes_attached_bottom.png",
		"testnodes_attached_side.png",
	},
	groups = { attached_node = 3, dig_immediate = 3 },
	color = "#FF8080",
})

-- This node attaches to the ceiling and drops as item
-- when the ceiling is gone.
minetest.register_node("testnodes:attached_top", {
	description = S("Ceiling-Attached Node"),
	tiles = {
		"testnodes_attached_bottom.png",
		"testnodes_attached_top.png",
		"testnodes_attached_side.png^[transformR180",
	},
	groups = { attached_node = 4, dig_immediate = 3 },
})

-- Same as wallmounted attached, but for facedir
minetest.register_node("testnodes:attached_facedir", {
	description = S("Facedir Attached Node"),
	paramtype2 = "facedir",
	tiles = {
		"testnodes_attachedf_side.png^[transformR180",
		"testnodes_attachedf_side.png",
		"testnodes_attachedf_side.png^[transformR90",
		"testnodes_attachedf_side.png^[transformR270",
		"testnodes_attachedf_bottom.png",
		"testnodes_attachedf_top.png",
	},
	groups = { attached_node = 2, dig_immediate = 3 },
})

-- Same as facedir attached, but for 4dir
minetest.register_node("testnodes:attached_4dir", {
	description = S("4dir Attached Node"),
	paramtype2 = "4dir",
	tiles = {
		"testnodes_attached4_side.png^[transformR180",
		"testnodes_attached4_side.png",
		"testnodes_attached4_side.png^[transformR90",
		"testnodes_attached4_side.png^[transformR270",
		"testnodes_attached4_bottom.png",
		"testnodes_attached4_top.png",
	},
	groups = { attached_node = 2, dig_immediate = 3 },
})

-- Jump disabled
minetest.register_node("testnodes:nojump", {
	description = S("Non-jumping Node").."\n"..
		S("You can't jump on it"),
	groups = {disable_jump=1, dig_immediate=3},
	tiles = {"testnodes_nojump_top.png", "testnodes_nojump_side.png"},
})

-- Jump disabled plant
minetest.register_node("testnodes:nojump_walkable", {
	description = S("Non-jumping Plant Node").."\n"..
		S("You can't jump while your feet are in it"),
	drawtype = "plantlike",
	groups = {disable_jump=1, dig_immediate=3},
	walkable = false,
	tiles = {"testnodes_nojump_top.png"},
})

-- Climbable up and down with jump and sneak keys
minetest.register_node("testnodes:climbable", {
	description = S("Climbable Node").."\n"..
		S("You can climb up and down"),
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
	description = S("Downwards-climbable Node").."\n"..
		S("You can climb only downwards"),
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
	description = S("Non-jumping Liquid Source Node").."\n"..
		S("Swimmable liquid, but you can't swim upwards"),
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
	description = S("Non-jumping Flowing Liquid Node").."\n"..
		S("Swimmable liquid, but you can't swim upwards"),
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
	description = S("No-swim Liquid Source Node").."\n"..
		S("Liquid node, but swimming is disabled"),
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
	description = S("No-swim Flowing Liquid Node").."\n"..
		S("Liquid node, but swimming is disabled"),
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
			subname = "NEG"..string.format("%03d", math.abs(i))
			descnum = tostring(i)
		else
			subname = string.format("%03d", i)
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
local MAX_BOUNCE_JUMPY = 180
local MAX_BOUNCE_NONJUMPY = 140
for i=-MAX_BOUNCE_NONJUMPY, MAX_BOUNCE_JUMPY, 20 do
	if i ~= 0 then
		local desc
		local val = math.floor(((i-20)/200)*255)
		local val2 = math.max(0, 200 - val)
		local num = string.format("%03d", math.abs(i))
		if i > 0 then
			desc = S("Bouncy Node (@1%), jumpy", i).."\n"..
				S("Sneaking/jumping affects bounce")
			color = { r=255-val, g=255-val, b=255, a=255 }
		else
			desc = S("Bouncy Node (@1%), non-jumpy", math.abs(i)).."\n"..
				S("Sneaking/jumping does not affect bounce")
			color = { r=val, g=255, b=val, a=255 }
			num = "NEG"..num
		end
		minetest.register_node("testnodes:bouncy"..num, {
			description = desc,
			groups = {bouncy=i, dig_immediate=3},


			color = color,
			tiles ={"testnodes_bouncy.png"},
			is_ground_content = false,
		})
	end
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
			description = S("Move-resistant Node (@1)", r).."\n"..
				S("Reduces movement speed"),
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

	local mdesc, mcolor
	if r == 0 then
		mdesc = S("Liquidlike Movement Node").."\n"..
			S("Swimmable (no move resistance)")
		mcolor = { b = 255, g = 255, r = 128 }
	else
		mdesc = S("Move-resistant Node (@1), liquidlike", r).."\n"..
			S("Reduces movement speed; swimmable")
		mcolor = { b = 255, g = 0, r = math.floor((r/7)*255), a = 255 }
	end


	minetest.register_node("testnodes:move_resistance_liquidlike"..r, {
		description = mdesc,
		walkable = false,
		move_resistance = r,
		liquid_move_physics = true,

		drawtype = "glasslike",
		paramtype = "light",
		sunlight_propagates = true,
		tiles = { "testnodes_move_resistance.png" },
		is_ground_content = false,
		groups = { dig_immediate = 3 },
		color = mcolor,
	})
end

minetest.register_node("testnodes:climbable_move_resistance_4", {
	description = S("Climbable Move-resistant Node (4)").."\n"..
		S("You can climb up and down; reduced movement speed"),
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
	description = S("\"buildable_to\" Node").."\n"..
		S("Placing a node on it will replace it"),
	buildable_to = true,
	tiles = {"testnodes_buildable_to.png"},
	is_ground_content = false,
	groups = {dig_immediate=3},
})

-- Nodes that deal damage to players that are inside them.
-- Negative damage nodes should heal.
for d=-3,3 do
	if d ~= 0 then
		local sub, tile, desc
		if d > 0 then
			sub = tostring(d)
			tile = "testnodes_damage.png"
			desc = S("Damage Node (@1 damage per second)", d)
		else
			sub = "m" .. tostring(math.abs(d))
			tile = "testnodes_damage_neg.png"
			desc = S("Healing Node (@1 HP per second)", math.abs(d))
		end
		if math.abs(d) == 2 then
			tile = tile .. "^[colorize:#000000:70"
		elseif math.abs(d) == 3 then
			tile = tile .. "^[colorize:#000000:140"
		end
		minetest.register_node("testnodes:damage_"..sub, {
			description = desc,
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
	description = S("Drowning Node (@1 damage)", 1).."\n"..
		S("You'll drown inside it"),
	drowning = 1,


	walkable = false,
	is_ground_content = false,
	drawtype = "allfaces",
	paramtype = "light",
	sunlight_propagates = true,
	tiles = { "testnodes_drowning.png" },
	groups = {dig_immediate=3},
})

