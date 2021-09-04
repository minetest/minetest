local player_huds = {}

local states = {
	{0, "Normal font"},
	{1, "Bold font"},
	{2, "Italic font"},
	{3, "Bold and italic font"},
	{4, "Monospace font"},
	{5, "Bold and monospace font"},
	{7, "ZOMG all the font styles"},
}


local default_def = {
	hud_elem_type = "text",
	position = {x = 0.5, y = 0.5},
	scale = {x = 2, y = 2},
	alignment = { x = 0, y = 0 },
}

local function add_hud(player, state)
	local def = table.copy(default_def)
	local statetbl = states[state]
	def.offset = {x = 0, y = 32 * state}
	def.style = statetbl[1]
	def.text = statetbl[2]
	return player:hud_add(def)
end

minetest.register_on_leaveplayer(function(player)
	player_huds[player:get_player_name()] = nil
end)

local etime = 0
local state = 0

minetest.register_globalstep(function(dtime)
	etime = etime + dtime
	if etime < 1 then
		return
	end
	etime = 0
	for _, player in ipairs(minetest.get_connected_players()) do
		local huds = player_huds[player:get_player_name()]
		if huds then
			for i, hud_id in ipairs(huds) do
				local statetbl = states[(state + i) % #states + 1]
				player:hud_change(hud_id, "style", statetbl[1])
				player:hud_change(hud_id, "text", statetbl[2])
			end
		end
	end
	state = state + 1
end)

minetest.register_chatcommand("hudfonts", {
	params = "",
	description = "Show/Hide some text on the HUD with various font options",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		local param = tonumber(param) or 0
		param = math.min(math.max(param, 1), #states)
		if player_huds[name] == nil then
			player_huds[name] = {}
			for i = 1, param do
				table.insert(player_huds[name], add_hud(player, i))
			end
			minetest.chat_send_player(name, ("%d HUD element(s) added."):format(param))
		else
			local huds = player_huds[name]
			if huds then
				for _, hud_id in ipairs(huds) do
					player:hud_remove(hud_id)
				end
				minetest.chat_send_player(name, "All HUD elements removed.")
			end
			player_huds[name] = nil
		end
		return true
	end,
})
