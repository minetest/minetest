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
function get_mods(path,retval,modpack)
	local mods = core.get_dir_list(path, true)

	for _, name in ipairs(mods) do
		if name:sub(1, 1) ~= "." then
			local prefix = path .. DIR_DELIM .. name .. DIR_DELIM
			local toadd = {}
			retval[#retval + 1] = toadd

			local mod_conf = Settings(prefix .. "mod.conf"):to_table()
			if mod_conf.name then
				name = mod_conf.name
			end

			toadd.name = name
			toadd.path = prefix

			if modpack ~= nil and modpack ~= "" then
				toadd.modpack = modpack
			else
				local modpackfile = io.open(prefix .. "modpack.txt")
				if modpackfile then
					modpackfile:close()
					toadd.is_modpack = true
					get_mods(prefix, retval, name)
				end
			end
		end
	end
end

--modmanager implementation
modmgr = {}

--------------------------------------------------------------------------------
function modmgr.extract(modfile)
	if modfile.type == "zip" then
		local tempfolder = os.tempfolder()

		if tempfolder ~= nil and
			tempfolder ~= "" then
			core.create_dir(tempfolder)
			if core.extract_zip(modfile.name,tempfolder) then
				return tempfolder
			end
		end
	end
	return nil
end

-------------------------------------------------------------------------------
function modmgr.getbasefolder(temppath)

	if temppath == nil then
		return {
		type = "invalid",
		path = ""
		}
	end

	local testfile = io.open(temppath .. DIR_DELIM .. "init.lua","r")
	if testfile ~= nil then
		testfile:close()
		return {
				type="mod",
				path=temppath
				}
	end

	testfile = io.open(temppath .. DIR_DELIM .. "modpack.txt","r")
	if testfile ~= nil then
		testfile:close()
		return {
				type="modpack",
				path=temppath
				}
	end

	local subdirs = core.get_dir_list(temppath, true)

	--only single mod or modpack allowed
	if #subdirs ~= 1 then
		return {
			type = "invalid",
			path = ""
			}
	end

	testfile =
	io.open(temppath .. DIR_DELIM .. subdirs[1] ..DIR_DELIM .."init.lua","r")
	if testfile ~= nil then
		testfile:close()
		return {
			type="mod",
			path= temppath .. DIR_DELIM .. subdirs[1]
			}
	end

	testfile =
	io.open(temppath .. DIR_DELIM .. subdirs[1] ..DIR_DELIM .."modpack.txt","r")
	if testfile ~= nil then
		testfile:close()
		return {
			type="modpack",
			path=temppath ..  DIR_DELIM .. subdirs[1]
			}
	end

	return {
		type = "invalid",
		path = ""
		}
end

--------------------------------------------------------------------------------
function modmgr.isValidModname(modpath)
	if modpath:find("-") ~= nil then
		return false
	end

	return true
end

--------------------------------------------------------------------------------
function modmgr.parse_register_line(line)
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
function modmgr.parse_dofile_line(modpath,line)
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
			return modmgr.identify_modname(modpath,filename)
		end
	end
	return nil
end

--------------------------------------------------------------------------------
function modmgr.identify_modname(modpath,filename)
	local testfile = io.open(modpath .. DIR_DELIM .. filename,"r")
	if testfile ~= nil then
		local line = testfile:read()

		while line~= nil do
			local modname = nil

			if line:find("minetest.register_tool") then
				modname = modmgr.parse_register_line(line)
			end

			if line:find("minetest.register_craftitem") then
				modname = modmgr.parse_register_line(line)
			end


			if line:find("minetest.register_node") then
				modname = modmgr.parse_register_line(line)
			end

			if line:find("dofile") then
				modname = modmgr.parse_dofile_line(modpath,line)
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
function modmgr.render_modlist(render_list)
	local retval = ""

	if render_list == nil then
		if modmgr.global_mods == nil then
			modmgr.refresh_globals()
		end
		render_list = modmgr.global_mods
	end

	local list = render_list:get_list()
	local last_modpack = nil
	local retval = {}
	for i, v in ipairs(list) do
		local color = ""
		if v.is_modpack then
			local rawlist = render_list:get_raw_list()
			color = mt_color_dark_green

			for j = 1, #rawlist, 1 do
				if rawlist[j].modpack == list[i].name and
						rawlist[j].enabled ~= true then
					-- Modpack not entirely enabled so showing as grey
					color = mt_color_grey
					break
				end
			end
		elseif v.is_game_content then
			color = mt_color_blue
		elseif v.enabled then
			color = mt_color_green
		end

		retval[#retval + 1] = color
		if v.modpack ~= nil or v.typ == "game_mod" then
			retval[#retval + 1] = "1"
		else
			retval[#retval + 1] = "0"
		end
		retval[#retval + 1] = core.formspec_escape(v.name)
	end

	return table.concat(retval, ",")
end

--------------------------------------------------------------------------------
function modmgr.get_dependencies(modfolder)
	local toadd_hard = ""
	local toadd_soft = ""
	if modfolder ~= nil then
		local filename = modfolder ..
					DIR_DELIM .. "depends.txt"

		local hard_dependencies = {}
		local soft_dependencies = {}
		local dependencyfile = io.open(filename,"r")
		if dependencyfile then
			local dependency = dependencyfile:read("*l")
			while dependency do
				dependency = dependency:gsub("\r", "")
				if string.sub(dependency, -1, -1) == "?" then
					table.insert(soft_dependencies, string.sub(dependency, 1, -2))
				else
					table.insert(hard_dependencies, dependency)
				end
				dependency = dependencyfile:read()
			end
			dependencyfile:close()
		end
		toadd_hard = table.concat(hard_dependencies, ",")
		toadd_soft = table.concat(soft_dependencies, ",")
	end

	return toadd_hard, toadd_soft
end

--------------------------------------------------------------------------------
function modmgr.get_worldconfig(worldpath)
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
			worldconfig.global_mods[key] = core.is_yes(value)
		else
			worldconfig[key] = value
		end
	end

	--read gamemods
	local gamespec = gamemgr.find_by_gameid(worldconfig.id)
	gamemgr.get_game_mods(gamespec, worldconfig.game_mods)

	return worldconfig
end

--------------------------------------------------------------------------------
function modmgr.installmod(modfilename,basename)
	local modfile = modmgr.identify_filetype(modfilename)
	local modpath = modmgr.extract(modfile)

	if modpath == nil then
		gamedata.errormessage = fgettext("Install Mod: file: \"$1\"", modfile.name) ..
			fgettext("\nInstall Mod: unsupported filetype \"$1\" or broken archive", modfile.type)
		return
	end

	local basefolder = modmgr.getbasefolder(modpath)

	if basefolder.type == "modpack" then
		local clean_path = nil

		if basename ~= nil then
			clean_path = "mp_" .. basename
		end

		if clean_path == nil then
			clean_path = get_last_folder(cleanup_path(basefolder.path))
		end

		if clean_path ~= nil then
			local targetpath = core.get_modpath() .. DIR_DELIM .. clean_path
			if not core.copy_dir(basefolder.path,targetpath) then
				gamedata.errormessage = fgettext("Failed to install $1 to $2", basename, targetpath)
			end
		else
			gamedata.errormessage = fgettext("Install Mod: unable to find suitable foldername for modpack $1", modfilename)
		end
	end

	if basefolder.type == "mod" then
		local targetfolder = basename

		if targetfolder == nil then
			targetfolder = modmgr.identify_modname(basefolder.path,"init.lua")
		end

		--if heuristic failed try to use current foldername
		if targetfolder == nil then
			targetfolder = get_last_folder(basefolder.path)
		end

		if targetfolder ~= nil and modmgr.isValidModname(targetfolder) then
			local targetpath = core.get_modpath() .. DIR_DELIM .. targetfolder
			core.copy_dir(basefolder.path,targetpath)
		else
			gamedata.errormessage = fgettext("Install Mod: unable to find real modname for: $1", modfilename)
		end
	end

	core.delete_dir(modpath)

	modmgr.refresh_globals()

end

--------------------------------------------------------------------------------
function modmgr.preparemodlist(data)
	local retval = {}

	local global_mods = {}
	local game_mods = {}

	--read global mods
	local modpath = core.get_modpath()

	if modpath ~= nil and
		modpath ~= "" then
		get_mods(modpath,global_mods)
	end

	for i=1,#global_mods,1 do
		global_mods[i].typ = "global_mod"
		retval[#retval + 1] = global_mods[i]
	end

	--read game mods
	local gamespec = gamemgr.find_by_gameid(data.gameid)
	gamemgr.get_game_mods(gamespec, game_mods)

	if #game_mods > 0 then
		-- Add title
		retval[#retval + 1] = {
			typ = "game",
			is_game_content = true,
			name = fgettext("Subgame Mods")
		}
	end

	for i=1,#game_mods,1 do
		game_mods[i].typ = "game_mod"
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
				element.enabled = core.is_yes(value)
			else
				core.log("info", "Mod: " .. key .. " " .. dump(value) .. " but not found")
			end
		end
	end

	return retval
end

--------------------------------------------------------------------------------
function modmgr.comparemod(elem1,elem2)
	if elem1 == nil or elem2 == nil then
		return false
	end
	if elem1.name ~= elem2.name then
		return false
	end
	if elem1.is_modpack ~= elem2.is_modpack then
		return false
	end
	if elem1.typ ~= elem2.typ then
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
function modmgr.mod_exists(basename)

	if modmgr.global_mods == nil then
		modmgr.refresh_globals()
	end

	if modmgr.global_mods:raw_index_by_uid(basename) > 0 then
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function modmgr.get_global_mod(idx)

	if modmgr.global_mods == nil then
		return nil
	end

	if idx == nil or idx < 1 or
		idx > modmgr.global_mods:size() then
		return nil
	end

	return modmgr.global_mods:get_list()[idx]
end

--------------------------------------------------------------------------------
function modmgr.refresh_globals()
	modmgr.global_mods = filterlist.create(
					modmgr.preparemodlist, --refresh
					modmgr.comparemod, --compare
					function(element,uid) --uid match
						if element.name == uid then
							return true
						end
					end,
					nil, --filter
					{}
					)
	modmgr.global_mods:add_sort_mechanism("alphabetic", sort_mod_list)
	modmgr.global_mods:set_sortmode("alphabetic")
end

--------------------------------------------------------------------------------
function modmgr.identify_filetype(name)

	if name:sub(-3):lower() == "zip" then
		return {
				name = name,
				type = "zip"
				}
	end

	if name:sub(-6):lower() == "tar.gz" or
		name:sub(-3):lower() == "tgz"then
		return {
				name = name,
				type = "tgz"
				}
	end

	if name:sub(-6):lower() == "tar.bz2" then
		return {
				name = name,
				type = "tbz"
				}
	end

	if name:sub(-2):lower() == "7z" then
		return {
				name = name,
				type = "7z"
				}
	end

	return {
		name = name,
		type = "ukn"
	}
end
