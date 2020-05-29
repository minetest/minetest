-- Minetest: builtin/client/chatcommands.lua


core.register_on_sending_chat_message(function(message)
	if message:sub(1,2) == ".." then
		return false
	end

	local first_char = message:sub(1,1)
	if first_char == "/" or first_char == "." then
		core.display_chat_message(core.gettext("issued command: ") .. message)
	end

	if first_char ~= "." then
		return false
	end

	local cmd, param = string.match(message, "^%.([^ ]+) *(.*)")
	param = param or ""

	if not cmd then
		core.display_chat_message(core.gettext("-!- Empty command"))
		return true
	end

	local cmd_def = core.registered_chatcommands[cmd]
	if cmd_def then
		core.set_last_run_mod(cmd_def.mod_origin)
		local _, result = cmd_def.func(param)
		if result then
			core.display_chat_message(result)
		end
	else
		core.display_chat_message(core.gettext("-!- Invalid command: ") .. cmd)
	end

	return true
end)

core.register_chatcommand("list_players", {
	description = core.gettext("List online players"),
	func = function(param)
		local player_names = core.get_player_names()
		if not player_names then
			return false, core.gettext("This command is disabled by server.")
		end

		local players = table.concat(player_names, ", ")
		return true, core.gettext("Online players: ") .. players
	end
})

core.register_chatcommand("disconnect", {
	description = core.gettext("Exit to main menu"),
	func = function(param)
		core.disconnect()
	end,
})

core.register_chatcommand("clear_chat_queue", {
	description = core.gettext("Clear the out chat queue"),
	func = function(param)
		core.clear_out_chat_queue()
		return true, core.gettext("The out chat queue is now empty")
	end,
})

core.register_chatcommand("worldsaving", {
	params="[ on | off | toggle | status ]",
	description = core.gettext("Switch local world saving on/off"),
	func = function(param)
		local onoff

		if ((param == "") or (param == "status")) then
			onoff = core.get_worldsaving()
			return true, core.gettext("Currently world saving is turned " .. onoff .. ".")
		end

		if ((param ~= "on") and (param ~= "off") and (param ~= "toggle")) then
			return false, core.gettext("Try .help worldsaving first please")
		end

		core.set_worldsaving(param)

		onoff = param

		if (param == "toggle") then
			onoff = core.get_worldsaving()
		end

		return true, core.gettext("OK, Worldsaving toggled " .. onoff)
	end,
})

function core.run_server_chatcommand(cmd, param)
	core.send_chat_message("/" .. cmd .. " " .. param)
end
