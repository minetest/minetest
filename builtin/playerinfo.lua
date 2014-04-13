-- Minetest: builtin/playerinfo.lua

playerinfo = {}

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

-- Stores each player's player list, to avoid changes while the formspec is refreshed
local formspec_players = {}

local function formspec_players_set(name)
	formspec_players[name] = {}
	for i, player in pairs(minetest.get_connected_players()) do
		-- Add player name
		local pname = player:get_player_name()
		-- Add player ping
		local pping = minetest.get_player_information(pname).avg_rtt / 2
		pping = math.floor(pping * 1000) -- Milliseconds

		table.insert(formspec_players[name], {name = pname, ping = pping})
	end
end

local function formspec_string(name, index)
	local formspec="size[8,6]"

	-- Turn the player table into a string
	local players = ""
	for i, entry in pairs(formspec_players[name]) do
		local ping_color = "#AAFFAA"
		if entry.ping > 500 then
			ping_color = "#FFAAAA"
		elseif entry.ping > 100 then
			ping_color = "#FFFF0AA"
		end

		players = players..ping_color..entry.name.." ("..entry.ping.."ms)"
		if i < #formspec_players[name] then
			players = players..","
		end
	end
	formspec = formspec.."label[4,0;Players: "..#formspec_players[name].." / "..minetest.setting_get("max_users").."]"
	formspec = formspec.."button[6,0;2,1;player_list_refresh;Refresh]"
	formspec = formspec.."textlist[4,1;4,4;player_list;"..players..";"..index..";true]"

	local player = minetest.get_player_by_name(formspec_players[name][index].name)
	if player then
		local name = player:get_player_name()
		formspec = formspec.."label[0,0;"..name.."]"
		formspec = formspec.."image[1,1;2,2;"..playerinfo.get_icon(name).."]"
	else
		-- The player we selected left while the window was open
		formspec = formspec.."label[0,0;Player disconnected]"
	end

	formspec = formspec.."button_exit[0,5;8,1;player_list_exit;Close]"
	return formspec
end

local function formspec(name, index)
	-- No index means we must set the player list
	local pindex = index
	if pindex < 1 then
		formspec_players_set(name)
		pindex = 1
	end

	minetest.show_formspec(name, "playerinfo", formspec_string(name, pindex))
end

--
-- Commands
--

minetest.register_on_player_receive_fields(function(player, formname, fields)
	if formname ~= "playerinfo" then return end
	local name = player:get_player_name()
	local event = minetest.explode_textlist_event(fields["player_list"])
	if fields["quit"] then
		formspec_players[name] = nil
	elseif fields["player_list_refresh"] then
		formspec(name, 0)
	elseif event.type == "CHG" then
		formspec(name, event.index)
	end
end)

minetest.register_chatcommand("players", {
	params = "",
	description = "show connected player information",
	privs = {},
	func = function(name, param)
		if minetest.is_singleplayer() then
			print("This command only works in multiplayer!")
		else
			formspec(name, 0)
		end
	end,
})
