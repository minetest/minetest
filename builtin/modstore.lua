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
	
	modstore.current_list = nil
	
	modstore.details_cache = {}
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
	
	
		is_modstore_tab = true
	end
	
	if is_modstore_tab then
		return modstore.tabheader(tabname) .. retval
	end
	
	if tabname == "modstore_mod_installed" then
		return "size[6,2]label[0.25,0.25;Mod: " .. modstore.lastmodtitle .. 
				" installed successfully]" ..
				"button[2.5,1.5;1,0.5;btn_confirm_mod_successfull;ok]"
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

	modstore.lastmodtitle = ""
	
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
	
	if fields["btn_confirm_mod_successfull"] then
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
			
			local moddetails = modstore.get_details(modstore.current_list.data[modlistentry].id)
			
			local fullurl = engine.setting_get("modstore_download_url") ..
								moddetails.download_url
			local modfilename = os.tempfolder() .. ".zip"
			
			if engine.download_file(fullurl,modfilename) then
			
				modmgr.installmod(modfilename,moddetails.basename)
				
				os.remove(modfilename)
				modstore.lastmodtitle = modstore.current_list.data[modlistentry].title
				
				return {
					current_tab = "modstore_mod_installed",
					is_dialog = true,
					show_buttons = false
				}
			else
				gamedata.errormessage = "Unable to download " .. 
					moddetails.download_url .. " (internet connection?)"
			end
		end
	end
end

--------------------------------------------------------------------------------
function modstore.update_modlist()
	modstore.modlist_unsorted = {}
	modstore.modlist_unsorted.data = engine.get_modstore_list()
		
	if modstore.modlist_unsorted.data ~= nil then
		modstore.modlist_unsorted.pagecount = 
			math.ceil((#modstore.modlist_unsorted.data / modstore.modsperpage))
	else
		modstore.modlist_unsorted.data = {}
		modstore.modlist_unsorted.pagecount = 1
	end
	modstore.modlist_unsorted.page = 0
end

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
		local details = modstore.get_details(list.data[i].id)
	
		if details ~= nil then
			local screenshot_ypos = (i-1 - (list.page * modstore.modsperpage))*1.9 +0.2
			
			retval = retval .. "box[0," .. screenshot_ypos .. ";11.4,1.75;#FFFFFF]"
			
			--screenshot
			if details.screenshot_url ~= nil and
				details.screenshot_url ~= "" then
				if list.data[i].texturename == nil then
					local fullurl = engine.setting_get("modstore_download_url") ..
								details.screenshot_url
					local filename = os.tempfolder()
					
					if engine.download_file(fullurl,filename) then
						list.data[i].texturename = filename
					end
				end
			end
			
			if list.data[i].texturename == nil then
				list.data[i].texturename = modstore.basetexturedir .. "no_screenshot.png"
			end
			
			retval = retval .. "image[0,".. screenshot_ypos .. ";3,2;" .. 
					engine.formspec_escape(list.data[i].texturename) .. "]"
			
			--title + author
			retval = retval .."label[2.75," .. screenshot_ypos .. ";" .. 
				engine.formspec_escape(details.title) .. " (" .. details.author .. ")]"
			
			--description
			local descriptiony = screenshot_ypos + 0.5
			retval = retval .. "textarea[3," .. descriptiony .. ";6.5,1.55;;" .. 
				engine.formspec_escape(details.description) .. ";]"
			--rating
			local ratingy = screenshot_ypos + 0.6
			retval = retval .."label[10.1," .. ratingy .. ";" .. 
							fgettext("Rating") .. ": " .. details.rating .."]"
			
			--install button
			local buttony = screenshot_ypos + 1.2
			local buttonnumber = (i - (list.page * modstore.modsperpage))
			retval = retval .."button[9.6," .. buttony .. ";2,0.5;btn_install_mod_" .. buttonnumber .. ";"
			
			if modmgr.mod_exists(details.basename) then
				retval = retval .. fgettext("re-Install") .."]"
			else
				retval = retval .. fgettext("Install") .."]"
			end
		end
	end
	
	modstore.current_list = list
	
	return retval
end

--------------------------------------------------------------------------------
function modstore.get_details(modid) 

	if modstore.details_cache[modid] ~= nil then
		return modstore.details_cache[modid]
	end
	
	local retval = engine.get_modstore_details(tostring(modid))
	modstore.details_cache[modid] = retval
	return retval
end

