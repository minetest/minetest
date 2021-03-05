-- Minetest: builtin/common/chatcommands.lua

-- For server-side translations (if INIT == "game")
-- Otherwise, use core.gettext
local S = core.get_translator("__builtin")

core.registered_chatcommands = {}

function core.register_chatcommand(cmd, def)
	def = def or {}
	def.params = def.params or ""
	def.description = def.description or ""
	def.privs = def.privs or {}
	def.mod_origin = core.get_current_modname() or "??"
	core.registered_chatcommands[cmd] = def
end

function core.unregister_chatcommand(name)
	if core.registered_chatcommands[name] then
		core.registered_chatcommands[name] = nil
	else
		core.log("warning", "Not unregistering chatcommand " ..name..
			" because it doesn't exist.")
	end
end

function core.override_chatcommand(name, redefinition)
	local chatcommand = core.registered_chatcommands[name]
	assert(chatcommand, "Attempt to override non-existent chatcommand "..name)
	for k, v in pairs(redefinition) do
		rawset(chatcommand, k, v)
	end
	core.registered_chatcommands[name] = chatcommand
end

local function do_help_cmd(name, param)
	local function format_help_line(cmd, def)
		local cmd_marker = "/"
		if INIT == "client" then
			cmd_marker = "."
		end
		local msg = core.colorize("#00ffff", cmd_marker .. cmd)
		if def.params and def.params ~= "" then
			msg = msg .. " " .. def.params
		end
		if def.description and def.description ~= "" then
			msg = msg .. ": " .. def.description
		end
		return msg
	end
	if param == "" then
		local cmds = {}
		for cmd, def in pairs(core.registered_chatcommands) do
			if INIT == "client" or core.check_player_privs(name, def.privs) then
				cmds[#cmds + 1] = cmd
			end
		end
		table.sort(cmds)
		local msg
		if INIT == "game" then
			msg = S("Available commands: @1",
				table.concat(cmds, " ")) .. "\n"
				.. S("Use '/help <cmd>' to get more "
				.. "information, or '/help all' to list "
				.. "everything.")
		else
			msg = core.gettext("Available commands: ")
				.. table.concat(cmds, " ") .. "\n"
				.. core.gettext("Use '.help <cmd>' to get more "
				.. "information, or '.help all' to list "
				.. "everything.")
		end
		return true, msg
	elseif param == "all" then
		local cmds = {}
		for cmd, def in pairs(core.registered_chatcommands) do
			if INIT == "client" or core.check_player_privs(name, def.privs) then
				cmds[#cmds + 1] = format_help_line(cmd, def)
			end
		end
		table.sort(cmds)
		local msg
		if INIT == "game" then
			msg = S("Available commands:")
		else
			msg = core.gettext("Available commands:")
		end
		return true, msg.."\n"..table.concat(cmds, "\n")
	elseif INIT == "game" and param == "privs" then
		local privs = {}
		for priv, def in pairs(core.registered_privileges) do
			privs[#privs + 1] = priv .. ": " .. def.description
		end
		table.sort(privs)
		return true, S("Available privileges:").."\n"..table.concat(privs, "\n")
	else
		local cmd = param
		local def = core.registered_chatcommands[cmd]
		if not def then
			local msg
			if INIT == "game" then
				msg = S("Command not available: @1", cmd)
			else
				msg = core.gettext("Command not available: ") .. cmd
			end
			return false, msg
		else
			return true, format_help_line(cmd, def)
		end
	end
end

if INIT == "client" then
	core.register_chatcommand("help", {
		params = core.gettext("[all | <cmd>]"),
		description = core.gettext("Get help for commands"),
		func = function(param)
			return do_help_cmd(nil, param)
		end,
	})
else
	core.register_chatcommand("help", {
		params = S("[all | privs | <cmd>]"),
		description = S("Get help for commands or list privileges"),
		func = do_help_cmd,
	})
end
