-- Minetest: builtin/misc.lua

--
-- Misc. API functions
--

function core.check_player_privs(name, ...)
	if core.is_player(name) then
		name = name:get_player_name()
	elseif type(name) ~= "string" then
		error("core.check_player_privs expects a player or playername as " ..
			"argument.", 2)
	end

	local requested_privs = {...}
	local player_privs = core.get_player_privs(name)
	local missing_privileges = {}

	if type(requested_privs[1]) == "table" then
		-- We were provided with a table like { privA = true, privB = true }.
		for priv, value in pairs(requested_privs[1]) do
			if value and not player_privs[priv] then
				missing_privileges[#missing_privileges + 1] = priv
			end
		end
	else
		-- Only a list, we can process it directly.
		for key, priv in pairs(requested_privs) do
			if not player_privs[priv] then
				missing_privileges[#missing_privileges + 1] = priv
			end
		end
	end

	if #missing_privileges > 0 then
		return false, missing_privileges
	end

	return true, ""
end

local player_list = {}

core.register_on_joinplayer(function(player)
	local player_name = player:get_player_name()
	player_list[player_name] = player
	if not minetest.is_singleplayer() then
		core.chat_send_all("*** " .. player_name .. " joined the game.")
	end
end)

core.register_on_leaveplayer(function(player, timed_out)
	local player_name = player:get_player_name()
	player_list[player_name] = nil
	local announcement = "*** " ..  player_name .. " left the game."
	if timed_out then
		announcement = announcement .. " (timed out)"
	end
	core.chat_send_all(announcement)
end)

function core.get_connected_players()
	local temp_table = {}
	for index, value in pairs(player_list) do
		if value:is_player_connected() then
			temp_table[#temp_table + 1] = value
		end
	end
	return temp_table
end


function core.is_player(player)
	-- a table being a player is also supported because it quacks sufficiently
	-- like a player if it has the is_player function
	local t = type(player)
	return (t == "userdata" or t == "table") and
		type(player.is_player) == "function" and player:is_player()
end


function minetest.player_exists(name)
	return minetest.get_auth_handler().get_auth(name) ~= nil
end

-- Returns two position vectors representing a box of `radius` in each
-- direction centered around the player corresponding to `player_name`
function core.get_player_radius_area(player_name, radius)
	local player = core.get_player_by_name(player_name)
	if player == nil then
		return nil
	end

	local p1 = player:getpos()
	local p2 = p1

	if radius then
		p1 = vector.subtract(p1, radius)
		p2 = vector.add(p2, radius)
	end

	return p1, p2
end

function core.hash_node_position(pos)
	return (pos.z+32768)*65536*65536 + (pos.y+32768)*65536 + pos.x+32768
end

function core.get_position_from_hash(hash)
	local pos = {}
	pos.x = (hash%65536) - 32768
	hash = math.floor(hash/65536)
	pos.y = (hash%65536) - 32768
	hash = math.floor(hash/65536)
	pos.z = (hash%65536) - 32768
	return pos
end

function core.get_item_group(name, group)
	if not core.registered_items[name] or not
			core.registered_items[name].groups[group] then
		return 0
	end
	return core.registered_items[name].groups[group]
end

function core.get_node_group(name, group)
	core.log("deprecated", "Deprecated usage of get_node_group, use get_item_group instead")
	return core.get_item_group(name, group)
end

function core.setting_get_pos(name)
	local value = core.settings:get(name)
	if not value then
		return nil
	end
	return core.string_to_pos(value)
end

-- To be overriden by protection mods
function core.is_protected(pos, name)
	return false
end

function core.record_protection_violation(pos, name)
	for _, func in pairs(core.registered_on_protection_violation) do
		func(pos, name)
	end
end

local raillike_ids = {}
local raillike_cur_id = 0
function core.raillike_group(name)
	local id = raillike_ids[name]
	if not id then
		raillike_cur_id = raillike_cur_id + 1
		raillike_ids[name] = raillike_cur_id
		id = raillike_cur_id
	end
	return id
end

-- HTTP callback interface
function core.http_add_fetch(httpenv)
	httpenv.fetch = function(req, callback)
		local handle = httpenv.fetch_async(req)

		local function update_http_status()
			local res = httpenv.fetch_async_get(handle)
			if res.completed then
				callback(res)
			else
				core.after(0, update_http_status)
			end
		end
		core.after(0, update_http_status)
	end

	return httpenv
end

function core.close_formspec(player_name, formname)
	return minetest.show_formspec(player_name, formname, "")
end

function core.cancel_shutdown_requests()
	core.request_shutdown("", false, -1)
end

