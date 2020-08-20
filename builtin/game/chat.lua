-- Minetest: builtin/game/chat.lua

-- Helper function that implements search and replace without pattern matching
-- Returns the string and a boolean indicating whether or not the string was modified
local function safe_gsub(s, replace, with)
	local i1, i2 = s:find(replace, 1, true)
	if not i1 then
		return s, false
	end

	return s:sub(1, i1 - 1) .. with .. s:sub(i2 + 1), true
end

--
-- Chat message formatter
--

-- Implemented in Lua to allow redefinition
function core.format_chat_message(name, message)
	local error_str = "Invalid chat message format - missing %s"
	local str = core.settings:get("chat_message_format")
	local replaced

	-- Name
	str, replaced = safe_gsub(str, "@name", name)
	if not replaced then
		error(error_str:format("@name"), 2)
	end

	-- Timestamp
	str = safe_gsub(str, "@timestamp", os.date("%H:%M:%S", os.time()))

	-- Insert the message into the string only after finishing all other processing
	str, replaced = safe_gsub(str, "@message", message)
	if not replaced then
		error(error_str:format("@message"), 2)
	end

	return str
end

--
-- Chat command handler
--

core.chatcommands = core.registered_chatcommands -- BACKWARDS COMPATIBILITY

core.register_on_chat_message(function(name, message)
	if message:sub(1,1) ~= "/" then
		return
	end

	local cmd, param = string.match(message, "^/([^ ]+) *(.*)")
	if not cmd then
		core.chat_send_player(name, "-!- Empty command")
		return true
	end

	param = param or ""

	local cmd_def = core.registered_chatcommands[cmd]
	if not cmd_def then
		core.chat_send_player(name, "-!- Invalid command: " .. cmd)
		return true
	end
	local has_privs, missing_privs = core.check_player_privs(name, cmd_def.privs)
	if has_privs then
		core.set_last_run_mod(cmd_def.mod_origin)
		local _, result = cmd_def.func(name, param)
		if result then
			core.chat_send_player(name, result)
		end
	else
		core.chat_send_player(name, "You don't have permission"
				.. " to run this command (missing privileges: "
				.. table.concat(missing_privs, ", ") .. ")")
	end
	return true  -- Handled chat message
end)

if core.settings:get_bool("profiler.load") then
	-- Run after register_chatcommand and its register_on_chat_message
	-- Before any chatcommands that should be profiled
	profiler.init_chatcommand()
end

-- Parses a "range" string in the format of "here (number)" or
-- "(x1, y1, z1) (x2, y2, z2)", returning two position vectors
local function parse_range_str(player_name, str)
	local p1, p2
	local args = str:split(" ")

	if args[1] == "here" then
		p1, p2 = core.get_player_radius_area(player_name, tonumber(args[2]))
		if p1 == nil then
			return false, "Unable to get player " .. player_name .. " position"
		end
	else
		p1, p2 = core.string_to_area(str)
		if p1 == nil then
			return false, "Incorrect area format. Expected: (x1,y1,z1) (x2,y2,z2)"
		end
	end

	return p1, p2
end

--
-- Chat commands
--
core.register_chatcommand("me", {
	params = "<action>",
	description = "Show chat action (e.g., '/me orders a pizza' displays"
			.. " '<player name> orders a pizza')",
	privs = {shout=true},
	func = function(name, param)
		core.chat_send_all("* " .. name .. " " .. param)
		return true
	end,
})

core.register_chatcommand("admin", {
	description = "Show the name of the server owner",
	func = function(name)
		local admin = core.settings:get("name")
		if admin then
			return true, "The administrator of this server is " .. admin .. "."
		else
			return false, "There's no administrator named in the config file."
		end
	end,
})

core.register_chatcommand("privs", {
	params = "[<name>]",
	description = "Show privileges of yourself or another player",
	func = function(caller, param)
		param = param:trim()
		local name = (param ~= "" and param or caller)
		if not core.player_exists(name) then
			return false, "Player " .. name .. " does not exist."
		end
		return true, "Privileges of " .. name .. ": "
			.. core.privs_to_string(
				core.get_player_privs(name), ", ")
	end,
})

