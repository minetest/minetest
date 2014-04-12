-- Minetest: builtin/playerinfo.lua

playerinfo = {}

local MAX_COLUMNS = 6
local MAX_ROWS = 8
local COLUMN_WIDTH = 3

--
-- Player icon handler
--

local playericons = {}

function playerinfo.set_icon(player, texture)
	local pname = player
	if type(pname) ~= "string" then
		pname = player:get_player_name()
	end
	local tname = texture
	if not tname or tname == "" then
		tname = "logo.png"
	end
	playericons[pname] = tname
end

function playerinfo.get_icon(player)
	local pname = player
	if type(pname) ~= "string" then
		pname = player:get_player_name()
	end
	if not playericons[pname] then
		playericons[pname] = "logo.png"
	end
	return playericons[pname]
end

--
-- Player list handler
--

function get_playerlist_formspec()
	local total_players = #minetest.get_connected_players()
	local total_columns = math.min(total_players, MAX_COLUMNS)
	local total_rows = math.min(math.ceil(total_players / MAX_COLUMNS), MAX_ROWS)
	local formspec="size["..(total_columns * COLUMN_WIDTH)..","..(total_rows + 1).."]"

	local current_column = 0
	local current_row = 0
	for i, player in pairs(minetest.get_connected_players()) do
		if i > total_columns * total_rows then break end
		if current_column >= (MAX_COLUMNS * COLUMN_WIDTH) then
			current_column = 0
			current_row = current_row + 1
		end

		local name = player:get_player_name()
		local ping = minetest.get_player_information(name).avg_rtt / 2
		ping = math.floor(ping * 1000) -- Seconds to miliseconds

		formspec = formspec.."image["..current_column..","..current_row..";1,1;"..playerinfo.get_icon(player).."]"
		formspec = formspec.."label["..(current_column + 1)..","..current_row..";"..name.."\nPing: "..ping.."]"

		current_column = current_column + COLUMN_WIDTH
	end

	formspec = formspec.."button_exit[0,"..total_rows..";"..(total_columns * COLUMN_WIDTH)..",1;exit;Close]"
	return formspec
end

--
-- Chat command
--

minetest.register_chatcommand("players", {
	params = "",
	description = "show a list of connected players",
	privs = {},
	func = function(name, param)
		if minetest.is_singleplayer() then
			print("This command only works in multiplayer!")
		else
			minetest.show_formspec(name, "players", get_playerlist_formspec())
		end
	end,
})
