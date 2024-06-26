local SIZE_TAG = "size[11,5.5,true]"

local function menu_formspec()
	local simple_singleplayer_mode = true
	local ypos = simple_singleplayer_mode and 0.7 or 0.1
	local fs = {
		"formspec_version[1]",
		SIZE_TAG
	}
	
	if not simple_singleplayer_mode then
		fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_change_password;%s]"):format(
			ypos, fgettext("Change Password"))
	else
		fs[#fs + 1] = "field[4.95,0;5,1.5;;" .. fgettext("Game paused") .. ";]"
	end
	ypos = ypos + 1
	
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_key_config;%s]"):format(
		ypos, fgettext("Controls"))
	ypos = ypos + 1
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_settings;%s]"):format(
		ypos, fgettext("Settings"))
	ypos = ypos + 1
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_exit_menu;%s]"):format(
		ypos, fgettext("Exit to Menu"))
	ypos = ypos + 1
	fs[#fs + 1] = ("button_exit[4,%f;3,0.5;btn_exit_os;%s]"):format(
		ypos, fgettext("Exit to OS"))
	ypos = ypos + 1
	
	return table.concat(fs, "")
end

function core.show_pause_menu()
	local fs = menu_formspec()
	
	minetest.show_formspec("MT_PAUSE_MENU", fs)
end