core.register_chatcommand("haspriv", {
	params = "<privilege>",
	description = "Return list of all online players with privilege.",
	privs = {basic_privs = true},
	func = function(caller, param)
		param = param:trim()
		if param == "" then
			return false, "Invalid parameters (see /help haspriv)"
		end
		if not core.registered_privileges[param] then
			return false, "Unknown privilege!"
		end
		local privs = core.string_to_privs(param)
		local players_with_priv = {}
		for _, player in pairs(core.get_connected_players()) do
			local player_name = player:get_player_name()
			if core.check_player_privs(player_name, privs) then
				table.insert(players_with_priv, player_name)
			end
		end
		return true, "Players online with the \"" .. param .. "\" privilege: " ..
			table.concat(players_with_priv, ", ")
	end
})

local function handle_grant_command(caller, grantname, grantprivstr)
	local caller_privs = core.get_player_privs(caller)
	if not (caller_privs.privs or caller_privs.basic_privs) then
		return false, "Your privileges are insufficient."
	end

	if not core.get_auth_handler().get_auth(grantname) then
		return false, "Player " .. grantname .. " does not exist."
	end
	local grantprivs = core.string_to_privs(grantprivstr)
	if grantprivstr == "all" then
		grantprivs = core.registered_privileges
	end
	local privs = core.get_player_privs(grantname)
	local privs_unknown = ""
	local basic_privs =
		core.string_to_privs(core.settings:get("basic_privs") or "interact,shout")
	for priv, _ in pairs(grantprivs) do
		if not basic_privs[priv] and not caller_privs.privs then
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
	for priv, _ in pairs(grantprivs) do
		-- call the on_grant callbacks
		core.run_priv_callbacks(grantname, priv, caller, "grant")
	end
	core.set_player_privs(grantname, privs)
	core.log("action", caller..' granted ('..core.privs_to_string(grantprivs, ', ')..') privileges to '..grantname)
	if grantname ~= caller then
		core.chat_send_player(grantname, caller
				.. " granted you privileges: "
				.. core.privs_to_string(grantprivs, ' '))
	end
	return true, "Privileges of " .. grantname .. ": "
		.. core.privs_to_string(
			core.get_player_privs(grantname), ' ')
end

core.register_chatcommand("grant", {
	params = "<name> (<privilege> | all)",
	description = "Give privileges to player",
	func = function(name, param)
		local grantname, grantprivstr = string.match(param, "([^ ]+) (.+)")
		if not grantname or not grantprivstr then
			return false, "Invalid parameters (see /help grant)"
		end
		return handle_grant_command(name, grantname, grantprivstr)
	end,
})

core.register_chatcommand("grantme", {
	params = "<privilege> | all",
	description = "Grant privileges to yourself",
	func = function(name, param)
		if param == "" then
			return false, "Invalid parameters (see /help grantme)"
		end
		return handle_grant_command(name, name, param)
	end,
})

local function handle_revoke_command(caller, revokename, revokeprivstr)
	local caller_privs = core.get_player_privs(caller)
	if not (caller_privs.privs or caller_privs.basic_privs) then
		return false, "Your privileges are insufficient."
	end

	if not core.get_auth_handler().get_auth(revokename) then
		return false, "Player " .. revokename .. " does not exist."
	end

	local revokeprivs = core.string_to_privs(revokeprivstr)
	local privs = core.get_player_privs(revokename)
	local basic_privs =
		core.string_to_privs(core.settings:get("basic_privs") or "interact,shout")
	for priv, _ in pairs(revokeprivs) do
		if not basic_privs[priv] and not caller_privs.privs then
			return false, "Your privileges are insufficient."
		end
	end

	if revokeprivstr == "all" then
		revokeprivs = privs
		privs = {}
	else
		for priv, _ in pairs(revokeprivs) do
			privs[priv] = nil
		end
	end

	for priv, _ in pairs(revokeprivs) do
		-- call the on_revoke callbacks
		core.run_priv_callbacks(revokename, priv, caller, "revoke")
	end

	core.set_player_privs(revokename, privs)
	core.log("action", caller..' revoked ('
			..core.privs_to_string(revokeprivs, ', ')
			..') privileges from '..revokename)
	if revokename ~= caller then
		core.chat_send_player(revokename, caller
			.. " revoked privileges from you: "
			.. core.privs_to_string(revokeprivs, ' '))
	end
	return true, "Privileges of " .. revokename .. ": "
		.. core.privs_to_string(
			core.get_player_privs(revokename), ' ')
end

