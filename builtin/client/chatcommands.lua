-- Minetest: builtin/client/chatcommands.lua


core.register_on_sending_chat_messages(function(message)
	local first_char = message:sub(1,1)
	if first_char == "/" or first_char == "." then
		core.display_chat_message("issued command: " .. message)
	end

	if first_char ~= "." then
		return false
	end

	local cmd, param = string.match(message, "^%.([^ ]+) *(.*)")
	if not param then
		param = ""
	end

	if not cmd then
		core.display_chat_message("-!- Empty command")
		return true
	end

	local cmd_def = core.registered_chatcommands[cmd]
	if cmd_def then
		core.set_last_run_mod(cmd_def.mod_origin)
		local _, message = cmd_def.func(param)
		if message then
			core.display_chat_message(message)
		end
	else
		core.display_chat_message("-!- Invalid command: " .. cmd)
	end

	return true
end)
