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

local dd_filter_labels = {
	fgettext("No Filter"),
	fgettext("Bilinear Filter"),
	fgettext("Trilinear Filter")
}

local filters = {
	{dd_filter_labels[1]..","..dd_filter_labels[2]..","..dd_filter_labels[3]},
	{"", "bilinear_filter", "trilinear_filter"},
}

local dd_mipmap_labels = {
	fgettext("No Mipmap"),
	fgettext("Mipmap"),
	fgettext("Mipmap + Aniso. Filter")
}

local mipmap = {
	{dd_mipmap_labels[1]..","..dd_mipmap_labels[2]..","..dd_mipmap_labels[3]},
	{"", "mip_map", "anisotropic_filter"},
}

local function getFilterSettingIndex()
	if (core.setting_get(filters[2][3]) == "true") then
		return 3
	end
	if (core.setting_get(filters[2][3]) == "false" and core.setting_get(filters[2][2]) == "true") then
		return 2
	end
	return 1
end

local function getMipmapSettingIndex()
	if (core.setting_get(mipmap[2][3]) == "true") then
		return 3
	end
	if (core.setting_get(mipmap[2][3]) == "false" and core.setting_get(mipmap[2][2]) == "true") then
		return 2
	end
	return 1
end

local function video_driver_fname_to_name(selected_driver)
	local video_drivers = core.get_video_drivers()

	for i=1, #video_drivers do
		if selected_driver == video_drivers[i].friendly_name then
			return video_drivers[i].name:lower()
		end
	end

	return ""
end

local function dlg_confirm_reset_formspec(data)
	local retval =
		"size[8,3]" ..
		"label[1,1;".. fgettext("Are you sure to reset your singleplayer world?") .. "]"..
		"button[1,2;2.6,0.5;dlg_reset_singleplayer_confirm;"..
				fgettext("Yes") .. "]" ..
		"button[4,2;2.8,0.5;dlg_reset_singleplayer_cancel;"..
				fgettext("No!!!") .. "]"
	return retval
end

local function dlg_confirm_reset_btnhandler(this, fields, dialogdata)

	if fields["dlg_reset_singleplayer_confirm"] ~= nil then
		local worldlist = core.get_worlds()
		local found_singleplayerworld = false

		for i=1,#worldlist,1 do
			if worldlist[i].name == "singleplayerworld" then
				found_singleplayerworld = true
				gamedata.worldindex = i
			end
		end

		if found_singleplayerworld then
			core.delete_world(gamedata.worldindex)
		end

		core.create_world("singleplayerworld", 1)

		worldlist = core.get_worlds()

		found_singleplayerworld = false

		for i=1,#worldlist,1 do
			if worldlist[i].name == "singleplayerworld" then
				found_singleplayerworld = true
				gamedata.worldindex = i
			end
		end
	end

	this.parent:show()
	this:hide()
	this:delete()
	return true
end

local function showconfirm_reset(tabview)
	local new_dlg = dialog_create("reset_spworld",
		dlg_confirm_reset_formspec,
		dlg_confirm_reset_btnhandler,
		nil)
	new_dlg:set_parent(tabview)
	tabview:hide()
	new_dlg:show()
end

local function gui_scale_to_scrollbar()
	local current_value = tonumber(core.setting_get("gui_scaling"))

	if (current_value == nil) or current_value < 0.25 then
		return 0
	end
	if current_value <= 1.25 then
		return ((current_value - 0.25)/ 1.0) * 700
	end
	if current_value <= 6 then
		return ((current_value -1.25) * 100) + 700
	end

	return 1000
end

local function scrollbar_to_gui_scale(value)
	value = tonumber(value)

	if (value <= 700) then
		return ((value / 700) * 1.0) + 0.25
	end
	if (value <=1000) then
		return ((value - 700) / 100) + 1.25
	end

	return 1
end