core.register_chatcommand("revoke", {
	params = "<name> (<privilege> | all)",
	description = "Remove privileges from player",
	privs = {},
	func = function(name, param)
		local revokename, revokeprivstr = string.match(param, "([^ ]+) (.+)")
		if not revokename or not revokeprivstr then
			return false, "Invalid parameters (see /help revoke)"
		end
		return handle_revoke_command(name, revokename, revokeprivstr)
	end,
})

core.register_chatcommand("revokeme", {
	params = "<privilege> | all",
	description = "Revoke privileges from yourself",
	privs = {},
	func = function(name, param)
		if param == "" then
			return false, "Invalid parameters (see /help revokeme)"
		end
		return handle_revoke_command(name, name, param)
	end,
})

core.register_chatcommand("setpassword", {
	params = "<name> <password>",
	description = "Set player's password",
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

		local act_str_past, act_str_pres
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

		core.log("action", name .. " " .. act_str_pres ..
				" password of " .. toname .. ".")

		return true, "Password of player \"" .. toname .. "\" " .. act_str_past
	end,
})

core.register_chatcommand("clearpassword", {
	params = "<name>",
	description = "Set empty password for a player",
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
	description = "Reload authentication data",
	privs = {server=true},
	func = function(name, param)
		local done = core.auth_reload()
		return done, (done and "Done." or "Failed.")
	end,
})

core.register_chatcommand("remove_player", {
	params = "<name>",
	description = "Remove a player's data",
	privs = {server=true},
	func = function(name, param)
		local toname = param
		if toname == "" then
			return false, "Name field required"
		end

		local rc = core.remove_player(toname)

		if rc == 0 then
			core.log("action", name .. " removed player data of " .. toname .. ".")
			return true, "Player \"" .. toname .. "\" removed."
		elseif rc == 1 then
			return true, "No such player \"" .. toname .. "\" to remove."
		elseif rc == 2 then
			return true, "Player \"" .. toname .. "\" is connected, cannot remove."
		end

		return false, "Unhandled remove_player return code " .. rc .. ""
	end,
})

