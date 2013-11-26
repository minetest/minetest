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
function modstore.init()
	modstore.tabnames = {}

	table.insert(modstore.tabnames,"dialog_modstore_unsorted")
	table.insert(modstore.tabnames,"dialog_modstore_search")

	modstore.modsperpage = 5

	modstore.basetexturedir = engine.get_texturepath() .. DIR_DELIM .. "base" ..
						DIR_DELIM .. "pack" .. DIR_DELIM

	modstore.lastmodtitle = ""

	modstore.current_list = nil
end

--------------------------------------------------------------------------------
function modstore.nametoindex(name)

	for i=1,#modstore.tabnames,1 do
		if modstore.tabnames[i] == name then
			return i
		end
	end

	return 1
end

--------------------------------------------------------------------------------
function modstore.gettab(tabname)
	local retval = ""

	local is_modstore_tab = false

	if tabname == "dialog_modstore_unsorted" then
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
		return "size[6,2]label[0.25,0.25;Mod(s): " .. modstore.lastmodtitle ..
				" installed successfully]" ..
				"button[2.5,1.5;1,0.5;btn_confirm_mod_successfull;ok]"
	end

	if tabname == "modstore_downloading" then
		return "size[6,2]label[0.25,0.25;Dowloading " .. modstore.lastmodtitle ..
				" please wait]"
	end

	return ""
end

--------------------------------------------------------------------------------
function modstore.tabheader(tabname)
	local retval  = "size[12,9.25]"
	retval = retval .. "tabheader[-0.3,-0.99;modstore_tab;" ..
				"Unsorted,Search;" ..
				modstore.nametoindex(tabname) .. ";true;false]"

	return retval
end

--------------------------------------------------------------------------------
function modstore.handle_buttons(current_tab,fields)

	if fields["modstore_tab"] then
		local index = tonumber(fields["modstore_tab"])

		if index > 0 and
			index <= #modstore.tabnames then
			return {
					current_tab = modstore.tabnames[index],
					is_dialog = true,
					show_buttons = false
			}
		end

		modstore.modlist_page = 0
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

	if fields["btn_hidden_close_download"] then
		return {
					current_tab = "modstore_mod_installed",
					is_dialog = true,
					show_buttons = false
			}
	end

	if fields["btn_confirm_mod_successfull"] then
		modstore.lastmodtitle = ""
		return {
					current_tab = modstore.tabnames[1],
					is_dialog = true,
					show_buttons = false
			}
	end

	for i=1, modstore.modsperpage, 1 do
		local installbtn = "btn_install_mod_" .. i

		if fields[installbtn] then
			local modlistentry =
				modstore.current_list.page * modstore.modsperpage + i

			if modstore.modlist_unsorted.data[modlistentry] ~= nil and
				modstore.modlist_unsorted.data[modlistentry].details ~= nil then

				local moddetails = modstore.modlist_unsorted.data[modlistentry].details

				if modstore.lastmodtitle ~= "" then
					modstore.lastmodtitle = modstore.lastmodtitle .. ", "
				end

				modstore.lastmodtitle = modstore.lastmodtitle .. moddetails.title

				engine.handle_async(
					function(param)
						local fullurl = engine.setting_get("modstore_download_url") ..
										param.moddetails.download_url

						if engine.download_file(fullurl,param.filename) then
							return {
								moddetails = param.moddetails,
								filename = param.filename,
								successfull = true
							}
						else
							return {
								modtitle = param.title,
								successfull = false
							}
						end
					end,
					{
						moddetails = moddetails,
						filename = os.tempfolder() .. ".zip"
					},
					function(result)
						if result.successfull then
							modmgr.installmod(result.filename,result.moddetails.basename)
							os.remove(result.filename)
						else
							gamedata.errormessage = "Failed to download " .. result.moddetails.title
						end

						engine.button_handler({btn_hidden_close_download=true})
					end
				)

				return {
					current_tab = "modstore_downloading",
					is_dialog = true,
					show_buttons = false,
					ignore_menu_quit = true
				}

			else
				gamedata.errormessage =
					"Internal modstore error please leave modstore and reopen! (Sorry)"
			end
		end
	end
end

--------------------------------------------------------------------------------
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

