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

-- Default settings
-- Note: Because we have two mapgens which uses same flag name: mgv5 and mgv7, we add prefix _n to caverns.
local default = {
	-- Water level
	water_level = 1,
	-- General attributes
	flag_list_general = {
		caves		= true,
		dungeons	= true,
		light		= true,
		decorations	= true
	},
	-- v5 mapgen
	flag_list_5 = {
		_5caverns	= true
	},
	-- v6 mapgen
	flag_list_6 = {
		jungles		= true,
		biomeblend	= true,
		mudflow		= true,
		snowbiomes	= true,
		flat		= false,
		trees		= true
	},
	-- v7 mapgen
	flag_list_7 = {
		mountains	= true,
		ridges		= true,
		floatlands	= false,
		_7caverns	= true
	},
	-- flat mapgen
	flag_list_flat = {
		lakes		= false,
		hills		= false
	},
	-- valleys mapgen
	flag_list_valleys = {
		altitude_chill	= true,
		humid_rivers	= true
	}
}

-- This table has data from minetest.conf which then will be from player's input.
mm_mg.ftemporary = {
	water_level = 1,
	flag_list_general = {},
	flag_list_5 = {},
	flag_list_6 = {},
	flag_list_7 = {},
	flag_list_flat = {},
	flag_list_valleys = {}
}

function update_mg_flags()
	local function mg_flags_helper(setting_name, list_name)
		-- Only write to minetest.conf when it has changed or already there.
		local changed = false
		if core.setting_get(setting_name) ~= nil then
			changed = true
		end
		if changed == false then
			for flag, val in pairs(default[list_name]) do
				if mm_mg.ftemporary[list_name][flag] ~= val then
					changed = true
					break
				end
			end
		end
		if changed == true then
			local flag_str = ""
			local first = true
			for flag, val in pairs(mm_mg.ftemporary[list_name]) do
				local name = flag
				if flag:sub(1, 1) == "_" then
					name = flag:sub(3)  -- Remove the prefix.
				end
				flag_str = flag_str .. (first and "" or ",") .. (val == true and "" or "no") .. name
				first = false
			end
			core.setting_set(setting_name, flag_str)
		end
	end

	mg_flags_helper("mg_flags", "flag_list_general")
	mg_flags_helper("mgv5_spflags", "flag_list_5")
	mg_flags_helper("mgv6_spflags", "flag_list_6")
	mg_flags_helper("mgv7_spflags", "flag_list_7")
	mg_flags_helper("mgflat_spflags", "flag_list_flat")
	mg_flags_helper("mg_valleys_spflags", "flag_list_valleys")
	if core.setting_get("water_level") ~= mm_mg.ftemporary.water_level then
		core.setting_set("water_level", mm_mg.ftemporary.water_level)
	end
end

function load_mg_flags()
	local function mg_flags_helper(list_name, text)
		mm_mg.ftemporary[list_name] = table.copy(default[list_name])
		if text == nil then
			return
		end
		local text = string.split(text, ",", false)
		for i = 1, #text do
			-- Dealing with two mapgens which uses same flag name: mgv5 and mgv7.
			local name = text[i]
			if name:sub(1, 2) == "no" then
				name = name:sub(3)
				if name == "caverns" then
					name = list_name:sub(10) .. name
				end
				mm_mg.ftemporary[list_name][name] = false
			else
				if name == "caverns" then
					name = list_name:sub(10) .. name
				end
				mm_mg.ftemporary[list_name][name] = true
			end
		end
	end

	mg_flags_helper("flag_list_general", core.setting_get("mg_flags"))
	mg_flags_helper("flag_list_5", core.setting_get("mgv5_spflags"))
	mg_flags_helper("flag_list_6", core.setting_get("mgv6_spflags"))
	mg_flags_helper("flag_list_7", core.setting_get("mgv7_spflags"))
	mg_flags_helper("flag_list_flat", core.setting_get("mgflat_spflags"))
	mg_flags_helper("flag_list_valleys", core.setting_get("mg_valleys_spflags"))
	mm_mg.ftemporary.water_level = core.setting_get("water_level")
end

