local vec = vector.new
local cameras = {}
local screens = {}

core.register_entity("cameras:screen", {
	initial_properties = {
		visual = "upright_sprite",
		visual_size = {x = 4, y = 3},
		static_save = false,
		shaded = false,
		glow = 15,
	},
})

core.register_chatcommand("set_camera", {
	description = "Add a camera of a given type",
	func = function(caller, param)
		local param1 = unpack(param:split" ")
		local player = core.get_player_by_name(caller)
		if not player then return end
		local player_pos = player:get_pos()

		if not cameras[caller] then
			cameras[caller] = player:add_camera()
		end

		player:set_camera(cameras[caller], {
			viewport = {x = 0.6, y = 0.6, w = 0.35, h = 0.2625},
			rotation = vec(0, 0, 0),

			interpolate_fov = {enabled = false, speed = 0},
			interpolate_pos = {enabled = false, speed = 0},
			interpolate_rotation = {enabled = false, speed = 0},
		})

		if param1 == "simple" then
			player:set_camera(cameras[caller], {
				enabled = true,
				pos = player_pos + vec(4, 4, 4),
				target = player_pos,
			})
		elseif param1 == "manip" then
			core.show_formspec(caller, "", "size[0,0]no_prepend[]bgcolor[#0000]")

			player:set_camera(cameras[caller], {
				enabled = true,
				pos = player_pos + vec(2, 4, 2),
				viewport = {x = 0, y = 0, w = 1, h = 1},
				target = player_pos + vec(0, 1.4, 0),
			})
		elseif param1 == "follow" then
			player:set_camera(cameras[caller], {
				enabled = true,
				pos = player_pos + vec(4, 4, 4),
				target = player_pos,
				attachment = { object = player, follow = true }
			})
		elseif param1 == "watch" then
			player:set_camera(cameras[caller], {
				enabled = true,
				pos = player_pos + vec(2, 2, 2),
				target = player_pos + vec(0, 0.5, 0),
				attachment = { object = player, follow = false },
			})
		elseif param1 == "entity" then
			player:set_camera(cameras[caller], {
				enabled = true,
				texture = {name = "default_camera", aspect_ratio = 4/3}
			})

			core.after(1, function()
				if not screens[caller] then
					screens[caller] = core.add_entity(player_pos + vec(0, 2, 0), "cameras:screen")
					screens[caller]:set_properties{textures = {"default_camera"}}
				end
			end)
		elseif param1 == "screen" then
			if screens[caller] then
				screens[caller]:remove()
				screens[caller] = nil
			end

			player:set_camera(cameras[caller], {texture = {name = ""}})
		elseif param1 == "interpolation" then
			player:set_camera(cameras[caller], {
				enabled = true,
				pos = player_pos + vec(-2, 2, 0),
				target = player_pos + vec(0, 1, 0),
			})

			core.after(1, function()
				player:set_camera(cameras[caller], {
					fov = 120,
					pos = player_pos + vec(-8, 8, 0),
					rotation = vec(0, 0, 360),

					interpolate_fov = {enabled = true, speed = 1},
					interpolate_pos = {enabled = true, speed = 1},
					interpolate_rotation = {enabled = true, speed = 0.2},
				})

				core.after(4, function()
					player:set_camera(cameras[caller], {
						fov = 72,
						pos = player_pos + vec(-2, 2, 0),

						interpolate_fov = {enabled = true, speed = 1},
						interpolate_pos = {enabled = true, speed = 1},
					})
				end)
			end)
		elseif param1 == "off" then
			player:remove_camera(cameras[caller])
		end
	end
})

core.register_chatcommand("get_camera", {
	description = "Get current secondary camera parameters",
	func = function(caller, param)
		local player = core.get_player_by_name(caller)
		if not player then return end
		if not cameras[caller] then return end

		core.log("action", dump(player:get_camera(cameras[caller])))
		core.chat_send_player(caller, "Camera parameters logged.")
	end
})
