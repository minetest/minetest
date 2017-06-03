--Minetest
--Copyright (C) 2013 sapier
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

--------------------------------------------------------------------------------

local enabled_all = false

local function modname_valid(name)
	return not name:find("[^a-z0-9_]")
end

local function get_formspec(data)
	local mod = data.list:get_list()[data.selected_mod]

	local retval =
		"size[11.5,7.5,true]" ..
		"label[0.5,0;" .. fgettext("World:") .. "]" ..
		"label[1.75,0;" .. data.worldspec.name .. "]"

	if mod == nil then
		mod = {name=""}
	end

	local hard_deps, soft_deps = modmgr.get_dependencies(mod.path)

	retval = retval ..
		"label[0,0.7;" .. fgettext("Mod:") .. "]" ..
		"label[0.75,0.7;" .. mod.name .. "]" ..
		"label[0,1.25;" .. fgettext("Dependencies:") .. "]" ..
		"textlist[0,1.75;5,2.125;world_config_depends;" ..
		hard_deps .. ";0]" ..
		"label[0,3.875;" .. fgettext("Optional dependencies:") .. "]" ..
		"textlist[0,4.375;5,1.8;world_config_optdepends;" ..
		soft_deps .. ";0]" ..
		"button[3.25,7;2.5,0.5;btn_config_world_save;" .. fgettext("Save") .. "]" ..
		"button[5.75,7;2.5,0.5;btn_config_world_cancel;" .. fgettext("Cancel") .. "]"

	if mod and mod.name ~= "" and not mod.is_game_content then
		if mod.is_modpack then
			local rawlist = data.list:get_raw_list()

			local all_enabled = true
			for j = 1, #rawlist, 1 do
				if rawlist[j].modpack == mod.name and not rawlist[j].enabled then
					all_enabled = false
					break
				end
			end

			if all_enabled then
				retval = retval .. "button[5.5,0.125;2.5,0.5;btn_mp_disable;" ..
					fgettext("Disable MP") .. "]"
			else
				retval = retval .. "button[5.5,0.125;2.5,0.5;btn_mp_enable;" ..
					fgettext("Enable MP") .. "]"
			end
		else
			if mod.enabled then
				retval = retval .. "checkbox[5.5,-0.125;cb_mod_enable;" ..
					fgettext("enabled") .. ";true]"
			else
				retval = retval .. "checkbox[5.5,-0.125;cb_mod_enable;" ..
					fgettext("enabled") .. ";false]"
			end
		end
	end
	if enabled_all then
		retval = retval ..
			"button[8.75,0.125;2.5,0.5;btn_disable_all_mods;" .. fgettext("Disable all") .. "]"
	else
		retval = retval ..
			"button[8.75,0.125;2.5,0.5;btn_enable_all_mods;" .. fgettext("Enable all") .. "]"
	end
	retval = retval ..
		"tablecolumns[color;tree;text]" ..
		"table[5.5,0.75;5.75,6;world_config_modlist;"
	retval = retval .. modmgr.render_modlist(data.list)
	retval = retval .. ";" .. data.selected_mod .."]"

	return retval
end

local function enable_mod(this, toset)
	local mod = this.data.list:get_list()[this.data.selected_mod]

	if mod.is_game_content then
		-- game mods can't be enabled or disabled
	elseif not mod.is_modpack then
		if toset == nil then
			mod.enabled = not mod.enabled
		else
			mod.enabled = toset
		end
	else
		local list = this.data.list:get_raw_list()
		for i=1,#list,1 do
			if list[i].modpack == mod.name then
				if toset == nil then
					toset = not list[i].enabled
				end
				list[i].enabled = toset
			end
		end
	end
end


