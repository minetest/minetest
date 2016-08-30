--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

mt_color_grey  = "#AAAAAA"
mt_color_blue  = "#0000DD"
mt_color_green = "#00DD00"
mt_color_dark_green = "#003300"

--for all other colors ask sfan5 to complete his work!

local menu_path = core.get_mainmenu_path()
local base_path = core.get_builtin_path()
default_texture_dir = core.get_texturepath_share() .. "/base/pack"

dofile(base_path .. "/common/async_event.lua")
dofile(base_path .. "/common/filterlist.lua")
dofile(base_path .. "/fstk/buttonbar.lua")
dofile(base_path .. "/fstk/dialog.lua")
dofile(base_path .. "/fstk/tabview.lua")
dofile(base_path .. "/fstk/ui.lua")
dofile(menu_path .. "/common.lua")
dofile(menu_path .. "/gamemgr.lua")
dofile(menu_path .. "/modmgr.lua")
dofile(menu_path .. "/store.lua")
dofile(menu_path .. "/textures.lua")

dofile(menu_path .. "/dlg_config_world.lua")
dofile(menu_path .. "/dlg_settings_advanced.lua")
if PLATFORM ~= "Android" then
	dofile(menu_path .. "/dlg_create_world.lua")
	dofile(menu_path .. "/dlg_delete_mod.lua")
	dofile(menu_path .. "/dlg_delete_world.lua")
	dofile(menu_path .. "/dlg_rename_modpack.lua")
end

local tabs = {}

tabs.settings = dofile(menu_path .. "/tab_settings.lua")
tabs.mods = dofile(menu_path .. "/tab_mods.lua")
tabs.credits = dofile(menu_path .. "/tab_credits.lua")
if PLATFORM == "Android" then
	tabs.simple_main = dofile(menu_path .. "/tab_simple_main.lua")
else
	tabs.singleplayer = dofile(menu_path .. "/tab_singleplayer.lua")
	tabs.multiplayer = dofile(menu_path .. "/tab_multiplayer.lua")
	tabs.server = dofile(menu_path .. "/tab_server.lua")
	tabs.texturepacks = dofile(menu_path .. "/tab_texturepacks.lua")
end

--------------------------------------------------------------------------------
local function main_event_handler(tabview, event)
	if event == "MenuQuit" then
		core.close()
	end
	return true
end

--------------------------------------------------------------------------------
local function init_globals()
	-- Init gamedata
	gamedata.worldindex = 0

	if PLATFORM == "Android" then
		local world_list = core.get_worlds()
		local world_index

		local found_singleplayerworld = false
		for i, world in ipairs(world_list) do
			if world.name == "singleplayerworld" then
				found_singleplayerworld = true
				world_index = i
				break
			end
		end

		if not found_singleplayerworld then
			core.create_world("singleplayerworld", 1)

			world_list = core.get_worlds()

			for i, world in ipairs(world_list) do
				if world.name == "singleplayerworld" then
					world_index = i
					break
				end
			end
		end

		gamedata.worldindex = world_index
	else
		menudata.worldlist = filterlist.create(
			core.get_worlds,
			compare_worlds,
			-- Unique id comparison function
			function(element, uid)
				return element.name == uid
			end,
			-- Filter function
			function(element, gameid)
				return element.gameid == gameid
			end
		)

		menudata.worldlist:add_sort_mechanism("alphabetic", sort_worlds_alphabetic)
		menudata.worldlist:set_sortmode("alphabetic")

		if not core.setting_get("menu_last_game") then
			local default_game = core.setting_get("default_game") or "minetest"
			core.setting_set("menu_last_game", default_game)
		end

		mm_texture.init()
	end

	-- Create main tabview
	local tv_main = tabview_create("maintab", {x = 12, y = 5.2}, {x = 0, y = 0})

	if PLATFORM == "Android" then
		tv_main:add(tabs.simple_main)
		tv_main:add(tabs.settings)
	else
		tv_main:set_autosave_tab(true)
		tv_main:add(tabs.singleplayer)
		tv_main:add(tabs.multiplayer)
		tv_main:add(tabs.server)
		tv_main:add(tabs.settings)
		tv_main:add(tabs.texturepacks)
	end

	tv_main:add(tabs.mods)
	tv_main:add(tabs.credits)

	tv_main:set_global_event_handler(main_event_handler)
	tv_main:set_fixed_size(false)

	if PLATFORM ~= "Android" then
		tv_main:set_tab(core.setting_get("maintab_LAST"))
	end
	ui.set_default("maintab")
	tv_main:show()

	-- Create modstore ui
	if PLATFORM == "Android" then
		modstore.init({x = 12, y = 6}, 3, 2)
	else
		modstore.init({x = 12, y = 8}, 4, 3)
	end

	ui.update()

	core.sound_play("main_menu", true)
end

init_globals()

