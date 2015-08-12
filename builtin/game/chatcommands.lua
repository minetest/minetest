-- Minetest: builtin/chatcommands.lua

--
-- Chat command handler
--

core.chatcommands = {}
function core.register_chatcommand(cmd, def)
	def = def or {}
	def.params = def.params or ""
	def.description = def.description or ""
	def.privs = def.privs or {}
	def.mod_origin = core.get_current_modname() or "??"
	core.chatcommands[cmd] = def
end

if core.setting_getbool("mod_profiling") then
	local tracefct = profiling_print_log
	profiling_print_log = nil
	core.register_chatcommand("save_mod_profile",
			{
				params      = "",
				description = "save mod profiling data to logfile " ..
						"(depends on default loglevel)",
				func        = tracefct,
				privs       = { server=true }
			})
end

core.register_on_chat_message(function(name, message)
	local cmd, param = string.match(message, "^/([^ ]+) *(.*)")
	if not param then
		param = ""
	end
	local cmd_def = core.chatcommands[cmd]
	if not cmd_def then
		return false
	end
	local has_privs, missing_privs = core.check_player_privs(name, cmd_def.privs)
	if has_privs then
		core.set_last_run_mod(cmd_def.mod_origin)
		local success, message = cmd_def.func(name, param)
		if message then
			core.chat_send_player(name, message)
		end
	else
		core.chat_send_player(name, "You don't have permission"
				.. " to run this command (missing privileges: "
				.. table.concat(missing_privs, ", ") .. ")")
	end
	return true  -- Handled chat message
end)

--
-- Chat commands
--
core.register_chatcommand("me", {
	params = "<action>",
	description = "chat action (eg. /me orders a pizza)",
	privs = {shout=true},
	func = function(name, param)
		core.chat_send_all("* " .. name .. " " .. param)
	end,
})

core.register_chatcommand("help", {
	privs = {},
	params = "[all/privs/<cmd>]",
	description = "Get help for commands or list privileges",
	func = function(name, param)
		local function format_help_line(cmd, def)
			local msg = "/"..cmd
			if def.params and def.params ~= "" then
				msg = msg .. " " .. def.params
			end
			if def.description and def.description ~= "" then
				msg = msg .. ": " .. def.description
			end
			return msg
		end
		if param == "" then
			local msg = ""
			local cmds = {}
			for cmd, def in pairs(core.chatcommands) do
				if core.check_player_privs(name, def.privs) then
					table.insert(cmds, cmd)
				end
			end
			table.sort(cmds)
			return true, "Available commands: " .. table.concat(cmds, " ") .. "\n"
					.. "Use '/help <cmd>' to get more information,"
					.. " or '/help all' to list everything."
		elseif param == "all" then
			local cmds = {}
			for cmd, def in pairs(core.chatcommands) do
				if core.check_player_privs(name, def.privs) then
					table.insert(cmds, format_help_line(cmd, def))
				end
			end
			table.sort(cmds)
			return true, "Available commands:\n"..table.concat(cmds, "\n")
		elseif param == "privs" then
			local privs = {}
			for priv, def in pairs(core.registered_privileges) do
				table.insert(privs, priv .. ": " .. def.description)
			end
			table.sort(privs)
			return true, "Available privileges:\n"..table.concat(privs, "\n")
		else
			local cmd = param
			local def = core.chatcommands[cmd]
			if not def then
				return false, "Command not available: "..cmd
			else
				return true, format_help_line(cmd, def)
			end
		end
	end,
})

