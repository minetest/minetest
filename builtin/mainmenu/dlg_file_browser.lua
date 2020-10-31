-- Minetest
-- Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>
-- Copyright (C) 2020 EvidenceBKidscode

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

local ESC = core.formspec_escape
local PATH = os.getenv("HOME") or os.getenv("HOMEPATH") or core.get_worldpath()
local tabdata = {}

local function filesize(name)
	local f = io.open(name)
	if not f then return end
	local size = f:seek("end")
	f:close()

	return size
end

local function get_dirs()
	local dirs

	if PATH == "\\" then -- Windows root dir
		dirs = minetest.get_dir_list(os.getenv("HOMEDRIVE"))
	else
		dirs = minetest.get_dir_list(PATH)
	end

	local new = {}
	for _, f in ipairs(dirs) do
		if f:sub(1,1) ~= "." then
			local is_file = f:match("^.+(%..+)$")

			if not is_file then
				new[#new + 1] = f
			end
		end
	end

	table.sort(new)
	return new
end

local function make_fs()
	local dirs = get_dirs()
	local _path, value = "", ""

	for dirname in string.gmatch(PATH, "([^" .. DIR_DELIM .. "]+)") do
		value = value .. DIR_DELIM:gsub("\\", "\\\\") .. (dirname:gsub("_", "_u"):gsub(" ", "_s"))
		_path = ("%s %s <action name=%s>%s</action>")
			:format(_path, DIR_DELIM:gsub("\\", "\\\\\\\\"), value, dirname)
	end

	local fs = "size[10,7]" ..
		"real_coordinates[true]" ..
		"image_button[0.2,0.15;0.5,0.5;" ..
			ESC(defaulttexturedir .. "arrow_up.png") .. ";updir;]" ..
		"hypertext[1,0.25;9,0.5;path;" ..
			"<tag name=action color=#ffffcc hovercolor=#ffff00>" ..
			"<style size=20>" .. _path .. "</style>]" ..
		"tablecolumns[image," ..
			"0=" .. ESC(defaulttexturedir .. "folder.png") .. "," ..
			"1=" .. ESC(defaulttexturedir .. "file.png")   .. "," ..
			"2=" .. ESC(defaulttexturedir .. "blank.png") ..
			";text;text,align=right,padding=1]" ..
		"field[0.2,5.7;6.8,0.5;select;Selected file:;" ..
			(tabdata.filename or "") .. "]" ..
		"dropdown[7.2,5.7;2.6,0.5;extension;All files;" ..
			(tabdata.dd_selected or 1) .. "]" ..
		"button[2.8,6.35;2,0.5;ok;Open]" ..
		"button[5,6.35;2,0.5;cancel;Cancel]"

	local _dirs = ""

	if not next(dirs) then
		_dirs = "2,,"
	end

	for _, f in ipairs(dirs) do
		local is_file = f:match("^.+(%..+)$")
		_dirs = _dirs .. (is_file and "1," or "0,") .. f .. ","

		if is_file then
			local size = filesize(PATH .. DIR_DELIM .. f)
			if size ~= nil then
				size = size / 1000
				local unit = "KB"

				if size >= 1000 then
					unit = "MB"
					size = size / 1000
				end

				_dirs = _dirs .. string.format("%.1f", size) .. " " .. unit .. ","
			else
				_dirs = _dirs .. ","
			end
		else
			_dirs = _dirs .. ","
		end
	end

	fs = fs .. "table[0.2,0.8;9.6,4.4;dirs;" .. _dirs:sub(1,-2) ..
		";" .. (tabdata.selected or 1) .. "]"

	return fs
end

local function fields_handler(this, fields)
	if fields.cancel then
		this:delete()
		return true

	elseif fields.ok and tabdata.filename and tabdata.filename ~= "" then
		this:delete()
		return true
	end

	local dirs = get_dirs()

	if fields.dirs then
		local event, idx = fields.dirs:sub(1,3), tonumber(fields.dirs:match("%d+"))
		local filename = dirs[idx]
		local is_file = (filename ~= nil) and filename:find("^.+(%..+)$")

		if event == "CHG" then
			tabdata.filename = is_file and filename or ""
			tabdata.selected = idx

			core.update_formspec(this:get_formspec())
			return true

		elseif event == "DCL" and filename then
			if not is_file then
				PATH = PATH .. (PATH == DIR_DELIM and "" or DIR_DELIM) .. filename
				tabdata.selected = 1
			end

			core.update_formspec(this:get_formspec())
			return true
		end

	elseif fields.updir then
		PATH = string.split(PATH, DIR_DELIM)
		PATH[#PATH] = nil
		PATH = table.concat(PATH, DIR_DELIM)
		PATH = DIR_DELIM .. PATH

		tabdata.selected = 1

		core.update_formspec(this:get_formspec())
		return true

	elseif fields.path then
		local dir = fields.path:match("action:(.*)$")
		PATH = dir:gsub("_s", " "):gsub("_u", "_")
		core.update_formspec(this:get_formspec())
		return true

	elseif fields.extension == "All files" then
		tabdata.dd_selected = 1

		core.update_formspec(this:get_formspec())
		return true
	end

	return false
end

function create_file_browser_dlg()
	return dialog_create("file_browser", make_fs, fields_handler)
end
