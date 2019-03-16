local COLOR_BLUE = "#47F"
local COLOR_GREEN = "#7F7"
local COLOR_GRAY = "#BBB"

local LIST_FORMSPEC = [[
		size[13,6.5]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,5.5;_cmds;%s;0]
		button_exit[5.5,6;2,1;quit;%s]
	]]

local formspec_escape = core.formspec_escape
local check_player_privs = core.check_player_privs


-- CHAT COMMANDS FORMSPEC

local mod_cmds = {}

local function load_mod_command_tree()
	mod_cmds = {}

	local check_player_privs = core.check_player_privs
	for name, def in pairs(core.registered_chatcommands) do
		mod_cmds[def.mod_origin] = mod_cmds[def.mod_origin] or {}
		local cmds = mod_cmds[def.mod_origin]

		-- Could be simplified, but avoid the priv checks whenever possible
		cmds[#cmds + 1] = { name, def }
	end
	local sorted_mod_cmds = {}
	for modname, cmds in pairs(mod_cmds) do
		table.sort(cmds, function(a, b) return a[1] < b[1] end)
		sorted_mod_cmds[#sorted_mod_cmds + 1] = { modname, cmds }
	end
	table.sort(sorted_mod_cmds, function(a, b) return a[1] < b[1] end)
	mod_cmds = sorted_mod_cmds
end

core.after(0, load_mod_command_tree)

local function build_chatcommands_formspec(name)
	local rows = {}
	rows[1] = "#FFF,0,Command,Parameters"

	for i, data in ipairs(mod_cmds) do
		rows[#rows + 1] = COLOR_BLUE .. ",0," .. formspec_escape(data[1]) .. ","
		for j, cmds in ipairs(data[2]) do
			local has_priv = check_player_privs(name, cmds[2].privs)
			rows[#rows + 1] = ("%s,1,%s,%s"):format(
				has_priv and COLOR_GREEN or COLOR_GRAY,
				cmds[1], formspec_escape(cmds[2].params))
		end
	end

	return LIST_FORMSPEC:format(
			"Available commands: (see also: /help <cmd>)",
			table.concat(rows, ","),
			"Close"
		)
end


-- 	PRIVILEGES FORMSPEC

local function build_privs_formspec(name)
	local privs = {}
	for name, def in pairs(core.registered_privileges) do
		privs[#privs + 1] = { name, def }
	end
	table.sort(privs, function(a, b) return a[1] < b[1] end)

	local rows = {}
	rows[1] = "#FFF,0,Privilege,Description"

	local player_privs = core.get_player_privs(name)
	for i, data in ipairs(privs) do
		rows[#rows + 1] = ("%s,0,%s,%s"):format(
			player_privs[data[1]] and COLOR_GREEN or COLOR_GRAY,
				data[1], formspec_escape(data[2].description))
	end

	return LIST_FORMSPEC:format(
			"Available privileges:",
			table.concat(rows, ","),
			"Close"
		)
end

local help_command = core.registered_chatcommands["help"]
local old_help_func = help_command.func

help_command.func = function(name, param)
	if param == "privs" then
		core.show_formspec(name, "__builtin:help_privs",
			build_privs_formspec(name))
		return
	end
	if param == "" or param == "all" then
		core.show_formspec(name, "__builtin:help_cmds",
			build_chatcommands_formspec(name))
		return 
	end

	old_help_func(name, param)
end

