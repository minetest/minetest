--Minetest
--Copyright (C) 2016 srifqi, Muhammad Rifqi Priyo Susanto
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

-- Use default if not defined

-- Water level
local water_level = 1

-- General attributes
local flag_list_g = {
	caves		= true,
	dungeons	= true,
	light		= true,
	decorations	= true
}

-- v6 mapgen
local flag_list_6 = {
	biomeblend	= true,
	mudflow		= true,
	snowbiomes	= true,
	jungles		= true,
	flat		= false,
	trees		= true
}

-- v7 mapgen
local flag_list_7 = {
	mountains	= true,
	ridges		= true
}

-- flat mapgen
local flag_list_f = {
	lakes		= false,
	hills		= false
}

-- valleys mapgen
local flag_list_v = {
	altitude_chill	= true,
	humid_rivers	= true
}

local function update_mg_flags()
	local function mg_flags_helper(flag_list)
		local first = true
		local flag_str = ""
		for flag, val in pairs(flag_list) do
			flag_str = flag_str .. (first and "" or ", ") .. (val == true and "" or "no") .. flag
			first = false
		end
		return flag_str
	end
	
	core.setting_set("mg_flags", mg_flags_helper(flag_list_g))
	core.setting_set("mgv6_spflags", mg_flags_helper(flag_list_6))
	core.setting_set("mgv7_spflags", mg_flags_helper(flag_list_7))
	core.setting_set("mgflat_spflags", mg_flags_helper(flag_list_f))
	core.setting_set("mg_valleys_spflags", mg_flags_helper(flag_list_v))
	core.setting_set("water_level", water_level)
end

-- Load default settings if not exist
if core.setting_get("water_level") == nil then
	core.setting_set("water_level", 1)
end
if core.setting_get("mg_flags") == nil then
	core.setting_set("mg_flags", "caves, dungeons, light, decorations")
end
if core.setting_get("mgv6_spflags") == nil then
	core.setting_set("mgv6_spflags", "jungles, biomeblend, mudflow, snowbiomes, trees")
end
if core.setting_get("mgv7_spflags") == nil then
	core.setting_set("mgv7_spflags", "mountains, ridges")
end
if core.setting_get("mgflat_spflags") == nil then
	-- prevents error because of nil value
	core.setting_set("mgflat_spflags", "")
end
if core.setting_get("mg_valleys_spflags") == nil then
	core.setting_set("mg_valleys_spflags", "altitude_chill, humid_rivers")
end

local function load_mg_flags()
	local function mg_flags_helper(flag_list,text)
		local text = string.split(text, ", ")
		for i = 1, #text do
			if text[i]:sub(1, 2) == "no" then
				flag_list[text[i]:sub(3)] = false
			else
				flag_list[text[i]] = true
			end
		end
	end
	mg_flags_helper(flag_list_g, core.setting_get("mg_flags"))
	mg_flags_helper(flag_list_6, core.setting_get("mgv6_spflags"))
	mg_flags_helper(flag_list_7, core.setting_get("mgv7_spflags"))
	mg_flags_helper(flag_list_f, core.setting_get("mgflat_spflags"))
	mg_flags_helper(flag_list_v, core.setting_get("mg_valleys_spflags"))
	water_level = core.setting_get("water_level")
	
	update_mg_flags()
end

