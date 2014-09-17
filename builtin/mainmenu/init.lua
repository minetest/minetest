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

local menupath = core.get_mainmenu_path()
local basepath = core.get_builtin_path()
defaulttexturedir = core.get_texturepath_share() .. DIR_DELIM .. "base" ..
					DIR_DELIM .. "pack" .. DIR_DELIM

dofile(basepath .. DIR_DELIM .. "common" .. DIR_DELIM .. "async_event.lua")
dofile(basepath .. DIR_DELIM .. "common" .. DIR_DELIM .. "filterlist.lua")
dofile(basepath .. DIR_DELIM .. "fstk" .. DIR_DELIM .. "buttonbar.lua")
dofile(basepath .. DIR_DELIM .. "fstk" .. DIR_DELIM .. "dialog.lua")
dofile(basepath .. DIR_DELIM .. "fstk" .. DIR_DELIM .. "tabview.lua")
dofile(basepath .. DIR_DELIM .. "fstk" .. DIR_DELIM .. "ui.lua")
dofile(menupath .. DIR_DELIM .. "common.lua")
dofile(menupath .. DIR_DELIM .. "gamemgr.lua")
dofile(menupath .. DIR_DELIM .. "modmgr.lua")
dofile(menupath .. DIR_DELIM .. "store.lua")
dofile(menupath .. DIR_DELIM .. "dlg_config_world.lua")
dofile(menupath .. DIR_DELIM .. "dlg_create_world.lua")
dofile(menupath .. DIR_DELIM .. "dlg_delete_mod.lua")
dofile(menupath .. DIR_DELIM .. "dlg_delete_world.lua")
dofile(menupath .. DIR_DELIM .. "dlg_rename_modpack.lua")
dofile(menupath .. DIR_DELIM .. "tab_credits.lua")
dofile(menupath .. DIR_DELIM .. "tab_mods.lua")
dofile(menupath .. DIR_DELIM .. "tab_multiplayer.lua")
dofile(menupath .. DIR_DELIM .. "tab_server.lua")
dofile(menupath .. DIR_DELIM .. "tab_settings.lua")
dofile(menupath .. DIR_DELIM .. "tab_singleplayer.lua")
dofile(menupath .. DIR_DELIM .. "tab_texturepacks.lua")
dofile(menupath .. DIR_DELIM .. "textures.lua")

--------------------------------------------------------------------------------
local function main_event_handler(tabview,event)
	if event == "MenuQuit" then
		core.close()
	end
	return true
end

--------------------------------------------------------------------------------
local function init_globals()
	--init gamedata
	gamedata.worldindex = 0

	menudata.worldlist = filterlist.create(
					core.get_worlds,
					compare_worlds,
					function(element,uid)
						if element.name == uid then
							return true
						end
						return false
					end, --unique id compare fct
					function(element,gameid)
						if element.gameid == gameid then
							return true
						end
						return false
					end --filter fct
					)

	menudata.worldlist:add_sort_mechanism("alphabetic",sort_worlds_alphabetic)
	menudata.worldlist:set_sortmode("alphabetic")

	if not core.setting_get("menu_last_game") then
		local default_game = core.setting_get("default_game") or "minetest"
		core.setting_set("menu_last_game", default_game )
	end

	mm_texture.init()

	--create main tabview
	local tv_main = tabview_create("maintab",{x=12,y=5.2},{x=0,y=0})
	tv_main:set_autosave_tab(true)
	tv_main:add(tab_singleplayer)
	tv_main:add(tab_multiplayer)
	tv_main:add(tab_server)
	tv_main:add(tab_settings)
	tv_main:add(tab_texturepacks)
	tv_main:add(tab_mods)
	tv_main:add(tab_credits)

	tv_main:set_global_event_handler(main_event_handler)

	tv_main:set_tab(core.setting_get("maintab_LAST"))
	ui.set_default("maintab")
	tv_main:show()

	--create modstore ui
	modstore.init({x=12,y=8},4,3)

	ui.update()

	core.sound_play("main_menu", true)
end

init_globals()
