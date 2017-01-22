-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_shutdown(function()
	print("[PREVIEW] shutdown client")
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
	func = function(name, param)
		return true, dump(_G)
	end,
})

core.after(2, function()
	print("After 2")
end)
