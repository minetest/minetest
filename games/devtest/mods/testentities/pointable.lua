-- Pointability test Entities

minetest.register_entity("testentities:pointable", {
	initial_properties = {
		visual = "cube",
		visual_size = {x = 0.6, y = 0.6, z = 0.6},
		textures = {
			"testnodes_pointable.png",
			"testnodes_pointable.png",
			"testnodes_pointable.png",
			"testnodes_pointable.png",
			"testnodes_pointable.png",
			"testnodes_pointable.png"
		},
		pointable = true,
	},
	on_activate = function(self)
		self.object:set_armor_groups({pointable_test = 1})
	end
})

minetest.register_entity("testentities:not_pointable", {
	initial_properties = {
		visual = "cube",
		visual_size = {x = 0.6, y = 0.6, z = 0.6},
		textures = {
			"testnodes_not_pointable.png",
			"testnodes_not_pointable.png",
			"testnodes_not_pointable.png",
			"testnodes_not_pointable.png",
			"testnodes_not_pointable.png",
			"testnodes_not_pointable.png"
		},
		pointable = false,
	},
	on_activate = function(self)
		self.object:set_armor_groups({not_pointable_test = 1})
	end
})

minetest.register_entity("testentities:blocking_pointable", {
	initial_properties = {
		visual = "cube",
		visual_size = {x = 0.6, y = 0.6, z = 0.6},
		textures = {
			"testnodes_blocking_pointable.png",
			"testnodes_blocking_pointable.png",
			"testnodes_blocking_pointable.png",
			"testnodes_blocking_pointable.png",
			"testnodes_blocking_pointable.png",
			"testnodes_blocking_pointable.png"
		},
		pointable = "blocking",
	},
	on_activate = function(self)
		self.object:set_armor_groups({blocking_pointable_test = 1})
	end
})