local function handle_buttons(this, fields)
	if fields["world_config_modlist"] ~= nil then
		local event = core.explode_table_event(fields["world_config_modlist"])
		this.data.selected_mod = event.row
		core.settings:set("world_config_selected_mod", event.row)

		if event.type == "DCL" then
			enable_mod(this)
		end

		return true
	end

	if fields["key_enter"] ~= nil then
		enable_mod(this)
		return true
	end

	if fields["cb_mod_enable"] ~= nil then
		local toset = core.is_yes(fields["cb_mod_enable"])
		enable_mod(this,toset)
		return true
	end

	if fields["btn_mp_enable"] ~= nil or
		fields["btn_mp_disable"] then
		local toset = (fields["btn_mp_enable"] ~= nil)
		enable_mod(this,toset)
		return true
	end

	if fields["btn_config_world_save"] then
		local filename = this.data.worldspec.path ..
				DIR_DELIM .. "world.mt"

		local worldfile = Settings(filename)
		local mods = worldfile:to_table()

		local rawlist = this.data.list:get_raw_list()

		local i,mod
		for i,mod in ipairs(rawlist) do
			if not mod.is_modpack and
					not mod.is_game_content then
				if modname_valid(mod.name) then
					worldfile:set("load_mod_"..mod.name, tostring(mod.enabled))
				else
					if mod.enabled then
						gamedata.errormessage = fgettext_ne("Failed to enable mod \"$1\" as it contains disallowed characters. Only chararacters [a-z0-9_] are allowed.", mod.name)
					end
				end
				mods["load_mod_"..mod.name] = nil
			end
		end

		-- Remove mods that are not present anymore
		for key,value in pairs(mods) do
			if key:sub(1,9) == "load_mod_" then
				worldfile:remove(key)
			end
		end

		if not worldfile:write() then
			core.log("error", "Failed to write world config file")
		end

		this:delete()
		return true
	end

	if fields["btn_config_world_cancel"] then
		this:delete()
		return true
	end

	if fields.btn_enable_all_mods then
		local list = this.data.list:get_raw_list()

		for i = 1, #list do
			if not list[i].is_game_content
					and not list[i].is_modpack then
				list[i].enabled = true
			end
		end
		enabled_all = true
		return true
	end

	if fields.btn_disable_all_mods then
		local list = this.data.list:get_raw_list()

		for i = 1, #list do
			if not list[i].is_game_content
					and not list[i].is_modpack then
				list[i].enabled = false
			end
		end
		enabled_all = false
		return true
	end

	return false
end

function create_configure_world_dlg(worldidx)
	local dlg = dialog_create("sp_config_world",
					get_formspec,
					handle_buttons,
					nil)

	dlg.data.selected_mod = tonumber(core.settings:get("world_config_selected_mod"))
	if dlg.data.selected_mod == nil then
		dlg.data.selected_mod = 0
	end

	dlg.data.worldspec = core.get_worlds()[worldidx]
	if dlg.data.worldspec == nil then dlg:delete() return nil end

	dlg.data.worldconfig = modmgr.get_worldconfig(dlg.data.worldspec.path)

	if dlg.data.worldconfig == nil or dlg.data.worldconfig.id == nil or
			dlg.data.worldconfig.id == "" then

		dlg:delete()
		return nil
	end

	dlg.data.list = filterlist.create(
			modmgr.preparemodlist, --refresh
			modmgr.comparemod, --compare
			function(element,uid) --uid match
					if element.name == uid then
						return true
					end
				end,
				function(element, criteria)
					if criteria.hide_game and
							element.is_game_content then
						return false
					end

					if criteria.hide_modpackcontents and
							element.modpack ~= nil then
						return false
					end
					return true
				end, --filter
				{ worldpath= dlg.data.worldspec.path,
				  gameid = dlg.data.worldspec.gameid }
			)


	if dlg.data.selected_mod > dlg.data.list:size() then
		dlg.data.selected_mod = 0
	end

	dlg.data.list:set_filtercriteria(
		{
			hide_game=dlg.data.hide_gamemods,
			hide_modpackcontents= dlg.data.hide_modpackcontents
		})
	dlg.data.list:add_sort_mechanism("alphabetic", sort_mod_list)
	dlg.data.list:set_sortmode("alphabetic")

	return dlg
end
