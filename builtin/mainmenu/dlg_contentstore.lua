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

if not minetest.get_http_api then
	function create_store_dlg()
		return messagebox("store",
				fgettext("ContentDB is not available when Minetest was compiled without cURL"))
	end
	return
end

local store = { packages = {}, packages_full = {} }

local http = minetest.get_http_api()

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

local number_downloading = 0
local download_queue = {}

local filter_types_type = {
	nil,
	"game",
	"mod",
	"txp",
}


local function download_package(param)
	if core.download_file(param.package.url, param.filename) then
		return {
			filename = param.filename,
			successful = true,
		}
	else
		core.log("error", "downloading " .. dump(param.package.url) .. " failed")
		return {
			successful = false,
		}
	end
end

local function start_install(package)
	local params = {
		package = package,
		filename = os.tempfolder() .. "_MODNAME_" .. package.name .. ".zip",
	}

	number_downloading = number_downloading + 1

	local function callback(result)
		if result.successful then
			local path, msg = pkgmgr.install(package.type,
					result.filename, package.name,
					package.path)
			if not path then
				gamedata.errormessage = msg
			else
				core.log("action", "Installed package to " .. path)

				local conf_path
				local name_is_title = false
				if package.type == "mod" then
					local actual_type = pkgmgr.get_folder_type(path)
					if actual_type.type == "modpack" then
						conf_path = path .. DIR_DELIM .. "modpack.conf"
					else
						conf_path = path .. DIR_DELIM .. "mod.conf"
					end
				elseif package.type == "game" then
					conf_path = path .. DIR_DELIM .. "game.conf"
					name_is_title = true
				elseif package.type == "txp" then
					conf_path = path .. DIR_DELIM .. "texture_pack.conf"
				end

				if conf_path then
					local conf = Settings(conf_path)
					if name_is_title then
						conf:set("name",   package.title)
					else
						conf:set("title",  package.title)
						conf:set("name",   package.name)
					end
					if not conf:get("description") then
						conf:set("description", package.short_description)
					end
					conf:set("author",     package.author)
					conf:set("release",    package.release)
					conf:write()
				end
			end
			os.remove(result.filename)
		else
			gamedata.errormessage = fgettext("Failed to download $1", package.name)
		end

		package.downloading = false

		number_downloading = number_downloading - 1

		local next = download_queue[1]
		if next then
			table.remove(download_queue, 1)

			start_install(next)
		end

		ui.update()
	end

	package.queued = false
	package.downloading = true

	if not core.handle_async(download_package, params, callback) then
		core.log("error", "ERROR: async event failed")
		gamedata.errormessage = fgettext("Failed to download $1", package.name)
		return
	end
end

local function queue_download(package)
	local max_concurrent_downloads = tonumber(minetest.settings:get("contentdb_max_concurrent_downloads"))
	if number_downloading < max_concurrent_downloads then
		start_install(package)
	else
		table.insert(download_queue, package)
		package.queued = true
	end
end

