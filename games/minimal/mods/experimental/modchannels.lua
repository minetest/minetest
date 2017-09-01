--
-- Mod channels experimental handlers
--

core.after(2, function()
	core.mod_channel_join("experimental_preview")
end)

core.register_on_modchannel_message(function(channel, message)
	print("[minimal][modchannels] Server received message `" .. message
			.. "` on channel `" .. channel .. "`")
	core.mod_channel_send("experimental_preview", "experimental answers to preview")
end)

print("[minimal][modchannels] Code loaded!")
