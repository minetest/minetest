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
function modstore.init(size, unsortedmods, searchmods)

	modstore.mods_on_unsorted_page = unsortedmods
	modstore.mods_on_search_page = searchmods
	modstore.modsperpage = modstore.mods_on_unsorted_page

	modstore.basetexturedir = core.get_texturepath() .. DIR_DELIM .. "base" ..
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

	modstore.tv_store = tabview_create("modstore",size,{x=0,y=0})

	modstore.tv_store:set_global_event_handler(modstore.handle_events)

	modstore.tv_store:add(
		{
		name = "unsorted",
		caption = fgettext("Unsorted"),
		cbf_formspec       = modstore.unsorted_tab,
		cbf_button_handler = modstore.handle_buttons,
		on_change          =
			function() modstore.modsperpage = modstore.mods_on_unsorted_page end
		}
	)

	modstore.tv_store:add(
		{
		name = "search",
		caption            = fgettext("Search"),
		cbf_formspec       = modstore.getsearchpage,
		cbf_button_handler = modstore.handle_buttons,
		on_change          = modstore.activate_search_tab
		}
	)
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
-- @function [parent=#modstore] showdownloading
function modstore.showdownloading(title)
	local new_dlg = dialog_create("store_downloading",
		function(data)
			return "size[6,2]label[0.25,0.75;" ..
				fgettext("Downloading $1, please wait...", data.title) .. "]"
		end,
		function(this,fields)
			if fields["btn_hidden_close_download"] ~= nil then
				if fields["btn_hidden_close_download"].successfull then
					modstore.lastmodentry = fields["btn_hidden_close_download"]
					modstore.successfulldialog(this)
				else
					this.parent:show()
					this:delete()
					modstore.lastmodtitle = ""
				end

				return true
			end

			return false
		end,
		nil)

	new_dlg:set_parent(modstore.tv_store)
	modstore.tv_store:hide()
	new_dlg.data.title = title
	new_dlg:show()
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] successfulldialog
function modstore.successfulldialog(downloading_dlg)
	local new_dlg = dialog_create("store_downloading",
		function(data)
			local retval = ""
			retval = retval .. "size[6,2,true]"
			if modstore.lastmodentry ~= nil then
				retval = retval .. "label[0,0.25;" .. fgettext("Successfully installed:") .. "]"
				retval = retval .. "label[3,0.25;" .. modstore.lastmodentry.moddetails.title .. "]"
				retval = retval .. "label[0,0.75;" .. fgettext("Shortname:") .. "]"
				retval = retval .. "label[3,0.75;" .. core.formspec_escape(modstore.lastmodentry.moddetails.basename) .. "]"
			end
			retval = retval .. "button[2.2,1.5;1.5,0.5;btn_confirm_mod_successfull;" .. fgettext("Ok") .. "]"
			return retval
		end,
		function(this,fields)
			if fields["btn_confirm_mod_successfull"] ~= nil then
				this.parent:show()
				downloading_dlg:delete()
				this:delete()

				return true
			end

			return false
		end,
		nil)

	new_dlg:set_parent(modstore.tv_store)
	modstore.tv_store:hide()
	new_dlg:show()
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] handle_buttons
function modstore.handle_buttons(parent, fields, name, data)

	if fields["btn_modstore_page_up"] then
		if modstore.current_list ~= nil and modstore.current_list.page > 0 then
			modstore.current_list.page = modstore.current_list.page - 1
		end
		return true
	end

	if fields["btn_modstore_page_down"] then
		if modstore.current_list ~= nil and
			modstore.current_list.page <modstore.current_list.pagecount-1 then
			modstore.current_list.page = modstore.current_list.page +1
		end
		return true
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
		return true
	end

	if fields["btn_modstore_close"] then
		local maintab = ui.find_by_name("maintab")
		parent:hide()
		maintab:show()
		return true
	end

	for key,value in pairs(fields) do
		local foundat = key:find("btn_install_mod_")
		if ( foundat == 1) then
			local modid = tonumber(key:sub(17))
			for i=1,#modstore.modlist_unsorted.data,1 do
				if modstore.modlist_unsorted.data[i].id == modid then
					local moddetails = modstore.modlist_unsorted.data[i].details
					modstore.lastmodtitle = moddetails.title

					if not core.handle_async(
						function(param)
							local fullurl = core.setting_get("modstore_download_url") ..
											param.moddetails.download_url

							if param.version ~= nil then
								local found = false
								for i=1,#param.moddetails.versions, 1 do
									if param.moddetails.versions[i].date:sub(1,10) == param.version then
										fullurl = core.setting_get("modstore_download_url") ..
														param.moddetails.versions[i].download_url
										found = true
									end
								end

								if not found then
									core.log("error","no download url found for version " .. dump(param.version))
									return {
										moddetails = param.moddetails,
										successfull = false
									}
								end
							end

							if core.download_file(fullurl,param.filename) then
								return {
									texturename = param.texturename,
									moddetails = param.moddetails,
									filename = param.filename,
									successfull = true
								}
							else
								core.log("error","downloading " .. dump(fullurl) .. " failed")
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
							--print("Result from async: " .. dump(result.successfull))
							if result.successfull then
								modmgr.installmod(result.filename,result.moddetails.basename)
								os.remove(result.filename)
							else
								gamedata.errormessage = "Failed to download " .. result.moddetails.title
							end

							if gamedata.errormessage == nil then
								core.button_handler({btn_hidden_close_download=result})
							else
								core.button_handler({btn_hidden_close_download={successfull=false}})
							end
						end
					) then
						print("ERROR: async event failed")
						gamedata.errormessage = "Failed to download " .. modstore.lastmodtitle
					end

					modstore.showdownloading(modstore.lastmodtitle)
					return true
				end
			end
			return true
		end
	end

	return false
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] handle_events
function modstore.handle_events(this,event)
	if (event == "MenuQuit") then
		this:hide()
		return true
	end
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] update_modlist
function modstore.update_modlist()
	modstore.modlist_unsorted = {}
	modstore.modlist_unsorted.data = {}
	modstore.modlist_unsorted.pagecount = 1
	modstore.modlist_unsorted.page = 0

	core.handle_async(
		function(param)
			return core.get_modstore_list()
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
				core.event_handler("Refresh")
			end
		end
	)
