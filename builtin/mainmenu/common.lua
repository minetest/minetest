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
--------------------------------------------------------------------------------
menudata = {}

--------------------------------------------------------------------------------
-- Local cached values
--------------------------------------------------------------------------------
local min_supp_proto = core.get_min_supp_proto()
local max_supp_proto = core.get_max_supp_proto()

--------------------------------------------------------------------------------
-- Menu helper functions
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
local function render_client_count(n)
	if n > 99 then
		return '99+'
	elseif n >= 0 then
		return tostring(n)
	else
		return '?'
	end
end

local function configure_selected_world_params(idx)
	local worldconfig = modmgr.get_worldconfig(
		menudata.worldlist:get_list()[idx].path)

	if worldconfig.creative_mode ~= nil then
		core.setting_set("creative_mode", worldconfig.creative_mode)
	end
	if worldconfig.enable_damage ~= nil then
		core.setting_set("enable_damage", worldconfig.enable_damage)
	end
end

--------------------------------------------------------------------------------
function image_column(tooltip, flagname)
	return "image," ..
		"tooltip=" .. core.formspec_escape(tooltip) .. "," ..
		"0=" .. core.formspec_escape(defaulttexturedir .. "blank.png") .. "," ..
		"1=" .. core.formspec_escape(defaulttexturedir .. "server_flags_" .. flagname .. ".png")
end

--------------------------------------------------------------------------------
function order_favorite_list(list)
	local res = {}
	--orders the favorite list after support
	for i=1,#list,1 do
		local fav = list[i]
		if is_server_protocol_compat(fav.proto_min, fav.proto_max) then
			table.insert(res, fav)
		end
	end
	for i=1,#list,1 do
		local fav = list[i]
		if not is_server_protocol_compat(fav.proto_min, fav.proto_max) then
			table.insert(res, fav)
		end
	end
	return res
end

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
	local grey_out = not is_server_protocol_compat(spec.proto_max, spec.proto_min)

	if spec.clients ~= nil and spec.clients_max ~= nil then
		local clients_color = ''
		local clients_percent = 100 * spec.clients / spec.clients_max

		-- Choose a color depending on how many clients are connected
		-- (relatively to clients_max)
		if spec.clients == 0 then
			clients_color = ''        -- 0 players: default/white
		elseif spec.clients == spec.clients_max then
			clients_color = '#dd5b5b' -- full server: red (darker)
		elseif clients_percent <= 60 then
			clients_color = '#a1e587' -- 0-60%: green
		elseif clients_percent <= 90 then
			clients_color = '#ffdc97' -- 60-90%: yellow
		else
			clients_color = '#ffba97' -- 90-100%: orange
		end

		if grey_out then
			clients_color = '#aaaaaa'
		end

		details = details ..
				clients_color .. ',' ..
				render_client_count(spec.clients) .. ',' ..
				'/,' ..
				render_client_count(spec.clients_max) .. ','
	elseif grey_out then
		details = details .. '#aaaaaa,?,/,?,'
	else
		details = details .. ',?,/,?,'
	end

	if spec.creative then
		details = details .. "1,"
	else
		details = details .. "0,"
	end

	if spec.damage then
		details = details .. "1,"
	else
		details = details .. "0,"
	end

	if spec.pvp then
		details = details .. "1,"
	else
		details = details .. "0,"
	end

	return details .. (grey_out and '#aaaaaa,' or ',') .. text
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

			configure_selected_world_params(newidx)
		end
		return true
	end

	if fields["key_down"] then
		local oldidx = core.get_textlist_index(textlist)

		if oldidx ~= nil and oldidx < menudata.worldlist:size() then
			local newidx = oldidx + 1
			core.setting_set(settingname,
				menudata.worldlist:get_raw_index(newidx))

			configure_selected_world_params(newidx)
		end

		return true
	end

	return false
end

--------------------------------------------------------------------------------
function asyncOnlineFavourites()

	if not menudata.public_known then
		menudata.public_known = {{
			name = fgettext("Loading..."),
			description = fgettext_ne("Try reenabling public serverlist and check your internet connection.")
		}}
	end
	menudata.favorites = menudata.public_known
	core.handle_async(
		function(param)
			return core.get_favorites("online")
		end,
		nil,
		function(result)
			if core.setting_getbool("public_serverlist") then
				local favs = order_favorite_list(result)
				if favs[1] then
					menudata.public_known = favs
					menudata.favorites = menudata.public_known
				end
				core.event_handler("Refresh")
			end
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

--------------------------------------------------------------------------------
function is_server_protocol_compat(proto_min, proto_max)
	return not ((min_supp_proto > (proto_max or 24)) or (max_supp_proto < (proto_min or 13)))
end
--------------------------------------------------------------------------------
function is_server_protocol_compat_or_error(proto_min, proto_max)
	if not is_server_protocol_compat(proto_min, proto_max) then
		gamedata.errormessage = fgettext_ne("Protocol version mismatch, server " ..
			((proto_min ~= proto_max) and "supports protocols between $1 and $2" or "enforces protocol version $1") ..
			", we " ..
			((min_supp_proto ~= max_supp_proto) and "support protocols between version $3 and $4." or "only support protocol version $3"),
			proto_min or 13, proto_max or 24, min_supp_proto, max_supp_proto)
		return false
	end

	return true
end
--------------------------------------------------------------------------------
function menu_worldmt(selected, setting, value)
	local world = menudata.worldlist:get_list()[selected]
	if world then
		local filename = world.path .. DIR_DELIM .. "world.mt"
		local world_conf = Settings(filename)

		if value ~= nil then
			if not world_conf:write() then
				core.log("error", "Failed to write world config file")
			end
			world_conf:set(setting, value)
			world_conf:write()
		else
			return world_conf:get(setting)
		end
	else
		return nil
	end
end

function menu_worldmt_legacy(selected)
	local modes_names = {"creative_mode", "enable_damage", "server_announce"}
	for _, mode_name in pairs(modes_names) do
		local mode_val = menu_worldmt(selected, mode_name)
		if mode_val ~= nil then
			core.setting_set(mode_name, mode_val)
		else
			menu_worldmt(selected, mode_name, core.setting_get(mode_name))
		end
	end
end
