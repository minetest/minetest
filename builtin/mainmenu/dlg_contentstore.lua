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

local store = { packages = {}, packages_full = {} }
local package_dialog = {}

-- Screenshot
local screenshot_dir = core.get_cache_path() .. DIR_DELIM .. "cdb"
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
				core.log("action", "Installed package to " .. path)

				local conf_path
				local name_is_title = false
				if result.package.type == "mod" then
					local actual_type = pkgmgr.get_folder_type(path)
					if actual_type.type == "modpack" then
						conf_path = path .. DIR_DELIM .. "modpack.conf"
					else
						conf_path = path .. DIR_DELIM .. "mod.conf"
					end
				elseif result.package.type == "game" then
					conf_path = path .. DIR_DELIM .. "game.conf"
					name_is_title = true
				elseif result.package.type == "txp" then
					conf_path = path .. DIR_DELIM .. "texture_pack.conf"
				end

				if conf_path then
					local conf = Settings(conf_path)
					if name_is_title then
						conf:set("name",   result.package.title)
					else
						conf:set("title",  result.package.title)
						conf:set("name",   result.package.name)
					end
					if not conf:get("description") then
						conf:set("description", result.package.short_description)
					end
					conf:set("author",     result.package.author)
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
	if not package.thumbnail then
		return defaulttexturedir .. "no_screenshot.png"
	elseif screenshot_downloading[package.thumbnail] then
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
	if screenshot_downloaded[package.thumbnail] then
		return defaulttexturedir .. "error_screenshot.png"
	end

	-- Download

	local function download_screenshot(params)
		return core.download_file(params.url, params.dest)
	end
	local function callback(success)
		screenshot_downloading[package.thumbnail] = nil
		screenshot_downloaded[package.thumbnail] = true
		if not success then
			core.log("warning", "Screenshot download failed for some reason")
		end
		ui.update()
	end
	if core.handle_async(download_screenshot,
			{ dest = filepath, url = package.thumbnail }, callback) then
		screenshot_downloading[package.thumbnail] = true
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
		"image[0,1;4.5,3;", core.formspec_escape(get_screenshot(package)), ']',
		"label[3.8,1;",
		minetest.colorize(mt_color_green, core.formspec_escape(package.title)), "\n",
		minetest.colorize('#BFBFBF', "by " .. core.formspec_escape(package.author)), "]",
		"textarea[4,2;5.3,2;;;", core.formspec_escape(package.short_description), "]",
		"button[0,0;2,1;back;", fgettext("Back"), "]",
	}

	if not package.path then
		formspec[#formspec + 1] = "button[7,0;2,1;install;"
		formspec[#formspec + 1] = fgettext("Install")
		formspec[#formspec + 1] = "]"
	elseif package.installed_release < package.release then
		-- The install_ action also handles updating
		formspec[#formspec + 1] = "button[7,0;2,1;install;"
		formspec[#formspec + 1] = fgettext("Update")
		formspec[#formspec + 1] = "]"
		formspec[#formspec + 1] = "button[5,0;2,1;uninstall;"
		formspec[#formspec + 1] = fgettext("Uninstall")
		formspec[#formspec + 1] = "]"
	else
		formspec[#formspec + 1] = "button[7,0;2,1;uninstall;"
		formspec[#formspec + 1] = fgettext("Uninstall")
		formspec[#formspec + 1] = "]"
	end

	return table.concat(formspec, "")
end

function package_dialog.handle_submit(this, fields)
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
	local tmpdir = os.tempfolder()
	local target = tmpdir .. DIR_DELIM .. "packages.json"

	assert(core.create_dir(tmpdir))

	local base_url     = core.settings:get("contentdb_url")
	local url = base_url ..
		"/api/packages/?type=mod&type=game&type=txp&protocol_version=" ..
		core.get_max_supp_proto()

	for _, item in pairs(core.settings:get("contentdb_flag_blacklist"):split(",")) do
		item = item:trim()
		if item ~= "" then
			url = url .. "&hide=" .. item
		end
	end

	core.download_file(url, target)

	local file = io.open(target, "r")
	if file then
		store.packages_full = core.parse_json(file:read("*all")) or {}
		file:close()

		for _, package in pairs(store.packages_full) do
			package.url = base_url .. "/packages/" ..
				package.author .. "/" .. package.name ..
				"/releases/" .. package.release .. "/download/"

			local name_len = #package.name
			if package.type == "game" and name_len > 5 and package.name:sub(name_len - 4) == "_game" then
				package.id = package.author:lower() .. "/" .. package.name:sub(1, name_len - 5)
			else
				package.id = package.author:lower() .. "/" .. package.name
			end
		end

		store.packages = store.packages_full
		store.loaded = true
	end

	core.delete_dir(tmpdir)
end

function store.update_paths()
	local mod_hash = {}
	pkgmgr.refresh_globals()
	for _, mod in pairs(pkgmgr.global_mods:get_list()) do
		if mod.author then
			mod_hash[mod.author:lower() .. "/" .. mod.name] = mod
		end
	end

	local game_hash = {}
	pkgmgr.update_gamelist()
	for _, game in pairs(pkgmgr.games) do
		if game.author ~= "" then
			game_hash[game.author:lower() .. "/" .. game.id] = game
		end
	end

	local txp_hash = {}
	for _, txp in pairs(pkgmgr.get_texture_packs()) do
		if txp.author then
			txp_hash[txp.author:lower() .. "/" .. txp.name] = txp
		end
	end

	for _, package in pairs(store.packages_full) do
		local content
		if package.type == "mod" then
			content = mod_hash[package.id]
		elseif package.type == "game" then
			content = game_hash[package.id]
		elseif package.type == "txp" then
			content = txp_hash[package.id]
		end

		if content then
			package.path = content.path
			package.installed_release = content.release or 0
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

function store.get_formspec(dlgdata)
	store.update_paths()

	dlgdata.pagemax = math.max(math.ceil(#store.packages / num_per_page), 1)
	if cur_page > dlgdata.pagemax then
		cur_page = 1
	end

	local formspec
	if #store.packages_full > 0 then
		formspec = {
			"size[12,7;true]",
			"position[0.5,0.55]",
			"field[0.2,0.1;7.8,1;search_string;;",
			core.formspec_escape(search_string), "]",
			"field_close_on_enter[search_string;false]",
			"button[7.7,-0.2;2,1;search;",
			fgettext("Search"), "]",
			"dropdown[9.7,-0.1;2.4;type;",
			table.concat(filter_types_titles, ","),
			";", filter_type, "]",
			-- "textlist[0,1;2.4,5.6;a;",
			-- table.concat(taglist, ","), "]",

			-- Page nav buttons
			"container[0,",
			num_per_page + 1.5, "]",
			"button[-0.1,0;3,1;back;",
			fgettext("Back to Main Menu"), "]",
			"button[7.1,0;1,1;pstart;<<]",
			"button[8.1,0;1,1;pback;<]",
			"label[9.2,0.2;",
			tonumber(cur_page), " / ",
			tonumber(dlgdata.pagemax), "]",
			"button[10.1,0;1,1;pnext;>]",
			"button[11.1,0;1,1;pend;>>]",
			"container_end[]",
		}

		if #store.packages == 0 then
			formspec[#formspec + 1] = "label[4,3;"
			formspec[#formspec + 1] = fgettext("No results")
			formspec[#formspec + 1] = "]"
		end
	else
		formspec = {
			"size[12,7;true]",
			"position[0.5,0.55]",
			"label[4,3;", fgettext("No packages could be retrieved"), "]",
			"button[-0.1,",
			num_per_page + 1.5,
			";3,1;back;",
			fgettext("Back to Main Menu"), "]",
		}
	end

	local start_idx = (cur_page - 1) * num_per_page + 1
	for i=start_idx, math.min(#store.packages, start_idx+num_per_page-1) do
		local package = store.packages[i]
		formspec[#formspec + 1] = "container[0.5,"
		formspec[#formspec + 1] = (i - start_idx) * 1.1 + 1
		formspec[#formspec + 1] = "]"

		-- image
		formspec[#formspec + 1] = "image[-0.4,0;1.5,1;"
		formspec[#formspec + 1] = core.formspec_escape(get_screenshot(package))
		formspec[#formspec + 1] = "]"

		-- title
		formspec[#formspec + 1] = "label[1,-0.1;"
		formspec[#formspec + 1] = core.formspec_escape(
				minetest.colorize(mt_color_green, package.title) ..
				minetest.colorize("#BFBFBF", " by " .. package.author))
		formspec[#formspec + 1] = "]"

		-- description
		if package.path and package.installed_release < package.release then
			formspec[#formspec + 1] = "textarea[1.25,0.3;7.5,1;;;"
		else
			formspec[#formspec + 1] = "textarea[1.25,0.3;9,1;;;"
		end
		formspec[#formspec + 1] = core.formspec_escape(package.short_description)
		formspec[#formspec + 1] = "]"

		-- buttons
		if not package.path then
			formspec[#formspec + 1] = "button[9.9,0;1.5,1;install_"
			formspec[#formspec + 1] = tostring(i)
			formspec[#formspec + 1] = ";"
			formspec[#formspec + 1] = fgettext("Install")
			formspec[#formspec + 1] = "]"
		else
			if package.installed_release < package.release then
				-- The install_ action also handles updating
				formspec[#formspec + 1] = "button[8.4,0;1.5,1;install_"
				formspec[#formspec + 1] = tostring(i)
				formspec[#formspec + 1] = ";"
				formspec[#formspec + 1] = fgettext("Update")
				formspec[#formspec + 1] = "]"
			end

			formspec[#formspec + 1] = "button[9.9,0;1.5,1;uninstall_"
			formspec[#formspec + 1] = tostring(i)
			formspec[#formspec + 1] = ";"
			formspec[#formspec + 1] = fgettext("Uninstall")
			formspec[#formspec + 1] = "]"
		end

		--formspec[#formspec + 1] = "button[9.9,0;1.5,1;view_"
		--formspec[#formspec + 1] = tostring(i)
		--formspec[#formspec + 1] = ";"
		--formspec[#formspec + 1] = fgettext("View")
		--formspec[#formspec + 1] = "]"

		formspec[#formspec + 1] = "container_end[]"
	end

	return table.concat(formspec, "")
end

function store.handle_submit(this, fields)
	if fields.search or fields.key_enter_field == "search_string" then
		search_string = fields.search_string:trim()
		cur_page = 1
		store.filter_packages(search_string)
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
	if not store.loaded or #store.packages_full == 0 then
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
