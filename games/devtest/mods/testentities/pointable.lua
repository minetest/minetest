-- Entity pointability test

minetest.register_entity("testentities:pointable", {
	initial_properties = {
		visual = "cube",
		visual_size = {x = 0.6, y = 0.6, z = 0.6},
		
	},
	on_activate = function(self)
		local r = math.random()
		if r < 0.33 then
			self.object:set_armor_groups({pointable_test = 1})
			self.object:set_properties({
				pointable = true,
				textures = {
					"testnodes_pointable.png",
					"testnodes_pointable.png",
					"testnodes_pointable.png",
					"testnodes_pointable.png",
					"testnodes_pointable.png",
					"testnodes_pointable.png"
				},
			})
		elseif r < 0.66 then
			self.object:set_armor_groups({not_pointable_test = 1})
			self.object:set_properties({
				pointable = false,
				textures = {
					"testnodes_not_pointable.png",
					"testnodes_not_pointable.png",
					"testnodes_not_pointable.png",
					"testnodes_not_pointable.png",
					"testnodes_not_pointable.png",
					"testnodes_not_pointable.png"
				},
			})
		else
			self.object:set_armor_groups({blocking_pointable_test = 1})
			self.object:set_properties({
				pointable = "blocking",
				textures = {
					"testnodes_blocking_pointable.png",
					"testnodes_blocking_pointable.png",
					"testnodes_blocking_pointable.png",
					"testnodes_blocking_pointable.png",
					"testnodes_blocking_pointable.png",
					"testnodes_blocking_pointable.png"
				},
			})
		end
	end
})
