local function create_home_formspec(tabview, name, tabdata)
	local formspec = "size[20,12;true]" ..
			"bgcolor[#0000;true]" ..
			"image_button[3,3.5;4,4;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_internet_button.png;" ..
				"internet_button;;false;false;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_internet_button_pressed.png]" ..
			"label[4.5,7.5;Internet]" ..
			"image_button[8,3.5;4,4;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_singleplayer_button.png;" ..
				"singleplayer_button;;false;false;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_singleplayer_button_pressed.png]" ..
			"label[9.3,7.5;Singleplayer]" ..
			"image_button[13,3.5;4,4;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_lan_button.png;" ..
				"lan_button;;false;false;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_lan_button_pressed.png]" ..
			"label[14.7,7.5;LAN]"

	return formspec
end

local function handle_home_buttons(this, fields, tabname, tabdata)
	if fields["internet_button"] then
		-- TODO
		return true
	end

	if fields["singleplayer_button"] then
		local singleplayer_dialog = create_singleplayer()
		singleplayer_dialog:set_parent(this)
		this:hide()
		singleplayer_dialog:show()
		ui.update()
		return true
	end

	if fields["lan_button"] then
		-- TODO
		return true
	end

	return false
end

function create_home()
	local dlg = dialog_create("home",
				create_home_formspec,
				handle_home_buttons,
				nil)

	mm_texture.update(nil, nil)

	dlg.delete = function(self)
		core.close()
	end

	return dlg
end