end

--------------------------------------------------------------------------------
-- @function [parent=#modstore] fetchdetails
function modstore.fetchdetails()

	for i=1,#modstore.modlist_unsorted.data,1 do
		core.handle_async(
		function(param)
			param.details = core.get_modstore_details(tostring(param.modid))
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
				core.event_handler("Refresh")
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
			listentry.texturename = defaulttexturedir .. "no_screenshot.png"
		end

		return "image[0,".. ypos .. ";3,2;" ..
			core.formspec_escape(listentry.texturename) .. "]"
	end

	if listentry.details ~= nil and
		listentry.texturename == nil then
		--make sure we don't download multiple times
		listentry.texturename = "in progress"

		--prepare url and filename
		local fullurl = core.setting_get("modstore_download_url") ..
					listentry.details.screenshot_url
		local filename = os.tempfolder() .. "_MID_" .. listentry.id

		--trigger download
		core.handle_async(
			--first param is downloadfct
			function(param)
				param.successfull = core.download_file(param.fullurl,param.filename)
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
						core.event_handler("Refresh")
					else
						core.log("error","got screenshot but didn't find matching mod: " .. result.modid)
					end
				end
			end
		)
	end

	if listentry.texturename ~= nil and
		listentry.texturename ~= "in progress" then
		return "image[0,".. ypos .. ";3,2;" ..
			core.formspec_escape(listentry.texturename) .. "]"
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
		core.formspec_escape(details.title) .. " (" .. details.author .. ")]"

	--description
	local descriptiony = ypos + 0.5
	retval = retval .. "textarea[3," .. descriptiony .. ";6.5,1.55;;" ..
		core.formspec_escape(details.description) .. ";]"

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

	if yoffset == nil then
		yoffset = 0
	end

	local sb_y_start = 0.2 + yoffset
	local sb_y_end   = (modstore.modsperpage * 1.75) + ((modstore.modsperpage-1) * 0.15)
	local close_button = "button[4," .. (sb_y_end + 0.3 + yoffset) ..
			";4,0.5;btn_modstore_close;" .. fgettext("Close store") .. "]"

	if #list.data == 0 then
		return close_button
	end

	local scrollbar = ""
	scrollbar = scrollbar .. "label[0.1,".. (sb_y_end + 0.25 + yoffset) ..";"
				.. fgettext("Page $1 of $2", list.page+1, list.pagecount) .. "]"
	scrollbar = scrollbar .. "box[11.6," .. sb_y_start .. ";0.28," .. sb_y_end .. ";#000000]"
	local scrollbarpos = (sb_y_start + 0.5) +
				((sb_y_end -1.6)/(list.pagecount-1)) * list.page
	scrollbar = scrollbar .. "box[11.6," ..scrollbarpos .. ";0.28,0.5;#32CD32]"
	scrollbar = scrollbar .. "button[11.6," .. (sb_y_start)
				.. ";0.5,0.5;btn_modstore_page_up;^]"
	scrollbar = scrollbar .. "button[11.6," .. (sb_y_start + sb_y_end - 0.5)
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

	return retval .. scrollbar .. close_button
end

--------------------------------------------------------------------------------
--@function [parent=#modstore] getsearchpage
function modstore.getsearchpage(tabview, name, tabdata)
	local retval = ""
	local search = ""

	if modstore.last_search ~= nil then
		search = modstore.last_search
	end

	retval = retval ..
		"button[9.5,0.2;2.5,0.5;btn_modstore_search;".. fgettext("Search") .. "]" ..
		"field[0.5,0.5;9,0.5;te_modstore_search;;" .. search .. "]"

	retval = retval ..
		modstore.getmodlist(
			modstore.currentlist,
			1.75)

	return retval;
end

--------------------------------------------------------------------------------
--@function [parent=#modstore] unsorted_tab
function modstore.unsorted_tab()
	return modstore.getmodlist(modstore.modlist_unsorted)
end

--------------------------------------------------------------------------------
--@function [parent=#modstore] activate_search_tab
function modstore.activate_search_tab(type, old_tab, new_tab)

	if old_tab == new_tab then
		return
	end
	filterlist.set_filtercriteria(modstore.searchlist,modstore.last_search)
	filterlist.refresh(modstore.searchlist)
	modstore.modsperpage = modstore.mods_on_search_page
	modstore.currentlist = {
		page = 0,
		pagecount =
			math.ceil(filterlist.size(modstore.searchlist) / modstore.modsperpage),
		data = filterlist.get_list(modstore.searchlist),
	}
end

