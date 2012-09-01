-- Minetest: builtin/chatcommands.lua

--
-- Chat command handler
--

minetest.chatcommands = {}
function minetest.register_chatcommand(cmd, def)
	def = def or {}
	def.params = def.params or ""
	def.description = def.description or ""
	def.privs = def.privs or {}
	minetest.chatcommands[cmd] = def
end

minetest.register_on_chat_message(function(name, message)
	local cmd, param = string.match(message, "^/([^ ]+) *(.*)")
	if not param then
		param = ""
	end
	local cmd_def = minetest.chatcommands[cmd]
	if cmd_def then
		local has_privs, missing_privs = minetest.check_player_privs(name, cmd_def.privs)
		if has_privs then
			cmd_def.func(name, param)
		else
			minetest.chat_send_player(name, "You don't have permission to run this command (missing privileges: "..table.concat(missing_privs, ", ")..")")
		end
		return true -- handled chat message
	end
	return false
end)

--
-- Chat commands
--
minetest.register_chatcommand("me", {
	params = "<action>",
	description = "chat action (eg. /me orders a pizza)",
	privs = {shout=true},
	func = function(name, param)
		minetest.chat_send_all("* " .. name .. " " .. param)
	end,
})

minetest.register_chatcommand("help", {
	privs = {},
	params = "(nothing)/all/privs/<cmd>",
	description = "Get help for commands or list privileges",
	func = function(name, param)
		local format_help_line = function(cmd, def)
			local msg = "/"..cmd
			if def.params and def.params ~= "" then msg = msg .. " " .. def.params end
			if def.description and def.description ~= "" then msg = msg .. ": " .. def.description end
			return msg
		end
		if param == "" then
			local msg = ""
			cmds = {}
			for cmd, def in pairs(minetest.chatcommands) do
				if minetest.check_player_privs(name, def.privs) then
					table.insert(cmds, cmd)
				end
			end
			minetest.chat_send_player(name, "Available commands: "..table.concat(cmds, " "))
			minetest.chat_send_player(name, "Use '/help <cmd>' to get more information, or '/help all' to list everything.")
		elseif param == "all" then
			minetest.chat_send_player(name, "Available commands:")
			for cmd, def in pairs(minetest.chatcommands) do
				if minetest.check_player_privs(name, def.privs) then
					minetest.chat_send_player(name, format_help_line(cmd, def))
				end
			end
		elseif param == "privs" then
			minetest.chat_send_player(name, "Available privileges:")
			for priv, def in pairs(minetest.registered_privileges) do
				minetest.chat_send_player(name, priv..": "..def.description)
			end
		else
			local cmd = param
			def = minetest.chatcommands[cmd]
			if not def then
				minetest.chat_send_player(name, "Command not available: "..cmd)
			else
				minetest.chat_send_player(name, format_help_line(cmd, def))
			end
		end
	end,
})
minetest.register_chatcommand("privs", {
	params = "<name>",
	description = "print out privileges of player",
	func = function(name, param)
		if param == "" then
			param = name
		else
			--[[if not minetest.check_player_privs(name, {privs=true}) then
				minetest.chat_send_player(name, "Privileges of "..param.." are hidden from you.")
				return
			end]]
		end
		minetest.chat_send_player(name, "Privileges of "..param..": "..minetest.privs_to_string(minetest.get_player_privs(param), ' '))
	end,
})
minetest.register_chatcommand("grant", {
	params = "<name> <privilege>|all",
	description = "Give privilege to player",
	privs = {},
	func = function(name, param)
		if not minetest.check_player_privs(name, {privs=true}) and 
				not minetest.check_player_privs(name, {basic_privs=true}) then
			minetest.chat_send_player(name, "Your privileges are insufficient.")
			return
		end
		local grantname, grantprivstr = string.match(param, "([^ ]+) (.+)")
		if not grantname or not grantprivstr then
			minetest.chat_send_player(name, "Invalid parameters (see /help grant)")
			return
		end
		local grantprivs = minetest.string_to_privs(grantprivstr)
		if grantprivstr == "all" then
			grantprivs = minetest.registered_privileges
		end
		local privs = minetest.get_player_privs(grantname)
		local privs_known = true
		for priv, _ in pairs(grantprivs) do
			if priv ~= "interact" and priv ~= "shout" and priv ~= "interact_extra" and not minetest.check_player_privs(name, {privs=true}) then
				minetest.chat_send_player(name, "Your privileges are insufficient.")
				return
			end
			if not minetest.registered_privileges[priv] then
				minetest.chat_send_player(name, "Unknown privilege: "..priv)
				privs_known = false
			end
			privs[priv] = true
		end
		if not privs_known then
			return
		end
		minetest.set_player_privs(grantname, privs)
		minetest.chat_send_player(name, "Privileges of "..grantname..": "..minetest.privs_to_string(minetest.get_player_privs(grantname), ' '))
		if grantname ~= name then
			minetest.chat_send_player(grantname, name.." granted you privileges: "..minetest.privs_to_string(grantprivs, ' '))
		end
	end,
})
minetest.register_chatcommand("revoke", {
	params = "<name> <privilege>|all",
	description = "Remove privilege from player",
	privs = {},
	func = function(name, param)
		if not minetest.check_player_privs(name, {privs=true}) and 
				not minetest.check_player_privs(name, {basic_privs=true}) then
			minetest.chat_send_player(name, "Your privileges are insufficient.")
			return
		end
		local revokename, revokeprivstr = string.match(param, "([^ ]+) (.+)")
		if not revokename or not revokeprivstr then
			minetest.chat_send_player(name, "Invalid parameters (see /help revoke)")
			return
		end
		local revokeprivs = minetest.string_to_privs(revokeprivstr)
		local privs = minetest.get_player_privs(revokename)
		for priv, _ in pairs(revokeprivs) do
			if priv ~= "interact" and priv ~= "shout" and priv ~= "interact_extra" and not minetest.check_player_privs(name, {privs=true}) then
				minetest.chat_send_player(name, "Your privileges are insufficient.")
				return
			end
		end
		if revokeprivstr == "all" then
			privs = {}
		else
			for priv, _ in pairs(revokeprivs) do
				privs[priv] = nil
			end
		end
		minetest.set_player_privs(revokename, privs)
		minetest.chat_send_player(name, "Privileges of "..revokename..": "..minetest.privs_to_string(minetest.get_player_privs(revokename), ' '))
		if revokename ~= name then
			minetest.chat_send_player(revokename, name.." revoked privileges from you: "..minetest.privs_to_string(revokeprivs, ' '))
		end
	end,
})
minetest.register_chatcommand("setpassword", {
	params = "<name> <password>",
	description = "set given password",
	privs = {password=true},
	func = function(name, param)
		local toname, raw_password = string.match(param, "^([^ ]+) +(.+)$")
		if not toname then
			toname = string.match(param, "^([^ ]+) *$")
			raw_password = nil
		end
		if not toname then
			minetest.chat_send_player(name, "Name field required")
			return
		end
		local actstr = "?"
		if not raw_password then
			minetest.set_player_password(toname, "")
			actstr = "cleared"
		else
			minetest.set_player_password(toname, minetest.get_password_hash(toname, raw_password))
			actstr = "set"
		end
		minetest.chat_send_player(name, "Password of player \""..toname.."\" "..actstr)
		if toname ~= name then
			minetest.chat_send_player(toname, "Your password was "..actstr.." by "..name)
		end
	end,
})
minetest.register_chatcommand("clearpassword", {
	params = "<name>",
	description = "set empty password",
	privs = {password=true},
	func = function(name, param)
		toname = param
		if not toname then
			minetest.chat_send_player(toname, "Name field required")
			return
		end
		minetest.set_player_password(toname, '')
		minetest.chat_send_player(name, "Password of player \""..toname.."\" cleared")
	end,
})

