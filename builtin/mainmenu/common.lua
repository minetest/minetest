--Minetest
--Copyright (C) 2014 sapier
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
-- Global menu data
---------------------------------------------------------------------------------
menudata = {}

--------------------------------------------------------------------------------
-- Menu helper functions
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
function render_favorite(spec,render_details)
	local text = ""

	if spec.name ~= nil then
		text = text .. core.formspec_escape(spec.name:trim())

--		if spec.description ~= nil and
--			core.formspec_escape(spec.description):trim() ~= "" then
--			text = text .. " (" .. core.formspec_escape(spec.description) .. ")"
--		end
	else
		if spec.address ~= nil then
			text = text .. spec.address:trim()

			if spec.port ~= nil then
				text = text .. ":" .. spec.port
			end
		end
	end

	if not render_details then
		return text
	end

	local details = ""
	if spec.password == true then
		details = details .. "*"
	else
		details = details .. "_"
	end

	if spec.creative then
		details = details .. "C"
	else
		details = details .. "_"
	end

	if spec.damage then
		details = details .. "D"
	else
		details = details .. "_"
	end

	if spec.pvp then
		details = details .. "P"
	else
		details = details .. "_"
	end
	details = details .. " "

	local playercount = ""

	if spec.clients ~= nil and
		spec.clients_max ~= nil then
		playercount = string.format("%03d",spec.clients) .. "/" ..
						string.format("%03d",spec.clients_max) .. " "
	end

	return playercount .. core.formspec_escape(details) ..  text
end

--------------------------------------------------------------------------------
os.tempfolder = function()
	if core.setting_get("TMPFolder") then
		return core.setting_get("TMPFolder") .. DIR_DELIM .. "MT_" .. math.random(0,10000)
	end

	local filetocheck = os.tmpname()
	os.remove(filetocheck)

	local randname = "MTTempModFolder_" .. math.random(0,10000)
	if DIR_DELIM == "\\" then
		local tempfolder = os.getenv("TEMP")
		return tempfolder .. filetocheck
	else
		local backstring = filetocheck:reverse()
		return filetocheck:sub(0,filetocheck:len()-backstring:find(DIR_DELIM)+1) ..randname
	end

end

--------------------------------------------------------------------------------
function menu_render_worldlist()
	local retval = ""

	local current_worldlist = menudata.worldlist:get_list()

	for i,v in ipairs(current_worldlist) do
		if retval ~= "" then
			retval = retval ..","
		end

		retval = retval .. core.formspec_escape(v.name) ..
					" \\[" .. core.formspec_escape(v.gameid) .. "\\]"
	end

	return retval
end

--------------------------------------------------------------------------------
function menu_handle_key_up_down(fields,textlist,settingname)

	if fields["key_up"] then
		local oldidx = core.get_textlist_index(textlist)

		if oldidx ~= nil and oldidx > 1 then
			local newidx = oldidx -1
			core.setting_set(settingname,
				menudata.worldlist:get_raw_index(newidx))
		end
		return true
	end

	if fields["key_down"] then
		local oldidx = core.get_textlist_index(textlist)

		if oldidx ~= nil and oldidx < menudata.worldlist:size() then
			local newidx = oldidx + 1
			core.setting_set(settingname,
				menudata.worldlist:get_raw_index(newidx))
		end
		
		return true
	end
	
	return false
end

--------------------------------------------------------------------------------
function asyncOnlineFavourites()

	menudata.favorites = {}
	core.handle_async(
		function(param)
			return core.get_favorites("online")
		end,
		nil,
		function(result)
			menudata.favorites = result
			core.event_handler("Refresh")
		end
		)
end

--------------------------------------------------------------------------------
function text2textlist(xpos,ypos,width,height,tl_name,textlen,text,transparency)
	local textlines = core.splittext(text,textlen)
	
	local retval = "textlist[" .. xpos .. "," .. ypos .. ";"
								.. width .. "," .. height .. ";"
								.. tl_name .. ";"
	
	for i=1, #textlines, 1 do
		textlines[i] = textlines[i]:gsub("\r","")
		retval = retval .. core.formspec_escape(textlines[i]) .. ","
	end
	
	retval = retval .. ";0;"
	
	if transparency then
		retval = retval .. "true"
	end
	
	retval = retval .. "]"

	return retval
end
