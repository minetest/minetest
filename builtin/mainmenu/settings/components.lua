--Minetest
--Copyright (C) 2022 rubenwardy
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

local make = {}


-- This file defines various component constructors, of the form:
--
--     make.component(setting)
--
-- `setting` is a table representing the settingtype.
--
-- A component is a table with the following:
--
-- * `full_width`: (Optional) true if the component shouldn't reserve space for info / reset.
-- * `info_text`: (Optional) string, informational text shown in an info icon.
-- * `setting`: (Optional) the setting.
-- * `max_w`: (Optional) maximum width, `avail_w` will never exceed this.
-- * `resettable`: (Optional) if this is true, a reset button is shown.
-- * `get_formspec = function(self, avail_w)`:
--     * `avail_w` is the available width for the component.
--     * Returns `fs, used_height`.
--     * `fs` is a string for the formspec.
--       Components should be relative to `0,0`, and not exceed `avail_w` or the returned `used_height`.
--     * `used_height` is the space used by components in `fs`.
-- * `on_submit = function(self, fields, parent)`:
--     * `fields`: submitted formspec fields
--     * `parent`: the fstk element for the settings UI, use to show dialogs
--     * Return true if the event was handled, to prevent future components receiving it.


local function get_label(setting)
	local show_technical_names = core.settings:get_bool("show_technical_names")
	if not show_technical_names and setting.readable_name then
		return fgettext(setting.readable_name)
	end
	return setting.name
end


local function is_valid_number(value)
	return type(value) == "number" and not (value ~= value or value >= math.huge or value <= -math.huge)
end


function make.heading(text)
	return {
		full_width = true,
		get_formspec = function(self, avail_w)
			return ("label[0,0.6;%s]box[0,0.9;%f,0.05;#ccc6]"):format(core.formspec_escape(text), avail_w), 1.2
		end,
	}
end


--- Used for string and numeric style fields
---
--- @param converter Function to coerce values from strings.
--- @param validator Validator function, optional. Returns true when valid.
--- @param stringifier Function to convert values to strings, optional.
local function make_field(converter, validator, stringifier)
	return function(setting)
		return {
			info_text = setting.comment,
			setting = setting,

			get_formspec = function(self, avail_w)
				local value = core.settings:get(setting.name) or setting.default
				self.resettable = core.settings:has(setting.name)

				local fs = ("field[0,0.3;%f,0.8;%s;%s;%s]"):format(
					avail_w - 1.5, setting.name, get_label(setting), core.formspec_escape(value))
				fs = fs .. ("field_enter_after_edit[%s;true]"):format(setting.name)
				fs = fs .. ("button[%f,0.3;1.5,0.8;%s;%s]"):format(avail_w - 1.5, "set_" .. setting.name, fgettext("Set"))

				return fs, 1.1
			end,

			on_submit = function(self, fields)
				if fields["set_" .. setting.name] or fields.key_enter_field == setting.name then
					local value = converter(fields[setting.name])
					if value == nil or (validator and not validator(value)) then
						return true
					end

					if setting.min then
						value = math.max(value, setting.min)
					end
					if setting.max then
						value = math.min(value, setting.max)
					end
					core.settings:set(setting.name, (stringifier or tostring)(value))
					return true
				end
			end,
		}
	end
end


make.float = make_field(tonumber, is_valid_number, function(x)
	local str = tostring(x)
	if str:match("^[+-]?%d+$") then
		str = str .. ".0"
	end
	return str
end)
make.int = make_field(function(x)
	local value = tonumber(x)
	return value and math.floor(value)
end, is_valid_number)
make.string = make_field(tostring, nil)


function make.bool(setting)
	return {
		info_text = setting.comment,
		setting = setting,

		get_formspec = function(self, avail_w)
			local value = core.settings:get_bool(setting.name, core.is_yes(setting.default))
			self.resettable = core.settings:has(setting.name)

			local fs = ("checkbox[0,0.25;%s;%s;%s]"):format(
				setting.name, get_label(setting), tostring(value))
			return fs, 0.5
		end,

		on_submit = function(self, fields)
			if fields[setting.name] == nil then
				return false
			end

			core.settings:set_bool(setting.name, core.is_yes(fields[setting.name]))
			return true
		end,
	}
