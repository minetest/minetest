playerScoresHUD = {}
playerReturnsHUD = {}

minetest.register_on_joinplayer(function(player)
	local meta = player:get_meta()
	meta:set_int("score", 0)
	meta:set_int("return", 0)

	local player_name = player:get_player_name()
	local idxScore = player:hud_add({
		type = "text",
		position = {x = 1, y = 0}, -- right, top
		offset = {x=-40, y = 0},
		scale = {x = 100, y = 100},
		alignment = {x = -1, y = 1}, -- position specifies top right corner position
		text = "Score: " .. meta:get_string("score"),
		name = "score",
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
	playerScoresHUD[player_name] = idxScore
	playerReturnsHUD[player_name] = idxReturn
end)

-- Doesn't always increment consistently as expected, not sure what's wrong
minetest.register_globalstep(function(dtime)
	local players = minetest.get_connected_players()
	for _, player in ipairs(players) do
		local name = player:get_player_name()
		local meta = player:get_meta()
		local prev_score = meta:get_int("score")
		meta:set_int("score", prev_score + 1)
		meta:set_int("return", meta:get_int("return") + meta:get_int("score"))
		player:hud_change(playerReturnsHUD[name], "text", "Return: " .. meta:get("return"))
		player:hud_change(playerScoresHUD[name], "text", "Score: " .. meta:get("score"))
	end
end)
