--Minetest
--Copyright (C) 2018 rubenwardy
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

local store = {}
local package_dialog = {}

-- Screenshot
local screenshot_dir = os.tempfolder()
assert(core.create_dir(screenshot_dir))
local screenshot_downloading = {}
local screenshot_downloaded = {}

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

local filter_types_type = {
	nil,
	"game",
	"mod",
	"txp",
}




local function download_package(param)
	if core.download_file(param.package.url, param.filename) then
		return {
			package = param.package,
			filename = param.filename,
			successful = true,
		}
	else
		core.log("error", "downloading " .. dump(param.package.url) .. " failed")
		return {
			package = param.package,
			successful = false,
		}
	end
end

local function start_install(calling_dialog, package)
	local params = {
		package = package,
		filename = os.tempfolder() .. "_MODNAME_" .. package.name .. ".zip",
	}

	local function callback(result)
		if result.successful then
			local path, msg = pkgmgr.install(result.package.type,
					result.filename, result.package.name,
					result.package.path)
			if not path then
				gamedata.errormessage = msg
			else
				local conf_path
				local name_is_title = false
				if result.package.type == "mod" then
					conf_path = path .. DIR_DELIM .. "mod.conf"
				elseif result.package.type == "game" then
					conf_path = path .. DIR_DELIM .. "game.conf"
					name_is_title = true
				elseif result.package.type == "txp" then
					conf_path = path .. DIR_DELIM .. "texture_pack.conf"
				end

				if conf_path then
					local conf = Settings(conf_path)
					local function set_def(key, value)
						if conf:get(key) == nil then
							conf:set(key, value)
						end
					end
					if name_is_title then
						set_def("name",    result.package.title)
					else
						set_def("title",   result.package.title)
						set_def("name",    result.package.name)
					end
					set_def("description", result.package.short_description)
					set_def("author",      result.package.author)
					conf:set("release",    result.package.release)
					conf:write()
				end
			end
			os.remove(result.filename)
		else
			gamedata.errormessage = fgettext("Failed to download $1", package.name)
		end

		if gamedata.errormessage == nil then
			core.button_handler({btn_hidden_close_download=result})
		else
			core.button_handler({btn_hidden_close_download={successful=false}})
		end
	end

	if not core.handle_async(download_package, params, callback) then
		core.log("error", "ERROR: async event failed")
		gamedata.errormessage = fgettext("Failed to download $1", package.name)
	end

	local new_dlg = dialog_create("store_downloading",
		function(data)
			return "size[7,2]label[0.25,0.75;" ..
				fgettext("Downloading and installing $1, please wait...", data.title) .. "]"
		end,
		function(this,fields)
			if fields["btn_hidden_close_download"] ~= nil then
				this:delete()
				return true
			end

			return false
		end,
		nil)

	new_dlg:set_parent(calling_dialog)
	new_dlg.data.title = package.title
	calling_dialog:hide()
	new_dlg:show()
end

local function get_screenshot(package)
	if #package.screenshots == 0 then
		return defaulttexturedir .. "no_screenshot.png"
	elseif screenshot_downloading[package.screenshots[1]] then
		return defaulttexturedir .. "loading_screenshot.png"
	end

	-- Get tmp screenshot path
	local filepath = screenshot_dir .. DIR_DELIM ..
		package.type .. "-" .. package.author .. "-" .. package.name .. ".png"

	-- Return if already downloaded
	local file = io.open(filepath, "r")
	if file then
		file:close()
		return filepath
	end

	-- Show error if we've failed to download before
	if screenshot_downloaded[package.screenshots[1]] then
		return defaulttexturedir .. "error_screenshot.png"
	end

	-- Download

	local function download_screenshot(params)
		return core.download_file(params.url, params.dest)
	end
	local function callback(success)
		screenshot_downloading[package.screenshots[1]] = nil
		screenshot_downloaded[package.screenshots[1]] = true
		if not success then
			core.log("warning", "Screenshot download failed for some reason")
		end

		local ele = ui.childlist.store
		if ele and not ele.hidden then
			core.update_formspec(ele:formspec())
		end
	end
	if core.handle_async(download_screenshot,
			{ dest = filepath, url = package.screenshots[1] }, callback) then
		screenshot_downloading[package.screenshots[1]] = true
	else
		core.log("error", "ERROR: async event failed")
		return defaulttexturedir .. "error_screenshot.png"
	end

	return defaulttexturedir .. "loading_screenshot.png"
end



