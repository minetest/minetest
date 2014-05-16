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

--------------------------------------------------------------------------------
local function filter_texture_pack_list(list)
	retval = {"None"}
	for _,i in ipairs(list) do
		if i~="base" then
			table.insert(retval, i)
		end
	end
	return retval
end

--------------------------------------------------------------------------------
local function render_texture_pack_list(list)
	local retval = ""

	for i, v in ipairs(list) do
		if retval ~= "" then
			retval = retval ..","
		end

		retval = retval .. core.formspec_escape(v)
	end

	return retval
end

--------------------------------------------------------------------------------
local function get_formspec(tabview, name, tabdata)
	
	local retval = "label[4,-0.25;".. fgettext("Select texture pack:") .. "]"..
			"vertlabel[0,-0.25;".. fgettext("TEXTURE PACKS") .. "]" ..
			"textlist[4,0.25;7.5,5.0;TPs;"

	local current_texture_path = core.setting_get("texture_path")
	local list = filter_texture_pack_list(core.get_dirlist(core.get_texturepath(), true))
	local index = tonumber(core.setting_get("mainmenu_last_selected_TP"))

	if index == nil then index = 1 end

	if current_texture_path == "" then
		retval = retval ..
			render_texture_pack_list(list) ..
			";" .. index .. "]"
		return retval
	end

	local infofile = current_texture_path ..DIR_DELIM.."info.txt"
	local infotext = ""
	local f = io.open(infofile, "r")
	if f==nil then
		infotext = fgettext("No information available")
	else
		infotext = f:read("*all")
		f:close()
	end

	local screenfile = current_texture_path..DIR_DELIM.."screenshot.png"
	local no_screenshot = nil
	if not file_exists(screenfile) then
		screenfile = nil
		no_screenshot = defaulttexturedir .. "no_screenshot.png"
	end

	return	retval ..
			render_texture_pack_list(list) ..
			";" .. index .. "]" ..
			"image[0.65,0.25;4.0,3.7;"..core.formspec_escape(screenfile or no_screenshot).."]"..
			"textarea[1.0,3.25;3.7,1.5;;"..core.formspec_escape(infotext or "")..";]"
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)
	if fields["TPs"] ~= nil then
		local event = core.explode_textlist_event(fields["TPs"])
		if event.type == "CHG" or event.type == "DCL" then
			local index = core.get_textlist_index("TPs")
			core.setting_set("mainmenu_last_selected_TP",
				index)
			local list = filter_texture_pack_list(core.get_dirlist(core.get_texturepath(), true))
			local current_index = core.get_textlist_index("TPs")
			if current_index ~= nil and #list >= current_index then
				local new_path = core.get_texturepath()..DIR_DELIM..list[current_index]
				if list[current_index] == "None" then new_path = "" end

				core.setting_set("texture_path", new_path)
			end
		end
		return true
	end
	return false
end

--------------------------------------------------------------------------------
tab_texturepacks = {
	name = "texturepacks",
	caption = fgettext("Texturepacks"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = nil
	}