core.register_chatcommand("teleport", {
	params = "<X>,<Y>,<Z> | <to_name> | (<name> <X>,<Y>,<Z>) | (<name> <to_name>)",
	description = "Teleport to position or player",
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

		local p = {}
		p.x, p.y, p.z = string.match(param, "^([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
		p.x = tonumber(p.x)
		p.y = tonumber(p.y)
		p.z = tonumber(p.z)
		if p.x and p.y and p.z then

			local lm = 31000
			if p.x < -lm or p.x > lm or p.y < -lm or p.y > lm or p.z < -lm or p.z > lm then
				return false, "Cannot teleport out of map bounds!"
			end
			local teleportee = core.get_player_by_name(name)
			if teleportee then
				if teleportee:get_attach() then
					return false, "Can't teleport, you're attached to an object!"
				end
				teleportee:set_pos(p)
				return true, "Teleporting to "..core.pos_to_string(p)
			end
		end

		local target_name = param:match("^([^ ]+)$")
		local teleportee = core.get_player_by_name(name)

		p = nil
		if target_name then
			local target = core.get_player_by_name(target_name)
			if target then
				p = target:get_pos()
			end
		end

		if teleportee and p then
			if teleportee:get_attach() then
				return false, "Can't teleport, you're attached to an object!"
			end
			p = find_free_position_near(p)
			teleportee:set_pos(p)
			return true, "Teleporting to " .. target_name
					.. " at "..core.pos_to_string(p)
		end

		if not core.check_player_privs(name, {bring=true}) then
			return false, "You don't have permission to teleport other players (missing bring privilege)"
		end

		teleportee = nil
		p = {}
		local teleportee_name
		teleportee_name, p.x, p.y, p.z = param:match(
				"^([^ ]+) +([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
		p.x, p.y, p.z = tonumber(p.x), tonumber(p.y), tonumber(p.z)
		if teleportee_name then
			teleportee = core.get_player_by_name(teleportee_name)
		end
		if teleportee and p.x and p.y and p.z then
			if teleportee:get_attach() then
				return false, "Can't teleport, player is attached to an object!"
			end
			teleportee:set_pos(p)
			return true, "Teleporting " .. teleportee_name
					.. " to " .. core.pos_to_string(p)
		end

		teleportee = nil
		p = nil
		teleportee_name, target_name = string.match(param, "^([^ ]+) +([^ ]+)$")
		if teleportee_name then
			teleportee = core.get_player_by_name(teleportee_name)
		end
		if target_name then
			local target = core.get_player_by_name(target_name)
			if target then
				p = target:get_pos()
			end
		end
		if teleportee and p then
			if teleportee:get_attach() then
				return false, "Can't teleport, player is attached to an object!"
			end
			p = find_free_position_near(p)
			teleportee:set_pos(p)
			return true, "Teleporting " .. teleportee_name
					.. " to " .. target_name
					.. " at " .. core.pos_to_string(p)
		end

		return false, 'Invalid parameters ("' .. param
				.. '") or player not found (see /help teleport)'
	end,
})

core.register_chatcommand("set", {
	params = "([-n] <name> <value>) | <name>",
	description = "Set or read server configuration setting",
	privs = {server=true},
	func = function(name, param)
		local arg, setname, setvalue = string.match(param, "(-[n]) ([^ ]+) (.+)")
		if arg and arg == "-n" and setname and setvalue then
			core.settings:set(setname, setvalue)
			return true, setname .. " = " .. setvalue
		end

		setname, setvalue = string.match(param, "([^ ]+) (.+)")
		if setname and setvalue then
			if not core.settings:get(setname) then
				return false, "Failed. Use '/set -n <name> <value>' to create a new setting."
			end
			core.settings:set(setname, setvalue)
			return true, setname .. " = " .. setvalue
		end

		setname = string.match(param, "([^ ]+)")
		if setname then
			setvalue = core.settings:get(setname)
			if not setvalue then
				setvalue = "<not set>"
			end
			return true, setname .. " = " .. setvalue
		end

		return false, "Invalid parameters (see /help set)."
	end,
})

local function emergeblocks_callback(pos, action, num_calls_remaining, ctx)
	if ctx.total_blocks == 0 then
		ctx.total_blocks   = num_calls_remaining + 1
		ctx.current_blocks = 0
	end
	ctx.current_blocks = ctx.current_blocks + 1

	if ctx.current_blocks == ctx.total_blocks then
		core.chat_send_player(ctx.requestor_name,
			string.format("Finished emerging %d blocks in %.2fms.",
			ctx.total_blocks, (os.clock() - ctx.start_time) * 1000))
	end
end

local function emergeblocks_progress_update(ctx)
	if ctx.current_blocks ~= ctx.total_blocks then
		core.chat_send_player(ctx.requestor_name,
			string.format("emergeblocks update: %d/%d blocks emerged (%.1f%%)",
			ctx.current_blocks, ctx.total_blocks,
			(ctx.current_blocks / ctx.total_blocks) * 100))

		core.after(2, emergeblocks_progress_update, ctx)
	end
end

core.register_chatcommand("emergeblocks", {
	params = "(here [<radius>]) | (<pos1> <pos2>)",
	description = "Load (or, if nonexistent, generate) map blocks "
		.. "contained in area pos1 to pos2 (<pos1> and <pos2> must be in parentheses)",
	privs = {server=true},
	func = function(name, param)
		local p1, p2 = parse_range_str(name, param)
		if p1 == false then
			return false, p2
		end

		local context = {
			current_blocks = 0,
			total_blocks   = 0,
			start_time     = os.clock(),
			requestor_name = name
		}

		core.emerge_area(p1, p2, emergeblocks_callback, context)
		core.after(2, emergeblocks_progress_update, context)

		return true, "Started emerge of area ranging from " ..
			core.pos_to_string(p1, 1) .. " to " .. core.pos_to_string(p2, 1)
	end,
})

core.register_chatcommand("deleteblocks", {
	params = "(here [<radius>]) | (<pos1> <pos2>)",
	description = "Delete map blocks contained in area pos1 to pos2 "
		.. "(<pos1> and <pos2> must be in parentheses)",
	privs = {server=true},
	func = function(name, param)
		local p1, p2 = parse_range_str(name, param)
		if p1 == false then
			return false, p2
		end

		if core.delete_area(p1, p2) then
			return true, "Successfully cleared area ranging from " ..
				core.pos_to_string(p1, 1) .. " to " .. core.pos_to_string(p2, 1)
		else
			return false, "Failed to clear one or more blocks in area"
		end
	end,
})

core.register_chatcommand("fixlight", {
	params = "(here [<radius>]) | (<pos1> <pos2>)",
	description = "Resets lighting in the area between pos1 and pos2 "
		.. "(<pos1> and <pos2> must be in parentheses)",
	privs = {server = true},
	func = function(name, param)
		local p1, p2 = parse_range_str(name, param)
		if p1 == false then
			return false, p2
		end

		if core.fix_light(p1, p2) then
			return true, "Successfully reset light in the area ranging from " ..
				core.pos_to_string(p1, 1) .. " to " .. core.pos_to_string(p2, 1)
		else
			return false, "Failed to load one or more blocks in area"
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
	elseif (not itemstack:is_known()) or (itemstack:get_name() == "unknown") then
		return false, "Cannot give an unknown item"
	-- Forbid giving 'ignore' due to unwanted side effects
	elseif itemstack:get_name() == "ignore" then
		return false, "Giving 'ignore' is not allowed"
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
		local msg = "%q %sadded to inventory."
		return true, msg:format(stackstring, partiality)
	else
		core.chat_send_player(receiver, ("%q %sadded to inventory.")
				:format(stackstring, partiality))
		local msg = "%q %sadded to %s's inventory."
		return true, msg:format(stackstring, partiality, receiver)
	end
end

core.register_chatcommand("give", {
	params = "<name> <ItemString> [<count> [<wear>]]",
	description = "Give item to player",
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
	params = "<ItemString> [<count> [<wear>]]",
	description = "Give item to yourself",
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
		if not core.registered_entities[entityname] then
			return false, "Cannot spawn an unknown entity"
		end
		if p == "" then
			p = player:get_pos()
		else
			p = core.string_to_pos(p)
			if p == nil then
				return false, "Invalid parameters ('" .. param .. "')"
			end
		end
		p.y = p.y + 1
		local obj = core.add_entity(p, entityname)
		local msg = obj and "%q spawned." or "%q failed to spawn."
		return true, msg:format(entityname)
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
		local wielded_item = player:get_wielded_item()
		if wielded_item:is_empty() then
			return false, "Unable to pulverize, no item in hand."
		end
		core.log("action", name .. " pulverized \"" ..
			wielded_item:get_name() .. " " .. wielded_item:get_count() .. "\"")
		player:set_wielded_item(nil)
		return true, "An item was pulverized."
	end,
})

-- Key = player name
core.rollback_punch_callbacks = {}

core.register_on_punchnode(function(pos, node, puncher)
	local name = puncher and puncher:get_player_name()
	if name and core.rollback_punch_callbacks[name] then
		core.rollback_punch_callbacks[name](pos, node, puncher)
		core.rollback_punch_callbacks[name] = nil
	end
end)

core.register_chatcommand("rollback_check", {
	params = "[<range>] [<seconds>] [<limit>]",
	description = "Check who last touched a node or a node near it"
			.. " within the time specified by <seconds>. Default: range = 0,"
			.. " seconds = 86400 = 24h, limit = 5. Set <seconds> to inf for no time limit",
	privs = {rollback=true},
	func = function(name, param)
		if not core.settings:get_bool("enable_rollback_recording") then
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
	params = "(<name> [<seconds>]) | (:<actor> [<seconds>])",
	description = "Revert actions of a player. Default for <seconds> is 60. Set <seconds> to inf for no time limit",
	privs = {rollback=true},
	func = function(name, param)
		if not core.settings:get_bool("enable_rollback_recording") then
			return false, "Rollback functions are disabled."
		end
		local target_name, seconds = string.match(param, ":([^ ]+) *(%d*)")
		if not target_name then
			local player_name
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
	description = "Show server status",
	func = function(name, param)
		local status = core.get_server_status(name, false)
		if status and status ~= "" then
			return true, status
		end
		return false, "This command was disabled by a mod or game"
	end,
})

core.register_chatcommand("time", {
	params = "[<0..23>:<0..59> | <0..24000>]",
	description = "Show or set time of day",
	privs = {},
	func = function(name, param)
		if param == "" then
			local current_time = math.floor(core.get_timeofday() * 1440)
			local minutes = current_time % 60
			local hour = (current_time - minutes) / 60
			return true, ("Current time is %d:%02d"):format(hour, minutes)
		end
		local player_privs = core.get_player_privs(name)
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

core.register_chatcommand("days", {
	description = "Show day count since world creation",
	func = function(name, param)
		return true, "Current day is " .. core.get_day_count()
	end
})

core.register_chatcommand("shutdown", {
	params = "[<delay_in_seconds> | -1] [reconnect] [<message>]",
	description = "Shutdown server (-1 cancels a delayed shutdown)",
	privs = {server=true},
	func = function(name, param)
		local delay, reconnect, message
		delay, param = param:match("^%s*(%S+)(.*)")
		if param then
			reconnect, param = param:match("^%s*(%S+)(.*)")
		end
		message = param and param:match("^%s*(.+)") or ""
		delay = tonumber(delay) or 0

		if delay == 0 then
			core.log("action", name .. " shuts down server")
			core.chat_send_all("*** Server shutting down (operator request).")
		end
		core.request_shutdown(message:trim(), core.is_yes(reconnect), delay)
		return true
	end,
})

core.register_chatcommand("ban", {
	params = "[<name>]",
	description = "Ban the IP of a player or show the ban list",
	privs = {ban=true},
	func = function(name, param)
		if param == "" then
			local ban_list = core.get_ban_list()
			if ban_list == "" then
				return true, "The ban list is empty."
			else
				return true, "Ban list: " .. ban_list
			end
		end
		if not core.get_player_by_name(param) then
			return false, "Player is not online."
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
	params = "<name> | <IP_address>",
	description = "Remove IP ban belonging to a player/IP",
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
	params = "<name> [<reason>]",
	description = "Kick a player",
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
	params = "[full | quick]",
	description = "Clear all objects in world",
	privs = {server=true},
	func = function(name, param)
		local options = {}
		if param == "" or param == "quick" then
			options.mode = "quick"
		elseif param == "full" then
			options.mode = "full"
		else
			return false, "Invalid usage, see /help clearobjects."
		end

		core.log("action", name .. " clears all objects ("
				.. options.mode .. " mode).")
		core.chat_send_all("Clearing all objects. This may take a long time."
				.. " You may experience a timeout. (by "
				.. name .. ")")
		core.clear_objects(options)
		core.log("action", "Object clearing done.")
		core.chat_send_all("*** Cleared all objects.")
		return true
	end,
})

core.register_chatcommand("msg", {
	params = "<name> <message>",
	description = "Send a direct message to a player",
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
		core.log("action", "DM from " .. name .. " to " .. sendto
				.. ": " .. message)
		core.chat_send_player(sendto, "DM from " .. name .. ": "
				.. message)
		return true, "Message sent."
	end,
})

core.register_chatcommand("last-login", {
	params = "[<name>]",
	description = "Get the last login time of a player or yourself",
	func = function(name, param)
		if param == "" then
			param = name
		end
		local pauth = core.get_auth_handler().get_auth(param)
		if pauth and pauth.last_login and pauth.last_login ~= -1 then
			-- Time in UTC, ISO 8601 format
			return true, param.."'s last login time was " ..
				os.date("!%Y-%m-%dT%H:%M:%SZ", pauth.last_login)
		end
		return false, param.."'s last login time is unknown"
	end,
})

core.register_chatcommand("clearinv", {
	params = "[<name>]",
	description = "Clear the inventory of yourself or another player",
	func = function(name, param)
		local player
		if param and param ~= "" and param ~= name then
			if not core.check_player_privs(name, {server=true}) then
				return false, "You don't have permission"
						.. " to clear another player's inventory (missing privilege: server)"
			end
			player = core.get_player_by_name(param)
			core.chat_send_player(param, name.." cleared your inventory.")
		else
			player = core.get_player_by_name(name)
		end

		if player then
			player:get_inventory():set_list("main", {})
			player:get_inventory():set_list("craft", {})
			player:get_inventory():set_list("craftpreview", {})
			core.log("action", name.." clears "..player:get_player_name().."'s inventory")
			return true, "Cleared "..player:get_player_name().."'s inventory."
		else
			return false, "Player must be online to clear inventory!"
		end
	end,
})

local function handle_kill_command(killer, victim)
	if core.settings:get_bool("enable_damage") == false then
		return false, "Players can't be killed, damage has been disabled."
	end
	local victimref = core.get_player_by_name(victim)
	if victimref == nil then
		return false, string.format("Player %s is not online.", victim)
	elseif victimref:get_hp() <= 0 then
		if killer == victim then
			return false, "You are already dead."
		else
			return false, string.format("%s is already dead.", victim)
		end
	end
	if not killer == victim then
		core.log("action", string.format("%s killed %s", killer, victim))
	end
	-- Kill victim
	victimref:set_hp(0)
	return true, string.format("%s has been killed.", victim)
end

core.register_chatcommand("kill", {
	params = "[<name>]",
	description = "Kill player or yourself",
	privs = {server=true},
	func = function(name, param)
		return handle_kill_command(name, param == "" and name or param)
	end,
})