function package_dialog.get_formspec()
	local package = package_dialog.package

	store.update_paths()

	local formspec = {
		"size[9,4;true]",
		"label[2.5,0.2;", core.formspec_escape(package.title), "]",
		"textarea[0.2,1;9,3;;;", core.formspec_escape(package.short_description), "]",
		"button[0,0;2,1;back;", fgettext("Back"), "]",
	}

	if not package.path then
		formspec[#formspec + 1] = "button[7,0;2,1;install;"
		formspec[#formspec + 1] = fgettext("Install")
		formspec[#formspec + 1] = "]"
	elseif package.installed_release < package.release then
		formspec[#formspec + 1] = "button[7,0;2,1;install;"
		formspec[#formspec + 1] = fgettext("Update")
		formspec[#formspec + 1] = "]"
		formspec[#formspec + 1] = "button[7,1;2,1;uninstall;"
		formspec[#formspec + 1] = fgettext("Uninstall")
		formspec[#formspec + 1] = "]"
	else
		formspec[#formspec + 1] = "button[7,0;2,1;uninstall;"
		formspec[#formspec + 1] = fgettext("Uninstall")
		formspec[#formspec + 1] = "]"
	end

	-- TODO: screenshots

	return table.concat(formspec, "")
end

function package_dialog.handle_submit(this, fields, tabname, tabdata)
	if fields.back then
		this:delete()
		return true
	end

	if fields.install then
		start_install(this, package_dialog.package)
		return true
	end

	if fields.uninstall then
		local dlg_delmod = create_delete_content_dlg(package_dialog.package)
		dlg_delmod:set_parent(this)
		this:hide()
		dlg_delmod:show()
		return true
	end

	return false
end

function package_dialog.create(package)
	package_dialog.package = package
	return dialog_create("package_view",
		package_dialog.get_formspec,
		package_dialog.handle_submit,
		nil)
end

function store.load()
	store.packages_full = core.get_package_list()
	store.packages = store.packages_full
	store.loaded = true
end

function store.update_paths()
	local mod_hash = {}
	pkgmgr.refresh_globals()
	for _, mod in pairs(pkgmgr.global_mods:get_list()) do
		mod_hash[mod.name] = mod
	end

	local game_hash = {}
	pkgmgr.update_gamelist()
	for _, game in pairs(pkgmgr.games) do
		game_hash[game.id] = game
	end

	local txp_hash = {}
	for _, txp in pairs(pkgmgr.get_texture_packs()) do
		txp_hash[txp.name] = txp
	end

	for _, package in pairs(store.packages_full) do
		local content
		if package.type == "mod" then
			content = mod_hash[package.name]
		elseif package.type == "game" then
			content = game_hash[package.name]
		elseif package.type == "txp" then
			content = txp_hash[package.name]
		end

		if content and content.author == package.author then
			package.path = content.path
			package.installed_release = content.release
		else
			package.path = nil
		end
	end
end

function store.filter_packages(query)
	if query == "" and filter_type == 1 then
		store.packages = store.packages_full
		return
	end

	local keywords = {}
	for word in query:lower():gmatch("%S+") do
		table.insert(keywords, word)
	end

	local function matches_keywords(package, keywords)
		for k = 1, #keywords do
			local keyword = keywords[k]

			if string.find(package.name:lower(), keyword, 1, true) or
					string.find(package.title:lower(), keyword, 1, true) or
					string.find(package.author:lower(), keyword, 1, true) or
					string.find(package.short_description:lower(), keyword, 1, true) then
				return true
			end
		end

		return false
	end

	store.packages = {}
	for _, package in pairs(store.packages_full) do
		if (query == "" or matches_keywords(package, keywords)) and
				(filter_type == 1 or package.type == filter_types_type[filter_type]) then
			store.packages[#store.packages + 1] = package
		end
	end

end

function store.get_formspec()
	assert(store.loaded)

	store.update_paths()

	local pages = math.ceil(#store.packages / num_per_page)
	if cur_page > pages then
		cur_page = 1
	end

	local formspec = {
		"size[12,6.5;true]",
		"field[0.3,0.1;10.2,1;search_string;;", core.formspec_escape(search_string), "]",
		"field_close_on_enter[search_string;false]",
		"button[10.2,-0.2;2,1;search;", fgettext("Search"), "]",
		"dropdown[0,1;2.4;type;",
		table.concat(filter_types_titles, ","),
		";",
		filter_type,
		"]",
		-- "textlist[0,1;2.4,5.6;a;",
		-- table.concat(taglist, ","),
		-- "]"
	}

	local start_idx = (cur_page - 1) * num_per_page + 1
	for i=start_idx, math.min(#store.packages, start_idx+num_per_page-1) do
		local package = store.packages[i]
		formspec[#formspec + 1] = "container[3,"
		formspec[#formspec + 1] = i - start_idx + 1
		formspec[#formspec + 1] = "]"

		-- image
		formspec[#formspec + 1] = "image[-0.4,0;1.5,1;"
		formspec[#formspec + 1] = get_screenshot(package)
		formspec[#formspec + 1] = "]"

		-- title
		formspec[#formspec + 1] = "label[1,-0.1;"
		formspec[#formspec + 1] = core.formspec_escape(package.title ..
				" by " .. package.author)
		formspec[#formspec + 1] = "]"

		-- description
		formspec[#formspec + 1] = "textarea[1.25,0.3;5,1;;;"
		formspec[#formspec + 1] = core.formspec_escape(package.short_description)
		formspec[#formspec + 1] = "]"

		-- buttons
		if not package.path then
			formspec[#formspec + 1] = "button[6,0;1.5,1;install_"
			formspec[#formspec + 1] = tostring(i)
			formspec[#formspec + 1] = ";"
			formspec[#formspec + 1] = fgettext("Install")
			formspec[#formspec + 1] = "]"
		elseif package.installed_release < package.release then
			formspec[#formspec + 1] = "button[6,0;1.5,1;install_"
			formspec[#formspec + 1] = tostring(i)
			formspec[#formspec + 1] = ";"
			formspec[#formspec + 1] = fgettext("Update")
			formspec[#formspec + 1] = "]"
		else
			formspec[#formspec + 1] = "button[6,0;1.5,1;uninstall_"
			formspec[#formspec + 1] = tostring(i)
			formspec[#formspec + 1] = ";"
			formspec[#formspec + 1] = fgettext("Uninstall")
			formspec[#formspec + 1] = "]"
		end
		formspec[#formspec + 1] = "button[7.5,0;1.5,1;view_"
		formspec[#formspec + 1] = tostring(i)
		formspec[#formspec + 1] = ";"
		formspec[#formspec + 1] = fgettext("View")
		formspec[#formspec + 1] = "]"

		formspec[#formspec + 1] = "container_end[]"
	end

	formspec[#formspec + 1] = "container[0,"
	formspec[#formspec + 1] = num_per_page + 1
	formspec[#formspec + 1] = "]"
	formspec[#formspec + 1] = "button[2.6,0;3,1;back;"
	formspec[#formspec + 1] = fgettext("Back to Main Menu")
	formspec[#formspec + 1] = "]"
	formspec[#formspec + 1] = "button[7,0;1,1;pstart;<<]"
	formspec[#formspec + 1] = "button[8,0;1,1;pback;<]"
	formspec[#formspec + 1] = "label[9.2,0.2;"
	formspec[#formspec + 1] = tonumber(cur_page)
	formspec[#formspec + 1] = " / "
	formspec[#formspec + 1] = tonumber(pages)
	formspec[#formspec + 1] = "]"
	formspec[#formspec + 1] = "button[10,0;1,1;pnext;>]"
	formspec[#formspec + 1] = "button[11,0;1,1;pend;>>]"
	formspec[#formspec + 1] = "container_end[]"

	formspec[#formspec + 1] = "]"
	return table.concat(formspec, "")
end

function store.handle_submit(this, fields, tabname, tabdata)
	if fields.search or fields.key_enter_field == "search_string" then
		search_string = fields.search_string:trim()
		cur_page = 1
		store.filter_packages(search_string)
		core.update_formspec(store.get_formspec())
		return true
	end

	if fields.back then
		this:delete()
		return true
	end

	if fields.pstart then
		cur_page = 1
		core.update_formspec(store.get_formspec())
		return true
	end

	if fields.pend then
		cur_page = math.ceil(#store.packages / num_per_page)
		core.update_formspec(store.get_formspec())
		return true
	end

	if fields.pnext then
		cur_page = cur_page + 1
		local pages = math.ceil(#store.packages / num_per_page)
		if cur_page > pages then
			cur_page = 1
		end
		core.update_formspec(store.get_formspec())
		return true
	end

	if fields.pback then
		if cur_page == 1 then
			local pages = math.ceil(#store.packages / num_per_page)
			cur_page = pages
		else
			cur_page = cur_page - 1
		end
		core.update_formspec(store.get_formspec())
		return true
	end

	if fields.type then
		local new_type = table.indexof(filter_types_titles, fields.type)
		if new_type ~= filter_type then
			filter_type = new_type
			store.filter_packages(search_string)
			return true
		end
	end

	local start_idx = (cur_page - 1) * num_per_page + 1
	assert(start_idx ~= nil)
	for i=start_idx, math.min(#store.packages, start_idx+num_per_page-1) do
		local package = store.packages[i]
		assert(package)

		if fields["install_" .. i] then
			start_install(this, package)
			return true
		end

		if fields["uninstall_" .. i] then
			local dlg_delmod = create_delete_content_dlg(package)
			dlg_delmod:set_parent(this)
			this:hide()
			dlg_delmod:show()
			return true
		end

		if fields["view_" .. i] then
			local dlg = package_dialog.create(package)
			dlg:set_parent(this)
			this:hide()
			dlg:show()
			return true
		end
	end

	return false
end

function create_store_dlg(type)
	if not store.loaded then
		store.load()
	end

	search_string = ""
	cur_page = 1
	store.filter_packages(search_string)

	return dialog_create("store",
			store.get_formspec,
			store.handle_submit,
			nil)
end
