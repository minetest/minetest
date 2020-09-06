local simple_nodes = {
	footstep = { "Footstep Sound Node", "soundstuff_node_footstep.png" },
	dig = { "Dig Sound Node", "soundstuff_node_dig.png" },
	dug = { "Dug Sound Node", "soundstuff_node_dug.png" },
	place = { "Place Sound Node", "soundstuff_node_place.png" },
	place_failed = { "Place Failed Sound Node", "soundstuff_node_place_failed.png" },
}

for k,v in pairs(simple_nodes) do
	minetest.register_node("soundstuff:"..k, {
		description = v[1],
		tiles = {"soundstuff_node_sound.png","soundstuff_node_sound.png",v[2]},
		groups = {dig_immediate=2},
		sounds = {
			[k] = { name = "soundstuff_mono", gain = 1.0 },
		}
	})
end

minetest.register_node("soundstuff:place_failed_attached", {
	description = "Attached Place Failed Sound Node",
	tiles = {"soundstuff_node_sound.png", "soundstuff_node_sound.png", "soundstuff_node_place_failed.png"},
	groups = {dig_immediate=2, attached_node=1},
	drawtype = "nodebox",
	paramtype = "light",
	node_box = { type = "fixed", fixed = {
		{ -7/16, -7/16, -7/16, 7/16, 7/16, 7/16 },
		{ -0.5, -0.5, -0.5, 0.5, -7/16, 0.5 },
	}},
	sounds = {
		place_failed = { name = "soundstuff_mono", gain = 1.0 },
	},
})

minetest.register_node("soundstuff:fall", {
	description = "Fall Sound Node",
	tiles = {"soundstuff_node_sound.png", "soundstuff_node_sound.png", "soundstuff_node_fall.png"},
	groups = {dig_immediate=2, falling_node=1},
	sounds = {
		fall = { name = "soundstuff_mono", gain = 1.0 },
	}
})

minetest.register_node("soundstuff:fall_attached", {
	description = "Attached Fall Sound Node",
	tiles = {"soundstuff_node_sound.png", "soundstuff_node_sound.png", "soundstuff_node_fall.png"},
	groups = {dig_immediate=2, attached_node=1},
	drawtype = "nodebox",
	paramtype = "light",
	node_box = { type = "fixed", fixed = {
		{ -7/16, -7/16, -7/16, 7/16, 7/16, 7/16 },
		{ -0.5, -0.5, -0.5, 0.5, -7/16, 0.5 },
	}},
	sounds = {
		fall = { name = "soundstuff_mono", gain = 1.0 },
	}
})

minetest.register_node("soundstuff:footstep_liquid", {
	description = "Liquid Footstep Sound Node",
	drawtype = "liquid",
	tiles = {
		"soundstuff_node_sound.png^[colorize:#0000FF:127",
	},
	special_tiles = {
		{name = "soundstuff_node_sound.png^[colorize:#0000FF:127", backface_culling = false},
		{name = "soundstuff_node_sound.png^[colorize:#0000FF:127", backface_culling = true},
	},
	liquids_pointable = true,
	liquidtype = "source",
	liquid_alternative_flowing = "soundstuff:footstep_liquid",
	liquid_alternative_source = "soundstuff:footstep_liquid",
	liquid_renewable = false,
	liquid_range = 0,
	liquid_viscosity = 0,
	alpha = 190,
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	is_ground_content = false,
	post_effect_color = {a = 64, r = 0, g = 0, b = 200},
	sounds = {
		footstep = { name = "soundstuff_mono", gain = 1.0 },
	}
})

minetest.register_node("soundstuff:footstep_climbable", {
	description = "Climbable Footstep Sound Node",
	drawtype = "allfaces",
	tiles = {
		"soundstuff_node_climbable.png",
	},
	alpha = 120,
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	climbable = true,
	is_ground_content = false,
	groups = { dig_immediate = 2 },
	sounds = {
		footstep = { name = "soundstuff_mono", gain = 1.0 },
	}
})



minetest.register_craftitem("soundstuff:eat", {
	description = "Eat Sound Item".."\n"..
		"Makes a sound when 'eaten' (with punch key)",
	inventory_image = "soundstuff_eat.png",
	on_use = minetest.item_eat(0),
	sound = {
		eat = { name = "soundstuff_mono", gain = 1.0 },
	}
})

minetest.register_tool("soundstuff:breaks", {
	description = "Break Sound Tool".."\n"..
		"Digs cracky=3 and more".."\n"..
		"Makes a sound when it breaks",
	inventory_image = "soundstuff_node_dug.png",
	sound = {
		breaks = { name = "soundstuff_mono", gain = 1.0 },
	},
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			cracky={times={[2]=2.00, [3]=1.20}, uses=1, maxlevel=0},
			choppy={times={[2]=2.00, [3]=1.20}, uses=1, maxlevel=0},
			snappy={times={[2]=2.00, [3]=1.20}, uses=1, maxlevel=0},
			crumbly={times={[2]=2.00, [3]=1.20}, uses=1, maxlevel=0},
		},
	},
})

-- Plays sound repeatedly
minetest.register_node("soundstuff:positional", {
	description = "Positional Sound Node",
	on_construct = function(pos)
		local timer = minetest.get_node_timer(pos)
		timer:start(0)
	end,
	on_timer = function(pos, elapsed)
		local node = minetest.get_node(pos)
		local dist = node.param2
		if dist == 0 then
			dist = nil
		end
		minetest.sound_play("soundstuff_mono", { pos = pos, max_hear_distance = dist })
		local timer = minetest.get_node_timer(pos)
		timer:start(0.7)
	end,
	on_rightclick = function(pos, node, clicker)
		node.param2 = (node.param2 + 1) % 64
		minetest.set_node(pos, node)
		if clicker and clicker:is_player() then
			local dist = node.param2
			local diststr
			if dist == 0 then
				diststr = "<default>"
			else
				diststr = tostring(dist)
			end
			minetest.chat_send_player(clicker:get_player_name(), "max_hear_distance = " .. diststr)
		end
	end,

	groups = { dig_immediate = 2 },
	tiles = { "soundstuff_node_sound.png" },
})

