local function create_home_formspec(tabview, name, tabdata)
	local formspec = "size[20,12;true]" ..
			"bgcolor[#0000;true]" ..
			"image_button[3,0.3;4,4;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_internet_button.png;" ..
				"internet_button;;false;false;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_internet_button_pressed.png]" ..
			"label[4.5,4.1;Internet]" ..
			"image_button[8,0.3;4,4;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_lan_button.png;" ..
				"lan_button;;false;false;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_lan_button_pressed.png]" ..
			"label[9.7,4.1;LAN]" ..
			"image_button[13,0.3;4,4;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_settings_button.png;" ..
				"settings_button;;false;false;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_settings_button_pressed.png]" ..
			"label[14.5,4.1;Settings]"
			--[["image_button[8,7.3;4,4;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_singleplayer_button.png;" ..
				"singleplayer_button;;false;false;" ..
				defaulttexturedir .. DIR_DELIM .. "mainmenu_singleplayer_button_pressed.png]"]] ..
			"label[9.3,11.1;Singleplayer]"

	for i=1,#gamemgr.games,1 do
		local button_name = "singleplayer_button_" .. gamemgr.games[i].id
		
		local image = nil
		local text = nil
		local tooltip = core.formspec_escape(gamemgr.games[i].name)
		
		if gamemgr.games[i].menuicon_path ~= nil and
			gamemgr.games[i].menuicon_path ~= "" then
			image = core.formspec_escape(gamemgr.games[i].menuicon_path)
		else
		
			local part1 = gamemgr.games[i].id:sub(1,5)
			local part2 = gamemgr.games[i].id:sub(6,10)
			local part3 = gamemgr.games[i].id:sub(11)
			
			text = part1 .. "\n" .. part2
			if part3 ~= nil and
				part3 ~= "" then
				text = text .. "\n" .. part3
			end
		end

		local w = 2
		local h = 2
		local dx = 2.4
		local dy = 2.4
		local xi = math.floor((i - 1) / 2)
		local yi = i - math.floor(i / 2) * 2
		local games_div_2 = math.floor((#gamemgr.games + 1) / 2)
		local x0 = 10 - games_div_2 / 2 * dx + dx - w
		local y0 = 6.0
		local x = x0 + xi * dx
		local y = y0 + yi * dy

		if image == nil then image = "" end
		if text == nil then text = "" end
		if tooltip == nil then tooltip = "" end

		formspec = formspec ..
			string.format("image_button[%f,%f;%f,%f;%s;%s;%s;true;%s]tooltip[%s;%s]",
					x, y, w, h,
					image, button_name, text, false, button_name, tooltip)
	end

	return formspec
end

local function handle_home_buttons(this, fields, tabname, tabdata)
	if fields["internet_button"] then
		-- TODO
		return true
	end

	if fields["lan_button"] then
		-- TODO
		return true
	end

	if fields["settings_button"] then
		local settings_dialog = create_settings()
		settings_dialog:set_parent(this)
		this:hide()
		settings_dialog:show()
		ui.update()
		return true
	end

	for i=1,#gamemgr.games,1 do
		local button_name = "singleplayer_button_" .. gamemgr.games[i].id
		if fields[button_name] then
			local singleplayer_dialog = create_singleplayer()
			singleplayer_dialog:set_parent(this)
			this:hide()
			singleplayer_dialog:show()
			ui.update()

			singleplayer_set_game(i)
			return true
		end
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
