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

-- An entity and arrows for testing automatic collision/selection boxes rotation
minetest.register_entity("testentities:3d_dungeon_master", {
	visual = "mesh",
	mesh = "testentities_3d_dungeon_master.b3d",
	textures = {"testentities_3d_dungeon_master.png"},
	collisionbox = {-1.0, -1, -0.4, 1.0, 1.6, 0.7},
	synchronize_cbox_rotation_with_dir = true,
	synchronize_sbox_rotation_with_dir = true,
	get_staticdata = function(self)
		return minetest.write_json({rotation_info=self.rot_info})
	end,
	on_activate = function(self, staticdata, dtime_s)
		if staticdata ~= "" and staticdata ~= nil then
			self.rot_info = minetest.parse_json(staticdata).rotation_info
		else
			self.rot_info = {x=false, y=false, z=false}
		end
	end,
	on_rightclick = function(self, clicker)
		local witem = clicker:get_wielded_item()
		
		if witem:get_name() == "testentities:rotation_x" then
			self.rot_info.x = not self.rot_info.x
		elseif witem:get_name() == "testentities:rotation_y" then
			self.rot_info.y = not self.rot_info.y
		elseif witem:get_name() == "testentities:rotation_z" then
			self.rot_info.z = not self.rot_info.z
		end
	end,
	on_step = function(self, dtime)
		local target_rot = self.object:get_rotation()
		
		if self.rot_info.x then
			target_rot.x = target_rot.x + math.rad(3)
		end
		if self.rot_info.y then
			target_rot.y = target_rot.y + math.rad(3)
		end
		if self.rot_info.z then
			target_rot.z = target_rot.z + math.rad(3)
		end
                                                           
		self.object:set_rotation(target_rot)
	end
})

minetest.register_craftitem("testentities:rotation_x", {
	description = "X Arrow (force the dungeon master to rotate around X axis)",
	inventory_image = "testentities_rotation_x.png"
})

minetest.register_craftitem("testentities:rotation_y", {
	description = "Y Arrow (force the dungeon master to rotate around Y axis)",
	inventory_image = "testentities_rotation_y.png"
})

minetest.register_craftitem("testentities:rotation_z", {
	description = "Z Arrow (force the dungeon master to rotate around Z axis)",
	inventory_image = "testentities_rotation_z.png"
})

minetest.register_entity("testentities:nametag", {
	initial_properties = {
		visual = "sprite",
		textures = { "testentities_sprite.png" },
	},

	on_activate = function(self, staticdata)
		if staticdata ~= "" then
			self.color = minetest.deserialize(staticdata).color
		else
			self.color = {
				r = math.random(0, 255),
				g = math.random(0, 255),
				b = math.random(0, 255),
			}
		end

		assert(self.color)
		self.object:set_properties({
			nametag = tostring(math.random(1000, 10000)),
			nametag_color = self.color,
		})
	end,

	get_staticdata = function(self)
		return minetest.serialize({ color = self.color })
	end,
})
