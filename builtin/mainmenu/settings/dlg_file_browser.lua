-- Minetest
-- Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>
-- Copyright (C) 2020 EvidenceBKidscode
-- Copyright (C) 2023 ROllerozxa
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

local PATH = os.getenv("HOME") -- Linux & similar
		or os.getenv("HOMEDRIVE") .. os.getenv("HOMEPATH") -- Windows
		or core.get_user_path() -- If nothing else works

local tabdata = {}

local function texture(filename)
	return core.formspec_escape(defaulttexturedir .. filename .. ".png")
end

local function get_dirs()
	local dirs

	if PATH == "\\" then -- Windows root dir
		dirs = core.get_dir_list(os.getenv("HOMEDRIVE"))
	else
		dirs = core.get_dir_list(PATH)
	end

	local new = {}
	for _, f in ipairs(dirs) do
		if f:sub(1, 1) ~= "." then
			new[#new + 1] = core.formspec_escape(f)
		end
	end

	table.sort(new)
	return new
end

local function make_fs(dialogdata)
	local dirs = get_dirs()

	local _dirs = {}

	if not next(dirs) then
		_dirs[#_dirs + 1] = "2,,"
	end

	for _, f in ipairs(dirs) do
		local is_file = not core.is_dir(PATH .. DIR_DELIM .. f)
		_dirs[#_dirs + 1] = (is_file and "1," or "0,") .. f .. ","
	end

	local field_label
	if dialogdata.type == 'file' then
		field_label = fgettext("Selected file:")
	elseif dialogdata.type == 'folder' then
		field_label = fgettext("Selected folder:")
	end

	return table.concat({
		"formspec_version[4]",
		"size[14,9.2]",
		"image_button[0.2,0.2;0.65,0.65;", texture('up_icon'), ";updir;]",
		"field[1,0.2;12,0.65;path;;", core.formspec_escape(PATH), "]",
		"tablecolumns[image,",
			"0=", texture('folder'), ",",
			"1=", texture('file'), ",",
			"2=", texture('blank'),
			";text]",
		"table[0.2,1.1;13.6,6;dirs;", table.concat(_dirs):sub(1, -2),
			";", (tabdata.selected or 1), "]",
		"field[0.2,8;9,0.8;select;", field_label, ";", (tabdata.filename or ""), "]",
		"button[9.5,8;2,0.8;ok;", fgettext("Open"), "]",
		"button[11.7,8;2,0.8;cancel;", fgettext("Cancel"), "]"
	})
end

local function fields_handler(this, fields)
	if fields.cancel then
		this:delete()
		return true

	elseif fields.ok and tabdata.filename and tabdata.filename ~= "" then
		local is_file = not core.is_dir(PATH .. DIR_DELIM .. tabdata.filename)

		if (this.data.type == "file" and is_file)
		or (this.data.type == "folder" and not is_file) then
			core.settings:set(this.data.setting, PATH .. DIR_DELIM .. tabdata.filename)

			this:delete()
			return true
		end
	end

	local dirs = get_dirs()

	if fields.dirs then
		local event, idx = fields.dirs:sub(1, 3), tonumber(fields.dirs:match("%d+"))
		local filename = dirs[idx]
		local is_file = not core.is_dir(PATH .. DIR_DELIM .. filename)

		if event == "CHG" then
			tabdata.filename = filename
			tabdata.selected = idx

			return true

		elseif event == "DCL" and filename then
			if not is_file then
				PATH = PATH .. (PATH == DIR_DELIM and "" or DIR_DELIM) .. filename
				tabdata.selected = 1
			end

			return true
		end

	elseif fields.updir then
		PATH = string.split(PATH, DIR_DELIM)
		PATH[#PATH] = nil
		PATH = table.concat(PATH, DIR_DELIM)
		if PLATFORM ~= "Windows" then
			PATH = DIR_DELIM .. PATH
		end

		tabdata.selected = 1
		return true

	elseif fields.path then
		PATH = fields.path
		tabdata.selected = 1
		return true
	end

	return false
end

-- picker_type can be either 'file' or 'folder' for picking a file respectively a folder
function create_file_browser_dlg(setting, picker_type)
	local dlg = dialog_create("file_browser", make_fs, fields_handler)

	dlg.data.type = picker_type
	dlg.data.setting = setting

	return dlg
end
