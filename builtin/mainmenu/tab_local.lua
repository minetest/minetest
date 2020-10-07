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


local enable_gamebar = PLATFORM ~= "Android"
local current_game, singleplayer_refresh_gamebar
if enable_gamebar then
	function current_game()
		local last_game_id = core.settings:get("menu_last_game")
		local game = pkgmgr.find_by_gameid(last_game_id)

		return game
	end

	function singleplayer_refresh_gamebar()

		local old_bar = ui.find_by_name("game_button_bar")

		if old_bar ~= nil then
			old_bar:delete()
		end

		local function game_buttonbar_button_handler(fields)
			if fields.game_open_cdb then
				local maintab = ui.find_by_name("maintab")
				local dlg = create_store_dlg("game")
				dlg:set_parent(maintab)
				maintab:hide()
				dlg:show()
				return true
			end

			for key,value in pairs(fields) do
				for j=1,#pkgmgr.games,1 do
					if ("game_btnbar_" .. pkgmgr.games[j].id == key) then
						mm_texture.update("singleplayer", pkgmgr.games[j])
						core.set_topleft_text(pkgmgr.games[j].name)
						core.settings:set("menu_last_game",pkgmgr.games[j].id)
						menudata.worldlist:set_filtercriteria(pkgmgr.games[j].id)
						local index = filterlist.get_current_index(menudata.worldlist,
							tonumber(core.settings:get("mainmenu_last_selected_world")))
						if not index or index < 1 then
							local selected = core.get_textlist_index("sp_worlds")
							if selected ~= nil and selected < #menudata.worldlist:get_list() then
								index = selected
							else
								index = #menudata.worldlist:get_list()
							end
						end
						menu_worldmt_legacy(index)
						return true
					end
				end
			end
		end

		local btnbar = buttonbar_create("game_button_bar",
			game_buttonbar_button_handler,
			{x=-0.3,y=5.9}, "horizontal", {x=12.4,y=1.15})

		for i=1,#pkgmgr.games,1 do
			local btn_name = "game_btnbar_" .. pkgmgr.games[i].id

			local image = nil
			local text = nil
			local tooltip = core.formspec_escape(pkgmgr.games[i].name)

			if pkgmgr.games[i].menuicon_path ~= nil and
				pkgmgr.games[i].menuicon_path ~= "" then
				image = core.formspec_escape(pkgmgr.games[i].menuicon_path)
			else

				local part1 = pkgmgr.games[i].id:sub(1,5)
				local part2 = pkgmgr.games[i].id:sub(6,10)
				local part3 = pkgmgr.games[i].id:sub(11)

				text = part1 .. "\n" .. part2
				if part3 ~= nil and
					part3 ~= "" then
					text = text .. "\n" .. part3
				end
			end
			btnbar:add_button(btn_name, text, image, tooltip)
		end

		local plus_image = core.formspec_escape(defaulttexturedir .. "plus.png")
		btnbar:add_button("game_open_cdb", "", plus_image, fgettext("Install games from ContentDB"))
	end
else
	function current_game()
		return nil
	end
end

local function get_formspec(tabview, name, tabdata)
	local retval = ""

	local index = filterlist.get_current_index(menudata.worldlist,
				tonumber(core.settings:get("mainmenu_last_selected_world"))
				)

	retval = retval ..
			"button[3.9,3.8;2.8,1;world_delete;".. fgettext("Delete") .. "]" ..
			"button[6.55,3.8;2.8,1;world_configure;".. fgettext("Configure") .. "]" ..
			"button[9.2,3.8;2.8,1;world_create;".. fgettext("New") .. "]" ..
			"label[3.9,-0.05;".. fgettext("Select World:") .. "]"..
			"checkbox[0,-0.20;cb_creative_mode;".. fgettext("Creative Mode") .. ";" ..
			dump(core.settings:get_bool("creative_mode")) .. "]"..
			"checkbox[0,0.25;cb_enable_damage;".. fgettext("Enable Damage") .. ";" ..
			dump(core.settings:get_bool("enable_damage")) .. "]"..
			"checkbox[0,0.7;cb_server;".. fgettext("Host Server") ..";" ..
			dump(core.settings:get_bool("enable_server")) .. "]" ..
			"textlist[3.9,0.4;7.9,3.45;sp_worlds;" ..
			menu_render_worldlist() ..
			";" .. index .. "]"

	if core.settings:get_bool("enable_server") then
		retval = retval ..
				"button[7.9,4.75;4.1,1;play;".. fgettext("Host Game") .. "]" ..
				"checkbox[0,1.15;cb_server_announce;" .. fgettext("Announce Server") .. ";" ..
				dump(core.settings:get_bool("server_announce")) .. "]" ..
				"field[0.3,2.85;3.8,0.5;te_playername;" .. fgettext("Name") .. ";" ..
				core.formspec_escape(core.settings:get("name")) .. "]" ..
				"pwdfield[0.3,4.05;3.8,0.5;te_passwd;" .. fgettext("Password") .. "]"

		local bind_addr = core.settings:get("bind_address")
		if bind_addr ~= nil and bind_addr ~= "" then
			retval = retval ..
				"field[0.3,5.25;2.5,0.5;te_serveraddr;" .. fgettext("Bind Address") .. ";" ..
				core.formspec_escape(core.settings:get("bind_address")) .. "]" ..
				"field[2.85,5.25;1.25,0.5;te_serverport;" .. fgettext("Port") .. ";" ..
				core.formspec_escape(core.settings:get("port")) .. "]"
		else
			retval = retval ..
				"field[0.3,5.25;3.8,0.5;te_serverport;" .. fgettext("Server Port") .. ";" ..
				core.formspec_escape(core.settings:get("port")) .. "]"
		end
	else
		retval = retval ..
				"button[7.9,4.75;4.1,1;play;" .. fgettext("Play Game") .. "]"
	end

	return retval