local function mg_flags_formspec(dialogdata)
	-- Load minetest.conf for first time
	if mg_first_time == true then
		load_mg_flags()
		update_mg_flags()
	end
	mg_first_time = false
	
	local mgname = "[" .. core.setting_get("mg_name") .. "]"
	local formspec =
		"size[8,5,true]" ..
		"button[5.5,4.6;2.5,0.5;mg_close;" .. fgettext("Close") .. "]" ..
		
	-- Special mapgen-specific flags
		"label[0,0;" .. minetest.formspec_escape(mgname) .. " " .. fgettext("Mapgen flags") .. "]"
	if core.setting_get("mg_name") == "v5" then
		formspec = formspec ..
			"label[0,1.0;" .. fgettext("No flags in v5 mapgen") .. "]"
	elseif core.setting_get("mg_name") == "v6" then
		-- If snowbiomes enabled, tell player if jungle is auto-enabled, too.
		if flag_list_6["snowbiomes"] == true then
			flag_list_6["jungles"] = true  -- Do not make the player get confused
			formspec = formspec ..
			"label[0.7,2.2;" .. fgettext("(with Jungles)") .. "]"
		else
			formspec = formspec ..
			"checkbox[0.25,2.0;cb_jungles;" .. fgettext("Jungles") .. ";" ..
				dump(flag_list_6["jungles"]) .. "]"
		end
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_biomeblend;" .. fgettext("Biome blend") .. ";" ..
				dump(flag_list_6["biomeblend"]) .. "]" ..
			"checkbox[0.25,1.0;cb_mudflow;" .. fgettext("Mud flow") .. ";" ..
				dump(flag_list_6["mudflow"]) .. "]" ..
			"checkbox[0.25,1.5;cb_snowbiomes;" .. fgettext("Additional Biomes") .. ";" ..
				dump(flag_list_6["snowbiomes"]) .. "]" ..
			"checkbox[0.25,2.5;cb_flat;" .. fgettext("Flat world") .. ";" ..
				dump(flag_list_6["flat"]) .. "]" ..
			"checkbox[0.25,3.0;cb_trees;" .. fgettext("Trees") .. ";" ..
				dump(flag_list_6["trees"]) .. "]"
	elseif core.setting_get("mg_name") == "v7" then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_mountains;" .. fgettext("Mountains") .. ";" ..
				dump(flag_list_7["mountains"]) .. "]" ..
			"checkbox[0.25,1.0;cb_ridges;" .. fgettext("Ridges") .. ";" ..
				dump(flag_list_7["ridges"]) .. "]"
	elseif core.setting_get("mg_name") == "flat" then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_lakes;" .. fgettext("Lakes") .. ";" ..
				dump(flag_list_f["lakes"]) .. "]" ..
			"checkbox[0.25,1.0;cb_hills;" .. fgettext("Hills") .. ";" ..
				dump(flag_list_f["hills"]) .. "]"
	elseif core.setting_get("mg_name") == "valleys" then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_altitude_chill;" .. fgettext("Height temperature") .. ";" ..
				dump(flag_list_v["altitude_chill"]) .. "]" ..
			"checkbox[0.25,1.0;cb_humid_rivers;" .. fgettext("Humid rivers") .. ";" ..
				dump(flag_list_v["humid_rivers"]) .. "]"
	elseif core.setting_get("mg_name") == "fractal" then
		formspec = formspec ..
			"label[0,1.0;" .. fgettext("No flags in fractal mapgen for now") .. "]"
	elseif core.setting_get("mg_name") == "singlenode" then
		formspec = formspec ..
			"label[0,1.0;" .. fgettext("No flags in singlenode mapgen") .. "]"
	end
	
	-- General attributes a.k.a. generated structure
	formspec = formspec ..
		"label[4,0;" .. fgettext("Generated structure") .. "]" ..
		"checkbox[4.25,0.5;cb_caves;" .. fgettext("Caves") .. ";" ..
			dump(flag_list_g["caves"]) .. "]" ..
		"checkbox[4.25,1.0;cb_dungeons;" .. fgettext("Dungeons") .. ";" ..
			dump(flag_list_g["dungeons"]) .. "]" ..
		"checkbox[4.25,1.5;cb_decorations;" .. fgettext("Decorations") .. ";" ..
			dump(flag_list_g["decorations"]) .. "]"
	
	-- Water level
	formspec = formspec ..
		"field[0.5,4.9;2.5,0.5;te_water_level;" .. fgettext("Water level") .. ";" .. water_level .. "] " ..
		"button[2.75,4.6;2.75,0.5;mg_water_level;" .. fgettext("Set water level") .. "]"
	
	return formspec
end

local function mg_flags_buttonhandler(this, fields)
	if fields["mg_close"] then
		update_mg_flags()
		this:delete()
		return true
	end
	
	local flag_lists_list = {flag_list_g, flag_list_6, flag_list_7, flag_list_f, flag_list_v}
	for idx, flag_list in pairs(flag_lists_list) do
		for flag, val in pairs(flag_list) do
			local cbflag = "cb_" .. flag
			if fields[cbflag] then
				flag_list[flag] = core.is_yes(fields[cbflag])
				update_mg_flags()
				return true
			end
		end
	end
	
	-- Player need to press enter on the textbox to run this
	-- Or just click the button
	if fields["te_water_level"] or fields["mg_water_level"] then
		water_level = tonumber(fields["te_water_level"])
		update_mg_flags()
		return true
	end
	
	return false
end

function create_mg_flags_dlg()
	local retval = dialog_create("sp_mg_flags",
					mg_flags_formspec,
					mg_flags_buttonhandler,
					nil)
	
	return retval
end