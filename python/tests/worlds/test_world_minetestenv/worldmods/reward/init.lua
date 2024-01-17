playerRewardsHUD = {}
playerReturnsHUD = {}

minetest.register_on_joinplayer(function(player)
	local meta = player:get_meta()
	meta:set_int("reward", 1)
	meta:set_int("return", 0)

	local player_name = player:get_player_name()
	local idxReward = player:hud_add({
		type = "text",
		position = {x = 1, y = 0}, -- right, top
		offset = {x=-40, y = 0},
		scale = {x = 100, y = 100},
		alignment = {x = -1, y = 1}, -- position specifies top right corner position
		text = "Reward: " .. meta:get_string("reward"),
		name = "reward",
	})
	local idxReturn = player:hud_add({
		type = "text",
		position = {x = 1, y = 0}, -- right, top
		offset = {x=-40, y = 20},
		scale = {x = 100, y = 100},
		alignment = {x = -1, y = 1}, -- position specifies top right corner position
		text = "Return: " .. meta:get_string("return"),
		name = "return",
	})
	playerRewardsHUD[player_name] = idxReward
	playerReturnsHUD[player_name] = idxReturn
end)

-- Doesn't always increment consistently as expected, not sure what's wrong
minetest.register_globalstep(function(dtime)
	local players = minetest.get_connected_players()
	for _, player in ipairs(players) do
		local name = player:get_player_name()
		local meta = player:get_meta()
		meta:set_int("reward", 1)
		meta:set_int("return", meta:get_int("return") + meta:get_int("reward"))
		player:hud_change(playerReturnsHUD[name], "text", "Return: " .. meta:get("return"))
		player:hud_change(playerRewardsHUD[name], "text", "Reward: " .. meta:get("reward"))
	end
end)
