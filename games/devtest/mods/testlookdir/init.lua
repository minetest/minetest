

minetest.register_entity("testlookdir:fire", {
	initial_properties = {
		visual = "cube",
		textures = {
			"testlookdir_fire_side.png",
			"testlookdir_fire_side.png",
			"testlookdir_fire_side.png",
			"testlookdir_fire_side.png",
			"testlookdir_fire_front.png",
			"testlookdir_fire_back.png",
		},
		visual_size = vector.new(0.1,0.1,1),
		physical = true,
		collide_with_objects = false,
	},
	on_activate = function(self, staticdata)
		self.from_pos = self.object:get_pos()
		print("activate on "..minetest.pos_to_string(self.from_pos))
	end,
	on_step = function(self, dtime, moveresult)
		if moveresult and moveresult.collides then
			self.object:set_velocity(vector.zero())
		end
		if vector.distance(self.from_pos, self.object:get_pos()) > 25 then
			self.object:remove()
		end
	end,
	on_deactivate = function(self)
		print("deactivate")
	end,
})

local function fire_fire_entity(user, lookdir)
	local eye_pos;
	if user.get_eye_pos then
		eye_pos = user:get_eye_pos();
	else
		local props = user:get_properties();
		eye_pos = user:get_pos();
		eye_pos.y = eye_pos.y + props.eye_height;
	end
	local obj = minetest.add_entity(eye_pos, "testlookdir:fire")
	obj:set_velocity(lookdir)
	obj:set_rotation(vector.dir_to_rotation(lookdir))
end

local function on_secondary_use(user)
	local rot = vector.new(-user:get_look_vertical(), user:get_look_horizontal(), 0)
	local lookdir = vector.rotate(vector.new(0,0,1), rot)
	fire_fire_entity(user, lookdir)
end

minetest.register_craftitem("testlookdir:fire", {
		description = "Dig + zoom -> Fire in direction of parent forward vector (use vector.rotate)\n"..
			"Dig -> Fire in direction calculated by get_look_dir.\n"..
			"Place -> Fire in direction calculated from look_verical and look_horizontal.",
		inventory_image = "testlookdir_fire_inv.png",
		on_use = function (itemstack, user)
				if not user:get_player_control().zoom then
					fire_fire_entity(user, user:get_look_dir())
				else
					local parent, bone, pos, rot, vis = user:get_attach()
					if parent then
						rot = parent:get_rotation()
						fire_fire_entity(user, vector.rotate(vector.new(0, 0, 1), rot))
					end
				end
			end,
		on_place = function (itemstack, user)
				on_secondary_use(user)
			end,
		on_secondary_use = function (itemstack, user)
				on_secondary_use(user)
			end,
	})