local function formspec(tabview, name, tabdata)
	local video_drivers = core.get_video_drivers()
	local current_video_driver = core.setting_get("video_driver"):lower()

	local driver_formspec_string = ""
	local driver_current_idx = 0

	for i=2, #video_drivers do
		driver_formspec_string = driver_formspec_string .. video_drivers[i].friendly_name
		if i ~= #video_drivers then
			driver_formspec_string = driver_formspec_string .. ","
		end

		if current_video_driver == video_drivers[i].name:lower() then
			driver_current_idx = i - 1
		end
	end

	local tab_string =
		"box[0,0;3.5,3.9;#999999]" ..
		"checkbox[0.25,0;cb_smooth_lighting;".. fgettext("Smooth Lighting")
				.. ";".. dump(core.setting_getbool("smooth_lighting")) .. "]"..
		"tooltip[cb_smooth_lighting;"..
		fgettext(
[[If enabled, the light on surfaces will appear to be smooth. If disabled,
the light will look rather flat, but is more processor-friendly.]]) .. "]" ..
		"checkbox[0.25,0.5;cb_particles;".. fgettext("Enable Particles") .. ";"
				.. dump(core.setting_getbool("enable_particles"))	.. "]"..
		"tooltip[cb_particles;"..
		fgettext(
[[If enabled, particles are displayed for a variety of events, most often
for digging blocks.]]) .. "]" ..
		"checkbox[0.25,1;cb_3d_clouds;".. fgettext("3D Clouds") .. ";"
				.. dump(core.setting_getbool("enable_3d_clouds")) .. "]"..
		"tooltip[cb_3d_clouds;"..
		fgettext("If disabled, the clouds will be flat instead of 3-dimensional.") .. "]" ..
		"checkbox[0.25,1.5;cb_fancy_trees;".. fgettext("Fancy Trees") .. ";"
				.. dump(core.setting_getbool("new_style_leaves")) .. "]"..
		"tooltip[cb_fancy_trees;"..
		fgettext(
[[If enabled, it is possible to look through leaves (and some other blocks).
If disabled, these blocks are opaque.

It may be neccessary to disable this effect on slow machines.]]) .. "]" ..
		"checkbox[0.25,2.0;cb_opaque_water;".. fgettext("Opaque Water") .. ";"
				.. dump(core.setting_getbool("opaque_water")) .. "]"..
		"tooltip[cb_opaque_water;"..
		fgettext("If enabled, you can't look through water (and some other blocks).") .. "]" ..
		"checkbox[0.25,2.5;cb_connected_glass;".. fgettext("Connected Glass") .. ";"
				.. dump(core.setting_getbool("connected_glass"))	.. "]"..
		"tooltip[cb_connected_glass;"..
		fgettext(
[[If enabled, certain glass blocks (and other, similar blocks) will form larger
shapes when placed next to each other.
If disabled, each such block will be displayed as "singleton", without
connections.]]) .. "]" ..
		"checkbox[0.25,3.0;cb_node_highlighting;".. fgettext("Node Highlighting") .. ";"
				.. dump(core.setting_getbool("enable_node_highlighting")) .. "]"..
		"tooltip[cb_node_highlighting;"..
		fgettext(
[[If enabled, the pointed node (block) will be surrounded by a bright
translucent cuboid in a certain color.
If disabled, the pointed node will have drawn a thin wireframe around it.]]) .. "]" ..
		"box[3.75,0;3.75,3.45;#999999]" ..
		"label[3.85,0.1;".. fgettext("Texturing:") .. "]"..
		"dropdown[3.85,0.55;3.85;dd_filters;" .. filters[1][1] .. ";"
				.. getFilterSettingIndex() .. "]" ..
		"tooltip[dd_filters;"..
		fgettext(
[[This changes the way how the surface images (textures) of blocks look.
Using no filters will show the textures in their original form, which usually
gives them a "pixelated" look.
The bilinear filter smoothens the textures.
The trilinear filter smoothens them even further.]]) .. "]" ..
		"dropdown[3.85,1.35;3.85;dd_mipmap;" .. mipmap[1][1] .. ";"
				.. getMipmapSettingIndex() .. "]" ..
		"tooltip[dd_mipmap;" ..
		fgettext(
[[By using mip-mapping, objects which are further away will be using images of
lower resolution.
The difference is barely visually noticable, but mip-mapping is great for slow
machines.

The anisotropic filter is another filter which will make the block surfaces
look more smooth.]]) .. "]" ..

		"label[3.85,2.15;".. fgettext("Rendering:") .. "]"..
		"dropdown[3.85,2.6;3.85;dd_video_driver;"
				.. driver_formspec_string .. ";" .. driver_current_idx .. "]" ..
		"tooltip[dd_video_driver;" ..
		fgettext(
[[OpenGL is a cross-platform renderer which needs support from your video card.
It supports shaders.
DirectX 9.0 (direct3d9) is the native renderer for Windows and is only
available on Windows systems and if your video card supports it.
Burning's video is a legacy driver which is not well-supported by Minetest,
it is not recommended to use it unless you know what you do.
The software renderer works without a video card, but produces the worst
results.

You must restart Minetest for a renderer change to take effect.]]) .. "]" ..
		"box[7.75,0;4,4;#999999]" ..
		"checkbox[8,0;cb_shaders;".. fgettext("Shaders") .. ";"
				.. dump(core.setting_getbool("enable_shaders")) .. "]" ..
		"tooltip[cb_shaders;"..
		fgettext(
[[This enables a variety of shading and other techniques, which make the
shadows look more realistic.
Shaders are only available if you choose OpenGL as renderer.
Disable shaders on slow systems.]]) .. "]"

	if PLATFORM ~= "Android" then
		tab_string = tab_string ..
		"button[8,4.75;3.75,0.5;btn_change_keys;".. fgettext("Change key-bindings") .. "]"
	else
		tab_string = tab_string ..
		"button[8,4.75;3.75,0.5;btn_reset_singleplayer;".. fgettext("Reset singleplayer world") .. "]"
	end

	local scale_tooltip
	local scale_tooltip_pt1 = fgettext("WARNING: The changes will immediately take effect.")
	local scale_tooltip_pt2 = fgettext("Currently, a scaling factor of %.2f is applied "..
			"to the GUI elements.")
	local gui_scale = tonumber(core.setting_get("gui_scaling"))
	if gui_scale ~= nil then
		scale_tooltip = scale_tooltip_pt1 .. "\n\n" .. string.format(scale_tooltip_pt2, gui_scale)
	else
		scale_tooltip = scale_tooltip_pt1
	end
	tab_string = tab_string ..

		"box[0,4.25;3.5,1.1;#999999]" ..
		"label[0.25,4.25;" .. fgettext("GUI scale factor") .. "]" ..
		"scrollbar[0.25,4.75;3,0.4;sb_gui_scaling;horizontal;" ..
		 gui_scale_to_scrollbar() .. "]" ..
		"tooltip[sb_gui_scaling;" .. scale_tooltip.. "]"

	if PLATFORM == "Android" then
		tab_string = tab_string ..
		"box[3.75,3.55;3.75,1.8;#999999]" ..
		"checkbox[3.9,3.45;cb_touchscreen_target;".. fgettext("Touch free target") .. ";"
				.. dump(core.setting_getbool("touchtarget")) .. "]"
	end

	if core.setting_get("touchscreen_threshold") ~= nil then
		tab_string = tab_string ..
				"label[4.3,4.1;" .. fgettext("Touchthreshold (px)") .. "]" ..
				"dropdown[3.85,4.55;3.85;dd_touchthreshold;0,10,20,30,40,50;" ..
				((tonumber(core.setting_get("touchscreen_threshold"))/10)+1) .. "]"
	end

	if core.setting_getbool("enable_shaders") then
		tab_string = tab_string ..
				"checkbox[8,0.5;cb_bumpmapping;".. fgettext("Bumpmapping") .. ";"
						.. dump(core.setting_getbool("enable_bumpmapping")) .. "]"..
				"tooltip[cb_bumpmapping;" ..
				fgettext(
[[Bump-mapping simulates bumps and wrinkles on an otherwise flat surface.
It will enhance the looks of surfaces and is not too demanding on the hardware.]]) .. "]" ..
				"checkbox[8,1.0;cb_generate_normalmaps;".. fgettext("Generate Normalmaps") .. ";"
						.. dump(core.setting_getbool("generate_normalmaps")) .. "]"..
				"checkbox[8,1.5;cb_parallax;".. fgettext("Parallax Occlusion") .. ";"
						.. dump(core.setting_getbool("enable_parallax_occlusion")) .. "]"..
				"tooltip[cb_parallax;" ..
				fgettext(
[[Gives more the illusion of depth to the entire scene by using shadows.
This effect is not very much demanding on your hardware.]]) .. "]" ..
				"checkbox[8,2.0;cb_waving_water;".. fgettext("Waving Water") .. ";"
						.. dump(core.setting_getbool("enable_waving_water")) .. "]"..
				"tooltip[cb_waving_water;" ..
				fgettext(
[[If enabled, water blocks (and other liquids) will have a subtle wave animation.]]) .. "]" ..
				"checkbox[8,2.5;cb_waving_leaves;".. fgettext("Waving Leaves") .. ";"
						.. dump(core.setting_getbool("enable_waving_leaves")) .. "]"..
				"tooltip[cb_waving_leaves;" ..
				fgettext(
[[If enabled, leaf blocks (and other, similar blocks) will have a waving
animation, as if the wind blows into the leaves.]]) .. "]" ..
				"checkbox[8,3.0;cb_waving_plants;".. fgettext("Waving Plants") .. ";"
						.. dump(core.setting_getbool("enable_waving_plants")) .. "]" ..
				"tooltip[cb_waving_plants;" ..
				fgettext(
[[If enabled, plants (and other, similar-looking blocks) will wiggle, as if the
wind blows into them.]]) .. "]"

	else
		tab_string = tab_string ..
				"textlist[8.33,0.7;4,1;;#888888" .. fgettext("Bumpmapping") .. ";0;true]" ..
				"textlist[8.33,1.2;4,1;;#888888" .. fgettext("Generate Normalmaps") .. ";0;true]" ..
				"textlist[8.33,1.7;4,1;;#888888" .. fgettext("Parallax Occlusion") .. ";0;true]" ..
				"textlist[8.33,2.2;4,1;;#888888" .. fgettext("Waving Water") .. ";0;true]" ..
				"textlist[8.33,2.7;4,1;;#888888" .. fgettext("Waving Leaves") .. ";0;true]" ..
				"textlist[8.33,3.2;4,1;;#888888" .. fgettext("Waving Plants") .. ";0;true]"
		end
	return tab_string