end


function make.enum(setting)
	return {
		info_text = setting.comment,
		setting = setting,
		max_w = 4.5,

		get_formspec = function(self, avail_w)
			local value = core.settings:get(setting.name) or setting.default
			self.resettable = core.settings:has(setting.name)

			local labels = setting.option_labels or {}

			local items = {}
			for i, option in ipairs(setting.values) do
				items[i] = core.formspec_escape(labels[option] or option)
			end

			local selected_idx = table.indexof(setting.values, value)
			local fs = "label[0,0.1;" .. get_label(setting) .. "]"

			fs = fs .. ("dropdown[0,0.3;%f,0.8;%s;%s;%d;true]"):format(
				avail_w, setting.name, table.concat(items, ","), selected_idx, value)

			return fs, 1.1
		end,

		on_submit = function(self, fields)
			local old_value = core.settings:get(setting.name) or setting.default
			local idx = tonumber(fields[setting.name]) or 0
			local value = setting.values[idx]
			if value == nil or value == old_value then
				return false
			end

			core.settings:set(setting.name, value)
			return true
		end,
	}
end


local function make_path(setting)
	return {
		info_text = setting.comment,
		setting = setting,

		get_formspec = function(self, avail_w)
			local value = core.settings:get(setting.name) or setting.default
			self.resettable = core.settings:has(setting.name)

			local fs = ("field[0,0.3;%f,0.8;%s;%s;%s]"):format(
				avail_w - 3, setting.name, get_label(setting), core.formspec_escape(value))
			fs = fs .. ("button[%f,0.3;1.5,0.8;%s;%s]"):format(avail_w - 3, "pick_" .. setting.name, fgettext("Browse"))
			fs = fs .. ("button[%f,0.3;1.5,0.8;%s;%s]"):format(avail_w - 1.5, "set_" .. setting.name, fgettext("Set"))

			return fs, 1.1
		end,

		on_submit = function(self, fields)
			local dialog_name = "dlg_path_" .. setting.name
			if fields["pick_" .. setting.name] then
				local is_file = setting.type ~= "path"
				core.show_path_select_dialog(dialog_name,
					is_file and fgettext_ne("Select file") or fgettext_ne("Select directory"), is_file)
				return true
			end
			if fields[dialog_name .. "_accepted"] then
				local value = fields[dialog_name .. "_accepted"]
				if value ~= nil then
					core.settings:set(setting.name, value)
				end
				return true
			end
			if fields["set_" .. setting.name] or fields.key_enter_field == setting.name then
				local value = fields[setting.name]
				if value ~= nil then
					core.settings:set(setting.name, value)
				end
				return true
			end
		end,
	}
end

if PLATFORM == "Android" then
	-- The Irrlicht file picker doesn't work on Android.
	make.path = make.string
	make.filepath = make.string
else
	make.path = make_path
	make.filepath = make_path
end


function make.v3f(setting)
	return {
		info_text = setting.comment,
		setting = setting,

		get_formspec = function(self, avail_w)
			local value = vector.from_string(core.settings:get(setting.name) or setting.default)
			self.resettable = core.settings:has(setting.name)

			-- Allocate space for "Set" button
			avail_w = avail_w - 1

			local fs = "label[0,0.1;" .. get_label(setting) .. "]"

			local field_width = (avail_w - 3*0.25) / 3

			fs = fs .. ("field[%f,0.6;%f,0.8;%s;%s;%s]"):format(
				0, field_width, setting.name .. "_x", "X", value.x)
			fs = fs .. ("field[%f,0.6;%f,0.8;%s;%s;%s]"):format(
				field_width + 0.25, field_width, setting.name .. "_y", "Y", value.y)
			fs = fs .. ("field[%f,0.6;%f,0.8;%s;%s;%s]"):format(
				2 * (field_width + 0.25), field_width, setting.name .. "_z", "Z", value.z)

			fs = fs .. ("button[%f,0.6;1,0.8;%s;%s]"):format(avail_w, "set_" .. setting.name, fgettext("Set"))

			return fs, 1.4
		end,

		on_submit = function(self, fields)
			if fields["set_" .. setting.name]  or
					fields.key_enter_field == setting.name .. "_x" or
					fields.key_enter_field == setting.name .. "_y" or
					fields.key_enter_field == setting.name .. "_z" then
				local x = tonumber(fields[setting.name .. "_x"])
				local y = tonumber(fields[setting.name .. "_y"])
				local z = tonumber(fields[setting.name .. "_z"])
				if is_valid_number(x) and is_valid_number(y) and is_valid_number(z) then
					core.settings:set(setting.name, vector.new(x, y, z):to_string())
				else
					core.log("error", "Invalid vector: " .. dump({x, y, z}))
				end
				return true
			end
		end,
	}