minetest.register_chatcommand("auth_reload", {
	params = "",
	description = "reload authentication data",
	privs = {server=true},
	func = function(name, param)
		local done = minetest.auth_reload()
		if done then
			minetest.chat_send_player(name, "Done.")
		else
			minetest.chat_send_player(name, "Failed.")
		end
	end,
})

minetest.register_chatcommand("teleport", {
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
				local n = minetest.env:get_node(p)
				if not minetest.registered_nodes[n.name].walkable then
					return p, true
				end
			end
			return pos, false
		end

		local teleportee = nil
		local p = {}
		p.x, p.y, p.z = string.match(param, "^([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
		teleportee = minetest.env:get_player_by_name(name)
		if teleportee and p.x and p.y and p.z then
			minetest.chat_send_player(name, "Teleporting to ("..p.x..", "..p.y..", "..p.z..")")
			teleportee:setpos(p)
			return
		end
		
		local teleportee = nil
		local p = nil
		local target_name = nil
		target_name = string.match(param, "^([^ ]+)$")
		teleportee = minetest.env:get_player_by_name(name)
		if target_name then
			local target = minetest.env:get_player_by_name(target_name)
			if target then
				p = target:getpos()
			end
		end
		if teleportee and p then
			p = find_free_position_near(p)
			minetest.chat_send_player(name, "Teleporting to "..target_name.." at ("..p.x..", "..p.y..", "..p.z..")")
			teleportee:setpos(p)
			return
		end
		
		if minetest.check_player_privs(name, {bring=true}) then
			local teleportee = nil
			local p = {}
			local teleportee_name = nil
			teleportee_name, p.x, p.y, p.z = string.match(param, "^([^ ]+) +([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
			if teleportee_name then
				teleportee = minetest.env:get_player_by_name(teleportee_name)
			end
			if teleportee and p.x and p.y and p.z then
				minetest.chat_send_player(name, "Teleporting "..teleportee_name.." to ("..p.x..", "..p.y..", "..p.z..")")
				teleportee:setpos(p)
				return
			end
			
			local teleportee = nil
			local p = nil
			local teleportee_name = nil
			local target_name = nil
			teleportee_name, target_name = string.match(param, "^([^ ]+) +([^ ]+)$")
			if teleportee_name then
				teleportee = minetest.env:get_player_by_name(teleportee_name)
			end
			if target_name then
				local target = minetest.env:get_player_by_name(target_name)
				if target then
					p = target:getpos()
				end
			end
			if teleportee and p then
				p = find_free_position_near(p)
				minetest.chat_send_player(name, "Teleporting "..teleportee_name.." to "..target_name.." at ("..p.x..", "..p.y..", "..p.z..")")
				teleportee:setpos(p)
				return
			end
		end

		minetest.chat_send_player(name, "Invalid parameters (\""..param.."\") or player not found (see /help teleport)")
		return
	end,
})

minetest.register_chatcommand("set", {
	params = "[-n] <name> <value> | <name>",
	description = "set or read server configuration setting",
	privs = {server=true},
	func = function(name, param)
		local arg, setname, setvalue = string.match(param, "(-[n]) ([^ ]+) (.+)")
		if arg and arg == "-n" and setname and setvalue then
			minetest.setting_set(setname, setvalue)
			minetest.chat_send_player(name, setname.." = "..setvalue)
			return
		end
		local setname, setvalue = string.match(param, "([^ ]+) (.+)")
		if setname and setvalue then
			if not minetest.setting_get(setname) then
				minetest.chat_send_player(name, "Failed. Use '/set -n <name> <value>' to create a new setting.")
				return
			end
			minetest.setting_set(setname, setvalue)
			minetest.chat_send_player(name, setname.." = "..setvalue)
			return
		end
		local setname = string.match(param, "([^ ]+)")
		if setname then
			local setvalue = minetest.setting_get(setname)
			if not setvalue then
				setvalue = "<not set>"
			end
			minetest.chat_send_player(name, setname.." = "..setvalue)
			return
		end
		minetest.chat_send_player(name, "Invalid parameters (see /help set)")
	end,
})

minetest.register_chatcommand("mods", {
	params = "",
	description = "lists mods installed on the server",
	privs = {},
	func = function(name, param)
		local response = ""
		local modnames = minetest.get_modnames()
		for i, mod in ipairs(modnames) do
			response = response .. mod
			-- Add space if not at the end
			if i ~= #modnames then
				response = response .. " "
			end
		end
		minetest.chat_send_player(name, response)
	end,
})

local function handle_give_command(cmd, giver, receiver, stackstring)
	minetest.log("action", giver.." invoked "..cmd..', stackstring="'
			..stackstring..'"')
	minetest.log(cmd..' invoked, stackstring="'..stackstring..'"')
	local itemstack = ItemStack(stackstring)
	if itemstack:is_empty() then
		minetest.chat_send_player(giver, 'error: cannot give an empty item')
		return
	elseif not itemstack:is_known() then
		minetest.chat_send_player(giver, 'error: cannot give an unknown item')
		return
	end
	local receiverref = minetest.env:get_player_by_name(receiver)
	if receiverref == nil then
		minetest.chat_send_player(giver, receiver..' is not a known player')
		return
	end
	local leftover = receiverref:get_inventory():add_item("main", itemstack)
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
		minetest.chat_send_player(giver, '"'..stackstring
			..'" '..partiality..'added to inventory.');
	else
		minetest.chat_send_player(giver, '"'..stackstring
			..'" '..partiality..'added to '..receiver..'\'s inventory.');
		minetest.chat_send_player(receiver, '"'..stackstring
			..'" '..partiality..'added to inventory.');
	end
end

minetest.register_chatcommand("give", {
	params = "<name> <itemstring>",
	description = "give item to player",
	privs = {give=true},
	func = function(name, param)
		local toname, itemstring = string.match(param, "^([^ ]+) +(.+)$")
		if not toname or not itemstring then
			minetest.chat_send_player(name, "name and itemstring required")
			return
		end
		handle_give_command("/give", name, toname, itemstring)
	end,
})
minetest.register_chatcommand("giveme", {
	params = "<itemstring>",
	description = "give item to yourself",
	privs = {give=true},
	func = function(name, param)
		local itemstring = string.match(param, "(.+)$")
		if not itemstring then
			minetest.chat_send_player(name, "itemstring required")
			return
		end
		handle_give_command("/giveme", name, name, itemstring)
	end,
})
minetest.register_chatcommand("spawnentity", {
	params = "<entityname>",
	description = "spawn entity at your position",
	privs = {give=true, interact=true},
	func = function(name, param)
		local entityname = string.match(param, "(.+)$")
		if not entityname then
			minetest.chat_send_player(name, "entityname required")
			return
		end
		print('/spawnentity invoked, entityname="'..entityname..'"')
		local player = minetest.env:get_player_by_name(name)
		if player == nil then
			print("Unable to spawn entity, player is nil")
			return true -- Handled chat message
		end
		local p = player:getpos()
		p.y = p.y + 1
		minetest.env:add_entity(p, entityname)
		minetest.chat_send_player(name, '"'..entityname
				..'" spawned.');
	end,
})
minetest.register_chatcommand("pulverize", {
	params = "",
	description = "delete item in hand",
	privs = {},
	func = function(name, param)
		local player = minetest.env:get_player_by_name(name)
		if player == nil then
			print("Unable to pulverize, player is nil")
			return true -- Handled chat message
		end
		if player:get_wielded_item():is_empty() then
			minetest.chat_send_player(name, 'Unable to pulverize, no item in hand.')
		else
			player:set_wielded_item(nil)
			minetest.chat_send_player(name, 'An item was pulverized.')
		end
	end,
})

-- Key = player name
minetest.rollback_punch_callbacks = {}

minetest.register_on_punchnode(function(pos, node, puncher)
	local name = puncher:get_player_name()
	if minetest.rollback_punch_callbacks[name] then
		minetest.rollback_punch_callbacks[name](pos, node, puncher)
		minetest.rollback_punch_callbacks[name] = nil
	end
end)

minetest.register_chatcommand("rollback_check", {
	params = "[<range>] [<seconds>]",
	description = "check who has last touched a node or near it, "..
			"max. <seconds> ago (default range=0, seconds=86400=24h)",
	privs = {rollback=true},
	func = function(name, param)
		local range, seconds = string.match(param, "(%d+) *(%d*)")
		range = tonumber(range) or 0
		seconds = tonumber(seconds) or 86400
		minetest.chat_send_player(name, "Punch a node (limits set: range="..
				dump(range).." seconds="..dump(seconds).."s)")
		minetest.rollback_punch_callbacks[name] = function(pos, node, puncher)
			local name = puncher:get_player_name()
			minetest.chat_send_player(name, "Checking...")
			local actor, act_p, act_seconds =
					minetest.rollback_get_last_node_actor(pos, range, seconds)
			if actor == "" then
				minetest.chat_send_player(name, "Nobody has touched the "..
						"specified location in "..dump(seconds).." seconds")
				return
			end
			local nodedesc = "this node"
			if act_p.x ~= pos.x or act_p.y ~= pos.y or act_p.z ~= pos.z then
				nodedesc = minetest.pos_to_string(act_p)
			end
			local nodename = minetest.env:get_node(act_p).name
			minetest.chat_send_player(name, "Last actor on "..nodedesc..
					" was "..actor..", "..dump(act_seconds)..
					"s ago (node is now "..nodename..")")
		end
	end,
})

minetest.register_chatcommand("rollback", {
	params = "<player name> [<seconds>] | :<actor> [<seconds>]",
	description = "revert actions of a player; default for <seconds> is 60",
	privs = {rollback=true},
	func = function(name, param)
		local target_name, seconds = string.match(param, ":([^ ]+) *(%d*)")
		if not target_name then
			local player_name = nil;
			player_name, seconds = string.match(param, "([^ ]+) *(%d*)")
			if not player_name then
				minetest.chat_send_player(name, "Invalid parameters. See /help rollback and /help rollback_check")
				return
			end
			target_name = "player:"..player_name
		end
		seconds = tonumber(seconds) or 60
		minetest.chat_send_player(name, "Reverting actions of "..
				dump(target_name).." since "..dump(seconds).." seconds.")
		local success, log = minetest.rollback_revert_actions_by(
				target_name, seconds)
		if #log > 10 then
			minetest.chat_send_player(name, "(log is too long to show)")
		else
			for _,line in ipairs(log) do
				minetest.chat_send_player(name, line)
			end
		end
		if success then
			minetest.chat_send_player(name, "Reverting actions succeeded.")
		else
			minetest.chat_send_player(name, "Reverting actions FAILED.")
		end
	end,
})

minetest.register_chatcommand("status", {
	params = "",
	description = "print server status line",
	privs = {},
	func = function(name, param)
		minetest.chat_send_player(name, minetest.get_server_status())
	end,
})

minetest.register_chatcommand("time", {
	params = "<0...24000>",
	description = "set time of day",
	privs = {settime=true},
	func = function(name, param)
		if param == "" then
			minetest.chat_send_player(name, "Missing parameter")
			return
		end
		local newtime = tonumber(param)
		if newtime == nil then
			minetest.chat_send_player(name, "Invalid time")
		else
			minetest.env:set_timeofday((newtime % 24000) / 24000)
			minetest.chat_send_player(name, "Time of day changed.")
			minetest.log("action", name .. " sets time " .. newtime)
		end
	end,
})

minetest.register_chatcommand("shutdown", {
	params = "",
	description = "shutdown server",
	privs = {server=true},
	func = function(name, param)
		minetest.log("action", name .. " shuts down server")
		minetest.request_shutdown()
		minetest.chat_send_all("*** Server shutting down (operator request).")
	end,
})

minetest.register_chatcommand("ban", {
	params = "<name>",
	description = "ban IP of player",
	privs = {ban=true},
	func = function(name, param)
		if param == "" then
			minetest.chat_send_player(name, "Ban list: " .. minetest.get_ban_list())
			return
		end
		if not minetest.env:get_player_by_name(param) then
			minetest.chat_send_player(name, "No such player")
			return
		end
		if not minetest.ban_player(param) then
			minetest.chat_send_player(name, "Failed to ban player")
		else
			local desc = minetest.get_ban_description(param)
			minetest.chat_send_player(name, "Banned " .. desc .. ".")
			minetest.log("action", name .. " bans " .. desc .. ".")
		end
	end,
})

minetest.register_chatcommand("unban", {
	params = "<name/ip>",
	description = "remove IP ban",
	privs = {ban=true},
	func = function(name, param)
		if not minetest.unban_player_or_ip(param) then
			minetest.chat_send_player(name, "Failed to unban player/IP")
		else
			minetest.chat_send_player(name, "Unbanned " .. param)
			minetest.log("action", name .. " unbans " .. param)
		end
	end,
})

minetest.register_chatcommand("clearobjects", {
	params = "",
	description = "clear all objects in world",
	privs = {server=true},
	func = function(name, param)
		minetest.log("action", name .. " clears all objects")
		minetest.chat_send_all("Clearing all objects.  This may take long.  You may experience a timeout.  (by " .. name .. ")")
		minetest.env:clear_objects()
		minetest.log("action", "object clearing done")
		minetest.chat_send_all("*** Cleared all objects.")
	end,
})
