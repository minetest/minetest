-- Minetest: builtin/client/chatcommands.lua


core.register_on_sending_chat_messages(function(message)
	if not (message:sub(1,1) == "/") then
		return false
	end

	core.display_chat_message("issued command: " .. message)

	local cmd, param = string.match(message, "^/([^ ]+) *(.*)")
	if not param then
		param = ""
	end

	local cmd_def = core.registered_chatcommands[cmd]

	if cmd_def then
		core.set_last_run_mod(cmd_def.mod_origin)
		local _, message = cmd_def.func(param)
		if message then
			core.display_chat_message(message)
		end
		return true
	end

	return false
end)
