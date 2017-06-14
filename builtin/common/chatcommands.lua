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

local function gettext(...)
	return ...
end

local function gettext_replace(text, replace)
	return text:gsub("$1", replace)
end


if INIT == "client" then
	cmd_marker = "."
	gettext = core.gettext
	gettext_replace = fgettext_ne
end

local function do_help_cmd(name, param)
	local function format_help_line(cmd, def)
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
		return true, gettext("Available commands: ") .. table.concat(cmds, " ") .. "\n"
				.. gettext_replace("Use '$1help <cmd>' to get more information,"
				.. " or '$1help all' to list everything.", cmd_marker)
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
			privs[#privs + 1] = priv .. ": " .. def.description
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
