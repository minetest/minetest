local function handle_give_command(cmd, giver, receiver, stackstring)
	if not minetest.get_player_privs(giver)["give"] then
		minetest.chat_send_player(giver, "error: you don't have permission to give")
		return
	end
	minetest.debug("DEBUG: "..cmd..' invoked, stackstring="'..stackstring..'"')
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

minetest.register_on_chat_message(function(name, message)
	--print("default on_chat_message: name="..dump(name).." message="..dump(message))
	local cmd = "/giveme"
	if message:sub(0, #cmd) == cmd then
		local stackstring = string.match(message, cmd.." (.*)")
		if stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' stackstring')
			return true -- Handled chat message
		end
		handle_give_command(cmd, name, name, stackstring)
		return true
	end
	local cmd = "/give"
	if message:sub(0, #cmd) == cmd then
		local receiver, stackstring = string.match(message, cmd.." ([%a%d_-]+) (.*)")
		if receiver == nil or stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' name stackstring')
			return true -- Handled chat message
		end
		handle_give_command(cmd, name, receiver, stackstring)
		return true
	end
	local cmd = "/spawnentity"
	if message:sub(0, #cmd) == cmd then
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to spawn (give)")
			return true -- Handled chat message
		end
		if not minetest.get_player_privs(name)["interact"] then
			minetest.chat_send_player(name, "you don't have permission to interact")
			return true -- Handled chat message
		end
		local entityname = string.match(message, cmd.." (.*)")
		if entityname == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' entityname')
			return true -- Handled chat message
		end
		print(cmd..' invoked, entityname="'..entityname..'"')
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
		return true -- Handled chat message
	end
	local cmd = "/pulverize"
	if message:sub(0, #cmd) == cmd then
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
		return true
	end
end)