
local S = minetest.get_translator("testentities")

minetest.register_entity("testentities:attach", {
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
		infotext = "Right click/place -> attach/detach\n"..
			"AUX1 -> change mode\n"..
			"Right/Left -> change X axis of vector\n"..
			"Up/Down -> change Z axis of vector\n"..
			"Jump/Sneak -> change Y axis of vector",
	},
	control_mode = "parent_rotation",
	
	on_activate = function(self, staticdata)
		local my_data = minetest.deserialize(staticdata);
		self.attach_pos = vector.zero()
		self.attach_rot = vector.zero()
		if my_data and my_data.attach_pos then
			self.attach_pos = my_data.attach_pos
		end
		if my_data and my_data.attach_rot then
			self.attach_rot = my_data.attach_rot
		end
	end,
	on_rightclick = function(self, clicker)
		if (not self.player_name) then
			clicker:set_attach(self.object, nil, self.attach_pos, self.attach_rot, true)
			self.player_name = clicker:get_player_name()
			self:chat_control_mode_info()
		else
			clicker:set_detach()
			self.player_name = false
		end
	end,
	on_step = function(self, dtime, moveresult)
		if self.player_name then
			local player = minetest.get_player_by_name(self.player_name);
			if (not player) then
				self.player_name = false
				return
			end
			local controls = player:get_player_control()
			if self.control_mode == "parent_rotation" then
				local rot = self.object:get_rotation()
				if controls.right then
					rot.x = rot.x + dtime * math.pi/10;
				elseif controls.left then
					rot.x = rot.x - dtime * math.pi/10;
				elseif controls.up then
					rot.z = rot.z + dtime * math.pi/10;
				elseif controls.down then
					rot.z = rot.z - dtime * math.pi/10;
				elseif controls.jump then
					rot.y = rot.y + dtime * math.pi/10;
				elseif controls.sneak then
					rot.y = rot.y - dtime * math.pi/10;
				elseif controls.aux1 and not self.aux1 then
					self.control_mode = "parent_move"
					self:chat_control_mode_info()
				end
				self.object:set_rotation(rot)
			elseif self.control_mode == "parent_move" then
				local pos = self.object:get_pos()
				if controls.up then
					pos.x = pos.x + dtime;
				elseif controls.down then
					pos.x = pos.x - dtime;
				elseif controls.right then
					pos.z = pos.z + dtime;
				elseif controls.left then
					pos.z = pos.z - dtime;
				elseif controls.jump then
					pos.y = pos.y + dtime;
				elseif controls.sneak then
					pos.y = pos.y - dtime;
				elseif controls.aux1 and not self.aux1 then
					self.control_mode = "attach_rotation"
					self:chat_control_mode_info()
				end
				self.object:set_pos(pos)
			elseif self.control_mode == "attach_rotation" then
				local rot = self.attach_rot
				if controls.right then
					rot.x = rot.x + dtime * 10;
				elseif controls.left then
					rot.x = rot.x - dtime * 10;
				elseif controls.up then
					rot.z = rot.z + dtime * 10;
				elseif controls.down then
					rot.z = rot.z - dtime * 10;
				elseif controls.jump then
					rot.y = rot.y + dtime * 10;
				elseif controls.sneak then
					rot.y = rot.y - dtime * 10;
				elseif controls.aux1 and not self.aux1 then
					self.control_mode = "attach_move"
					self:chat_control_mode_info()
				end
				player:set_attach(self.object, nil, self.attach_pos, self.attach_rot, true)
			elseif self.control_mode == "attach_move" then
				local pos = self.attach_pos
				if controls.up then
					pos.x = pos.x + dtime*10;
				elseif controls.down then
					pos.x = pos.x - dtime*10;
				elseif controls.right then
					pos.z = pos.z + dtime*10;
				elseif controls.left then
					pos.z = pos.z - dtime*10;
				elseif controls.jump then
					pos.y = pos.y + dtime*10;
				elseif controls.sneak then
					pos.y = pos.y - dtime*10;
				elseif controls.aux1 and not self.aux1 then
					self.control_mode = "parent_rotation"
					self:chat_control_mode_info()
				end
				player:set_attach(self.object, nil, self.attach_pos, self.attach_rot, true)
			end
			self.aux1 = controls.aux1
		end
	end,
	get_staticdata = function(self)
		return minetest.serialize({attach_pos = self.attach_pos, attach_rot = self.attach_rot})
	end,
	chat_control_mode_info = function(self)
		if self.control_mode == "parent_rotation" then
			minetest.chat_send_player(self.player_name, S("Now, you are controling parent rotation."))
		elseif self.control_mode == "parent_move" then
			minetest.chat_send_player(self.player_name, S("Now, you are controling parent move."))
		elseif self.control_mode == "attach_rotation" then
			minetest.chat_send_player(self.player_name, S("Now, you are controling attach rotation."))
		elseif self.control_mode == "attach_move" then
			minetest.chat_send_player(self.player_name, S("Now, you are controling attach position."))
		else
			self.control_mode = "parent_rotation"
			minetest.chat_send_player(self.player_name, S("Unknown control mode detected, reseting to control of parent rotation."))
		end
	end
})
