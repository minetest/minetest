-- This is an example function to ensure it's working properly, should be removed before merge
core.register_on_shutdown(function()
	print("shutdown client")
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

core.register_on_hp_modification(function(hp)
	print("[PREVIEW] HP modified " .. hp)
end)

core.register_on_damage_taken(function(hp)
	print("[PREVIEW] Damage taken " .. hp)
end)