local function mg_flags_formspec(dialogdata)
	-- Load minetest.conf for first time.
	if mg_first_time == true then
		load_mg_flags()
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
			"checkbox[0.25,0.5;cb__5caverns;" .. fgettext("Caverns") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_5["_5caverns"]) .. "]"
	elseif core.setting_get("mg_name") == "v6" then
		-- If snowbiomes enabled, tell player if jungle is auto-enabled, too.
		if mm_mg.ftemporary.flag_list_6["snowbiomes"] == true then
			mm_mg.ftemporary.flag_list_6["jungles"] = true  -- Do not make the player get confused.
			formspec = formspec ..
			"label[0.7,2.2;" .. fgettext("(with Jungles)") .. "]"
		else
			formspec = formspec ..
			"checkbox[0.25,2.0;cb_jungles;" .. fgettext("Jungles") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_6["jungles"]) .. "]"
		end
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_biomeblend;" .. fgettext("Biome blend") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_6["biomeblend"]) .. "]" ..
			"checkbox[0.25,1.0;cb_mudflow;" .. fgettext("Mud flow") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_6["mudflow"]) .. "]" ..
			"checkbox[0.25,1.5;cb_snowbiomes;" .. fgettext("Additional Biomes") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_6["snowbiomes"]) .. "]" ..
			"checkbox[0.25,2.5;cb_flat;" .. fgettext("Flat world") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_6["flat"]) .. "]" ..
			"checkbox[0.25,3.0;cb_trees;" .. fgettext("Trees") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_6["trees"]) .. "]"
	elseif core.setting_get("mg_name") == "v7" then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_mountains;" .. fgettext("Mountains") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_7["mountains"]) .. "]" ..
			"checkbox[0.25,1.0;cb_ridges;" .. fgettext("Ridges") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_7["ridges"]) .. "]" ..
			"checkbox[0.25,1.5;cb_floatlands;" .. fgettext("Floatlands") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_7["floatlands"]) .. "]" ..
			"checkbox[0.25,2.0;cb__7caverns;" .. fgettext("Caverns") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_7["_7caverns"]) .. "]"
	elseif core.setting_get("mg_name") == "flat" then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_lakes;" .. fgettext("Lakes") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_flat["lakes"]) .. "]" ..
			"checkbox[0.25,1.0;cb_hills;" .. fgettext("Hills") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_flat["hills"]) .. "]"
	elseif core.setting_get("mg_name") == "valleys" then
		formspec = formspec ..
			"checkbox[0.25,0.5;cb_altitude_chill;" .. fgettext("Height temperature") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_valleys["altitude_chill"]) .. "]" ..
			"checkbox[0.25,1.0;cb_humid_rivers;" .. fgettext("Humid rivers") .. ";" ..
				tostring(mm_mg.ftemporary.flag_list_valleys["humid_rivers"]) .. "]"
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
			tostring(mm_mg.ftemporary.flag_list_general["caves"]) .. "]" ..
		"checkbox[4.25,1.0;cb_dungeons;" .. fgettext("Dungeons") .. ";" ..
			tostring(mm_mg.ftemporary.flag_list_general["dungeons"]) .. "]" ..
		"checkbox[4.25,1.5;cb_decorations;" .. fgettext("Decorations") .. ";" ..
			tostring(mm_mg.ftemporary.flag_list_general["decorations"]) .. "]"
	-- Water level
	formspec = formspec ..
		"field[0.5,4.9;2.0,0.5;te_water_level;" .. fgettext("Water level") .. ";" .. mm_mg.ftemporary.water_level .. "] " ..
		"button[2.25,4.6;2.75,0.5;mg_water_level;" .. fgettext("Set water level") .. "]"
	return formspec
end

local function mg_flags_buttonhandler(this, fields)
	if fields["mg_close"] then
		this:delete()
		return true
	end

	local flag_lists_name = {
		"flag_list_general", "flag_list_5", "flag_list_6",
		"flag_list_7", "flag_list_flat", "flag_list_valleys"
	}
	for idx, flag_list in pairs(flag_lists_name) do
		for flag, val in pairs(default[flag_list]) do
			local cbflag = "cb_" .. flag
			if fields[cbflag] then
				mm_mg.ftemporary[flag_list][flag] = core.is_yes(fields[cbflag])
				return true
			end
		end
	end

	-- Player need to press enter on the textbox to run this or just click the button.
	if fields["te_water_level"] or fields["mg_water_level"] then
		mm_mg.ftemporary.water_level = tonumber(fields["te_water_level"])
		return true
	end
	return false
end

function create_mg_flags_dlg()
	return dialog_create("sp_mg_flags",
			mg_flags_formspec,
			mg_flags_buttonhandler,
			nil)
end
