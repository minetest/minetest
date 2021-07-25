local force_default_fontpath_input = false
local sync_mono = false
local force_mono_fontpath_input = false

local default_fonts  = {
	"Arimo",
	fgettext("Monospace"),
	fgettext("Custom path"),
}

local mono_fonts = {
	"Cousine",
	fgettext("Custom path"),
}

local fontpaths = {
	["Unifont"] = {only = "unifont.ttf"},
	["Unifont (JP)"] = {only = "unifont_jp.ttf"},
}

local function get_font_file_for(fontname, variant)
	local f = fontpaths[fontname]
	local fn
	if f then
		if f.only then
			fn = f.only
		elseif f[string.lower(variant)] then
			fn = f[string.lower(variant)]
		elseif f.base then
			fn = string.format("%s-%s.ttf", f.base, variant)
		end
	end
	if not fn then
		fn = string.format("%s-%s.ttf", fontname, variant)
	end
	return fn
end

local function match_font(filepath, fontname, variant)
	-- Plain text matching is used here to avoid potential misinterpretation of font file paths as patterns
	-- The pattern argument is prepended with DIR_DELIM to avoid potential false positives
	local fontfile = get_font_file_for(fontname, variant or "Regular")
	if filepath == fontfile then
		return true
	end
	local _, e = string.find(filepath, DIR_DELIM .. fontfile, 1, true)
	if e == #filepath then
		return true
	end
	return false
end

local fonttest = string.gsub([[
	<justify>Regular   <b>Bold</b>   <i>Italic   <b>BoldItalic</b></i></justify>
	<justify><mono>Mono   <b>Bold</b>   <i>Italic</i>   <b>BoldItalic</b></i></mono></justify>
]],"[\t]+","")

local function create_settings_formspec()
	local default_font_selection, mono_font_selection
	local default_font = minetest.settings:get("font_path")
	local mono_font = minetest.settings:get("mono_font_path")
	-- Get default font preset
	for i = 1, #default_fonts-2 do
		if match_font(default_font, default_fonts[i], "Regular") then
			default_font_selection = i
			break
		end
	end
	if default_font == mono_font then
		default_font_selection = #default_fonts-1
	end
	if force_default_fontpath_input or not default_font_selection then
		default_font_selection = #default_fonts
	end
	-- Get mono font preset
	for i = 1, #mono_fonts-1 do
		if match_font(mono_font, mono_fonts[i], "Regular") then
			mono_font_selection = i
			break
		end
	end
	if force_mono_fontpath_input or not mono_font_selection then
		mono_font_selection = #mono_fonts
	end
	local formspec = "size[12,5.4;true]" ..
			"label[0,0;" .. fgettext("Default font") .. "]" ..
			"dropdown[0,0.5;3,1;default_font_preset;" .. table.concat(default_fonts,",") .. ";" ..
				default_font_selection .. ";true]" ..
			"label[0,1.25;" .. fgettext("Monospace font") .. "]" ..
			"dropdown[0,1.75;3,1;mono_font_preset;" .. table.concat(mono_fonts,",") .. ";" ..
				mono_font_selection .. ";true]" ..
			"label[0,2.5;" .. fgettext("Font preview") .. "]" ..
			"box[0,3;11.75,1.5;#999999]" ..
			"hypertext[0.3,3;11.75,1.5;fonttest;" .. fonttest .. "]" ..
			"button[0,4.75;3.95,1;btn_back;" .. fgettext("< Back to Settings page") .. "]" ..
			"button[4,4.75;3.95,1;btn_update;" .. fgettext("Update") .. "]" ..
			"button[8,4.75;3.95,1;btn_reset;" .. fgettext("Restore Default") .. "]"
	if default_font_selection == #default_fonts then
		formspec = formspec .. "field[3.5,0.75;8.75,1;default_font_path;;" .. core.formspec_escape(default_font) .. "]"
	end
	if mono_font_selection == #mono_fonts then
		formspec = formspec .. "field[3.5,2;8.75,1;mono_font_path;;" .. core.formspec_escape(mono_font) .. "]"
	end
	return formspec