end

--------------------------------------------------------------------------------
local function handle_settings_buttons(this, fields, tabname, tabdata)
	if fields["cb_fancy_trees"] then
		core.setting_set("new_style_leaves", fields["cb_fancy_trees"])
		return true
	end
	if fields["cb_smooth_lighting"] then
		core.setting_set("smooth_lighting", fields["cb_smooth_lighting"])
		return true
	end
	if fields["cb_3d_clouds"] then
		core.setting_set("enable_3d_clouds", fields["cb_3d_clouds"])
		return true
	end
	if fields["cb_opaque_water"] then
		core.setting_set("opaque_water", fields["cb_opaque_water"])
		return true
	end
	if fields["cb_shaders"] then
		if (core.setting_get("video_driver") == "direct3d8" or core.setting_get("video_driver") == "direct3d9") then
			core.setting_set("enable_shaders", "false")
			gamedata.errormessage = fgettext("To enable shaders the OpenGL driver needs to be used.")
		else
			core.setting_set("enable_shaders", fields["cb_shaders"])
		end
		return true
	end
	if fields["cb_connected_glass"] then
		core.setting_set("connected_glass", fields["cb_connected_glass"])
		return true
	end
	if fields["cb_node_highlighting"] then
		core.setting_set("enable_node_highlighting", fields["cb_node_highlighting"])
		return true
	end
	if fields["cb_particles"] then
		core.setting_set("enable_particles", fields["cb_particles"])
		return true
	end
	if fields["cb_bumpmapping"] then
		core.setting_set("enable_bumpmapping", fields["cb_bumpmapping"])
	end
	if fields["cb_generate_normalmaps"] then
		core.setting_set("generate_normalmaps", fields["cb_generate_normalmaps"])
	end
	if fields["cb_parallax"] then
		core.setting_set("enable_parallax_occlusion", fields["cb_parallax"])
		return true
	end
	if fields["cb_waving_water"] then
		core.setting_set("enable_waving_water", fields["cb_waving_water"])
		return true
	end
	if fields["cb_waving_leaves"] then
		core.setting_set("enable_waving_leaves", fields["cb_waving_leaves"])
	end
	if fields["cb_waving_plants"] then
		core.setting_set("enable_waving_plants", fields["cb_waving_plants"])
		return true
	end
	if fields["btn_change_keys"] ~= nil then
		core.show_keys_menu()
		return true
	end

	if fields["sb_gui_scaling"] then
		local event = core.explode_scrollbar_event(fields["sb_gui_scaling"])

		if event.type == "CHG" then
			local tosave = string.format("%.2f",scrollbar_to_gui_scale(event.value))
			core.setting_set("gui_scaling",tosave)
			return true
		end
	end
	if fields["cb_touchscreen_target"] then
		core.setting_set("touchtarget", fields["cb_touchscreen_target"])
		return true
	end
	if fields["btn_reset_singleplayer"] then
		showconfirm_reset(this)
		return true
	end

	--Note dropdowns have to be handled LAST!
	local ddhandled = false

	if fields["dd_touchthreshold"] then
		core.setting_set("touchscreen_threshold",fields["dd_touchthreshold"])
		ddhandled = true
	end

	if fields["dd_video_driver"] then
		core.setting_set("video_driver",
			video_driver_fname_to_name(fields["dd_video_driver"]))
		ddhandled = true
	end
	if fields["dd_filters"] == dd_filter_labels[1] then
		core.setting_set("bilinear_filter", "false")
		core.setting_set("trilinear_filter", "false")
		ddhandled = true
	end
	if fields["dd_filters"] == dd_filter_labels[2] then
		core.setting_set("bilinear_filter", "true")
		core.setting_set("trilinear_filter", "false")
		ddhandled = true
	end
	if fields["dd_filters"] == dd_filter_labels[3] then
		core.setting_set("bilinear_filter", "false")
		core.setting_set("trilinear_filter", "true")
		ddhandled = true
	end
	if fields["dd_mipmap"] == dd_mipmap_labels[1] then
		core.setting_set("mip_map", "false")
		core.setting_set("anisotropic_filter", "false")
		ddhandled = true
	end
	if fields["dd_mipmap"] == dd_mipmap_labels[2] then
		core.setting_set("mip_map", "true")
		core.setting_set("anisotropic_filter", "false")
		ddhandled = true
	end
	if fields["dd_mipmap"] == dd_mipmap_labels[3] then
		core.setting_set("mip_map", "true")
		core.setting_set("anisotropic_filter", "true")
		ddhandled = true
	end

	return ddhandled
end

tab_settings = {
	name = "settings",
	caption = fgettext("Settings"),
	cbf_formspec = formspec,
	cbf_button_handler = handle_settings_buttons
}
