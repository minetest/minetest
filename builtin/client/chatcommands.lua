-- Minetest: builtin/client/chatcommands.lua


minetest.register_on_sending_chat_message(function(message)
	if message:sub(1,2) == ".." then
		return false
	end

	local first_char = message:sub(1,1)
	if first_char == "/" or first_char == "." then
		minetest.display_chat_message(minetest.gettext("issued command: ") .. message)
	end

	if first_char ~= "." then
		return false
	end

	local cmd, param = string.match(message, "^%.([^ ]+) *(.*)")
	param = param or ""

	if not cmd then
		minetest.display_chat_message(minetest.gettext("-!- Empty command"))
		return true
	end

	local cmd_def = minetest.registered_chatcommands[cmd]
	if cmd_def then
		minetest.set_last_run_mod(cmd_def.mod_origin)
		local _, result = cmd_def.func(param)
		if result then
			minetest.display_chat_message(result)
		end
	else
		minetest.display_chat_message(minetest.gettext("-!- Invalid command: ") .. cmd)
	end

	return true
end)

minetest.register_chatcommand("list_players", {
	description = minetest.gettext("List online players"),
	func = function(param)
		local player_names = minetest.get_player_names()
		if not player_names then
			return false, minetest.gettext("This command is disabled by server.")
		end

		local players = table.concat(player_names, ", ")
		return true, minetest.gettext("Online players: ") .. players
	end
})

minetest.register_chatcommand("disconnect", {
	description = minetest.gettext("Exit to main menu"),
	func = function(param)
		minetest.disconnect()
	end,
})

minetest.register_chatcommand("clear_chat_queue", {
	description = minetest.gettext("Clear the out chat queue"),
	func = function(param)
		minetest.clear_out_chat_queue()
		return true, minetest.gettext("The out chat queue is now empty")
	end,
})

function minetest.run_server_chatcommand(cmd, param)
	minetest.send_chat_message("/" .. cmd .. " " .. param)
end
