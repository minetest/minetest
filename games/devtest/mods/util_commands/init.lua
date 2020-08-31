minetest.register_chatcommand("hotbar", {
	params = "<size>",
	description = "Set hotbar size",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local size = tonumber(param)
		if not size then
			return false, "Missing or incorrect size parameter!"
		end
		local ok = player:hud_set_hotbar_itemcount(size)
		if ok then
			return true
		else
			return false, "Invalid item count!"
		end
	end,
})

minetest.register_chatcommand("hp", {
	params = "<hp>",
	description = "Set your health",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local hp = tonumber(param)
		if not hp then
			return false, "Missing or incorrect hp parameter!"
		end
		player:set_hp(hp)
		return true
	end,
})

minetest.register_chatcommand("zoom", {
	params = "[<zoom_fov>]",
	description = "Set or display your zoom_fov",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		if param == "" then
			local fov = player:get_properties().zoom_fov
			return true, "zoom_fov = "..tostring(fov)
		end
		local fov = tonumber(param)
		if not fov then
			return false, "Missing or incorrect zoom_fov parameter!"
		end
		player:set_properties({zoom_fov = fov})
		fov = player:get_properties().zoom_fov
		return true, "zoom_fov = "..tostring(fov)
	end,
})



local s_infplace = minetest.settings:get("devtest_infplace")
if s_infplace == "true" then
	infplace = true
elseif s_infplace == "false" then
	infplace = false
else
	infplace = minetest.is_creative_enabled("")
end

minetest.register_chatcommand("infplace", {
	params = "",
	description = "Toggle infinite node placement",
	func = function(name, param)
		infplace = not infplace
		if infplace then
			minetest.chat_send_all("Infinite node placement enabled!")
			minetest.log("action", "Infinite node placement enabled")
		else
			minetest.chat_send_all("Infinite node placement disabled!")
			minetest.log("action", "Infinite node placement disabled")
		end
		return true
	end,
})

minetest.register_chatcommand("detach", {
	params = "[<radius>]",
	description = "Detach all objects nearby",
	func = function(name, param)
		local radius = tonumber(param)
		if type(radius) ~= "number" then
			radius = 8
		end
		if radius < 1 then
			radius = 1
		end
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end
		local objs = minetest.get_objects_inside_radius(player:get_pos(), radius)
		local num = 0
		for o=1, #objs do
			if objs[o]:get_attach() then
				objs[o]:set_detach()
				num = num + 1
			end
		end
		return true, string.format("%d object(s) detached.", num)
	end,
})

-- Use this to test waypoint capabilities
minetest.register_chatcommand("test_waypoints", {
	params = "[change_immediate]",
	description = "tests waypoint capabilities",
	func = function(name, params)
		local player = minetest.get_player_by_name(name)
		local regular = player:hud_add {
			hud_elem_type = "waypoint",
			name = "regular waypoint",
			text = "m",
			number = 0xFF0000,
			world_pos = vector.add(player:get_pos(), {x = 0, y = 1.5, z = 0})
		}
		local reduced_precision = player:hud_add {
			hud_elem_type = "waypoint",
			name = "better waypoint",
			text = "m (0.5 steps, precision = 2)",
			precision = 10,
			number = 0xFFFF00,
			world_pos = vector.add(player:get_pos(), {x = 0, y = 1, z = 0})
		}
		local function change()
			if regular then
				player:hud_change(regular, "world_pos", vector.add(player:get_pos(), {x = 0, y = 3, z = 0}))
			end
			if reduced_precision then
				player:hud_change(reduced_precision, "precision", 2)
			end
		end
		if params ~= "" then
			-- change immediate
			change()
		else
			minetest.after(0.5, change)
		end
		regular = regular or "error"
		reduced_precision = reduced_precision or "error"
		local hidden_distance = player:hud_add {
			hud_elem_type = "waypoint",
			name = "waypoint with hidden distance",
			text = "this text is hidden as well (precision = 0)",
			precision = 0,
			number = 0x0000FF,
			world_pos = vector.add(player:get_pos(), {x = 0, y = 0.5, z = 0})
		} or "error"
		local image_waypoint = player:hud_add {
			hud_elem_type = "image_waypoint",
			text = "wieldhand.png",
			world_pos = player:get_pos(),
			scale = {x = 10, y = 10},
			offset = {x = 0, y = -32}
		} or "error"
		minetest.chat_send_player(name, "Waypoint IDs: regular: " .. regular .. ", reduced precision: " .. reduced_precision ..
			", hidden distance: " .. hidden_distance .. ", image waypoint: " .. image_waypoint)
	end
})

-- Unlimited node placement
minetest.register_on_placenode(function(pos, newnode, placer, oldnode, itemstack)
	if placer and placer:is_player() then
		return infplace
	end
end)

-- Don't pick up if the item is already in the inventory
local old_handle_node_drops = minetest.handle_node_drops
function minetest.handle_node_drops(pos, drops, digger)
	if not digger or not digger:is_player() or not infplace then
		return old_handle_node_drops(pos, drops, digger)
	end
	local inv = digger:get_inventory()
	if inv then
		for _, item in ipairs(drops) do
			if not inv:contains_item("main", item, true) then
				inv:add_item("main", item)
			end
		end
	end
end