end


function make.flags(setting)
	local checkboxes = {}

	return {
		info_text = setting.comment,
		setting = setting,

		get_formspec = function(self, avail_w)
			local fs = {
				"label[0,0.1;" .. get_label(setting) .. "]",
			}

			self.resettable = core.settings:has(setting.name)

			checkboxes = {}
			for _, name in ipairs(setting.possible) do
				checkboxes[name] = false
			end
			local function apply_flags(flag_string, what)
				local prefixed_flags = {}
				for _, name in ipairs(flag_string:split(",")) do
					prefixed_flags[name:trim()] = true
				end
				for _, name in ipairs(setting.possible) do
					local enabled = prefixed_flags[name]
					local disabled = prefixed_flags["no" .. name]
					if enabled and disabled then
						core.log("warning", "Flag " .. name .. " in " .. what .. " " ..
								setting.name .. " both enabled and disabled, ignoring")
					elseif enabled then
						checkboxes[name] = true
					elseif disabled then
						checkboxes[name] = false
					end
				end
			end
			-- First apply the default, which is necessary since flags
			-- which are not overridden may be missing from the value.
			apply_flags(setting.default, "default for setting")
			local value = core.settings:get(setting.name)
			if value then
				apply_flags(value, "setting")
			end

			local columns = math.max(math.floor(avail_w / 2.5), 1)
			local column_width = avail_w / columns
			local x = 0
			local y = 0.55

			for _, possible in ipairs(setting.possible) do
				if x >= avail_w then
					x = 0
					y = y + 0.5
				end

				local is_checked = checkboxes[possible]
				fs[#fs + 1] = ("checkbox[%f,%f;%s;%s;%s]"):format(
					x, y, setting.name .. "_" .. possible,
					core.formspec_escape(possible), tostring(is_checked))
				x = x + column_width
			end

			return table.concat(fs, ""), y + 0.25
		end,

		on_submit = function(self, fields)
			local changed = false
			for name, _ in pairs(checkboxes) do
				local value = fields[setting.name .. "_" .. name]
				if value ~= nil then
					checkboxes[name] = core.is_yes(value)
					changed = true
				end
			end

			if changed then
				local values = {}
				for _, name in ipairs(setting.possible) do
					if checkboxes[name] then
						table.insert(values, name)
					else
						table.insert(values, "no" .. name)
					end
				end

				core.settings:set(setting.name, table.concat(values, ","))
			end
			return changed
		end
	}
end


local function make_noise_params(setting)
	return {
		info_text = setting.comment,
		setting = setting,

		get_formspec = function(self, avail_w)
			-- The "defaults" noise parameter flag doesn't reset a noise
			-- setting to its default value, so we offer a regular reset button.
			self.resettable = core.settings:has(setting.name)

			local fs = "label[0,0.4;" .. get_label(setting) .. "]" ..
					("button[%f,0;2.5,0.8;%s;%s]"):format(avail_w - 2.5, "edit_" .. setting.name, fgettext("Edit"))
			return fs, 0.8
		end,

		on_submit = function(self, fields, tabview)
			if fields["edit_" .. setting.name] then
				local dlg = create_change_mapgen_flags_dlg(setting)
				dlg:set_parent(tabview)
				tabview:hide()
				dlg:show()

				return true
			end
		end,
	}
