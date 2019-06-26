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


local function current_game()
	local last_game_id = core.settings:get("menu_last_game")
	local game, index = pkgmgr.find_by_gameid(last_game_id)

	return game
end

local function get_formspec(tabview, name, tabdata)
	local retval = ""

	local index = filterlist.get_current_index(menudata.worldlist,
			tonumber(core.settings:get("mainmenu_last_selected_world")))

	retval = {
		"container[0.375,0.375]",
		--"field[1.75,0;5.125,0.8;query;;", query, "]",
		--"button[6.875,0;1.5,0.8;search;", fgettext("Search"), "]",
		"textlist[0,0;8.375,7.2;sp_worlds;", menu_render_worldlist(), ";", index, "]",
	}

	if index > 0 then
		retval[#retval + 1] = "container[0,7.45]"
		retval[#retval + 1] = "button[0,0;2.625,0.8;world_delete;"
		retval[#retval + 1] = fgettext("Delete")
		retval[#retval + 1] = "]"
		retval[#retval + 1] = "button[2.875,0;2.625,0.8;world_configure;"
		retval[#retval + 1] = fgettext("Configure")
		retval[#retval + 1] = "]"
		retval[#retval + 1] = "style[play;bgcolor=#53AC56]"
		retval[#retval + 1] = "button[5.75,0;2.625,0.8;play;"
		retval[#retval + 1] = fgettext("Play World")
		retval[#retval + 1] = "]"
	--	"button[4,3.95;2.6,1;world_delete;", fgettext("Delete"), "]",
	--"button[6.5,3.95;2.8,1;world_configure;", fgettext("Configure"), "]",
		retval[#retval + 1] = "container_end[]"
	else
		retval[#retval + 1] = "container[0,7.45]"
		retval[#retval + 1] = "box[0,0;2.625,0.8;#333]"
		retval[#retval + 1] = "box[2.875,0;2.625,0.8;#333]"
		retval[#retval + 1] = "box[5.75,0;2.625,0.8;#333]"
		retval[#retval + 1] = "container_end[]"
	end

	do
		retval[#retval + 1] = "container[8.75,1.15]"
		local enable_server = core.settings:get_bool("enable_server")
		local options = {
			{ name = "creative_mode", text = fgettext("Creative Mode") },
			{ name = "enable_damage", text = fgettext("Enable Damage") },
			{ name = "enable_server", text = fgettext("Host Server") },
		}

		if enable_server then
			options[#options + 1] = { name = "server_announce", text = fgettext("Announce Server") }
		end

		local y = 0
		for _, opt in pairs(options) do
			retval[#retval + 1] = "checkbox[0,"
			retval[#retval + 1] = y
			retval[#retval + 1] =";cb_"
			retval[#retval + 1] = opt.name
			retval[#retval + 1] = ";"
			retval[#retval + 1] = opt.text
			retval[#retval + 1] = ";"
			retval[#retval + 1] = dump(core.settings:get_bool(opt.name))
			retval[#retval + 1] = "]"

			y = y + 0.5
		end

		y = y + 0.25

		if enable_server then
			retval[#retval + 1] = "container[0,"
			retval[#retval + 1] = y
			retval[#retval + 1] = "]label[0,0;"
			retval[#retval + 1] = fgettext("Name/Password")
			retval[#retval + 1] = "]"
			retval[#retval + 1] = "field[0,0.3;3.5,0.5;te_playername;;"
			retval[#retval + 1] = core.formspec_escape(core.settings:get("name"))
			retval[#retval + 1] = "]"
			retval[#retval + 1] = "pwdfield[0,1;3.5,0.5;te_passwd;]"

			local bind_addr = core.settings:get("bind_address")
			if bind_addr ~= nil and bind_addr ~= "" then
				retval[#retval + 1] = "field[0,2.2;2.25,0.5;te_serveraddr;" .. fgettext("Bind Address")
				retval[#retval + 1] = ";"
				retval[#retval + 1] = ore.formspec_escape(core.settings:get("bind_address"))
				retval[#retval + 1] = "]"
				retval[#retval + 1] = "field[2.3,2.2;1.25,0.5;te_serverport;"
				retval[#retval + 1] = fgettext("Port")
				retval[#retval + 1] = ";"
				retval[#retval + 1] = core.formspec_escape(core.settings:get("port"))
				retval[#retval + 1] = "]"
			else
				retval[#retval + 1] = "field[0,2.2;3.5,0.5;te_serverport;"
				retval[#retval + 1] = fgettext("Server Port")
				retval[#retval + 1] = ";"
				retval[#retval + 1] = core.formspec_escape(core.settings:get("port"))
				retval[#retval + 1] = "]"
			end
			retval[#retval + 1] = "container_end[]"
		end
		retval[#retval + 1] = "container_end[]"
	end

	return table.concat(retval, "") .. "container_end[]"
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

	if fields["cb_enable_server"] then
		core.settings:set("enable_server", fields["cb_enable_server"])

		return true
	end

	if fields["cb_server_announce"] then
		core.settings:set("server_announce", fields["cb_server_announce"])
		local selected = core.get_textlist_index("srv_worlds")
		menu_worldmt(selected, "server_announce", fields["cb_server_announce"])

		return true
	end

	if fields["play"] ~= nil or world_doubleclick then
		local selected = core.get_textlist_index("sp_worlds")
		gamedata.selected_world = menudata.worldlist:get_raw_index(selected)

		if core.settings:get_bool("enable_server") then
			if selected ~= nil and gamedata.selected_world ~= 0 then
				gamedata.playername = fields["te_playername"]
				gamedata.password   = fields["te_passwd"]
				gamedata.port       = fields["te_serverport"]
				gamedata.address    = ""

				core.settings:set("port",gamedata.port)
				if fields["te_serveraddr"] ~= nil then
					core.settings:set("bind_address",fields["te_serveraddr"])
				end

				--update last game
				local world = menudata.worldlist:get_raw_element(gamedata.selected_world)
				if world then
					local game, index = pkgmgr.find_by_gameid(world.gameid)
					core.settings:set("menu_last_game", game.id)
				end

				core.start()
			else
				gamedata.errormessage =
					fgettext("No world created or selected!")
			end
		else
			if selected ~= nil and gamedata.selected_world ~= 0 then
				gamedata.singleplayer = true
				core.start()
			else
				gamedata.errormessage =
					fgettext("No world created or selected!")
			end
			return true
		end
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

local function on_change(type, old_tab, new_tab)
	if type == "ENTER" then
		local game = current_game()

		if game then
			menudata.worldlist:set_filtercriteria(game.id)
			core.set_topleft_text(game.name)
			mm_texture.update("singleplayer",game)
		end
	else
		menudata.worldlist:set_filtercriteria(nil)
		core.set_topleft_text("")
		mm_texture.update(new_tab,nil)
	end
end

--------------------------------------------------------------------------------
return {
	name = "local",
	caption = fgettext("Load World"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
