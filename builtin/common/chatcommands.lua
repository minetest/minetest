-- Minetest: builtin/common/chatcommands.lua

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

local cmd_marker = "/"

-- Dummy gettext function
local function gettext(...)
	return ...
end

-- Dummy gettext function which only supports up to 2 parameters
local function gettext_replace(text, replace1, replace2)
	local str = text:gsub("$1", replace1)
	if replace2 then
		str = str:gsub("$2", replace2)
	end
	return str
end

if INIT == "client" then
	cmd_marker = "."
	gettext = core.gettext
	gettext_replace = fgettext_ne
end

local function do_help_cmd(name, param)
	local function format_help_line(cmd, def, is_privilege)
		local msg
		if is_privilege then
			msg = core.colorize(core.COLOR_PRIV, cmd)
		else
			cmd = cmd_marker .. cmd
			msg = core.colorize_chatcommand(cmd, def.params)
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
		-- Intentionally not translated because the cmd names are static
		local help_cmd = core.colorize_chatcommand(
				string.format("%shelp", cmd_marker), "<cmd>")
		local help_all = core.colorize_chatcommand(
				string.format("%shelp", cmd_marker), "all")
		-- List available commands
		return true, gettext("Available commands: ")
				.. core.colorize(core.COLOR_COMMAND, table.concat(cmds, " "))
				.. "\n"
				.. gettext_replace("Use '$1' to get more information,"
				.. " or '$2' to list everything.", help_cmd, help_all)
	elseif param == "all" then
		local cmds = {}
		for cmd, def in pairs(core.registered_chatcommands) do
			if INIT == "client" or core.check_player_privs(name, def.privs) then
				cmds[#cmds + 1] = format_help_line(cmd, def)
			end
		end
		table.sort(cmds)
		return true, gettext("Available commands:").."\n"..table.concat(cmds, "\n")
	elseif INIT == "game" and param == "privs" then
		local privs = {}
		for priv, def in pairs(core.registered_privileges) do
			privs[#privs + 1] = format_help_line(priv, def, true)
		end
		table.sort(privs)
		return true, "Available privileges:\n"..table.concat(privs, "\n")
	else
		local cmd = param
		local def = core.registered_chatcommands[cmd]
		if not def then
			return false, gettext("Command not available: ")..cmd
		else
			return true, format_help_line(cmd, def)
		end
	end
end

if INIT == "client" then
	core.register_chatcommand("help", {
		params = gettext("[all | <cmd>]"),
		description = gettext("Get help for commands"),
		func = function(param)
			return do_help_cmd(nil, param)
		end,
	})
else
	core.register_chatcommand("help", {
		params = "[all | privs | <cmd>]",
		description = "Get help for commands or list privileges",
		func = do_help_cmd,
	})
end
