--Minetest
--Copyright (C) 2018-20 rubenwardy
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

if not core.get_http_api then
	function create_contentdb_dlg()
		return messagebox("contentdb",
				fgettext("ContentDB is not available when Minetest was compiled without cURL"))
	end
	return
end

-- Filter
local search_string = ""
local cur_page = 1
local num_per_page = 5
local filter_type = 1
local filter_types_titles = {
	fgettext("All packages"),
	fgettext("Games"),
	fgettext("Mods"),
	fgettext("Texture packs"),
}

-- Automatic package installation
local auto_install_spec = nil

local filter_types_type = {
	nil,
	"game",
	"mod",
	"txp",
}


local function install_or_update_package(this, package)
	local install_parent
	if package.type == "mod" then
		install_parent = core.get_modpath()
	elseif package.type == "game" then
		install_parent = core.get_gamepath()
	elseif package.type == "txp" then
		install_parent = core.get_texturepath()
	else
		error("Unknown package type: " .. package.type)
	end

	if package.queued or package.downloading then
		return
	end

	local function on_confirm()
		local has_hard_deps = contentdb.has_hard_deps(package)
		if has_hard_deps then
			local dlg = create_install_dialog(package)
			dlg:set_parent(this)
			this:hide()
			dlg:show()
		elseif has_hard_deps == nil then
			local dlg = messagebox("error_checking_deps",
					fgettext("Error getting dependencies for package"))
			dlg:set_parent(this)
			this:hide()
			dlg:show()
		else
			contentdb.queue_download(package, package.path and contentdb.REASON_UPDATE or contentdb.REASON_NEW)
		end
	end

	if package.type == "mod" and #pkgmgr.games == 0 then
		local dlg = messagebox("install_game",
			fgettext("You need to install a game before you can install a mod"))
		dlg:set_parent(this)
		this:hide()
		dlg:show()
	elseif not package.path and core.is_dir(install_parent .. DIR_DELIM .. package.name) then
		local dlg = create_confirm_overwrite(package, on_confirm)
		dlg:set_parent(this)
		this:hide()
		dlg:show()
	else
		on_confirm()
	end
end


-- Resolves the package specification stored in auto_install_spec into an actual package.
-- May only be called after the package list has been loaded successfully.
local function resolve_auto_install_spec()
	assert(contentdb.load_ok)

	if not auto_install_spec then
		return nil
	end

	local spec = contentdb.aliases[auto_install_spec] or auto_install_spec
	local resolved = nil

	for _, pkg in ipairs(contentdb.packages_full_unordered) do
		if pkg.id == spec then
			resolved = pkg
			break
		end
	end

	if not resolved then
		gamedata.errormessage = fgettext_ne("The package $1 was not found.", auto_install_spec)
		ui.update()

		auto_install_spec = nil
	end

	return resolved
end


-- Installs the package specified by auto_install_spec.
-- Only does something if:
-- a. The package list has been loaded successfully.
-- b. The ContentDB dialog is currently visible.
local function do_auto_install()
	if not contentdb.load_ok then
		return
	end

	local pkg = resolve_auto_install_spec()
	if not pkg then
		return
	end

	local contentdb_dlg = ui.find_by_name("contentdb")
	if not contentdb_dlg or contentdb_dlg.hidden then
		return
	end

	install_or_update_package(contentdb_dlg, pkg)
	auto_install_spec = nil
end


local function sort_and_filter_pkgs()
	contentdb.update_paths()
	contentdb.sort_packages()
	contentdb.filter_packages(search_string, filter_types_type[filter_type])

	local auto_install_pkg = resolve_auto_install_spec()
	if auto_install_pkg then
		local idx = table.indexof(contentdb.packages, auto_install_pkg)
		if idx ~= -1 then
			table.remove(contentdb.packages, idx)
			table.insert(contentdb.packages, 1, auto_install_pkg)
		end
	end
end


local function load()
	if contentdb.load_ok then
		sort_and_filter_pkgs()
		return
	end
	if contentdb.loading then
		return
	end
	contentdb.fetch_pkgs(function(result)
		if result then
			sort_and_filter_pkgs()
			do_auto_install()
		end
		ui.update()
	end)
