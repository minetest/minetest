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

--modstore implementation
modstore = {}

--------------------------------------------------------------------------------
-- @function [parent=#modstore] init
function modstore.init()
	modstore.tabnames = {}

	table.insert(modstore.tabnames,"dialog_modstore_unsorted")
	table.insert(modstore.tabnames,"dialog_modstore_search")

	modstore.modsperpage = 5

	modstore.basetexturedir = engine.get_texturepath() .. DIR_DELIM .. "base" ..
						DIR_DELIM .. "pack" .. DIR_DELIM

	modstore.lastmodtitle = ""
	modstore.last_search = ""
	
	modstore.searchlist = filterlist.create(
		function()
			if modstore.modlist_unsorted ~= nil and
				modstore.modlist_unsorted.data ~= nil then
				return modstore.modlist_unsorted.data
			end
			return {}
		end,
		function(element,modid)
			if element.id == modid then
				return true
			end
			return false
		end, --compare fct
		nil, --uid match fct
		function(element,substring)
			if substring == nil or
				substring == "" then
				return false
			end
			substring = substring:upper()
			
			if element.title ~= nil and
				element.title:upper():find(substring) ~= nil then
				return true
			end
			
			if element.details ~= nil and
				element.details.author ~= nil and
				element.details.author:upper():find(substring) ~= nil then
				return true
			end
			
			if element.details ~= nil and
				element.details.description ~= nil and
				element.details.description:upper():find(substring) ~= nil then
				return true
			end
			return false
		end --filter fct
		)

	modstore.current_list = nil
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] nametoindex
function modstore.nametoindex(name)

	for i=1,#modstore.tabnames,1 do
		if modstore.tabnames[i] == name then
			return i
		end
	end

	return 1
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] getsuccessfuldialog
function modstore.getsuccessfuldialog()
	local retval = ""
	retval = retval .. "size[6,2,true]"
	if modstore.lastmodentry ~= nil then
		retval = retval .. "label[0,0.25;" .. fgettext("Successfully installed:") .. "]"
		retval = retval .. "label[3,0.25;" .. modstore.lastmodentry.moddetails.title .. "]"
	
		
		retval = retval .. "label[0,0.75;" .. fgettext("Shortname:") .. "]"
		retval = retval .. "label[3,0.75;" .. engine.formspec_escape(modstore.lastmodentry.moddetails.basename) .. "]"

	end
	retval = retval .. "button[2.5,1.5;1,0.5;btn_confirm_mod_successfull;" .. fgettext("ok") .. "]"
				
				
	return retval
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] gettab
function modstore.gettab(tabname)
	local retval = ""

	local is_modstore_tab = false

	if tabname == "dialog_modstore_unsorted" then
		modstore.modsperpage = 5
		retval = modstore.getmodlist(modstore.modlist_unsorted)
		is_modstore_tab = true
	end

	if tabname == "dialog_modstore_search" then
		retval = modstore.getsearchpage()
		is_modstore_tab = true
	end

	if is_modstore_tab then
		return modstore.tabheader(tabname) .. retval
	end

	if tabname == "modstore_mod_installed" then
		return modstore.getsuccessfuldialog()
	end

	if tabname == "modstore_downloading" then
		return "size[6,2]label[0.25,0.75;" .. fgettext("Downloading") ..
				" " .. modstore.lastmodtitle .. " " ..
				fgettext("please wait...") .. "]"
	end

	return ""
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] tabheader
function modstore.tabheader(tabname)
	local retval  = "size[12,10.25,true]"
	retval = retval .. "tabheader[-0.3,-0.99;modstore_tab;" ..
				"Unsorted,Search;" ..
				modstore.nametoindex(tabname) .. ";true;false]" ..
				"button[4,9.9;4,0.5;btn_modstore_close;" ..
				fgettext("Close modstore") .. "]"

	return retval
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] handle_buttons
function modstore.handle_buttons(current_tab,fields)

	if fields["modstore_tab"] then
		local index = tonumber(fields["modstore_tab"])

		if index > 0 and
			index <= #modstore.tabnames then
			if modstore.tabnames[index] == "dialog_modstore_search" then
				filterlist.set_filtercriteria(modstore.searchlist,modstore.last_search)
				filterlist.refresh(modstore.searchlist)
				modstore.modsperpage = 4
				modstore.currentlist = {
					page = 0,
					pagecount =
						math.ceil(filterlist.size(modstore.searchlist) / modstore.modsperpage),
					data = filterlist.get_list(modstore.searchlist),
				}
			end
			
			return {
					current_tab = modstore.tabnames[index],
					is_dialog = true,
					show_buttons = false
			}
		end
		
	end

	if fields["btn_modstore_page_up"] then
		if modstore.current_list ~= nil and modstore.current_list.page > 0 then
			modstore.current_list.page = modstore.current_list.page - 1
		end
	end

	if fields["btn_modstore_page_down"] then
		if modstore.current_list ~= nil and
			modstore.current_list.page <modstore.current_list.pagecount-1 then
			modstore.current_list.page = modstore.current_list.page +1
		end
	end

	if fields["btn_hidden_close_download"] ~= nil then
		if fields["btn_hidden_close_download"].successfull then
			modstore.lastmodentry = fields["btn_hidden_close_download"]
			return {
					current_tab = "modstore_mod_installed",
					is_dialog = true,
					show_buttons = false
			}
		else
			modstore.lastmodtitle = ""
			return {
						current_tab = modstore.tabnames[1],
						is_dialog = true,
						show_buttons = false
				}
		end
	end

	if fields["btn_confirm_mod_successfull"] then
		modstore.lastmodentry = nil
		modstore.lastmodtitle = ""
		return {
					current_tab = modstore.tabnames[1],
					is_dialog = true,
					show_buttons = false
			}
	end

	if fields["btn_modstore_search"] or
		(fields["key_enter"] and fields["te_modstore_search"] ~= nil) then
		modstore.last_search = fields["te_modstore_search"]
		filterlist.set_filtercriteria(modstore.searchlist,fields["te_modstore_search"])
		filterlist.refresh(modstore.searchlist)
		modstore.currentlist = {
			page = 0,
			pagecount =  math.ceil(filterlist.size(modstore.searchlist) / modstore.modsperpage),
			data = filterlist.get_list(modstore.searchlist),
		}
	end
	
	
	if fields["btn_modstore_close"] then
		return {
			is_dialog = false,
			show_buttons = true,
			current_tab = engine.setting_get("main_menu_tab")
		}
	end
	
	for key,value in pairs(fields) do
		local foundat = key:find("btn_install_mod_")
		if ( foundat == 1) then
			local modid = tonumber(key:sub(17))
			for i=1,#modstore.modlist_unsorted.data,1 do
				if modstore.modlist_unsorted.data[i].id == modid then
					local moddetails = modstore.modlist_unsorted.data[i].details

					if modstore.lastmodtitle ~= "" then
						modstore.lastmodtitle = modstore.lastmodtitle .. ", "
					end
	
					modstore.lastmodtitle = modstore.lastmodtitle .. moddetails.title
	
					engine.handle_async(
						function(param)
						
							local fullurl = engine.setting_get("modstore_download_url") ..
											param.moddetails.download_url
											
							if param.version ~= nil then
								local found = false
								for i=1,#param.moddetails.versions, 1 do
									if param.moddetails.versions[i].date:sub(1,10) == param.version then
										fullurl = engine.setting_get("modstore_download_url") ..
														param.moddetails.versions[i].download_url
										found = true
									end
								end
								
								if not found then
									return {
										moddetails = param.moddetails,
										successfull = false
									}
								end
							end
	
							if engine.download_file(fullurl,param.filename) then
								return {
									texturename = param.texturename,
									moddetails = param.moddetails,
									filename = param.filename,
									successfull = true
								}
							else
								return {
									moddetails = param.moddetails,
									successfull = false
								}
							end
						end,
						{
							moddetails = moddetails,
							version = fields["dd_version" .. modid],
							filename = os.tempfolder() .. "_MODNAME_" .. moddetails.basename .. ".zip",
							texturename = modstore.modlist_unsorted.data[i].texturename
						},
						function(result)
							if result.successfull then
								modmgr.installmod(result.filename,result.moddetails.basename)
								os.remove(result.filename)
							else
								gamedata.errormessage = "Failed to download " .. result.moddetails.title
							end
	
							if gamedata.errormessage == nil then
								engine.button_handler({btn_hidden_close_download=result})
							else
								engine.button_handler({btn_hidden_close_download={successfull=false}})
							end
						end
					)
	
					return {
						current_tab = "modstore_downloading",
						is_dialog = true,
						show_buttons = false,
						ignore_menu_quit = true
					}
				end
			end
			break
		end
	end
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] update_modlist
function modstore.update_modlist()
	modstore.modlist_unsorted = {}
	modstore.modlist_unsorted.data = {}
	modstore.modlist_unsorted.pagecount = 1
	modstore.modlist_unsorted.page = 0

	engine.handle_async(
		function(param)
			return engine.get_modstore_list()
		end,
		nil,
		function(result)
			if result ~= nil then
				modstore.modlist_unsorted = {}
				modstore.modlist_unsorted.data = result

				if modstore.modlist_unsorted.data ~= nil then
					modstore.modlist_unsorted.pagecount =
						math.ceil((#modstore.modlist_unsorted.data / modstore.modsperpage))
				else
					modstore.modlist_unsorted.data = {}
					modstore.modlist_unsorted.pagecount = 1
				end
				modstore.modlist_unsorted.page = 0
				modstore.fetchdetails()
				engine.event_handler("Refresh")
			end
		end
	)
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] fetchdetails
function modstore.fetchdetails()

	for i=1,#modstore.modlist_unsorted.data,1 do
		engine.handle_async(
		function(param)
			param.details = engine.get_modstore_details(tostring(param.modid))
			return param
		end,
		{
			modid=modstore.modlist_unsorted.data[i].id,
			listindex=i
		},
		function(result)
			if result ~= nil and
				modstore.modlist_unsorted ~= nil
				and modstore.modlist_unsorted.data ~= nil and
				modstore.modlist_unsorted.data[result.listindex] ~= nil and
				modstore.modlist_unsorted.data[result.listindex].id ~= nil then

				modstore.modlist_unsorted.data[result.listindex].details = result.details
				engine.event_handler("Refresh")
			end
		end
		)
	end
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] getscreenshot
function modstore.getscreenshot(ypos,listentry)

	if	listentry.details ~= nil and
		(listentry.details.screenshot_url == nil or
		listentry.details.screenshot_url == "") then
		
		if listentry.texturename == nil then
			listentry.texturename = modstore.basetexturedir .. "no_screenshot.png"
		end
		
		return "image[0,".. ypos .. ";3,2;" ..
			engine.formspec_escape(listentry.texturename) .. "]"
	end
	
	if listentry.details ~= nil and
		listentry.texturename == nil then
		--make sure we don't download multiple times
		listentry.texturename = "in progress"
	
		--prepare url and filename
		local fullurl = engine.setting_get("modstore_download_url") ..
					listentry.details.screenshot_url
		local filename = os.tempfolder() .. "_MID_" .. listentry.id
		
		--trigger download
		engine.handle_async(
			--first param is downloadfct
			function(param)
				param.successfull = engine.download_file(param.fullurl,param.filename)
				return param
			end,
			--second parameter is data passed to async job
			{
				fullurl = fullurl,
				filename = filename,
				modid = listentry.id
			},
			--integrate result to raw list
			function(result)
				if result.successfull then
					local found = false
					for i=1,#modstore.modlist_unsorted.data,1 do
						if modstore.modlist_unsorted.data[i].id == result.modid then
							found = true
							modstore.modlist_unsorted.data[i].texturename = result.filename
							break
						end
					end
					if found then
						engine.event_handler("Refresh")
					else
						engine.log("error","got screenshot but didn't find matching mod: " .. result.modid)
					end
				end
			end
		)
	end

	if listentry.texturename ~= nil and
		listentry.texturename ~= "in progress" then
		return "image[0,".. ypos .. ";3,2;" ..
			engine.formspec_escape(listentry.texturename) .. "]"
	end
	
	return ""
