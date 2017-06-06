--Minetest
--Copyright (C) 2015 srifqi
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

local mg_first_time = true
-- always load default
local flag_list = {
	-- v5 mapgen
	blobs		= true,
	-- v6 mapgen
	biomeblend	= true,
	jungles		= true,
	mudflow		= true,
	-- v7 mapgen
	mountains	= true,
	ridges		= true,
}

local function update_mg_flags()
	-- v5 mapgen
	if(flag_list["blobs"] == true) then
		core.setting_set("mgv5_spflags","blobs")
	else
		core.setting_set("mgv5_spflags","noblobs")
	end
	
	-- v6 mapgen
	local mgv6_spflags = ""
	if(flag_list["biomeblend"] == true) then
		mgv6_spflags = mgv6_spflags .. "biomeblend, "
	else
		mgv6_spflags = mgv6_spflags .. "nobiomeblend, "
	end
	if(flag_list["jungles"] == true) then
		mgv6_spflags = mgv6_spflags .. "jungles, "
	else
		mgv6_spflags = mgv6_spflags .. "nojungles, "
	end
	if(flag_list["mudflow"] == true) then
		mgv6_spflags = mgv6_spflags .. "mudflow"
	else
		mgv6_spflags = mgv6_spflags .. "nomudflow"
	end
	core.setting_set("mgv6_spflags",mgv6_spflags)
	
	-- v7 mapgen
	local mgv7_spflags = ""
	if(flag_list["mountains"] == true) then
		mgv7_spflags = mgv7_spflags .. "mountains, "
	else
		mgv7_spflags = mgv7_spflags .. "nomountains, "
	end
	if(flag_list["ridges"] == true) then
		mgv7_spflags = mgv7_spflags .. "ridges"
	else
		mgv7_spflags = mgv7_spflags .. "noridges"
	end
	core.setting_set("mgv7_spflags",mgv7_spflags)
end

local function mg_flags_formspec(dialogdata)

	-- update the minetest.conf for the first time
	-- to load the default settings
	if(mg_first_time == true) then
		update_mg_flags()
	end
	mg_first_time = false
	-- this is it!
	local formspec =
		"size[4,3,true]" ..
		"label[0,0;" .. fgettext("Mapgen flags") .. "]" ..
		"button[0,2.5;3,0.5;mg_save;" .. fgettext("Save") .. "]"
	
	if(core.setting_get("mg_name") == "v5") then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_blobs;" .. fgettext("Blobs") .. ";" ..
				dump(flag_list["blobs"]) .. "]"
	elseif(core.setting_get("mg_name") == "v6") then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_biomeblend;" .. fgettext("Biome blend") .. ";" ..
				dump(flag_list["biomeblend"]) .. "]" ..
			"checkbox[0.25,1.0;cb_jungles;" .. fgettext("Jungles") .. ";" ..
				dump(flag_list["jungles"]) .. "]" ..
			"checkbox[0.25,1.5;cb_mudflow;" .. fgettext("Mud flow") .. ";" ..
				dump(flag_list["mudflow"]) .. "]"
	elseif(core.setting_get("mg_name") == "v7") then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_mountains;" .. fgettext("Mountains") .. ";" ..
				dump(flag_list["mountains"]) .. "]" ..
			"checkbox[0.25,1.0;cb_ridges;" .. fgettext("Ridges") .. ";" ..
				dump(flag_list["ridges"]) .. "]"
	elseif(core.setting_get("mg_name") == "singlenode") then
		formspec = formspec ..
			"label[0,1.0;" .. fgettext("No flags in singlenode mapgen") .. "]"
	end
	
	return formspec
end

local function mg_flags_buttonhandler(this, fields)

	if fields["mg_save"] then
		update_mg_flags()
		this:delete()
		return true
	end
	
	-- v5 mapgen
	-- blobs
	if fields["cb_blobs"] then
		flag_list["blobs"] = fields["cb_blobs"]
		update_mg_flags()
		return true
	end
	
	-- v6 mapgen
	-- biomeblend
	if fields["cb_biomeblend"] then
		flag_list["biomeblend"] = fields["cb_biomeblend"]
		update_mg_flags()
		return true
	end
	-- jungles
	if fields["cb_jungles"] then
		flag_list["jungles"] = fields["cb_jungles"]
		update_mg_flags()
		return true
	end
	-- mudflow
	if fields["cb_mudflow"] then
		flag_list["mudflow"] = fields["cb_mudflow"]
		update_mg_flags()
		return true
	end
	
	-- v7 mapgen
	-- mountains
	if fields["cb_mountains"] then
		flag_list["mountains"] = fields["cb_mountains"]
		update_mg_flags()
		return true
	end
	-- ridges
	if fields["cb_ridges"] then
		flag_list["ridges"] = fields["cb_ridges"]
		update_mg_flags()
		return true
	end
	-- mudflow
	if fields["cb_mudflow"] then
		flag_list["mudflow"] = fields["cb_mudflow"]
		update_mg_flags()
		return true
	end

	return false
end

local function mg_flags_eventhandler(this,event)
	--close dialog on esc
		if event == "MenuQuit" then
			update_mg_flags()
			this:delete()
			return true
		end
end

function create_mg_flags_dlg()
	local retval = dialog_create("sp_mg_flags",
					mg_flags_formspec,
					mg_flags_buttonhandler,
					mg_flags_eventhandler)
	
	return retval
end