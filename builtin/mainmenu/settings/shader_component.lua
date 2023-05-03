--Minetest
--Copyright (C) 2013 sapier
--Copyright (C) 2021-2 x2048
--Copyright (C) 2022-3 rubenwardy
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


local shadow_levels_labels = {
	fgettext("Disabled"),
	fgettext("Very Low"),
	fgettext("Low"),
	fgettext("Medium"),
	fgettext("High"),
	fgettext("Very High")
}


local function get_shadow_mapping_idx()
	local level = tonumber(core.settings:get("shadow_levels"))
	if level and level >= 0 and level < #shadow_levels_labels then
		return level + 1
	end
	return 1
end


local function set_shadow_mapping_idx(v)
	assert(v >= 1 and v <= #shadow_levels_labels)
	core.settings:set("shadow_levels", tostring(v - 1))
end


return {
	query_text = "Shaders",
	get_formspec = function(self, avail_w)
		local fs = ""

		local video_driver = core.get_active_driver()
		local shaders_enabled = core.settings:get_bool("enable_shaders")
		if video_driver == "opengl" then
			fs = fs ..
				"checkbox[0,0.25;cb_shaders;" .. fgettext("Shaders") .. ";"
						.. tostring(shaders_enabled) .. "]"
		elseif video_driver == "ogles2" then
			fs = fs ..
				"checkbox[0,0.25;cb_shaders;" .. fgettext("Shaders (experimental)") .. ";"
						.. tostring(shaders_enabled) .. "]"
		else
			core.settings:set_bool("enable_shaders", false)
			shaders_enabled = false
			fs = fs ..
				"label[0.13,0.25;" .. core.colorize("#888888",
						fgettext("Shaders (unavailable)")) .. "]"
		end

		if shaders_enabled then
			fs = fs ..
				"container[0,0.75]" ..
				"checkbox[0,0;cb_tonemapping;" .. fgettext("Tone mapping") .. ";"
						.. tostring(core.settings:get_bool("tone_mapping")) .. "]" ..
				"checkbox[0,0.5;cb_waving_water;" .. fgettext("Waving liquids") .. ";"
						.. tostring(core.settings:get_bool("enable_waving_water")) .. "]" ..
				"checkbox[0,1;cb_waving_leaves;" .. fgettext("Waving leaves") .. ";"
						.. tostring(core.settings:get_bool("enable_waving_leaves")) .. "]" ..
				"checkbox[0,1.5;cb_waving_plants;" .. fgettext("Waving plants") .. ";"
						.. tostring(core.settings:get_bool("enable_waving_plants")) .. "]"

			if video_driver == "opengl" then
				fs = fs ..
					"label[0,2.2;" .. fgettext("Dynamic shadows") .. "]" ..
					"dropdown[0,2.4;3,0.8;dd_shadows;" .. table.concat(shadow_levels_labels, ",") .. ";" ..
						get_shadow_mapping_idx() .. ";true]" ..
					"label[0,3.5;" .. core.colorize("#bbb", fgettext("(The game will need to enable shadows as well)")) .. "]"
			else
				fs = fs ..
					"label[0,2.2;" .. core.colorize("#888888", fgettext("Dynamic shadows")) .. "]"
			end

			fs = fs .. "container_end[]"
		else
			fs = fs ..
				"container[0.35,0.75]" ..
				"label[0,0;" .. core.colorize("#888888",
						fgettext("Tone mapping")) .. "]" ..
				"label[0,0.5;" .. core.colorize("#888888",
						fgettext("Waving liquids")) .. "]" ..
				"label[0,1;" .. core.colorize("#888888",
						fgettext("Waving leaves")) .. "]" ..
				"label[0,1.5;" .. core.colorize("#888888",
						fgettext("Waving plants")) .. "]"..
				"label[0,2;" .. core.colorize("#888888",
						fgettext("Dynamic shadows")) .. "]" ..
				"container_end[]"
		end
		return fs, 4.5
	end,

	on_submit = function(self, fields)
		if fields.cb_shaders then
			core.settings:set("enable_shaders", fields.cb_shaders)
			return true
		end
		if fields.cb_tonemapping then
			core.settings:set("tone_mapping", fields.cb_tonemapping)
			return true
		end
		if fields.cb_waving_water then
			core.settings:set("enable_waving_water", fields.cb_waving_water)
			return true
		end
		if fields.cb_waving_leaves then
			core.settings:set("enable_waving_leaves", fields.cb_waving_leaves)
			return true
		end
		if fields.cb_waving_plants then
			core.settings:set("enable_waving_plants", fields.cb_waving_plants)
			return true
		end

		if fields.dd_shadows then
			local old_shadow_level_idx = get_shadow_mapping_idx()
			local shadow_level_idx = tonumber(fields.dd_shadows)
			if shadow_level_idx == nil or shadow_level_idx == old_shadow_level_idx then
				return false
			end

			set_shadow_mapping_idx(shadow_level_idx)

			local shadow_presets = {
				[2] = { 62,  512,  "true", 0, "false" },
				[3] = { 93,  1024, "true", 0, "false" },
				[4] = { 140, 2048, "true", 1, "false" },
				[5] = { 210, 4096, "true", 2,  "true" },
				[6] = { 300, 8192, "true", 2,  "true" },
			}
			local preset = shadow_presets[shadow_level_idx]
			if preset then
				core.settings:set("enable_dynamic_shadows", "true")
				core.settings:set("shadow_map_max_distance", preset[1])
				core.settings:set("shadow_map_texture_size", preset[2])
				core.settings:set("shadow_map_texture_32bit", preset[3])
				core.settings:set("shadow_filters", preset[4])
				core.settings:set("shadow_map_color", preset[5])
			else
				core.settings:set("enable_dynamic_shadows", "false")
			end
			return true
		end
	end,
}
