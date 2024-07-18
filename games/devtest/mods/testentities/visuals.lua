-- Minimal test entities to test visuals

minetest.register_entity("testentities:sprite", {
	initial_properties = {
		visual = "sprite",
		textures = { "testentities_sprite.png" },
	},
})

minetest.register_entity("testentities:upright_sprite", {
	initial_properties = {
		visual = "upright_sprite",
		textures = {
			"testentities_upright_sprite1.png",
			"testentities_upright_sprite2.png",
		},
	},
})

minetest.register_entity("testentities:cube", {
	initial_properties = {
		visual = "cube",
		textures = {
			"testentities_cube1.png",
			"testentities_cube2.png",
			"testentities_cube3.png",
			"testentities_cube4.png",
			"testentities_cube5.png",
			"testentities_cube6.png",
		},
	},
})

minetest.register_entity("testentities:item", {
	initial_properties = {
		visual = "item",
		wield_item = "testnodes:normal",
	},
})

minetest.register_entity("testentities:wielditem", {
	initial_properties = {
		visual = "wielditem",
		wield_item = "testnodes:normal",
	},
})

minetest.register_entity("testentities:mesh", {
	initial_properties = {
		visual = "mesh",
		mesh = "testnodes_pyramid.obj",
		textures = {
			"testnodes_mesh_stripes2.png"
		},
	},
})

minetest.register_entity("testentities:mesh_unshaded", {
	initial_properties = {
		visual = "mesh",
		mesh = "testnodes_pyramid.obj",
		textures = {
			"testnodes_mesh_stripes2.png"
		},
		shaded = false,
	},
})

-- Advanced visual tests

-- An entity for testing animated and yaw-modulated sprites
minetest.register_entity("testentities:yawsprite", {
	initial_properties = {
		selectionbox = {-0.3, -0.5, -0.3, 0.3, 0.3, 0.3},
		visual = "sprite",
		visual_size = {x=0.6666, y=1},
		textures = {"testentities_dungeon_master.png^[makealpha:128,0,0^[makealpha:128,128,0"},
		spritediv = {x=6, y=5},
		initial_sprite_basepos = {x=0, y=0},
	},
	on_activate = function(self, staticdata)
		self.object:set_sprite({x=0, y=0}, 3, 0.5, true)
	end,
})

-- An entity for testing animated upright sprites
minetest.register_entity("testentities:upright_animated", {
	initial_properties = {
		visual = "upright_sprite",
		textures = {"testnodes_anim.png"},
		spritediv = {x = 1, y = 4},
	},
	on_activate = function(self)
		self.object:set_sprite({x=0, y=0}, 4, 1.0, false)
	end,
})

minetest.register_entity("testentities:nametag", {
	initial_properties = {
		visual = "sprite",
		textures = { "testentities_sprite.png" },
	},

	on_activate = function(self, staticdata)
		if staticdata ~= "" then
			local data = minetest.deserialize(staticdata)
			self.color = data.color
			self.bgcolor = data.bgcolor
			self.font = data.font
			self.bold = data.bold
			self.italic = data.italic
		else
			self.color = {
				r = math.random(0, 255),
				g = math.random(0, 255),
				b = math.random(0, 255),
			}

			if math.random(0, 10) > 5 then
				self.bgcolor = {
					r = math.random(0, 255),
					g = math.random(0, 255),
					b = math.random(0, 255),
					a = math.random(0, 255),
				}
			end

			self.font = math.random() < 0.5 and "normal" or "mono"
			self.bold = math.random() < 0.5
			self.italic = math.random() < 0.5
		end

		assert(self.color)
		self.object:set_properties({
			nametag = tostring(math.random(1000, 10000)),
			nametag_color = self.color,
			nametag_bgcolor = self.bgcolor,
			nametag_font = self.font,
			nametag_bold = self.bold,
			nametag_italic = self.italic,
		})
	end,

	on_punch = function(self, puncher)
		local prop = self.object:get_properties()
		-- flip attributes
		local rotator = prop.nametag_color.r%3
		if rotator == 0 then
			self.object:set_nametag_attributes({
				font = prop.nametag_font == "normal" and "mono" or "normal",
			})
		elseif rotator == 1 then
			self.object:set_nametag_attributes({
				bold = not prop.nametag_bold,
			})
		elseif rotator == 2 then
			self.object:set_nametag_attributes({
				italic = not prop.nametag_italic,
			})
		end
	end,

	get_staticdata = function(self)
		return minetest.serialize({ color = self.color, bgcolor = self.bgcolor })
	end,
})
