---------------------------------------------------------------------------------
-- Minetest
-- Copyright (C) 2015 BlockMen <blockmen2015@gmail.com>
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


-- Dont do anything when no damage or hunger
if core.setting_getbool("enable_damage") ~= true then
	return
end


local hunger_step = 300 -- time in seconds until player looses 1 hunger point
local healing_step = 4  -- Time in seconds after player gets 1 HP (when healing)
local healing = {}


function core.heal_timer()
	for player_name, check in pairs(healing) do
		if check == true then
			local player = core.get_player_by_name(player_name)
			if player and player:is_player() then
				player:set_hp(player:get_hp() + 1)
			end
		end

	end

	core.after(healing_step, core.heal_timer)
end


function core.hunger_timer(player, hunger_change)
	player:set_hunger(player:get_hunger() - hunger_change)
	core.after(hunger_step, core.hunger_timer, player, hunger_change)
end


local function check_healing_state(player)
	assert(player:is_player())

	local name = player:get_player_name()
	if not name or name == "" then
		return
	end

	local is_healing = (healing[name] == true)

	if player:get_hunger() > 3 and player:get_breath() > 0 then
		if not is_healing then
			healing[name] = true
		end
	else
		healing[name] = nil
	end
end


function core.hunger_event_handler(player, eventname)
	assert(player:is_player())

	local name = player:get_player_name()
	if not name or name == "" then
		return
	end

	if eventname == "health_changed" then
		local hp = player:get_hp()
		if hp > 19 then
			healing[name] = nil
			return
		else
			check_healing_state(player)
		end
	end

	if eventname == "breath_changed" or
			eventname == "hunger_changed" then
		check_healing_state(player)
	end
end


core.register_on_joinplayer(function(player)
	core.after(hunger_step, core.hunger_timer, player, 1)
end)
core.register_on_leaveplayer(function(player)
	local name = player:get_player_name()
	if not name or name == "" then
		return
	end
	healing[name] = nil
end)
core.register_playerevent(core.hunger_event_handler)

-- Init healing timer
core.heal_timer()