end

local function main_button_handler(this, fields, name, tabdata)

	assert(name == "local")

	local world_doubleclick = false

	if fields["sp_worlds"] ~= nil then
		local event = core.explode_textlist_event(fields["sp_worlds"])
		local selected = core.get_textlist_index("sp_worlds")

		menu_worldmt_legacy(selected)

		if event.type == "DCL" then
			world_doubleclick = true
		end

		if event.type == "CHG" and selected ~= nil then
			core.settings:set("mainmenu_last_selected_world",
				menudata.worldlist:get_raw_index(selected))
			return true
		end
	end

	if menu_handle_key_up_down(fields,"sp_worlds","mainmenu_last_selected_world") then
		return true
	end

	if fields["cb_creative_mode"] then
		core.settings:set("creative_mode", fields["cb_creative_mode"])
		local selected = core.get_textlist_index("sp_worlds")
		menu_worldmt(selected, "creative_mode", fields["cb_creative_mode"])

		return true
	end

	if fields["cb_enable_damage"] then
		core.settings:set("enable_damage", fields["cb_enable_damage"])
		local selected = core.get_textlist_index("sp_worlds")
		menu_worldmt(selected, "enable_damage", fields["cb_enable_damage"])

		return true
	end

	if fields["cb_server"] then
		core.settings:set("enable_server", fields["cb_server"])

		return true
	end

	if fields["cb_server_announce"] then
		core.settings:set("server_announce", fields["cb_server_announce"])
		local selected = core.get_textlist_index("srv_worlds")
		menu_worldmt(selected, "server_announce", fields["cb_server_announce"])

		return true
	end

	if fields["play"] ~= nil or world_doubleclick or fields["key_enter"] then
		local selected = core.get_textlist_index("sp_worlds")
		gamedata.selected_world = menudata.worldlist:get_raw_index(selected)

		if selected == nil or gamedata.selected_world == 0 then
			gamedata.errormessage =
					fgettext("No world created or selected!")
			return true
		end

		-- Update last game
		local world = menudata.worldlist:get_raw_element(gamedata.selected_world)
		if world then
			local game = pkgmgr.find_by_gameid(world.gameid)
			core.settings:set("menu_last_game", game.id)
		end

		if core.settings:get_bool("enable_server") then
			gamedata.playername = fields["te_playername"]
			gamedata.password   = fields["te_passwd"]
			gamedata.port       = fields["te_serverport"]
			gamedata.address    = ""

			core.settings:set("port",gamedata.port)
			if fields["te_serveraddr"] ~= nil then
				core.settings:set("bind_address",fields["te_serveraddr"])
			end
		else
			gamedata.singleplayer = true
		end

		core.start()
		return true
	end

	if fields["world_create"] ~= nil then
		local create_world_dlg = create_create_world_dlg(true)
		create_world_dlg:set_parent(this)
		this:hide()
		create_world_dlg:show()
		mm_texture.update("singleplayer", current_game())
		return true
	end

	if fields["world_delete"] ~= nil then
		local selected = core.get_textlist_index("sp_worlds")
		if selected ~= nil and
			selected <= menudata.worldlist:size() then
			local world = menudata.worldlist:get_list()[selected]
			if world ~= nil and
				world.name ~= nil and
				world.name ~= "" then
				local index = menudata.worldlist:get_raw_index(selected)
				local delete_world_dlg = create_delete_world_dlg(world.name,index)
				delete_world_dlg:set_parent(this)
				this:hide()
				delete_world_dlg:show()
				mm_texture.update("singleplayer",current_game())
			end
		end

		return true
	end

	if fields["world_configure"] ~= nil then
		local selected = core.get_textlist_index("sp_worlds")
		if selected ~= nil then
			local configdialog =
				create_configure_world_dlg(
						menudata.worldlist:get_raw_index(selected))

			if (configdialog ~= nil) then
				configdialog:set_parent(this)
				this:hide()
				configdialog:show()
				mm_texture.update("singleplayer",current_game())
			end
		end

		return true
	end
end

local on_change
if enable_gamebar then
	function on_change(type, old_tab, new_tab)
		if (type == "ENTER") then
			local game = current_game()

			if game then
				menudata.worldlist:set_filtercriteria(game.id)
				core.set_topleft_text(game.name)
				mm_texture.update("singleplayer",game)
			end

			singleplayer_refresh_gamebar()
			ui.find_by_name("game_button_bar"):show()
		else
			menudata.worldlist:set_filtercriteria(nil)
			local gamebar = ui.find_by_name("game_button_bar")
			if gamebar then
				gamebar:hide()
			end
			core.set_topleft_text("")
			mm_texture.update(new_tab,nil)
		end
	end
end

--------------------------------------------------------------------------------
return {
	name = "local",
	caption = fgettext("Start Game"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
