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
	local retval = {}

	for _, item in ipairs(list) do
		if item ~= "base" then
			retval[#retval + 1] = item
		end
	end

	table.sort(retval)
	table.insert(retval, 1, fgettext("None"))

	return retval
end

--------------------------------------------------------------------------------
local function render_texture_pack_list(list)
	local retval = ""

	for i, v in ipairs(list) do
		if v:sub(1, 1) ~= "." then
			if retval ~= "" then
				retval = retval .. ","
			end

			retval = retval .. core.formspec_escape(v)
		end
	end

	return retval
end

--------------------------------------------------------------------------------
local function get_formspec(tabview, name, tabdata)

	local retval = "label[4,-0.25;" .. fgettext("Select texture pack:") .. "]" ..
			"textlist[4,0.25;7.5,5.0;TPs;"

	local current_texture_path = core.settings:get("texture_path")
	local list = filter_texture_pack_list(core.get_dir_list(core.get_texturepath(), true))
	local index = tonumber(core.settings:get("mainmenu_last_selected_TP"))

	if not index then index = 1 end

	if current_texture_path == "" then
		retval = retval ..
			render_texture_pack_list(list) ..
			";" .. index .. "]"
		return retval
	end

	local infofile = current_texture_path .. DIR_DELIM .. "description.txt"
	-- This adds backwards compatibility for old texture pack description files named
	-- "info.txt", and should be removed once all such texture packs have been updated
	if not file_exists(infofile) then
		infofile = current_texture_path .. DIR_DELIM .. "info.txt"
		if file_exists(infofile) then
			core.log("deprecated", "info.txt is deprecated. description.txt should be used instead.")
		end
	end

	local infotext = ""
	local f = io.open(infofile, "r")
	if not f then
		infotext = fgettext("No information available")
	else
		infotext = f:read("*all")
		f:close()
	end

	local screenfile = current_texture_path .. DIR_DELIM .. "screenshot.png"
	local no_screenshot
	if not file_exists(screenfile) then
		screenfile = nil
		no_screenshot = defaulttexturedir .. "no_screenshot.png"
	end

	return	retval ..
			render_texture_pack_list(list) ..
			";" .. index .. "]" ..
			"image[0.25,0.25;4.05,2.7;" .. core.formspec_escape(screenfile or no_screenshot) .. "]" ..
			"textarea[0.6,2.85;3.7,1.5;;" .. core.formspec_escape(infotext or "") .. ";]"
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)
	if fields["TPs"] then
		local event = core.explode_textlist_event(fields["TPs"])
		if event.type == "CHG" or event.type == "DCL" then
			local index = core.get_textlist_index("TPs")
			core.settings:set("mainmenu_last_selected_TP", index)
			local list = filter_texture_pack_list(core.get_dir_list(core.get_texturepath(), true))
			local current_index = core.get_textlist_index("TPs")
			if current_index and #list >= current_index then
				local new_path = core.get_texturepath() .. DIR_DELIM .. list[current_index]
				if list[current_index] == fgettext("None") then
					new_path = ""
				end
				core.settings:set("texture_path", new_path)
			end
		end
		return true
	end
	return false
end

--------------------------------------------------------------------------------
return {
	name = "texturepacks",
	caption = fgettext("Texturepacks"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = nil
}
