local function random_color()
	return ("blank.png^[noalpha^[colorize:#%06X:255"):format(math.random(0, 0xFFFFFF))
end
minetest.register_entity("testentities:selectionbox", {
	initial_properties = {
		visual = "cube",
		infotext = "Punch to randomize rotation, rightclick to toggle rotation"
	},
	on_activate = function(self)
		local w, h, l = math.random(), math.random(), math.random()
		self.object:set_properties({
			textures = {random_color(), random_color(), random_color(), random_color(), random_color(), random_color()},
			selectionbox = {rotate = true, -w/2, -h/2, -l/2, w/2, h/2, l/2},
			visual_size = vector.new(w, h, l),
			automatic_rotate = 2 * (math.random() - 0.5)
		})
		assert(self.object:get_properties().selectionbox.rotate)
		self.object:set_armor_groups({punch_operable = 1})
	end,
	on_punch = function(self)
		self.object:set_rotation(2 * math.pi * vector.new(math.random(), math.random(), math.random()))
	end,
	on_rightclick = function(self)
		self.object:set_properties({
			automatic_rotate = self.object:get_properties().automatic_rotate == 0 and 2 * (math.random() - 0.5) or 0
		})
	end
})