end

--------------------------------------------------------------------------------
--@function [parent=#modstore] getshortmodinfo
function modstore.getshortmodinfo(ypos,listentry,details)
	local retval = ""
	
	retval = retval .. "box[0," .. ypos .. ";11.4,1.75;#FFFFFF]"

	--screenshot
	retval = retval .. modstore.getscreenshot(ypos,listentry)

	--title + author
	retval = retval .."label[2.75," .. ypos .. ";" ..
		engine.formspec_escape(details.title) .. " (" .. details.author .. ")]"

	--description
	local descriptiony = ypos + 0.5
	retval = retval .. "textarea[3," .. descriptiony .. ";6.5,1.55;;" ..
		engine.formspec_escape(details.description) .. ";]"
		
	--rating
	local ratingy = ypos
	retval = retval .."label[7," .. ratingy .. ";" ..
					fgettext("Rating") .. ":]"
	retval = retval .. "label[8.7," .. ratingy .. ";" .. details.rating .."]"
	
	--versions (IMPORTANT has to be defined AFTER rating)
	if details.versions ~= nil and
		#details.versions > 1 then
		local versiony = ypos + 0.05
		retval = retval .. "dropdown[9.1," .. versiony .. ";2.48,0.25;dd_version" .. details.id .. ";"
		local versions = ""
		for i=1,#details.versions , 1 do
			if versions ~= "" then
				versions = versions .. ","
			end
			
			versions = versions .. details.versions[i].date:sub(1,10)
		end
		retval = retval .. versions .. ";1]"
	end

	if details.basename then
		--install button
		local buttony = ypos + 1.2
		retval = retval .."button[9.1," .. buttony .. ";2.5,0.5;btn_install_mod_" .. details.id .. ";"

		if modmgr.mod_exists(details.basename) then
			retval = retval .. fgettext("re-Install") .."]"
		else
			retval = retval .. fgettext("Install") .."]"
		end
	end
	
	return retval
end

--------------------------------------------------------------------------------
--@function [parent=#modstore] getmodlist
function modstore.getmodlist(list,yoffset)

	modstore.current_list = list
	
	if #list.data == 0 then
		return ""
	end
	
	if yoffset == nil then
		yoffset = 0
	end

	local scrollbar = ""
	scrollbar = scrollbar .. "label[0.1,9.5;"
				.. fgettext("Page $1 of $2", list.page+1, list.pagecount) .. "]"
	scrollbar = scrollbar .. "box[11.6," .. (yoffset + 0.35) .. ";0.28,"
				.. (8.6 - yoffset) .. ";#000000]"
	local scrollbarpos = (yoffset + 0.75) +
				((7.7 -yoffset)/(list.pagecount-1)) * list.page
	scrollbar = scrollbar .. "box[11.6," ..scrollbarpos .. ";0.28,0.5;#32CD32]"
	scrollbar = scrollbar .. "button[11.6," .. (yoffset + (0.3))
				.. ";0.5,0.5;btn_modstore_page_up;^]"
	scrollbar = scrollbar .. "button[11.6," .. 9.0
				.. ";0.5,0.5;btn_modstore_page_down;v]"

	local retval = ""

	local endmod = (list.page * modstore.modsperpage) + modstore.modsperpage

	if (endmod > #list.data) then
		endmod = #list.data
	end

	for i=(list.page * modstore.modsperpage) +1, endmod, 1 do
		--getmoddetails
		local details = list.data[i].details

		if details == nil then
			details = {}
			details.title = list.data[i].title
			details.author = ""
			details.rating = -1
			details.description = ""
		end

		if details ~= nil then
			local screenshot_ypos =
				yoffset +(i-1 - (list.page * modstore.modsperpage))*1.9 +0.2
				
			retval = retval .. modstore.getshortmodinfo(screenshot_ypos,
														list.data[i],
														details)
		end
	end
	
	return retval .. scrollbar
end

--------------------------------------------------------------------------------
--@function [parent=#modstore] getsearchpage
function modstore.getsearchpage()
	local retval = ""
	local search = ""
	
	if modstore.last_search ~= nil then
		search = modstore.last_search
	end

	retval = retval ..
		"button[9.5,0.2;2.5,0.5;btn_modstore_search;".. fgettext("Search") .. "]" ..
		"field[0.5,0.5;9,0.5;te_modstore_search;;" .. search .. "]"

	
	--show 4 mods only
	modstore.modsperpage = 4
	retval = retval ..
		modstore.getmodlist(
			modstore.currentlist,
			1.75)

	return retval;
end