end


local function get_info_formspec(text)
	local H = 9.5
	return table.concat({
		"formspec_version[6]",
		"size[15.75,9.5]",
		core.settings:get_bool("enable_touch") and "padding[0.01,0.01]" or "position[0.5,0.55]",

		"label[4,4.35;", text, "]",
		"container[0,", H - 0.8 - 0.375, "]",
		"button[0.375,0;5,0.8;back;", fgettext("Back to Main Menu"), "]",
		"container_end[]",
	})
end


local function get_formspec(dlgdata)
	if contentdb.loading then
		return get_info_formspec(fgettext("Loading..."))
	end
	if contentdb.load_error then
		return get_info_formspec(fgettext("No packages could be retrieved"))
	end
	assert(contentdb.load_ok)

	contentdb.update_paths()

	dlgdata.pagemax = math.max(math.ceil(#contentdb.packages / num_per_page), 1)
	if cur_page > dlgdata.pagemax then
		cur_page = 1
	end

	local W = 15.75
	local H = 9.5
	local formspec = {
		"formspec_version[6]",
		"size[15.75,9.5]",
		core.settings:get_bool("enable_touch") and "padding[0.01,0.01]" or "position[0.5,0.55]",

		"style[status,downloading,queued;border=false]",

		"container[0.375,0.375]",
		"field[0,0;7.225,0.8;search_string;;", core.formspec_escape(search_string), "]",
		"field_enter_after_edit[search_string;true]",
		"image_button[7.3,0;0.8,0.8;", core.formspec_escape(defaulttexturedir .. "search.png"), ";search;]",
		"image_button[8.125,0;0.8,0.8;", core.formspec_escape(defaulttexturedir .. "clear.png"), ";clear;]",
		"dropdown[9.175,0;2.7875,0.8;type;", table.concat(filter_types_titles, ","), ";", filter_type, "]",
		"container_end[]",

		-- Page nav buttons
		"container[0,", H - 0.8 - 0.375, "]",
		"button[0.375,0;5,0.8;back;", fgettext("Back to Main Menu"), "]",

		"container[", W - 0.375 - 0.8*4 - 2,  ",0]",
		"image_button[0,0;0.8,0.8;", core.formspec_escape(defaulttexturedir), "start_icon.png;pstart;]",
		"image_button[0.8,0;0.8,0.8;", core.formspec_escape(defaulttexturedir), "prev_icon.png;pback;]",
		"style[pagenum;border=false]",
		"button[1.6,0;2,0.8;pagenum;", tonumber(cur_page), " / ", tonumber(dlgdata.pagemax), "]",
		"image_button[3.6,0;0.8,0.8;", core.formspec_escape(defaulttexturedir), "next_icon.png;pnext;]",
		"image_button[4.4,0;0.8,0.8;", core.formspec_escape(defaulttexturedir), "end_icon.png;pend;]",
		"container_end[]",

		"container_end[]",
	}

	if contentdb.number_downloading > 0 then
		formspec[#formspec + 1] = "button[12.5875,0.375;2.7875,0.8;downloading;"
		if #contentdb.download_queue > 0 then
			formspec[#formspec + 1] = fgettext("$1 downloading,\n$2 queued",
					contentdb.number_downloading, #contentdb.download_queue)
		else
			formspec[#formspec + 1] = fgettext("$1 downloading...", contentdb.number_downloading)
		end
		formspec[#formspec + 1] = "]"
	else
		local num_avail_updates = 0
		for i=1, #contentdb.packages_full do
			local package = contentdb.packages_full[i]
			if package.path and package.installed_release < package.release and
					not (package.downloading or package.queued) then
				num_avail_updates = num_avail_updates + 1
			end
		end

		if num_avail_updates == 0 then
			formspec[#formspec + 1] = "button[12.5875,0.375;2.7875,0.8;status;"
			formspec[#formspec + 1] = fgettext("No updates")
			formspec[#formspec + 1] = "]"
		else
			formspec[#formspec + 1] = "button[12.5875,0.375;2.7875,0.8;update_all;"
			formspec[#formspec + 1] = fgettext("Update All [$1]", num_avail_updates)
			formspec[#formspec + 1] = "]"
		end
	end

	if #contentdb.packages == 0 then
		formspec[#formspec + 1] = "label[4,4.75;"
		formspec[#formspec + 1] = fgettext("No results")
		formspec[#formspec + 1] = "]"
	end

	-- download/queued tooltips always have the same message
	local tooltip_colors = ";#dff6f5;#302c2e]"
	formspec[#formspec + 1] = "tooltip[downloading;" .. fgettext("Downloading...") .. tooltip_colors
	formspec[#formspec + 1] = "tooltip[queued;" .. fgettext("Queued") .. tooltip_colors

	local start_idx = (cur_page - 1) * num_per_page + 1
	for i=start_idx, math.min(#contentdb.packages, start_idx+num_per_page-1) do
		local package = contentdb.packages[i]
		local container_y = (i - start_idx) * 1.375 + (2*0.375 + 0.8)
		formspec[#formspec + 1] = "container[0.375,"
		formspec[#formspec + 1] = container_y
		formspec[#formspec + 1] = "]"

		-- image
		formspec[#formspec + 1] = "image[0,0;1.5,1;"
		formspec[#formspec + 1] = core.formspec_escape(get_screenshot(package))
		formspec[#formspec + 1] = "]"

		-- title
		formspec[#formspec + 1] = "label[1.875,0.1;"
		formspec[#formspec + 1] = core.formspec_escape(
				core.colorize(mt_color_green, package.title) ..
				core.colorize("#BFBFBF", " by " .. package.author))
		formspec[#formspec + 1] = "]"

		-- buttons
		local description_width = W - 2.625 - 2 * 0.7 - 2 * 0.15

		local second_base = "image_button[-1.55,0;0.7,0.7;" .. core.formspec_escape(defaulttexturedir)
		local third_base = "image_button[-2.4,0;0.7,0.7;" .. core.formspec_escape(defaulttexturedir)
		formspec[#formspec + 1] = "container["
		formspec[#formspec + 1] = W - 0.375*2
		formspec[#formspec + 1] = ",0.1]"

		if package.downloading then
			formspec[#formspec + 1] = "animated_image[-1.7,-0.15;1,1;downloading;"
			formspec[#formspec + 1] = core.formspec_escape(defaulttexturedir)
			formspec[#formspec + 1] = "cdb_downloading.png;3;400;]"
		elseif package.queued then
			formspec[#formspec + 1] = second_base
			formspec[#formspec + 1] = "cdb_queued.png;queued;]"
		elseif not package.path then
			local elem_name = "install_" .. i .. ";"
			formspec[#formspec + 1] = "style[" .. elem_name .. "bgcolor=#71aa34]"
			formspec[#formspec + 1] = second_base .. "cdb_add.png;" .. elem_name .. "]"
			formspec[#formspec + 1] = "tooltip[" .. elem_name .. fgettext("Install") .. tooltip_colors
		else
			if package.installed_release < package.release then
				-- The install_ action also handles updating
				local elem_name = "install_" .. i .. ";"
				formspec[#formspec + 1] = "style[" .. elem_name .. "bgcolor=#28ccdf]"
				formspec[#formspec + 1] = third_base .. "cdb_update.png;" .. elem_name .. "]"
				formspec[#formspec + 1] = "tooltip[" .. elem_name .. fgettext("Update") .. tooltip_colors

				description_width = description_width - 0.7 - 0.15
			end

			local elem_name = "uninstall_" .. i .. ";"
			formspec[#formspec + 1] = "style[" .. elem_name .. "bgcolor=#a93b3b]"
			formspec[#formspec + 1] = second_base .. "cdb_clear.png;" .. elem_name .. "]"
			formspec[#formspec + 1] = "tooltip[" .. elem_name .. fgettext("Uninstall") .. tooltip_colors
		end

		local web_elem_name = "view_" .. i .. ";"
		formspec[#formspec + 1] = "image_button[-0.7,0;0.7,0.7;" ..
			core.formspec_escape(defaulttexturedir) .. "cdb_viewonline.png;" .. web_elem_name .. "]"
		formspec[#formspec + 1] = "tooltip[" .. web_elem_name ..
			fgettext("View more information in a web browser") .. tooltip_colors
		formspec[#formspec + 1] = "container_end[]"

		-- description
		formspec[#formspec + 1] = "textarea[1.855,0.3;"
		formspec[#formspec + 1] = tostring(description_width)
		formspec[#formspec + 1] = ",0.8;;;"
		formspec[#formspec + 1] = core.formspec_escape(package.short_description)
		formspec[#formspec + 1] = "]"

		formspec[#formspec + 1] = "container_end[]"
	end

	return table.concat(formspec)
end


local function handle_submit(this, fields)
	if fields.search or fields.key_enter_field == "search_string" then
		search_string = fields.search_string:trim()
		cur_page = 1
		contentdb.filter_packages(search_string, filter_types_type[filter_type])
		return true
	end

	if fields.clear then
		search_string = ""
		cur_page = 1
		contentdb.filter_packages("", filter_types_type[filter_type])
		return true
	end

	if fields.back then
		this:delete()
		return true
	end

	if fields.pstart then
		cur_page = 1
		return true
	end

	if fields.pend then
		cur_page = this.data.pagemax
		return true
	end

	if fields.pnext then
		cur_page = cur_page + 1
		if cur_page > this.data.pagemax then
			cur_page = 1
		end
		return true
	end

	if fields.pback then
		if cur_page == 1 then
			cur_page = this.data.pagemax
		else
			cur_page = cur_page - 1
		end
		return true
	end

	if fields.type then
		local new_type = table.indexof(filter_types_titles, fields.type)
		if new_type ~= filter_type then
			filter_type = new_type
			cur_page = 1
			contentdb.filter_packages(search_string, filter_types_type[filter_type])
			return true
		end
	end

	if fields.update_all then
		for i=1, #contentdb.packages_full do
			local package = contentdb.packages_full[i]
			if package.path and package.installed_release < package.release and
					not (package.downloading or package.queued) then
				contentdb.queue_download(package, contentdb.REASON_UPDATE)
			end
		end
		return true
	end

	local start_idx = (cur_page - 1) * num_per_page + 1
	assert(start_idx ~= nil)
	for i=start_idx, math.min(#contentdb.packages, start_idx+num_per_page-1) do
		local package = contentdb.packages[i]
		assert(package)

		if fields["install_" .. i] then
			install_or_update_package(this, package)
			return true
		end

		if fields["uninstall_" .. i] then
			local dlg = create_delete_content_dlg(package)
			dlg:set_parent(this)
			this:hide()
			dlg:show()
			return true
		end

		if fields["view_" .. i] then
			local url = ("%s/packages/%s?protocol_version=%d"):format(
					core.settings:get("contentdb_url"), package.url_part,
					core.get_max_supp_proto())
			core.open_url(url)
			return true
		end
	end

	return false
end


local function handle_events(event)
	if event == "DialogShow" then
		-- On touchscreen, don't show the "MINETEST" header behind the dialog.
		mm_game_theme.set_engine(core.settings:get_bool("enable_touch"))

		-- If ContentDB is already loaded, auto-install packages here.
		do_auto_install()

		return true
	end

	return false
end


--- Creates a ContentDB dialog.
---
--- @param type string | nil
--- Sets initial package filter. "game", "mod", "txp" or nil (no filter).
--- @param install_spec table | nil
--- ContentDB ID of package as returned by pkgmgr.get_contentdb_id().
--- Sets package to install or update automatically.
function create_contentdb_dlg(type, install_spec)
	search_string = ""
	cur_page = 1
	if type then
		-- table.indexof does not work on tables that contain `nil`
		for i, v in pairs(filter_types_type) do
			if v == type then
				filter_type = i
				break
			end
		end
	else
		filter_type = 1
	end

	-- Keep the old auto_install_spec if the caller doesn't specify one.
	if install_spec then
		auto_install_spec = install_spec
	end

	load()

	return dialog_create("contentdb",
			get_formspec,
			handle_submit,
			handle_events)
end