core.register_chatcommand("privs", {
	params = "<name>",
	description = "print out privileges of player",
	func = function(name, param)
		param = (param ~= "" and param or name)
		return true, "Privileges of " .. param .. ": "
			.. core.privs_to_string(
				core.get_player_privs(param), ' ')
	end,
})
core.register_chatcommand("grant", {
	params = "<name> <privilege>|all",
	description = "Give privilege to player",
	func = function(name, param)
		if not core.check_player_privs(name, {privs=true}) and
				not core.check_player_privs(name, {basic_privs=true}) then
			return false, "Your privileges are insufficient."
		end
		local grantname, grantprivstr = string.match(param, "([^ ]+) (.+)")
		if not grantname or not grantprivstr then
			return false, "Invalid parameters (see /help grant)"
		elseif not core.auth_table[grantname] then
			return false, "Player " .. grantname .. " does not exist."
		end
		local grantprivs = core.string_to_privs(grantprivstr)
		if grantprivstr == "all" then
			grantprivs = core.registered_privileges
		end
		local privs = core.get_player_privs(grantname)
		local privs_unknown = ""
		for priv, _ in pairs(grantprivs) do
			if priv ~= "interact" and priv ~= "shout" and
					not core.check_player_privs(name, {privs=true}) then
				return false, "Your privileges are insufficient."
			end
			if not core.registered_privileges[priv] then
				privs_unknown = privs_unknown .. "Unknown privilege: " .. priv .. "\n"
			end
			privs[priv] = true
		end
		if privs_unknown ~= "" then
			return false, privs_unknown
		end
		core.set_player_privs(grantname, privs)
		core.log("action", name..' granted ('..core.privs_to_string(grantprivs, ', ')..') privileges to '..grantname)
		if grantname ~= name then
			core.chat_send_player(grantname, name
					.. " granted you privileges: "
					.. core.privs_to_string(grantprivs, ' '))
		end
		return true, "Privileges of " .. grantname .. ": "
			.. core.privs_to_string(
				core.get_player_privs(grantname), ' ')
	end,
})
core.register_chatcommand("revoke", {
	params = "<name> <privilege>|all",
	description = "Remove privilege from player",
	privs = {},
	func = function(name, param)
		if not core.check_player_privs(name, {privs=true}) and
				not core.check_player_privs(name, {basic_privs=true}) then
			return false, "Your privileges are insufficient."
		end
		local revoke_name, revoke_priv_str = string.match(param, "([^ ]+) (.+)")
		if not revoke_name or not revoke_priv_str then
			return false, "Invalid parameters (see /help revoke)"
		elseif not core.auth_table[revoke_name] then
			return false, "Player " .. revoke_name .. " does not exist."
		end
		local revoke_privs = core.string_to_privs(revoke_priv_str)
		local privs = core.get_player_privs(revoke_name)
		for priv, _ in pairs(revoke_privs) do
			if priv ~= "interact" and priv ~= "shout" and
					not core.check_player_privs(name, {privs=true}) then
				return false, "Your privileges are insufficient."
			end
		end
		if revoke_priv_str == "all" then
			privs = {}
		else
			for priv, _ in pairs(revoke_privs) do
				privs[priv] = nil
			end
		end
		core.set_player_privs(revoke_name, privs)
		core.log("action", name..' revoked ('
				..core.privs_to_string(revoke_privs, ', ')
				..') privileges from '..revoke_name)
		if revoke_name ~= name then
			core.chat_send_player(revoke_name, name
					.. " revoked privileges from you: "
					.. core.privs_to_string(revoke_privs, ' '))
		end
		return true, "Privileges of " .. revoke_name .. ": "
			.. core.privs_to_string(
				core.get_player_privs(revoke_name), ' ')
	end,
})

core.register_chatcommand("setpassword", {
	params = "<name> <password>",
	description = "set given password",
	privs = {password=true},
	func = function(name, param)
		local toname, raw_password = string.match(param, "^([^ ]+) +(.+)$")
		if not toname then
			toname = param:match("^([^ ]+) *$")
			raw_password = nil
		end
		if not toname then
			return false, "Name field required"
		end
		local act_str_past = "?"
		local act_str_pres = "?"
		if not raw_password then
			core.set_player_password(toname, "")
			act_str_past = "cleared"
			act_str_pres = "clears"
		else
			core.set_player_password(toname,
					core.get_password_hash(toname,
							raw_password))
			act_str_past = "set"
			act_str_pres = "sets"
		end
		if toname ~= name then
			core.chat_send_player(toname, "Your password was "
					.. act_str_past .. " by " .. name)
		end

		core.log("action", name .. " " .. act_str_pres
		.. " password of " .. toname .. ".")

		return true, "Password of player \"" .. toname .. "\" " .. act_str_past
	end,
})

core.register_chatcommand("clearpassword", {
	params = "<name>",
	description = "set empty password",
	privs = {password=true},
	func = function(name, param)
		local toname = param
		if toname == "" then
			return false, "Name field required"
		end
		core.set_player_password(toname, '')

		core.log("action", name .. " clears password of " .. toname .. ".")

		return true, "Password of player \"" .. toname .. "\" cleared"
	end,
})

core.register_chatcommand("auth_reload", {
	params = "",
	description = "reload authentication data",
	privs = {server=true},
	func = function(name, param)
		local done = core.auth_reload()
		return done, (done and "Done." or "Failed.")
	end,
})

