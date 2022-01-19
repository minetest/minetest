--Minetest
--Copyright (C) 2013 sapier
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
local function get_last_folder(text,count)
	local parts = text:split(DIR_DELIM)

	if count == nil then
		return parts[#parts]
	end

	local retval = ""
	for i=1,count,1 do
		retval = retval .. parts[#parts - (count-i)] .. DIR_DELIM
	end

	return retval
end

local function cleanup_path(temppath)

	local parts = temppath:split("-")
	temppath = ""
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath .. "_"
		end
		temppath = temppath .. parts[i]
	end

	parts = temppath:split(".")
	temppath = ""
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath .. "_"
		end
		temppath = temppath .. parts[i]
	end

	parts = temppath:split("'")
	temppath = ""
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath .. ""
		end
		temppath = temppath .. parts[i]
	end

	parts = temppath:split(" ")
	temppath = ""
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath
		end
		temppath = temppath .. parts[i]
	end

	return temppath
end

local function load_texture_packs(txtpath, retval)
	local list = core.get_dir_list(txtpath, true)
	local current_texture_path = core.settings:get("texture_path")

	for _, item in ipairs(list) do
		if item ~= "base" then
			local name = item

			local path = txtpath .. DIR_DELIM .. item .. DIR_DELIM
			if path == current_texture_path then
				name = fgettext("$1 (Enabled)", name)
			end

			local conf = Settings(path .. "texture_pack.conf")

			retval[#retval + 1] = {
				name = item,
				author = conf:get("author"),
				release = tonumber(conf:get("release")) or 0,
				list_name = name,
				type = "txp",
				path = path,
				enabled = path == current_texture_path,
			}
		end
	end
end

function get_mods(path,retval,modpack)
	local mods = core.get_dir_list(path, true)

	for _, name in ipairs(mods) do
		if name:sub(1, 1) ~= "." then
			local prefix = path .. DIR_DELIM .. name
			local toadd = {
				dir_name = name,
				parent_dir = path,
			}
			retval[#retval + 1] = toadd

			-- Get config file
			local mod_conf
			local modpack_conf = io.open(prefix .. DIR_DELIM .. "modpack.conf")
			if modpack_conf then
				toadd.is_modpack = true
				modpack_conf:close()

				mod_conf = Settings(prefix .. DIR_DELIM .. "modpack.conf"):to_table()
				if mod_conf.name then
					name = mod_conf.name
					toadd.is_name_explicit = true
				end
			else
				mod_conf = Settings(prefix .. DIR_DELIM .. "mod.conf"):to_table()
				if mod_conf.name then
					name = mod_conf.name
					toadd.is_name_explicit = true
				end
			end

			-- Read from config
			toadd.name = name
			toadd.author = mod_conf.author
			toadd.release = tonumber(mod_conf.release) or 0
			toadd.path = prefix
			toadd.type = "mod"

			-- Check modpack.txt
			-- Note: modpack.conf is already checked above
			local modpackfile = io.open(prefix .. DIR_DELIM .. "modpack.txt")
			if modpackfile then
				modpackfile:close()
				toadd.is_modpack = true
			end

			-- Deal with modpack contents
			if modpack and modpack ~= "" then
				toadd.modpack = modpack
			elseif toadd.is_modpack then
				toadd.type = "modpack"
				toadd.is_modpack = true
				get_mods(prefix, retval, name)
			end
		end
	end
end

--modmanager implementation
pkgmgr = {}

function pkgmgr.get_texture_packs()
	local txtpath = core.get_texturepath()
	local txtpath_system = core.get_texturepath_share()
	local retval = {}

	load_texture_packs(txtpath, retval)
	-- on portable versions these two paths coincide. It avoids loading the path twice
	if txtpath ~= txtpath_system then
		load_texture_packs(txtpath_system, retval)
	end

	table.sort(retval, function(a, b)
		return a.name > b.name
	end)

	return retval
end

--------------------------------------------------------------------------------
function pkgmgr.get_folder_type(path)
	local testfile = io.open(path .. DIR_DELIM .. "init.lua","r")
	if testfile ~= nil then
		testfile:close()
		return { type = "mod", path = path }
	end

	testfile = io.open(path .. DIR_DELIM .. "modpack.conf","r")
	if testfile ~= nil then
		testfile:close()
		return { type = "modpack", path = path }
	end

	testfile = io.open(path .. DIR_DELIM .. "modpack.txt","r")
	if testfile ~= nil then
		testfile:close()
		return { type = "modpack", path = path }
	end

	testfile = io.open(path .. DIR_DELIM .. "game.conf","r")
	if testfile ~= nil then
		testfile:close()
		return { type = "game", path = path }
	end

	testfile = io.open(path .. DIR_DELIM .. "texture_pack.conf","r")
	if testfile ~= nil then
		testfile:close()
		return { type = "txp", path = path }
	end

	return nil
end

-------------------------------------------------------------------------------
function pkgmgr.get_base_folder(temppath)
	if temppath == nil then
		return { type = "invalid", path = "" }
	end

	local ret = pkgmgr.get_folder_type(temppath)
	if ret then
		return ret
	end

	local subdirs = core.get_dir_list(temppath, true)
	if #subdirs == 1 then
		ret = pkgmgr.get_folder_type(temppath .. DIR_DELIM .. subdirs[1])
		if ret then
			return ret
		else
			return { type = "invalid", path = temppath .. DIR_DELIM .. subdirs[1] }
		end
	end

	return nil
end

--------------------------------------------------------------------------------
function pkgmgr.isValidModname(modpath)
	if modpath:find("-") ~= nil then
		return false
	end

	return true
end

--------------------------------------------------------------------------------
function pkgmgr.parse_register_line(line)
	local pos1 = line:find("\"")
	local pos2 = nil
	if pos1 ~= nil then
		pos2 = line:find("\"",pos1+1)
	end

	if pos1 ~= nil and pos2 ~= nil then
		local item = line:sub(pos1+1,pos2-1)

		if item ~= nil and
			item ~= "" then
			local pos3 = item:find(":")

			if pos3 ~= nil then
				local retval = item:sub(1,pos3-1)
				if retval ~= nil and
					retval ~= "" then
					return retval
				end
			end
		end
	end
	return nil
end

--------------------------------------------------------------------------------
function pkgmgr.parse_dofile_line(modpath,line)
	local pos1 = line:find("\"")
	local pos2 = nil
	if pos1 ~= nil then
		pos2 = line:find("\"",pos1+1)
	end

	if pos1 ~= nil and pos2 ~= nil then
		local filename = line:sub(pos1+1,pos2-1)

		if filename ~= nil and
			filename ~= "" and
			filename:find(".lua") then
			return pkgmgr.identify_modname(modpath,filename)
		end
	end
	return nil
end

--------------------------------------------------------------------------------
function pkgmgr.identify_modname(modpath,filename)
	local testfile = io.open(modpath .. DIR_DELIM .. filename,"r")
	if testfile ~= nil then
		local line = testfile:read()

		while line~= nil do
			local modname = nil

			if line:find("minetest.register_tool") then
				modname = pkgmgr.parse_register_line(line)
			end

			if line:find("minetest.register_craftitem") then
				modname = pkgmgr.parse_register_line(line)
			end


			if line:find("minetest.register_node") then
				modname = pkgmgr.parse_register_line(line)
			end

			if line:find("dofile") then
				modname = pkgmgr.parse_dofile_line(modpath,line)
			end

			if modname ~= nil then
				testfile:close()
				return modname
			end

			line = testfile:read()
		end
		testfile:close()
	end

	return nil
end
--------------------------------------------------------------------------------
function pkgmgr.render_packagelist(render_list)
	if not render_list then
		if not pkgmgr.global_mods then
			pkgmgr.refresh_globals()
		end
		render_list = pkgmgr.global_mods
	end

	local list = render_list:get_list()
	local retval = {}
	for i, v in ipairs(list) do
		local color = ""
		if v.is_modpack then
			local rawlist = render_list:get_raw_list()
			color = mt_color_dark_green

			for j = 1, #rawlist, 1 do
				if rawlist[j].modpack == list[i].name and
						not rawlist[j].enabled then
					-- Modpack not entirely enabled so showing as grey
					color = mt_color_grey
					break
				end
			end
		elseif v.is_game_content or v.type == "game" then
			color = mt_color_blue
		elseif v.enabled or v.type == "txp" then
			color = mt_color_green
		end

		retval[#retval + 1] = color
		if v.modpack ~= nil or v.loc == "game" then
			retval[#retval + 1] = "1"
		else
			retval[#retval + 1] = "0"
		end
		retval[#retval + 1] = core.formspec_escape(v.list_name or v.name)
	end

	return table.concat(retval, ",")
end

--------------------------------------------------------------------------------
function pkgmgr.get_dependencies(path)
	if path == nil then
		return {}, {}
	end

	local info = core.get_content_info(path)
	return info.depends or {}, info.optional_depends or {}
end

----------- tests whether all of the mods in the modpack are enabled -----------
function pkgmgr.is_modpack_entirely_enabled(data, name)
	local rawlist = data.list:get_raw_list()
	for j = 1, #rawlist do
		if rawlist[j].modpack == name and not rawlist[j].enabled then
			return false
		end
	end
	return true
end

---------- toggles or en/disables a mod or modpack and its dependencies --------
local function toggle_mod_or_modpack(list, toggled_mods, enabled_mods, toset, mod)
	if not mod.is_modpack then
		-- Toggle or en/disable the mod
		if toset == nil then
			toset = not mod.enabled
		end
		if mod.enabled ~= toset then
			mod.enabled = toset
			toggled_mods[#toggled_mods+1] = mod.name
		end
		if toset then
			-- Mark this mod for recursive dependency traversal
			enabled_mods[mod.name] = true
		end
	else
		-- Toggle or en/disable every mod in the modpack,
		-- interleaved unsupported
		for i = 1, #list do
			if list[i].modpack == mod.name then
				toggle_mod_or_modpack(list, toggled_mods, enabled_mods, toset, list[i])
			end
		end
	end
end

function pkgmgr.enable_mod(this, toset)
	local list = this.data.list:get_list()
	local mod = list[this.data.selected_mod]

	-- Game mods can't be enabled or disabled
	if mod.is_game_content then
		return
	end

	local toggled_mods = {}
	local enabled_mods = {}
	toggle_mod_or_modpack(list, toggled_mods, enabled_mods, toset, mod)
	toset = mod.enabled -- Update if toggled

	if not toset then
		-- Mod(s) were disabled, so no dependencies need to be enabled
		table.sort(toggled_mods)
		core.log("info", "Following mods were disabled: " ..
			table.concat(toggled_mods, ", "))
		return
	end

	-- Enable mods' depends after activation

	-- Make a list of mod ids indexed by their names
	local mod_ids = {}
	for id, mod2 in pairs(list) do
		if mod2.type == "mod" and not mod2.is_modpack then
			mod_ids[mod2.name] = id
		end
	end

	-- to_enable is used as a DFS stack with sp as stack pointer
	local to_enable = {}
	local sp = 0
	for name in pairs(enabled_mods) do
		local depends = pkgmgr.get_dependencies(list[mod_ids[name]].path)
		for i = 1, #depends do
			local dependency_name = depends[i]
			if not enabled_mods[dependency_name] then
				sp = sp+1
				to_enable[sp] = dependency_name
			end
		end
	end
	-- If sp is 0, every dependency is already activated
	while sp > 0 do
		local name = to_enable[sp]
		sp = sp-1

		if not enabled_mods[name] then
			enabled_mods[name] = true
			local mod_to_enable = list[mod_ids[name]]
			if not mod_to_enable then
				core.log("warning", "Mod dependency \"" .. name ..
					"\" not found!")
			else
				if mod_to_enable.enabled == false then
					mod_to_enable.enabled = true
					toggled_mods[#toggled_mods+1] = mod_to_enable.name
				end
				-- Push the dependencies of the dependency onto the stack
				local depends = pkgmgr.get_dependencies(mod_to_enable.path)
				for i = 1, #depends do
					if not enabled_mods[name] then
						sp = sp+1
						to_enable[sp] = depends[i]
					end
				end
			end
		end
	end

	-- Log the list of enabled mods
	table.sort(toggled_mods)
	core.log("info", "Following mods were enabled: " ..
		table.concat(toggled_mods, ", "))
end

--------------------------------------------------------------------------------
function pkgmgr.get_worldconfig(worldpath)
	local filename = worldpath ..
				DIR_DELIM .. "world.mt"

	local worldfile = Settings(filename)

	local worldconfig = {}
	worldconfig.global_mods = {}
	worldconfig.game_mods = {}

	for key,value in pairs(worldfile:to_table()) do
		if key == "gameid" then
			worldconfig.id = value
		elseif key:sub(0, 9) == "load_mod_" then
			-- Compatibility: Check against "nil" which was erroneously used
			-- as value for fresh configured worlds
			worldconfig.global_mods[key] = value ~= "false" and value ~= "nil"
				and value
		else
			worldconfig[key] = value
		end
	end

	--read gamemods
	local gamespec = pkgmgr.find_by_gameid(worldconfig.id)
	pkgmgr.get_game_mods(gamespec, worldconfig.game_mods)

	return worldconfig
end

--------------------------------------------------------------------------------
function pkgmgr.install_dir(type, path, basename, targetpath)
	local basefolder = pkgmgr.get_base_folder(path)

	-- There's no good way to detect a texture pack, so let's just assume
	-- it's correct for now.
	if type == "txp" then
		if basefolder and basefolder.type ~= "invalid" and basefolder.type ~= "txp" then
			return nil, fgettext("Unable to install a $1 as a texture pack", basefolder.type)
		end

		local from = basefolder and basefolder.path or path
		if targetpath then
			core.delete_dir(targetpath)
		else
			targetpath = core.get_texturepath() .. DIR_DELIM .. basename
		end
		if not core.copy_dir(from, targetpath, false) then
			return nil,
				fgettext("Failed to install $1 to $2", basename, targetpath)
		end
		return targetpath, nil

	elseif not basefolder then
		return nil, fgettext("Unable to find a valid mod or modpack")
	end

	--
	-- Get destination
	--
	if basefolder.type == "modpack" then
		if type ~= "mod" then
			return nil, fgettext("Unable to install a modpack as a $1", type)
		end

		-- Get destination name for modpack
		if targetpath then
			core.delete_dir(targetpath)
		else
			local clean_path = nil
			if basename ~= nil then
				clean_path = basename
			end
			if not clean_path then
				clean_path = get_last_folder(cleanup_path(basefolder.path))
			end
			if clean_path then
				targetpath = core.get_modpath() .. DIR_DELIM .. clean_path
			else
				return nil,
					fgettext("Install Mod: Unable to find suitable folder name for modpack $1",
					path)
			end
		end
	elseif basefolder.type == "mod" then
		if type ~= "mod" then
			return nil, fgettext("Unable to install a mod as a $1", type)
		end

		if targetpath then
			core.delete_dir(targetpath)
		else
			local targetfolder = basename
			if targetfolder == nil then
				targetfolder = pkgmgr.identify_modname(basefolder.path, "init.lua")
			end

			-- If heuristic failed try to use current foldername
			if targetfolder == nil then
				targetfolder = get_last_folder(basefolder.path)
			end

			if targetfolder ~= nil and pkgmgr.isValidModname(targetfolder) then
				targetpath = core.get_modpath() .. DIR_DELIM .. targetfolder
			else
				return nil, fgettext("Install Mod: Unable to find real mod name for: $1", path)
			end
		end

	elseif basefolder.type == "game" then
		if type ~= "game" then
			return nil, fgettext("Unable to install a game as a $1", type)
		end

		if targetpath then
			core.delete_dir(targetpath)
		else
			targetpath = core.get_gamepath() .. DIR_DELIM .. basename
		end
	end

	-- Copy it
	if not core.copy_dir(basefolder.path, targetpath, false) then
		return nil,
			fgettext("Failed to install $1 to $2", basename, targetpath)
	end

	if basefolder.type == "game" then
		pkgmgr.update_gamelist()
	else
		pkgmgr.refresh_globals()
	end

	return targetpath, nil
end

--------------------------------------------------------------------------------
function pkgmgr.preparemodlist(data)
	local retval = {}

	local global_mods = {}
	local game_mods = {}

	--read global mods
	local modpaths = core.get_modpaths()
	for _, modpath in ipairs(modpaths) do
		get_mods(modpath, global_mods)
	end

	for i=1,#global_mods,1 do
		global_mods[i].type = "mod"
		global_mods[i].loc = "global"
		retval[#retval + 1] = global_mods[i]
	end

	--read game mods
	local gamespec = pkgmgr.find_by_gameid(data.gameid)
	pkgmgr.get_game_mods(gamespec, game_mods)

	if #game_mods > 0 then
		-- Add title
		retval[#retval + 1] = {
			type = "game",
			is_game_content = true,
			name = fgettext("$1 mods", gamespec.name),
			path = gamespec.path
		}
	end

	for i=1,#game_mods,1 do
		game_mods[i].type = "mod"
		game_mods[i].loc = "game"
		game_mods[i].is_game_content = true
		retval[#retval + 1] = game_mods[i]
	end

	if data.worldpath == nil then
		return retval
	end

	--read world mod configuration
	local filename = data.worldpath ..
				DIR_DELIM .. "world.mt"

	local worldfile = Settings(filename)

	for key,value in pairs(worldfile:to_table()) do
		if key:sub(1, 9) == "load_mod_" then
			key = key:sub(10)
			local element = nil
			for i=1,#retval,1 do
				if retval[i].name == key and
					not retval[i].is_modpack then
					element = retval[i]
					break
				end
			end
			if element ~= nil then
				element.enabled = value ~= "false" and value ~= "nil" and value
			else
				core.log("info", "Mod: " .. key .. " " .. dump(value) .. " but not found")
			end
		end
	end

	return retval
end

function pkgmgr.compare_package(a, b)
	return a and b and a.name == b.name and a.path == b.path
end

--------------------------------------------------------------------------------
function pkgmgr.comparemod(elem1,elem2)
	if elem1 == nil or elem2 == nil then
		return false
	end
	if elem1.name ~= elem2.name then
		return false
	end
	if elem1.is_modpack ~= elem2.is_modpack then
		return false
	end
	if elem1.type ~= elem2.type then
		return false
	end
	if elem1.modpack ~= elem2.modpack then
		return false
	end

	if elem1.path ~= elem2.path then
		return false
	end

	return true
end

--------------------------------------------------------------------------------
function pkgmgr.mod_exists(basename)

	if pkgmgr.global_mods == nil then
		pkgmgr.refresh_globals()
	end

	if pkgmgr.global_mods:raw_index_by_uid(basename) > 0 then
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function pkgmgr.get_global_mod(idx)

	if pkgmgr.global_mods == nil then
		return nil
	end

	if idx == nil or idx < 1 or
		idx > pkgmgr.global_mods:size() then
		return nil
	end

	return pkgmgr.global_mods:get_list()[idx]
end

--------------------------------------------------------------------------------
function pkgmgr.refresh_globals()
	local function is_equal(element,uid) --uid match
		if element.name == uid then
			return true
		end
	end
	pkgmgr.global_mods = filterlist.create(pkgmgr.preparemodlist,
			pkgmgr.comparemod, is_equal, nil, {})
	pkgmgr.global_mods:add_sort_mechanism("alphabetic", sort_mod_list)
	pkgmgr.global_mods:set_sortmode("alphabetic")
end

--------------------------------------------------------------------------------
function pkgmgr.find_by_gameid(gameid)
	for i=1,#pkgmgr.games,1 do
		if pkgmgr.games[i].id == gameid then
			return pkgmgr.games[i], i
		end
	end
	return nil, nil
end

--------------------------------------------------------------------------------
function pkgmgr.get_game_mods(gamespec, retval)
	if gamespec ~= nil and
		gamespec.gamemods_path ~= nil and
		gamespec.gamemods_path ~= "" then
		get_mods(gamespec.gamemods_path, retval)
	end
end

--------------------------------------------------------------------------------
function pkgmgr.get_game_modlist(gamespec)
	local retval = ""
	local game_mods = {}
	pkgmgr.get_game_mods(gamespec, game_mods)
	for i=1,#game_mods,1 do
		if retval ~= "" then
			retval = retval..","
		end
		retval = retval .. game_mods[i].name
	end
	return retval
end

--------------------------------------------------------------------------------
function pkgmgr.get_game(index)
	if index > 0 and index <= #pkgmgr.games then
		return pkgmgr.games[index]
	end

	return nil
end

--------------------------------------------------------------------------------
function pkgmgr.update_gamelist()
	pkgmgr.games = core.get_games()
end

--------------------------------------------------------------------------------
function pkgmgr.gamelist()
	local retval = ""
	if #pkgmgr.games > 0 then
		retval = retval .. core.formspec_escape(pkgmgr.games[1].name)

		for i=2,#pkgmgr.games,1 do
			retval = retval .. "," .. core.formspec_escape(pkgmgr.games[i].name)
		end
	end
	return retval
end

--------------------------------------------------------------------------------
-- read initial data
--------------------------------------------------------------------------------
pkgmgr.update_gamelist()
