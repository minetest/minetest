---------------------------------------------------------------------------------
-- Minetest
-- Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License along
-- with this program; if not, write to the Free Software Foundation, Inc.,
-- 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
---------------------------------------------------------------------------------


local health_bar_definition = {
	hud_elem_type = "statbar",
	position = {x= 0.5, y = 1},
	text = "heart.png",
	number = 20,
	direction = 0,
	size = {x = 24, y = 24},
	offset = {x = (-10*24)-25, y = -(48+24+16)},
}

local hunger_bar_definition = {
	hud_elem_type = "statbar",
	position = {x= 0.5, y = 1},
	text = "bread.png",
	number = 20,
	direction = 0,
	size = {x = 24, y = 24},
	offset = {x = 25, y = -(48+24+16)},
}

local breath_bar_definition = {
	hud_elem_type = "statbar",
	position = {x= 0.5, y = 1},
	text = "bubble.png",
	number = 20,
	direction = 0,
	size = {x = 24, y = 24},
	offset = {x = 25, y = -(48+48+16)},
}


local hud_ids = {}

local function initialize_builtin_statbars(player)

	if not player:is_player() then
		return
	end

	local name = player:get_player_name()

	if name == "" then
		return
	end

	if (hud_ids[name] == nil) then
		hud_ids[name] = {}
		-- flags are not transmitted to client on connect, we need to make sure
		-- our current flags are transmitted by sending them actively
		player:hud_set_flags(player:hud_get_flags())
	end

	-- health bar
	if player:hud_get_flags().healthbar and
			core.is_yes(core.setting_get("enable_damage")) then
		if hud_ids[name].id_healthbar == nil then
			health_bar_definition.number = player:get_hp()
			hud_ids[name].id_healthbar  = player:hud_add(health_bar_definition)
		end
	else
		if hud_ids[name].id_healthbar ~= nil then
			player:hud_remove(hud_ids[name].id_healthbar)
			hud_ids[name].id_healthbar = nil
		end
	end

	-- breath bar
	if (player:get_breath() < 11) then
		if player:hud_get_flags().breathbar and
			core.is_yes(core.setting_get("enable_damage")) then
			if hud_ids[name].id_breathbar == nil then
				hud_ids[name].id_breathbar = player:hud_add(breath_bar_definition)
			end
		else
			if hud_ids[name].id_breathbar ~= nil then
				player:hud_remove(hud_ids[name].id_breathbar)
				hud_ids[name].id_breathbar = nil
			end
		end
	elseif hud_ids[name].id_breathbar ~= nil then
		player:hud_remove(hud_ids[name].id_breathbar)
		hud_ids[name].id_breathbar = nil
	end

	-- hungerbar
	if player:hud_get_flags().healthbar and
			core.is_yes(core.setting_get("enable_damage")) and
			core.is_yes(core.setting_getbool("enable_hunger")) then
		if hud_ids[name].id_hungerbar == nil then
			local hunger = player:get_hunger()
			if hunger > 20 then
				hunger = 20
			end
			hunger_bar_definition.number = hunger
			hud_ids[name].id_hungerbar  = player:hud_add(hunger_bar_definition)
		end
	else
		if hud_ids[name].id_hungerbar ~= nil then
			player:hud_remove(hud_ids[name].id_hungerbar)
			hud_ids[name].id_hungerbar = nil
		end
	end
end


local function cleanup_builtin_statbars(player)

	if not player:is_player() then
		return
	end

	local name = player:get_player_name()

	if name == "" then
		return
	end

	hud_ids[name] = nil
end


local function player_event_handler(player, eventname)
	assert(player:is_player())

	local name = player:get_player_name()

	if name == "" then
		return
	end

	if eventname == "health_changed" then
		initialize_builtin_statbars(player)

		if hud_ids[name].id_healthbar ~= nil then
			player:hud_change(hud_ids[name].id_healthbar, "number", player:get_hp())
			return true
		end
	end

	if eventname == "breath_changed" then
		initialize_builtin_statbars(player)

		if hud_ids[name].id_breathbar ~= nil then
			player:hud_change(hud_ids[name].id_breathbar, "number", player:get_breath() * 2)
			return true
		end
	end

	if eventname == "hunger_changed" then
		initialize_builtin_statbars(player)

		if hud_ids[name].id_hungerbar ~= nil then
			local hunger = player:get_hunger()
			if hunger > 20 then
				hunger = 20
			end
			player:hud_change(hud_ids[name].id_hungerbar, "number", hunger)
			return true
		end
	end

	if eventname == "hud_changed" then
		initialize_builtin_statbars(player)
		return true
	end

	return false
end


function core.hud_replace_builtin(name, definition)

	if definition == nil or
		type(definition) ~= "table" or
		definition.hud_elem_type ~= "statbar" then
		return false
	end

	if name == "health" then
		health_bar_definition = definition

		for name, ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if  player and hud_ids[name].id_healthbar then
				player:hud_remove(hud_ids[name].id_healthbar)
				initialize_builtin_statbars(player)
			end
		end
		return true
	end

	if name == "breath" then
		breath_bar_definition = definition

		for name, ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if  player and hud_ids[name].id_breathbar then
				player:hud_remove(hud_ids[name].id_breathbar)
				initialize_builtin_statbars(player)
			end
		end
		return true
	end

	if name == "hunger" then
		hunger_bar_definition = definition

		for name, ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if  player and hud_ids[name].id_hungerbar then
				player:hud_remove(hud_ids[name].id_hungerbar)
				initialize_builtin_statbars(player)
			end
		end
		return true
	end

	return false
end

core.register_on_joinplayer(initialize_builtin_statbars)
core.register_on_leaveplayer(cleanup_builtin_statbars)
core.register_playerevent(player_event_handler)