local function get_file_extension(path)
	local parts = path:split(".")
	return parts[#parts]
end

local function get_screenshot(package)
	if not package.thumbnail then
		return defaulttexturedir .. "no_screenshot.png"
	elseif screenshot_downloading[package.thumbnail] then
		return defaulttexturedir .. "loading_screenshot.png"
	end

	-- Get tmp screenshot path
	local ext = get_file_extension(package.thumbnail)
	local filepath = screenshot_dir .. DIR_DELIM ..
		("%s-%s-%s.%s"):format(package.type, package.author, package.name, ext)

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

function store.load()
	local version = core.get_version()
	local base_url = core.settings:get("contentdb_url")
	local url = base_url ..
		"/api/packages/?type=mod&type=game&type=txp&protocol_version=" ..
		core.get_max_supp_proto() .. "&engine_version=" .. version.string

	for _, item in pairs(core.settings:get("contentdb_flag_blacklist"):split(",")) do
		item = item:trim()
		if item ~= "" then
			url = url .. "&hide=" .. item
		end
	end

	local timeout = tonumber(minetest.settings:get("curl_file_download_timeout"))
	local response = http.fetch_sync({ url = url, timeout = timeout })
	if not response.succeeded then
		return
	end

	store.packages_full = core.parse_json(response.data) or {}

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

	local function matches_keywords(package)
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
		if (query == "" or matches_keywords(package)) and
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

	local W = 15.75
	local H = 9.5

	local formspec
	if #store.packages_full > 0 then
		formspec = {
			"formspec_version[3]",
			"size[15.75,9.5]",
			"position[0.5,0.55]",

			"style[status;border=false]",

			"container[0.375,0.375]",
			"field[0,0;7.225,0.8;search_string;;", core.formspec_escape(search_string), "]",
			"field_close_on_enter[search_string;false]",
			"button[7.225,0;2,0.8;search;", fgettext("Search"), "]",
			"dropdown[9.6,0;2.4,0.8;type;", table.concat(filter_types_titles, ","), ";", filter_type, "]",
			"container_end[]",

			-- Page nav buttons
			"container[0,", H - 0.8 - 0.375, "]",
			"button[0.375,0;4,0.8;back;", fgettext("Back to Main Menu"), "]",

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

		if number_downloading > 0 then
			formspec[#formspec + 1] = "button[12.75,0.375;2.625,0.8;status;"
			if #download_queue > 0 then
				formspec[#formspec + 1] = fgettext("$1 downloading,\n$2 queued", number_downloading, #download_queue)
			else
				formspec[#formspec + 1] = fgettext("$1 downloading...", number_downloading)
			end
			formspec[#formspec + 1] = "]"
		else
			local num_avail_updates = 0
			for i=1, #store.packages_full do
				local package = store.packages_full[i]
				if package.path and package.installed_release < package.release and
						not (package.downloading or package.queued) then
					num_avail_updates = num_avail_updates + 1
				end
			end

			if num_avail_updates == 0 then
				formspec[#formspec + 1] = "button[12.75,0.375;2.625,0.8;status;"
				formspec[#formspec + 1] = fgettext("No updates")
				formspec[#formspec + 1] = "]"
			else
				formspec[#formspec + 1] = "button[12.75,0.375;2.625,0.8;update_all;"
				formspec[#formspec + 1] = fgettext("Update All [$1]", num_avail_updates)
				formspec[#formspec + 1] = "]"
			end
		end

		if #store.packages == 0 then
			formspec[#formspec + 1] = "label[4,3;"
			formspec[#formspec + 1] = fgettext("No results")
			formspec[#formspec + 1] = "]"
		end
	else
		formspec = {
			"size[12,7]",
			"position[0.5,0.55]",
			"label[4,3;", fgettext("No packages could be retrieved"), "]",
			"container[0,", H - 0.8 - 0.375, "]",
			"button[0,0;4,0.8;back;", fgettext("Back to Main Menu"), "]",
			"container_end[]",
		}
	end

	local start_idx = (cur_page - 1) * num_per_page + 1
	for i=start_idx, math.min(#store.packages, start_idx+num_per_page-1) do
		local package = store.packages[i]
		formspec[#formspec + 1] = "container[0.375,"
		formspec[#formspec + 1] = (i - start_idx) * 1.375 + (2*0.375 + 0.8)
		formspec[#formspec + 1] = "]"

		-- image
		formspec[#formspec + 1] = "image[0,0;1.5,1;"
		formspec[#formspec + 1] = core.formspec_escape(get_screenshot(package))
		formspec[#formspec + 1] = "]"

		-- title
		formspec[#formspec + 1] = "label[1.875,0.1;"
		formspec[#formspec + 1] = core.formspec_escape(
				minetest.colorize(mt_color_green, package.title) ..
				minetest.colorize("#BFBFBF", " by " .. package.author))
		formspec[#formspec + 1] = "]"

		-- buttons
		local description_width = W - 0.375*5 - 1 - 2*1.5
		formspec[#formspec + 1] = "container["
		formspec[#formspec + 1] = W - 0.375*2
		formspec[#formspec + 1] = ",0.1]"

		if package.downloading then
			formspec[#formspec + 1] = "button[-3.5,0;2,0.8;status;"
			formspec[#formspec + 1] = fgettext("Downloading...")
			formspec[#formspec + 1] = "]"
		elseif package.queued then
			formspec[#formspec + 1] = "button[-3.5,0;2,0.8;status;"
			formspec[#formspec + 1] = fgettext("Queued")
			formspec[#formspec + 1] = "]"
		elseif not package.path then
			formspec[#formspec + 1] = "button[-3,0;1.5,0.8;install_"
			formspec[#formspec + 1] = tostring(i)
			formspec[#formspec + 1] = ";"
			formspec[#formspec + 1] = fgettext("Install")
			formspec[#formspec + 1] = "]"
		else
			if package.installed_release < package.release then
				description_width = description_width - 1.5

				-- The install_ action also handles updating
				formspec[#formspec + 1] = "button[-4.5,0;1.5,0.8;install_"
				formspec[#formspec + 1] = tostring(i)
				formspec[#formspec + 1] = ";"
				formspec[#formspec + 1] = fgettext("Update")
				formspec[#formspec + 1] = "]"
			end

			formspec[#formspec + 1] = "button[-3,0;1.5,0.8;uninstall_"
			formspec[#formspec + 1] = tostring(i)
			formspec[#formspec + 1] = ";"
			formspec[#formspec + 1] = fgettext("Uninstall")
			formspec[#formspec + 1] = "]"
		end

		formspec[#formspec + 1] = "button[-1.5,0;1.5,0.8;view_"
		formspec[#formspec + 1] = tostring(i)
		formspec[#formspec + 1] = ";"
		formspec[#formspec + 1] = fgettext("View")
		formspec[#formspec + 1] = "]"
		formspec[#formspec + 1] = "container_end[]"

		-- description
		formspec[#formspec + 1] = "textarea[1.855,0.3;"
		formspec[#formspec + 1] = tostring(description_width)
		formspec[#formspec + 1] = ",0.8;;;"
		formspec[#formspec + 1] = core.formspec_escape(package.short_description)
		formspec[#formspec + 1] = "]"

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

	if fields.update_all then
		for i=1, #store.packages_full do
			local package = store.packages_full[i]
			if package.path and package.installed_release < package.release and
					not (package.downloading or package.queued) then
				queue_download(package)
			end
		end
		return true
	end

	local start_idx = (cur_page - 1) * num_per_page + 1
	assert(start_idx ~= nil)
	for i=start_idx, math.min(#store.packages, start_idx+num_per_page-1) do
		local package = store.packages[i]
		assert(package)

		if fields["install_" .. i] then
			queue_download(package)
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
			local url = ("%s/packages/%s/%s?protocol_version=%d"):format(
					core.settings:get("contentdb_url"),
					package.author, package.name, core.get_max_supp_proto())
			core.open_url(url)
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

	if type then
		-- table.indexof does not work on tables that contain `nil`
		for i, v in pairs(filter_types_type) do
			if v == type then
				filter_type = i
				break
			end
		end
	end

	store.filter_packages(search_string)

	return dialog_create("store",
			store.get_formspec,
			store.handle_submit,
			nil)
end