end

make.noise_params_2d = make_noise_params
make.noise_params_3d = make_noise_params

-- These must not be translated, as they need to show in the local
-- language no matter the user's current language.
-- This list must be kept in sync with src/unsupported_language_list.txt.
core.language_names = {
	--ar: blacklisted
	be = "Беларуская",
	bg = "Български",
	ca = "Català",
	cs = "Česky",
	cy = "Cymraeg",
	da = "Dansk",
	de = "Deutsch",
	el = "Ελληνικά",
	en = "English",
	eo = "Esperanto",
	es = "Español",
	et = "Eesti",
	eu = "Euskara",
	fi = "Suomi",
	fil = "Wikang Filipino",
	fr = "Français",
	gd = "Gàidhlig",
	gl = "Galego",
	--he: blacklisted
	--hi: blacklisted
	hu = "Magyar",
	id = "Bahasa Indonesia",
	it = "Italiano",
	ja = "日本語",
	jbo = "Lojban",
	kk = "Қазақша",
	--kn: blacklisted
	ko = "한국어",
	ky = "Kırgızca / Кыргызча",
	lt = "Lietuvių",
	lv = "Latviešu",
	mn = "Монгол",
	mr = "मराठी",
	ms = "Bahasa Melayu",
	--ms_Arab: blacklisted
	nb = "Norsk Bokmål",
	nl = "Nederlands",
	nn = "Norsk Nynorsk",
	oc = "Occitan",
	pl = "Polski",
	pt = "Português",
	pt_BR = "Português do Brasil",
	ro = "Română",
	ru = "Русский",
	sk = "Slovenčina",
	sl = "Slovenščina",
	sr_Cyrl = "Српски",
	sr_Latn = "Srpski (Latinica)",
	sv = "Svenska",
	sw = "Kiswahili",
	--th: blacklisted
	tr = "Türkçe",
	tt = "Tatarça",
	uk = "Українська",
	vi = "Tiếng Việt",
	yue = "粵語",
	zh_CN = "中文 (简体)",
	zh_TW = "正體中文 (繁體)",
}

local language_options = {}
for k in pairs(core.language_names) do
	table.insert(language_options, k)
end
table.sort(language_options)

local function get_language_description(lang)
	if not lang or lang == "" then
		return
	end
	local code = lang
	local count
	while not core.language_names[lang] do
		lang, count = string.gsub(lang, "(.+)[_@].-$", "%1")
		if count == 0 then
			break
		end
	end
	if core.language_names[lang] then
		return ("%s [%s]"):format(core.language_names[lang], code)
	end
end

local language_dropdown = {}
for idx, langcode in ipairs(language_options) do
	language_dropdown[idx] = core.formspec_escape(get_language_description(langcode))
	language_options[langcode] = idx
end
language_dropdown = table.concat(language_dropdown, ",")

