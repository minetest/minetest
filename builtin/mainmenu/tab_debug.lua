		fs = fs .. "style[label_button2;border=false]" ..
			"button[0.1,6;5.3,1;label_button2;" ..
			fgettext("Active renderer:") .. "\n" ..
			core.formspec_escape(core.get_screen_info().render_info) .. "]"

		if PLATFORM == "Android" then
			fs = fs .. "button[0.5,5.1;4.5,0.8;share_debug;" .. fgettext("Share debug log") .. "]"
		else
			fs = fs .. "tooltip[userdata;" ..
					fgettext("Opens the directory that contains user-provided worlds, games, mods,\n" ..
							"and texture packs in a file manager / explorer.") .. "]"
			fs = fs .. "button[0.5,5.1;4.5,0.8;userdata;" .. fgettext("Open User Data Directory") .. "]"
		end
