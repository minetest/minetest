local modname = core.get_current_modname() or "??"
local modstorage = core.get_mod_storage()

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_shutdown(function()
	print("[PREVIEW] shutdown client")
end)

core.register_on_connect(function()
	print("[PREVIEW] Player connection completed")
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_receiving_chat_messages(function(message)
	print("[PREVIEW] Received message " .. message)
	return false
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_sending_chat_messages(function(message)
	print("[PREVIEW] Sending message " .. message)
	return false
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_hp_modification(function(hp)
	print("[PREVIEW] HP modified " .. hp)
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_damage_taken(function(hp)
	print("[PREVIEW] Damage taken " .. hp)
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_globalstep(function(dtime)
	-- print("[PREVIEW] globalstep " .. dtime)
end)

-- This is an example function to ensure it's working properly, should be removed before merge
core.register_chatcommand("dump", {
	func = function(param)
		return true, dump(_G)
	end,
})

core.register_chatcommand("colorize_test", {
	func = function(param)
		return true, core.colorize("red", param)
	end,
})

core.register_chatcommand("test_node", {
	func = function(param)
		core.display_chat_message(dump(core.get_node({x=0, y=0, z=0})))
		core.display_chat_message(dump(core.get_node_or_nil({x=0, y=0, z=0})))
	end,
})

local function preview_minimap()
	local minimap = core.ui.minimap
	minimap:set_mode(4)
	minimap:show()
	minimap:set_pos({x=5, y=50, z=5})
	minimap:toggle_shape()

	print("[PREVIEW] Minimap: mode => " .. dump(minimap:get_mode()) ..
			" position => " .. dump(minimap:get_pos()) ..
			" angle => " .. dump(minimap:get_angle()))
end

core.after(2, function()
	print("[PREVIEW] loaded " .. modname .. " mod")
	modstorage:set_string("current_mod", modname)
	print(modstorage:get_string("current_mod"))
	print("Server version:" .. core.get_protocol_version())
	preview_minimap()
end)

core.after(5, function()
	core.ui.minimap:show()

	print("[PREVIEW] Day count: " .. core.get_day_count() ..
		" time of day " .. core.get_timeofday())

	print("[PREVIEW] Node level: " .. core.get_node_level({x=0, y=20, z=0}) ..
		" max level " .. core.get_node_max_level({x=0, y=20, z=0}))

	print("[PREVIEW] Find node near: " .. dump(core.find_node_near({x=0, y=20, z=0}, 10,
		{"group:tree", "default:dirt", "default:stone"})))
end)

core.register_on_dignode(function(pos, node)
	print("The local player dug a node!")
	print("pos:" .. dump(pos))
	print("node:" .. dump(node))
	return false
end)

core.register_on_punchnode(function(pos, node)
	print("The local player punched a node!")
	local itemstack = core.get_wielded_item()
	--[[
	-- getters
	print(dump(itemstack:is_empty()))
	print(dump(itemstack:get_name()))
	print(dump(itemstack:get_count()))
	print(dump(itemstack:get_wear()))
	print(dump(itemstack:get_meta()))
	print(dump(itemstack:get_metadata()
	print(dump(itemstack:is_known()))
	--print(dump(itemstack:get_definition()))
	print(dump(itemstack:get_tool_capabilities()))
	print(dump(itemstack:to_string()))
	print(dump(itemstack:to_table()))
	-- setters
	print(dump(itemstack:set_name("default:dirt")))
	print(dump(itemstack:set_count("95")))
	print(dump(itemstack:set_wear(934)))
	print(dump(itemstack:get_meta()))
	print(dump(itemstack:get_metadata()))
	--]]
	print(dump(itemstack:to_table()))
	print("pos:" .. dump(pos))
	print("node:" .. dump(node))
	return false
end)