--------------------------------------------------------------------------------
function modstore.getmodlist(list)
	local retval = ""
	retval = retval .. "label[10,-0.4;" .. fgettext("Page $1 of $2", list.page+1, list.pagecount) .. "]"

	retval = retval .. "button[11.6,-0.1;0.5,0.5;btn_modstore_page_up;^]"
	retval = retval .. "box[11.6,0.35;0.28,8.6;#000000]"
	local scrollbarpos = 0.35 + (8.1/(list.pagecount-1)) * list.page
	retval = retval .. "box[11.6," ..scrollbarpos .. ";0.28,0.5;#32CD32]"
	retval = retval .. "button[11.6,9.0;0.5,0.5;btn_modstore_page_down;v]"


	if #list.data < (list.page * modstore.modsperpage) then
		return retval
	end

	local endmod = (list.page * modstore.modsperpage) + modstore.modsperpage

	if (endmod > #list.data) then
		endmod = #list.data
	end

	for i=(list.page * modstore.modsperpage) +1, endmod, 1 do
		--getmoddetails
		local details = list.data[i].details

--		if details == nil then
--			details = modstore.get_details(list.data[i].id)
--		end

		if details == nil then
			details = {}
			details.title = list.data[i].title
			details.author = ""
			details.rating = -1
			details.description = ""
		end

		if details ~= nil then
			local screenshot_ypos = (i-1 - (list.page * modstore.modsperpage))*1.9 +0.2

			retval = retval .. "box[0," .. screenshot_ypos .. ";11.4,1.75;#FFFFFF]"

			if details.basename then
				--screenshot
				if details.screenshot_url ~= nil and
					details.screenshot_url ~= "" then
					if list.data[i].texturename == nil then
						local fullurl = engine.setting_get("modstore_download_url") ..
									details.screenshot_url
						local filename = os.tempfolder() .. "_MID_" .. list.data[i].id
						list.data[i].texturename = "in progress"
						engine.handle_async(
							function(param)
								param.successfull = engine.download_file(param.fullurl,param.filename)
								return param
							end,
							{
								fullurl = fullurl,
								filename = filename,
								listindex = i,
								modid = list.data[i].id
							},
							function(result)
								if modstore.modlist_unsorted and
									modstore.modlist_unsorted.data and
									#modstore.modlist_unsorted.data >= result.listindex and
									modstore.modlist_unsorted.data[result.listindex].id == result.modid then
									if result.successfull then
										modstore.modlist_unsorted.data[result.listindex].texturename = result.filename
									else
										modstore.modlist_unsorted.data[result.listindex].texturename = modstore.basetexturedir .. "no_screenshot.png"
									end
									engine.event_handler("Refresh")
								end
							end
						)
					end
				else
					if list.data[i].texturename == nil then
						list.data[i].texturename = modstore.basetexturedir .. "no_screenshot.png"
					end
				end

				if list.data[i].texturename ~= nil and
					list.data[i].texturename ~= "in progress" then
					retval = retval .. "image[0,".. screenshot_ypos .. ";3,2;" ..
						engine.formspec_escape(list.data[i].texturename) .. "]"
				end
			end

			--title + author
			retval = retval .."label[2.75," .. screenshot_ypos .. ";" ..
				engine.formspec_escape(details.title) .. " (" .. details.author .. ")]"

			--description
			local descriptiony = screenshot_ypos + 0.5
			retval = retval .. "textarea[3," .. descriptiony .. ";6.5,1.55;;" ..
				engine.formspec_escape(details.description) .. ";]"
			--rating
			local ratingy = screenshot_ypos + 0.6
			retval = retval .."label[9.1," .. ratingy .. ";" ..
							fgettext("Rating") .. ":]"
			retval = retval .. "label[11.1," .. ratingy .. ";" .. details.rating .."]"

			if details.basename then
				--install button
				local buttony = screenshot_ypos + 1.2
				local buttonnumber = (i - (list.page * modstore.modsperpage))
				retval = retval .."button[9.1," .. buttony .. ";2.5,0.5;btn_install_mod_" .. buttonnumber .. ";"

				if modmgr.mod_exists(details.basename) then
					retval = retval .. fgettext("re-Install") .."]"
				else
					retval = retval .. fgettext("Install") .."]"
				end
			end
		end
	end

	modstore.current_list = list

	return retval
end

--------------------------------------------------------------------------------
function modstore.getsearchpage()
	local retval = ""

	--TODO implement search!

	return retval;
end