end

local function update_font_settings(prefix, regular, bold, italic, bolditalic)
	for k, v in pairs{
		[""] = regular,
		["_bold"] = bold,
		["_italic"] = italic,
		["_bold_italic"] = bolditalic,
	} do
		core.settings:set(string.format("%sfont_path%s", prefix, k), v)
	end
	core.settings:write()
end

local function generate_font_paths(fontname)
	local t = {}
	for i, v in pairs{"Regular", "Bold", "Italic", "BoldItalic"} do
		t[i] = "fonts" .. DIR_DELIM .. get_font_file_for(fontname, v)
	end
	return unpack(t)
end

local function is_valid_font_path(filepath)
	local basename = string.match(filepath, "^(.+)%.[Tt][Tt][Ff]$")
	if basename then
		return true
	else
		return false
	end
end

local function generate_font_paths_from_path(filepath)
	local basename, ext = string.match(filepath, "^(.+)(%.[^.]+)$")
	if not basename then
		return nil
	end
	local regular = basename
	if string.match(basename, "%-[Rr]egular") then
		basename = string.sub(basename, 1, -9)
	end
	return regular..ext, basename.."-Bold"..ext, basename.."-Italic"..ext, basename.."-BoldItalic"..ext
end

local function handle_settings_buttons(this, fields)
	local has_changes = false
	if fields["btn_update"] then
		force_default_fontpath_input = false
		force_mono_fontpath_input = false
		if fields["mono_font_path"] then
			local path = fields["mono_font_path"]
			if not is_valid_font_path(path) then
				this.data.error_message = fgettext("Invalid font path")
			else
				update_font_settings("mono_", generate_font_paths_from_path(path))
			end
		end
		if fields["default_font_path"] then
			sync_mono = false
			local path = fields["default_font_path"]
			if not is_valid_font_path(path) then
				this.data.error_message = fgettext("Invalid font path")
			else
				update_font_settings("", generate_font_paths_from_path(path))
			end
		end
		has_changes = true
	end
	if fields["mono_font_preset"] then
		local preset_index = tonumber(fields["mono_font_preset"])
		if preset_index and mono_fonts[preset_index] then
			if preset_index == #mono_fonts then
				force_mono_fontpath_input = true
			else
				force_mono_fontpath_input = false
				update_font_settings("mono_", generate_font_paths(mono_fonts[preset_index]))
			end
			has_changes = true
		end
	end
	if fields["default_font_preset"] then
		local preset_index = tonumber(fields["default_font_preset"])
		if preset_index and default_fonts[preset_index] then
			if preset_index == #default_fonts then
				sync_mono = false
				force_default_fontpath_input = true
			elseif preset_index == #default_fonts-1 then
				sync_mono = true
			else
				sync_mono = false
				force_default_fontpath_input = false
				update_font_settings("", generate_font_paths(default_fonts[preset_index]))
			end
			has_changes = true
		end
	end
	if sync_mono then
		force_default_fontpath_input = false
		for _, i in pairs{"", "_bold", "_italic", "_bold_italic"} do
			minetest.settings:set("font_path"..i, minetest.settings:get("mono_font_path"..i))
		end
	end
	if fields["btn_reset"] then
		force_default_fontpath_input = false
		force_mono_fontpath_input = false
		sync_mono = false
		for _, i in pairs{"", "mono_"} do
			for _, j in pairs{"", "_bold", "_italic", "_bold_italic"} do
				core.settings:remove(string.format("%sfont_path%s", i, j))
			end
		end
		has_changes = true
	end
	if has_changes then
		core.settings:write()
		core.update_formspec(this:get_formspec())
	end
	if fields["btn_back"] then
		this:delete()
		return true
	end
	return has_changes
end

function create_font_settings_dlg()
	return dialog_create("settings_font",
				create_settings_formspec,
				handle_settings_buttons,
				nil)
end
