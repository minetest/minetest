local COLOR_BLUE = "#7AF"
local COLOR_GREEN = "#7F7"
local COLOR_GRAY = "#BBB"
local COLOR_DARK_GREEN = "#25C191"

local LIST_FORMSPEC = [[
		size[13,6.5]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,5.5;list;%s;0]
		button_exit[5,6;3,1;quit;%s]
	]]

local LIST_FORMSPEC_DESCRIPTION = [[
		size[13,7.5]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,4.8;list;%s;%i]
		box[0,5.5;12.8,1.5;#000]
		textarea[0.3,5.5;13.05,1.9;;;%s]
		button_exit[5,7;3,1;quit;%s]
	]]

local LIST_FORMSPEC_MODSLIST = [[
		size[6,8]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text]
		table[0,0.5;6,7;modslist;%s;0]
		button_exit[1.5,7.5;3,1;quit;%s]
	]]

local F = core.formspec_escape
local S = core.get_translator("__builtin")


-- CHAT COMMANDS FORMSPEC

local mod_cmds = {}

local function load_mod_command_tree()
	mod_cmds = {}

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

local function build_chatcommands_formspec(name, sel, copy)
	local rows = {}
	rows[1] = "#FFF,0,"..F(S("Command"))..","..F(S("Parameters"))

	local description = S("For more information, click on "
		.. "any entry in the list.").. "\n" ..
		S("Double-click to copy the entry to the chat history.")

	local privs = core.get_player_privs(name)
	for i, data in ipairs(mod_cmds) do
		rows[#rows + 1] = COLOR_BLUE .. ",0," .. F(data[1]) .. ","
		for j, cmds in ipairs(data[2]) do
			local has_priv = privs[cmds[2].privs]
			rows[#rows + 1] = ("%s,1,%s,%s"):format(
				has_priv and COLOR_GREEN or COLOR_GRAY,
				cmds[1], F(cmds[2].params))
			if sel == #rows then
				description = cmds[2].description
				if copy then
					core.chat_send_player(name, S("Command: @1 @2",
						core.colorize("#0FF", "/" .. cmds[1]), cmds[2].params))
				end
			end
		end
	end

	return LIST_FORMSPEC_DESCRIPTION:format(
			F(S("Available commands: (see also: /help <cmd>)")),
			table.concat(rows, ","), sel or 0,
			F(description), F(S("Close"))
		)
end


-- PRIVILEGES FORMSPEC

local function build_privs_formspec(name)
	local privs = {}
	for priv_name, def in pairs(core.registered_privileges) do
		privs[#privs + 1] = { priv_name, def }
	end
	table.sort(privs, function(a, b) return a[1] < b[1] end)

	local rows = {}
	rows[1] = "#FFF,0,"..F(S("Privilege"))..","..F(S("Description"))

	local player_privs = core.get_player_privs(name)
	for i, data in ipairs(privs) do
		rows[#rows + 1] = ("%s,0,%s,%s"):format(
			player_privs[data[1]] and COLOR_GREEN or COLOR_GRAY,
				data[1], F(data[2].description))
	end

	return LIST_FORMSPEC:format(
			F(S("Available privileges:")),
			table.concat(rows, ","),
			F(S("Close"))
		)
end


-- If 'modname' contains inside some modpack, returns name of that modpack, otherwise nil.
local function get_modpack_name(modname)
	local path = core.get_modpath(modname)
	local dirs = path:split(DIR_DELIM)

	if dirs[#dirs-2] == "mods" then
		return dirs[#dirs-1]
	else
		return
	end
end


-- MODS FORMSPEC

local modslist_cache = ""

function build_mods_formspec()
	if modslist_cache ~= "" then
		return modslist_cache
	end

	local modnames = core.get_modnames()
	local mps_and_mods = {}
	local found_mps = {}
	local mps_mods = {}

	local rows = {}
	rows[1] = "#FFF,0," .. F(S("Modpack or mod name"))

	for _, mname in ipairs(modnames) do
		local mp_name = get_modpack_name(mname)

		if mp_name then
			if table.indexof(found_mps, mp_name) == -1 then
				table.insert(mps_and_mods, {mp_name, "modpack"})
				table.insert(found_mps, mp_name)
			end

			mps_mods[mp_name] = mps_mods[mp_name] or {}
			table.insert(mps_mods[mp_name], mname)
		else
			table.insert(mps_and_mods, {mname, "mod"})
		end
	end

	table.sort(mps_and_mods, function(a, b) return a[1] < b[1] end)

	for _, v in ipairs(mps_and_mods) do
		if v[2] == "mod" then
			table.insert(rows, ("%s, 0, %s"):format(COLOR_GREEN, v[1]))
		else
			table.insert(rows, ("%s, 0, %s"):format(COLOR_DARK_GREEN, v[1]))
			table.sort(mps_mods[v[1]], function(a, b) return a < b end)

			for _, modname in ipairs(mps_mods[v[1]]) do
				table.insert(rows, ("%s, 1, %s"):format(COLOR_GREEN, modname))
			end
		end
	end

	local mods_formspec_s = LIST_FORMSPEC_MODSLIST:format(
		F(S("List of installed mods:")),
		table.concat(rows, ","),
		F(S("Close"))
	)

	modslist_cache = mods_formspec_s

	return mods_formspec_s
end

-- DETAILED CHAT COMMAND INFORMATION

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname ~= "__builtin:help_cmds" or fields.quit then
		return
	end

	local event = core.explode_table_event(fields.list)
	if event.type ~= "INV" then
		local name = player:get_player_name()
		core.show_formspec(name, "__builtin:help_cmds",
			build_chatcommands_formspec(name, event.row, event.type == "DCL"))
	end
end)

function core.show_general_help_formspec(name)
	core.show_formspec(name, "__builtin:help_cmds",
		build_chatcommands_formspec(name))
end

function core.show_privs_help_formspec(name)
	core.show_formspec(name, "__builtin:help_privs",
		build_privs_formspec(name))
end