core.register_chatcommand("teleport", {
	params = "<X>,<Y>,<Z> | <to_name> | <name> <X>,<Y>,<Z> | <name> <to_name>",
	description = "teleport to given position",
	privs = {teleport=true},
	func = function(name, param)
		-- Returns (pos, true) if found, otherwise (pos, false)
		local function find_free_position_near(pos)
			local tries = {
				{x=1,y=0,z=0},
				{x=-1,y=0,z=0},
				{x=0,y=0,z=1},
				{x=0,y=0,z=-1},
			}
			for _, d in ipairs(tries) do
				local p = {x = pos.x+d.x, y = pos.y+d.y, z = pos.z+d.z}
				local n = core.get_node_or_nil(p)
				if n and n.name then
					local def = core.registered_nodes[n.name]
					if def and not def.walkable then
						return p, true
					end
				end
			end
			return pos, false
		end

		local teleportee = nil
		local p = {}
		p.x, p.y, p.z = string.match(param, "^([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
		p.x = tonumber(p.x)
		p.y = tonumber(p.y)
		p.z = tonumber(p.z)
		teleportee = core.get_player_by_name(name)
		if teleportee and p.x and p.y and p.z then
			teleportee:setpos(p)
			return true, "Teleporting to "..core.pos_to_string(p)
		end

		local teleportee = nil
		local p = nil
		local target_name = nil
		target_name = param:match("^([^ ]+)$")
		teleportee = core.get_player_by_name(name)
		if target_name then
			local target = core.get_player_by_name(target_name)
			if target then
				p = target:getpos()
			end
		end
		if teleportee and p then
			p = find_free_position_near(p)
			teleportee:setpos(p)
			return true, "Teleporting to " .. target_name
					.. " at "..core.pos_to_string(p)
		end

		if not core.check_player_privs(name, {bring=true}) then
			return false, "You don't have permission to teleport other players (missing bring privilege)"
		end

		local teleportee = nil
		local p = {}
		local teleportee_name = nil
		teleportee_name, p.x, p.y, p.z = param:match(
				"^([^ ]+) +([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
		p.x, p.y, p.z = tonumber(p.x), tonumber(p.y), tonumber(p.z)
		if teleportee_name then
			teleportee = core.get_player_by_name(teleportee_name)
		end
		if teleportee and p.x and p.y and p.z then
			teleportee:setpos(p)
			return true, "Teleporting " .. teleportee_name
					.. " to " .. core.pos_to_string(p)
		end

		local teleportee = nil
		local p = nil
		local teleportee_name = nil
		local target_name = nil
		teleportee_name, target_name = string.match(param, "^([^ ]+) +([^ ]+)$")
		if teleportee_name then
			teleportee = core.get_player_by_name(teleportee_name)
		end
		if target_name then
			local target = core.get_player_by_name(target_name)
			if target then
				p = target:getpos()
			end
		end
		if teleportee and p then
			p = find_free_position_near(p)
			teleportee:setpos(p)
			return true, "Teleporting " .. teleportee_name
					.. " to " .. target_name
					.. " at " .. core.pos_to_string(p)
		end

		return false, 'Invalid parameters ("' .. param
				.. '") or player not found (see /help teleport)'
	end,
})

core.register_chatcommand("set", {
	params = "[-n] <name> <value> | <name>",
	description = "set or read server configuration setting",
	privs = {server=true},
	func = function(name, param)
		local arg, setname, setvalue = string.match(param, "(-[n]) ([^ ]+) (.+)")
		if arg and arg == "-n" and setname and setvalue then
			core.setting_set(setname, setvalue)
			return true, setname .. " = " .. setvalue
		end
		local setname, setvalue = string.match(param, "([^ ]+) (.+)")
		if setname and setvalue then
			if not core.setting_get(setname) then
				return false, "Failed. Use '/set -n <name> <value>' to create a new setting."
			end
			core.setting_set(setname, setvalue)
			return true, setname .. " = " .. setvalue
		end
		local setname = string.match(param, "([^ ]+)")
		if setname then
			local setvalue = core.setting_get(setname)
			if not setvalue then
				setvalue = "<not set>"
			end
			return true, setname .. " = " .. setvalue
		end
		return false, "Invalid parameters (see /help set)."
	end,
})

core.register_chatcommand("deleteblocks", {
	params = "(here [radius]) | (<pos1> <pos2>)",
	description = "delete map blocks contained in area pos1 to pos2",
	privs = {server=true},
	func = function(name, param)
		local p1 = {}
		local p2 = {}
		local args = param:split(" ")
		if args[1] == "here" then
			local player = core.get_player_by_name(name)
			if player == nil then
				core.log("error", "player is nil")
				return false, "Unable to get current position; player is nil"
			end
			p1 = player:getpos()
			p2 = p1

			if #args >= 2 then
				local radius = tonumber(args[2]) or 0
				p1 = vector.add(p1, radius)
				p2 = vector.subtract(p2, radius)
			end
		else
			local pos1, pos2 = unpack(param:split(") ("))
			if pos1 == nil or pos2 == nil then
				return false, "Incorrect area format. Expected: (x1,y1,z1) (x2,y2,z2)"
			end

			p1 = core.string_to_pos(pos1 .. ")")
			p2 = core.string_to_pos("(" .. pos2)

			if p1 == nil or p2 == nil then
				return false, "Incorrect area format. Expected: (x1,y1,z1) (x2,y2,z2)"
			end
		end

		if core.delete_area(p1, p2) then
			return true, "Successfully cleared area ranging from " ..
				core.pos_to_string(p1, 1) .. " to " .. core.pos_to_string(p2, 1)
		else
			return false, "Failed to clear one or more blocks in area"
		end
	end,
})

core.register_chatcommand("mods", {
	params = "",
	description = "List mods installed on the server",
	privs = {},
	func = function(name, param)
		return true, table.concat(core.get_modnames(), ", ")
	end,
})

local function handle_give_command(cmd, giver, receiver, stackstring)
	core.log("action", giver .. " invoked " .. cmd
			.. ', stackstring="' .. stackstring .. '"')
	local itemstack = ItemStack(stackstring)
	if itemstack:is_empty() then
		return false, "Cannot give an empty item"
	elseif not itemstack:is_known() then
		return false, "Cannot give an unknown item"
	end
	local receiverref = core.get_player_by_name(receiver)
	if receiverref == nil then
		return false, receiver .. " is not a known player"
	end
	local leftover = receiverref:get_inventory():add_item("main", itemstack)
	local partiality
	if leftover:is_empty() then
		partiality = ""
	elseif leftover:get_count() == itemstack:get_count() then
		partiality = "could not be "
	else
		partiality = "partially "
	end
	-- The actual item stack string may be different from what the "giver"
	-- entered (e.g. big numbers are always interpreted as 2^16-1).
	stackstring = itemstack:to_string()
	if giver == receiver then
		return true, ("%q %sadded to inventory.")
				:format(stackstring, partiality)
	else
		core.chat_send_player(receiver, ("%q %sadded to inventory.")
				:format(stackstring, partiality))
		return true, ("%q %sadded to %s's inventory.")
				:format(stackstring, partiality, receiver)
	end
end

core.register_chatcommand("give", {
	params = "<name> <ItemString>",
	description = "give item to player",
	privs = {give=true},
	func = function(name, param)
		local toname, itemstring = string.match(param, "^([^ ]+) +(.+)$")
		if not toname or not itemstring then
			return false, "Name and ItemString required"
		end
		return handle_give_command("/give", name, toname, itemstring)
	end,
})

core.register_chatcommand("giveme", {
	params = "<ItemString>",
	description = "give item to yourself",
	privs = {give=true},
	func = function(name, param)
		local itemstring = string.match(param, "(.+)$")
		if not itemstring then
			return false, "ItemString required"
		end
		return handle_give_command("/giveme", name, name, itemstring)
	end,
})

core.register_chatcommand("spawnentity", {
	params = "<EntityName> [<X>,<Y>,<Z>]",
	description = "Spawn entity at given (or your) position",
	privs = {give=true, interact=true},
	func = function(name, param)
		local entityname, p = string.match(param, "^([^ ]+) *(.*)$")
		if not entityname then
			return false, "EntityName required"
		end
		core.log("action", ("%s invokes /spawnentity, entityname=%q")
				:format(name, entityname))
		local player = core.get_player_by_name(name)
		if player == nil then
			core.log("error", "Unable to spawn entity, player is nil")
			return false, "Unable to spawn entity, player is nil"
		end
		if p == "" then
			p = player:getpos()
		else
			p = core.string_to_pos(p)
			if p == nil then
				return false, "Invalid parameters ('" .. param .. "')"
			end
		end
		p.y = p.y + 1
		core.add_entity(p, entityname)
		return true, ("%q spawned."):format(entityname)
	end,
})

core.register_chatcommand("pulverize", {
	params = "",
	description = "Destroy item in hand",
	func = function(name, param)
		local player = core.get_player_by_name(name)
		if not player then
			core.log("error", "Unable to pulverize, no player.")
			return false, "Unable to pulverize, no player."
		end
		if player:get_wielded_item():is_empty() then
			return false, "Unable to pulverize, no item in hand."
		end
		player:set_wielded_item(nil)
		return true, "An item was pulverized."
	end,
})

-- Key = player name
core.rollback_punch_callbacks = {}

core.register_on_punchnode(function(pos, node, puncher)
	local name = puncher:get_player_name()
	if core.rollback_punch_callbacks[name] then
		core.rollback_punch_callbacks[name](pos, node, puncher)
		core.rollback_punch_callbacks[name] = nil
	end
end)

core.register_chatcommand("rollback_check", {
	params = "[<range>] [<seconds>] [limit]",
	description = "Check who has last touched a node or near it,"
			.. " max. <seconds> ago (default range=0,"
			.. " seconds=86400=24h, limit=5)",
	privs = {rollback=true},
	func = function(name, param)
		if not core.setting_getbool("enable_rollback_recording") then
			return false, "Rollback functions are disabled."
		end
		local range, seconds, limit =
			param:match("(%d+) *(%d*) *(%d*)")
		range = tonumber(range) or 0
		seconds = tonumber(seconds) or 86400
		limit = tonumber(limit) or 5
		if limit > 100 then
			return false, "That limit is too high!"
		end

		core.rollback_punch_callbacks[name] = function(pos, node, puncher)
			local name = puncher:get_player_name()
			core.chat_send_player(name, "Checking " .. core.pos_to_string(pos) .. "...")
			local actions = core.rollback_get_node_actions(pos, range, seconds, limit)
			if not actions then
				core.chat_send_player(name, "Rollback functions are disabled")
				return
			end
			local num_actions = #actions
			if num_actions == 0 then
				core.chat_send_player(name, "Nobody has touched"
						.. " the specified location in "
						.. seconds .. " seconds")
				return
			end
			local time = os.time()
			for i = num_actions, 1, -1 do
				local action = actions[i]
				core.chat_send_player(name,
					("%s %s %s -> %s %d seconds ago.")
						:format(
							core.pos_to_string(action.pos),
							action.actor,
							action.oldnode.name,
							action.newnode.name,
							time - action.time))
			end
		end

		return true, "Punch a node (range=" .. range .. ", seconds="
				.. seconds .. "s, limit=" .. limit .. ")"
	end,
})

core.register_chatcommand("rollback", {
	params = "<player name> [<seconds>] | :<actor> [<seconds>]",
	description = "revert actions of a player; default for <seconds> is 60",
	privs = {rollback=true},
	func = function(name, param)
		if not core.setting_getbool("enable_rollback_recording") then
			return false, "Rollback functions are disabled."
		end
		local target_name, seconds = string.match(param, ":([^ ]+) *(%d*)")
		if not target_name then
			local player_name = nil
			player_name, seconds = string.match(param, "([^ ]+) *(%d*)")
			if not player_name then
				return false, "Invalid parameters. See /help rollback"
						.. " and /help rollback_check."
			end
			target_name = "player:"..player_name
		end
		seconds = tonumber(seconds) or 60
		core.chat_send_player(name, "Reverting actions of "
				.. target_name .. " since "
				.. seconds .. " seconds.")
		local success, log = core.rollback_revert_actions_by(
				target_name, seconds)
		local response = ""
		if #log > 100 then
			response = "(log is too long to show)\n"
		else
			for _, line in pairs(log) do
				response = response .. line .. "\n"
			end
		end
		response = response .. "Reverting actions "
				.. (success and "succeeded." or "FAILED.")
		return success, response
	end,
})

core.register_chatcommand("status", {
	description = "Print server status",
	func = function(name, param)
		return true, core.get_server_status()
	end,
})

core.register_chatcommand("time", {
	params = "<0..23>:<0..59> | <0..24000>",
	description = "set time of day",
	privs = {},
	func = function(name, param)
		if param == "" then
			local current_time = math.floor(core.get_timeofday() * 1440)
			local minutes = current_time % 60
			local hour = (current_time - minutes) / 60
			return true, ("Current time is %d:%02d"):format(hour, minutes)
		end
		local player_privs = minetest.get_player_privs(name)
		if not player_privs.settime then
			return false, "You don't have permission to run this command " ..
				"(missing privilege: settime)."
		end
		local hour, minute = param:match("^(%d+):(%d+)$")
		if not hour then
			local new_time = tonumber(param)
			if not new_time then
				return false, "Invalid time."
			end
			-- Backward compatibility.
			core.set_timeofday((new_time % 24000) / 24000)
			core.log("action", name .. " sets time to " .. new_time)
			return true, "Time of day changed."
		end
		hour = tonumber(hour)
		minute = tonumber(minute)
		if hour < 0 or hour > 23 then
			return false, "Invalid hour (must be between 0 and 23 inclusive)."
		elseif minute < 0 or minute > 59 then
			return false, "Invalid minute (must be between 0 and 59 inclusive)."
		end
		core.set_timeofday((hour * 60 + minute) / 1440)
		core.log("action", ("%s sets time to %d:%02d"):format(name, hour, minute))
		return true, "Time of day changed."
	end,
})

core.register_chatcommand("shutdown", {
	description = "shutdown server",
	privs = {server=true},
	func = function(name, param)
		core.log("action", name .. " shuts down server")
		core.request_shutdown()
		core.chat_send_all("*** Server shutting down (operator request).")
	end,
})

core.register_chatcommand("ban", {
	params = "<name>",
	description = "Ban IP of player",
	privs = {ban=true},
	func = function(name, param)
		if param == "" then
			return true, "Ban list: " .. core.get_ban_list()
		end
		if not core.get_player_by_name(param) then
			return false, "No such player."
		end
		if not core.ban_player(param) then
			return false, "Failed to ban player."
		end
		local desc = core.get_ban_description(param)
		core.log("action", name .. " bans " .. desc .. ".")
		return true, "Banned " .. desc .. "."
	end,
})

core.register_chatcommand("unban", {
	params = "<name/ip>",
	description = "remove IP ban",
	privs = {ban=true},
	func = function(name, param)
		if not core.unban_player_or_ip(param) then
			return false, "Failed to unban player/IP."
		end
		core.log("action", name .. " unbans " .. param)
		return true, "Unbanned " .. param
	end,
})

core.register_chatcommand("kick", {
	params = "<name> [reason]",
	description = "kick a player",
	privs = {kick=true},
	func = function(name, param)
		local tokick, reason = param:match("([^ ]+) (.+)")
		tokick = tokick or param
		if not core.kick_player(tokick, reason) then
			return false, "Failed to kick player " .. tokick
		end
		local log_reason = ""
		if reason then
			log_reason = " with reason \"" .. reason .. "\""
		end
		core.log("action", name .. " kicks " .. tokick .. log_reason)
		return true, "Kicked " .. tokick
	end,
})

core.register_chatcommand("clearobjects", {
	description = "clear all objects in world",
	privs = {server=true},
	func = function(name, param)
		core.log("action", name .. " clears all objects.")
		core.chat_send_all("Clearing all objects.  This may take long."
				.. "  You may experience a timeout.  (by "
				.. name .. ")")
		core.clear_objects()
		core.log("action", "Object clearing done.")
		core.chat_send_all("*** Cleared all objects.")
	end,
})

core.register_chatcommand("msg", {
	params = "<name> <message>",
	description = "Send a private message",
	privs = {shout=true},
	func = function(name, param)
		local sendto, message = param:match("^(%S+)%s(.+)$")
		if not sendto then
			return false, "Invalid usage, see /help msg."
		end
		if not core.get_player_by_name(sendto) then
			return false, "The player " .. sendto
					.. " is not online."
		end
		core.log("action", "PM from " .. name .. " to " .. sendto
				.. ": " .. message)
		core.chat_send_player(sendto, "PM from " .. name .. ": "
				.. message)
		return true, "Message sent."
	end,
})

core.register_chatcommand("last-login", {
	params = "[name]",
	description = "Get the last login time of a player",
	func = function(name, param)
		if param == "" then
			param = name
		end
		local pauth = core.get_auth_handler().get_auth(param)
		if pauth and pauth.last_login then
			-- Time in UTC, ISO 8601 format
			return true, "Last login time was " ..
				os.date("!%Y-%m-%dT%H:%M:%SZ", pauth.last_login)
		end
		return false, "Last login time is unknown"
	end,
})