function make.language_list(setting)
	local selection_list = {}
	return {
		info_text = setting.comment,
		setting = setting,
		get_formspec = function(self, avail_w)
			local fs = {}
			local height = 0.8
			selection_list = string.split(core.settings:get(setting.name) or setting.default, ":")
			self.resettable = core.settings:has(setting.name)

			table.insert(fs, ("label[0,0.1;%s]"):format(get_label(setting)))
			-- TODO: Change the label to "Use system language: $1" when #14379 is merged
			table.insert(fs, ("checkbox[0,0.55;%s;%s;%s]"):format(
					setting.name, fgettext("Use system language"), tostring(#selection_list == 0)))

			-- Note that the "Remove" button is added implicitly and only when the move buttons are present.
			-- If the selection list includes only one language, removing it implies using the system language.
			local move_columns = 2 -- Move up|Move down
			if #selection_list == 1 then
				move_columns = 0
			elseif #selection_list == 2 then
				move_columns = 1 -- Move (i.e. swap order with the other entry)
			end
			local dropdown_width = avail_w
			if move_columns > 0 then
				dropdown_width = avail_w - 0.9*(move_columns+1)
			end

			for idx, lang in ipairs(selection_list) do
				local selid = language_options[lang]
				local dropdown_entries = language_dropdown
				if not selid then
					local label = fgettext("Other language: $1", lang)
					local altdesc = get_language_description(lang)
					if altdesc then
						label = fgettext("Other variant: $1", altdesc)
					end
					dropdown_entries = dropdown_entries .. "," .. label
					selid = #language_options+1
				end
				table.insert(fs, ("dropdown[0,%f;%f,0.8;sel_%d_%s;%s;%d;true]"):format(
						height, dropdown_width, idx, setting.name, dropdown_entries, selid))
				if move_columns > 0 then
					table.insert(fs, ("image_button[%f,%f;0.8,0.8;%s;rem_%d_%s;]"):format(
							avail_w-0.8, height,
							core.formspec_escape(defaulttexturedir .. "clear.png"), idx, setting.name))
					local move_button_x = avail_w-0.8-0.9*move_columns
					local move_button_width = 0.8
					if move_columns == 2 and (idx == 1 or idx == #selection_list) then
						move_button_width = 1.7
					end
					if idx < #selection_list then
						table.insert(fs, ("image_button[%f,%f;%f,0.8;%s;mdn_%d_%s;]"):format(
								move_button_x, height, move_button_width,
								core.formspec_escape(defaulttexturedir .. "down_icon.png"), idx, setting.name))
						move_button_x = move_button_x + 0.9
					end
					if idx > 1 then
						table.insert(fs, ("image_button[%f,%f;%f,0.8;%s;mup_%d_%s;]"):format(
								move_button_x, height, move_button_width,
								core.formspec_escape(defaulttexturedir .. "up_icon.png"), idx, setting.name))
					end
				end
				height = height+0.9
			end
			if #selection_list > 0 then
				local dropdown_entries = language_dropdown .. "," ..
						fgettext("Select language to add to list")
				local selid = #language_options+1
				table.insert(fs, ("dropdown[0,%f;%f,0.8;sel_%d_%s;%s;%d;true]"):format(
						height, dropdown_width, #selection_list+1, setting.name, dropdown_entries, selid))
				height = height+0.9
			end
			if setting.name == "language" then
				local langlist = {}
				for lang in string.gmatch(core.get_language(), "[^:]+") do
					table.insert(langlist, get_language_description(lang) or lang)
				end
				-- FIXME: How to generate "translated" lists?
				table.insert(fs, ("label[%f,%f;%s]"):format(0.1, height+0.3,
						fgettext("Effective language setting: $1", table.concat(langlist, ", "))))
				height = height+0.5
			end

			return table.concat(fs), height
		end,
		on_submit = function(self, fields)
			local old_value = core.settings:get(setting.name) or setting.default
			local value
			local function update_value_from_selection()
				value = table.concat(selection_list, ":")
			end
			if fields[setting.name] then
				if core.is_yes(fields[setting.name]) then
					value = ""
				else
					value = core.get_language_configuration()
					if value == "" then
						value = "en"
					end
				end
			end
			for k = 1, #selection_list do
				local basekey = ("_%d_%s"):format(k, setting.name)
				if fields["rem" .. basekey] then
					table.remove(selection_list, k)
					update_value_from_selection()
					break
				elseif fields["mdn" .. basekey] and k < #selection_list then
					selection_list[k], selection_list[k+1] = selection_list[k+1], selection_list[k]
					update_value_from_selection()
					break
				elseif fields["mup" .. basekey] and k > 1 then
					selection_list[k], selection_list[k-1] = selection_list[k-1], selection_list[k]
					update_value_from_selection()
					break
				end
			end
			if value == nil then
				for k = 1, #selection_list+1 do
					local selection = language_options[tonumber(fields[("sel_%d_%s"):format(k, setting.name)])]
					if selection then
						selection_list[k] = selection
					end
				end
				update_value_from_selection()
			end
			if value == nil or value == old_value then
				return false
			end
			core.settings:set(setting.name, value)
			return true
		end,
	}
end

return make
