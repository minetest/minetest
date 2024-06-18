local lastdir = {}

minetest.register_globalstep(function(dtime)
	for _, player in pairs(minetest.get_connected_players()) do
		local pname = player:get_player_name()
		local ldeg = -math.deg(player:get_look_vertical())
		if lastdir[pname] == nil then
			lastdir[pname] = 0
		end
		if lastdir[pname] then
			if math.abs(lastdir[pname] - ldeg) > 4 then
				lastdir[pname] = ldeg
				player:set_bone_override("Head", {rotation = {vector = {x = ldeg, y = 0, z = 0}, absolute = true },
				scale = {vector = { x = 1.5, y = 1.5, z = 1.5 }}})
			end
		end
	end
end)

minetest.register_on_leaveplayer(function(player)
	lastdir[player:get_player_name()] = nil
end)

minetest.register_chatcommand("headanim", {
	func = function(name)
		local player = assert(minetest.get_player_by_name(name))
		if lastdir[name] then
			lastdir[name] = false -- don't update Head in globalstep
			player:set_bone_override"Head" -- clear override
		else
			lastdir[name] = 0
		end
	end
})
