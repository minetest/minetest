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


local component_funcs =  dofile(core.get_mainmenu_path() .. DIR_DELIM ..
		"settings" .. DIR_DELIM .. "components.lua")

local quick_shader_component =  dofile(core.get_mainmenu_path() .. DIR_DELIM ..
		"settings" .. DIR_DELIM .. "shader_component.lua")


local full_settings = settingtypes.parse_config_file(false, true)
local info_icon_path = core.formspec_escape(defaulttexturedir .. "settings_info.png")
local reset_icon_path = core.formspec_escape(defaulttexturedir .. "settings_reset.png")

local gettext = fgettext_ne
local all_pages = {}
local page_by_id = {}
local filtered_pages = all_pages
local filtered_page_by_id = page_by_id

local function add_page(page)
	assert(type(page.id) == "string")
	assert(type(page.title) == "string")
	assert(page.section == nil or type(page.section) == "string")
	assert(type(page.content) == "table")

	assert(not page_by_id[page.id], "Page " .. page.id .. " already registered")

	all_pages[#all_pages + 1] = page
	page_by_id[page.id] = page
	return page
end


local change_keys = {
	query_text = "Change keys",
	get_formspec = function(self, avail_w)
		local btn_w = math.min(avail_w, 3)
		return ("button[0,0;%f,0.8;btn_change_keys;%s]"):format(btn_w, fgettext("Change keys")), 0.8
	end,
	on_submit = function(self, fields)
		if fields.btn_change_keys then
			core.show_keys_menu()
		end
	end,
}


add_page({
	id = "most_used",
	title = gettext("Most Used"),
	content = {
		change_keys,
		"language",
		"fullscreen",
		PLATFORM ~= "Android" and "autosave_screensize" or false,
		"touchscreen_threshold",
		{ heading = gettext("Scaling") },
		"gui_scaling",
		"hud_scaling",
		{ heading = gettext("Graphics / Performance") },
		"smooth_lighting",
		"enable_particles",
		"enable_3d_clouds",
		"opaque_water",
		"connected_glass",
		"node_highlighting",
		"leaves_style",
		{ heading = gettext("Shaders") },
		quick_shader_component,
	},
})

add_page({
	id = "accessibility",
	title = gettext("Accessibility"),
	content = {
		"font_size",
		"chat_font_size",
		"gui_scaling",
		"hud_scaling",
		"show_nametag_backgrounds",
		{ heading = gettext("Chat") },
		"console_height",
		"console_alpha",
		"console_color",
		{ heading = gettext("Controls") },
		"autojump",
		"safe_dig_and_place",
		{ heading = gettext("Movement") },
		"arm_inertia",
		"view_bobbing_amount",
		"fall_bobbing_amount",
	},
})


local tabsize = {
	width = 15.5,
	height= 12,
}


local function load_settingtypes()
	local page = nil
	local section = nil
	local function ensure_page_started()
		if not page then
			page = add_page({
				id = (section or "general"):lower():gsub(" ", "_"),
				title = section or gettext("General"),
				section = section,
				content = {},
			})
		end
	end

	for _, entry in ipairs(full_settings) do
		if entry.type == "category" then
			if entry.level == 0 then
				section = entry.name
				page = nil
			elseif entry.level == 1 then
				page = {
					id = ((section and section .. "_" or "") .. entry.name):lower():gsub(" ", "_"),
					title = entry.readable_name or entry.name,
					section = section,
					content = {},
				}

				if page.title:sub(1, 5) ~= "Hide:" then
					page = add_page(page)
				end
			elseif entry.level == 2 then
				ensure_page_started()
				page.content[#page.content + 1] = {
					heading = gettext(entry.readable_name or entry.name),
				}
			end
		else
			ensure_page_started()
			page.content[#page.content + 1] = entry.name
		end
	end
end
load_settingtypes()

table.insert(page_by_id.controls_keyboard_and_mouse.content, 1, change_keys)


local function get_setting_info(name)
	for _, entry in ipairs(full_settings) do
		if entry.type ~= "category" and entry.name == name then
			return entry
		end
	end

	return nil
end


-- See if setting matches keywords
local function get_setting_match_weight(entry, query_keywords)
	local setting_score = 0
	for _, keyword in ipairs(query_keywords) do
		if string.find(entry.name:lower(), keyword, 1, true) then
			setting_score = setting_score + 1
		end

		if entry.readable_name and
				string.find(fgettext(entry.readable_name):lower(), keyword, 1, true) then
			setting_score = setting_score + 1
		end

		if entry.comment and
				string.find(fgettext_ne(entry.comment):lower(), keyword, 1, true) then
			setting_score = setting_score + 1
		end
	end

	return setting_score
end


local function filter_page_content(page, query_keywords)
	if #query_keywords == 0 then
		return page.content
	end

	local retval = {}
	local i = 1
	local max_weight = 0
	for _, content in ipairs(page.content) do
		if type(content) == "string" then
			local setting = get_setting_info(content)
			assert(setting, "Unknown setting: " .. content)

			local weight = get_setting_match_weight(setting, query_keywords)
			if weight > 0 then
				max_weight = math.max(max_weight, weight)
				retval[i] = content
				i = i + 1
			end
		elseif type(content) == "table" and content.query_text then
			for _, keyword in ipairs(query_keywords) do
				if string.find(fgettext(content.query_text), keyword, 1, true) then
					max_weight = math.max(max_weight, 1)
					retval[i] = content
					i = i + 1
					break
				end
			end
		end
	end
	return retval, max_weight
end


local function update_filtered_pages(query)
	filtered_pages = {}
	filtered_page_by_id = {}

	if query == "" or query == nil then
		filtered_pages = all_pages
		filtered_page_by_id = page_by_id
		return filtered_pages[1].id
	end

	local query_keywords = {}
	for word in query:lower():gmatch("%S+") do
		table.insert(query_keywords, word)
	end

	local best_page = nil
	local best_page_weight = -1

	for _, page in ipairs(all_pages) do
		local content, page_weight = filter_page_content(page, query_keywords)
		if #content > 0 then
			local new_page = table.copy(page)
			new_page.content = content

			filtered_pages[#filtered_pages + 1] = new_page
			filtered_page_by_id[new_page.id] = new_page

			if page_weight > best_page_weight then
				best_page = new_page
				best_page_weight = page_weight
			end
		end
	end

	return best_page and best_page.id or nil
end


local function build_page_components(page)
	local retval = {}
	local j = 1
	for i, content in ipairs(page.content) do
		if content == false then
			-- false is used to disable components conditionally (ie: Android specific)
			j = j - 1
		elseif type(content) == "string" then
			local setting = get_setting_info(content)
			assert(setting, "Unknown setting: " .. content)

			local component_func = component_funcs[setting.type]
			assert(component_func, "Unknown setting type: " .. setting.type)
			retval[j] = component_func(setting)
		elseif content.get_formspec then
			retval[j] = content
		elseif content.heading then
			retval[j] = component_funcs.heading(content.heading)
		else
			error("Unknown content in page: " .. dump(content))
		end
		j = j + 1
	end
	return retval
end


--- Creates a scrollbaroptions for a scroll_container
--
-- @param visible_l the length of the scroll_container and scrollbar
-- @param total_l length of the scrollable area
-- @param scroll_factor as passed to scroll_container
local function make_scrollbaroptions_for_scroll_container(visible_l, total_l, scroll_factor)
	assert(total_l >= visible_l)
	local max = total_l - visible_l
	local thumb_size = (visible_l / total_l) * max
	return ("scrollbaroptions[min=0;max=%f;thumbsize=%f]"):format(max / scroll_factor, thumb_size / scroll_factor)
end


local formspec_show_hack = false


local function get_formspec(dialogdata)
	local page_id = dialogdata.page_id or "most_used"
	local page = filtered_page_by_id[page_id]

	local scrollbar_w = 0.4
	if PLATFORM == "Android" then
		scrollbar_w = 0.6
	end

	local left_pane_width = 4.25
	local search_width = left_pane_width + scrollbar_w - (0.75 * 2)

	local show_technical_names = core.settings:get_bool("show_technical_names")

	formspec_show_hack = not formspec_show_hack

	local fs = {
		"formspec_version[6]",
		"size[", tostring(tabsize.width), ",", tostring(tabsize.height + 1), "]",
		"bgcolor[#0000]",

		-- HACK: this is needed to allow resubmitting the same formspec
		formspec_show_hack and " " or "",

		"box[0,0;", tostring(tabsize.width), ",", tostring(tabsize.height), ";#0000008C]",

		"button[0,", tostring(tabsize.height + 0.2), ";3,0.8;back;", fgettext("Back"), "]",

		("box[%f,%f;5,0.8;#0000008C]"):format(tabsize.width - 5, tabsize.height + 0.2),
		"checkbox[", tostring(tabsize.width - 4.75), ",", tostring(tabsize.height + 0.6), ";show_technical_names;",
			fgettext("Show technical names"), ";", tostring(show_technical_names), "]",

		"field[0.25,0.25;", tostring(search_width), ",0.75;search_query;;",
			core.formspec_escape(dialogdata.query or ""), "]",
		"container[", tostring(search_width + 0.25), ", 0.25]",
			"image_button[0,0;0.75,0.75;", core.formspec_escape(defaulttexturedir .. "search.png"), ";search;]",
			"image_button[0.75,0;0.75,0.75;", core.formspec_escape(defaulttexturedir .. "clear.png"), ";search_clear;]",
			"tooltip[search;", fgettext("Search"), "]",
			"tooltip[search_clear;", fgettext("Clear"), "]",
		"container_end[]",
		"scroll_container[0.25,1.25;", tostring(left_pane_width), ",",
				tostring(tabsize.height - 1.5), ";leftscroll;vertical;0.1]",
		"style_type[button;border=false;bgcolor=#3333]",
		"style_type[button:hover;border=false;bgcolor=#6663]",
	}

	local y = 0
	local last_section = nil
	for _, other_page in ipairs(filtered_pages) do
		if other_page.section ~= last_section then
			fs[#fs + 1] = ("label[0.1,%f;%s]"):format(y + 0.41, core.colorize("#ff0", fgettext(other_page.section)))
			last_section = other_page.section
			y = y + 0.82
		end
		fs[#fs + 1] = ("box[0,%f;%f,0.8;%s]"):format(
			y, left_pane_width, other_page.id == page_id and "#467832FF" or "#3339")
		fs[#fs + 1] = ("button[0,%f;%f,0.8;page_%s;%s]")
			:format(y, left_pane_width, other_page.id, fgettext(other_page.title))
		y = y + 0.82
	end

	if #filtered_pages == 0 then
		fs[#fs + 1] = "label[0.1,0.41;"
		fs[#fs + 1] = fgettext("No results")
		fs[#fs + 1] = "]"
	end

	fs[#fs + 1] = "scroll_container_end[]"

	if y >= tabsize.height - 1.25 then
		fs[#fs + 1] = make_scrollbaroptions_for_scroll_container(tabsize.height - 1.5, y, 0.1)
		fs[#fs + 1] = ("scrollbar[%f,1.25;%f,%f;vertical;leftscroll;%f]"):format(
				left_pane_width + 0.25, scrollbar_w, tabsize.height - 1.5, dialogdata.leftscroll or 0)
	end

	fs[#fs + 1] = "style_type[button;border=;bgcolor=]"

	if not dialogdata.components then
		dialogdata.components = page and build_page_components(page) or {}
	end

	local right_pane_width = tabsize.width - left_pane_width - 0.375 - 2*scrollbar_w - 0.25
	fs[#fs + 1] = ("scroll_container[%f,0;%f,%f;rightscroll;vertical;0.1]"):format(
			tabsize.width - right_pane_width - scrollbar_w, right_pane_width, tabsize.height)

	y = 0.25
	for i, comp in ipairs(dialogdata.components) do
		fs[#fs + 1] = ("container[0,%f]"):format(y)

		local avail_w = right_pane_width - 0.25
		if not comp.full_width then
			avail_w = avail_w - 1.4
		end
		if comp.max_w then
			avail_w = math.min(avail_w, comp.max_w)
		end

		local comp_fs, used_h = comp:get_formspec(avail_w)
		fs[#fs + 1] = comp_fs

		fs[#fs + 1] = "style_type[image_button;border=false;padding=]"

		local show_reset = comp.changed and comp.setting and comp.setting.default
		local show_info = comp.info_text and comp.info_text ~= ""
		if show_reset or show_info then
			-- ensure there's enough space for reset/info
			used_h = math.max(used_h, 0.5)
		end
		local info_reset_y = used_h / 2 - 0.25

		if show_reset then
			fs[#fs + 1] = ("image_button[%f,%f;0.5,0.5;%s;%s;]"):format(
					right_pane_width - 1.4, info_reset_y, reset_icon_path, "reset_" .. i)
			fs[#fs + 1] = ("tooltip[%s;%s]"):format(
					"reset_" .. i, fgettext("Reset setting to default ($1)", tostring(comp.setting.default)))
		end

		if show_info then
			local info_x = right_pane_width - 0.75
			fs[#fs + 1] = ("image[%f,%f;0.5,0.5;%s]"):format(info_x, info_reset_y, info_icon_path)
			fs[#fs + 1] = ("tooltip[%f,%f;0.5,0.5;%s]"):format(info_x, info_reset_y, fgettext(comp.info_text))
		end

		fs[#fs + 1] = "style_type[image_button;border=;padding=]"

		fs[#fs + 1] = "container_end[]"

		if used_h > 0 then
			y = y + used_h + 0.25
		end
	end

	fs[#fs + 1] = "scroll_container_end[]"

	if y >= tabsize.height then
		fs[#fs + 1] = make_scrollbaroptions_for_scroll_container(tabsize.height, y + 0.375, 0.1)
		fs[#fs + 1] = ("scrollbar[%f,0;%f,%f;vertical;rightscroll;%f]"):format(
				tabsize.width - scrollbar_w, scrollbar_w, tabsize.height, dialogdata.rightscroll or 0)
	end

	return table.concat(fs, "")
end


local function buttonhandler(this, fields)
	local dialogdata = this.data
	dialogdata.leftscroll = core.explode_scrollbar_event(fields.leftscroll).value or dialogdata.leftscroll
	dialogdata.rightscroll = core.explode_scrollbar_event(fields.rightscroll).value or dialogdata.rightscroll
	dialogdata.query = fields.search_query

	if fields.back then
		this:delete()
		return true
	end

	if fields.show_technical_names ~= nil then
		local value = core.is_yes(fields.show_technical_names)
		core.settings:set_bool("show_technical_names", value)
		return true
	end

	if fields.search or fields.key_enter_field == "search_query" then
		dialogdata.components = nil
		dialogdata.leftscroll = 0
		dialogdata.rightscroll = 0

		dialogdata.page_id = update_filtered_pages(dialogdata.query)

		return true
	end
	if fields.search_clear then
		dialogdata.query = ""
		dialogdata.components = nil
		dialogdata.leftscroll = 0
		dialogdata.rightscroll = 0

		dialogdata.page_id = update_filtered_pages("")
		return true
	end

	for _, page in ipairs(all_pages) do
		if fields["page_" .. page.id] then
			dialogdata.page_id = page.id
			dialogdata.components = nil
			dialogdata.rightscroll = 0
			return true
		end
	end

	for i, comp in ipairs(dialogdata.components) do
		if comp.on_submit and comp:on_submit(fields, this) then
			return true
		end
		if comp.setting and fields["reset_" .. i] then
			core.settings:set(comp.setting.name, comp.setting.default)
			return true
		end
	end

	return false
end


function create_settings_dlg()
	return dialog_create("dlg_settings", get_formspec, buttonhandler, nil)
end
