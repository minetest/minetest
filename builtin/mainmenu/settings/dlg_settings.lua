local settingspath = core.get_builtin_path().."settings"

function core.get_mainmenu_path()
	return settingspath
end

defaulttexturedir = core.get_texturepath_share() .. DIR_DELIM .. "base" ..
	DIR_DELIM .. "pack" .. DIR_DELIM
dofile(settingspath .. DIR_DELIM .. "settingtypes.lua")
dofile(settingspath .. DIR_DELIM .. "gui_change_mapgen_flags.lua")
dofile(settingspath .. DIR_DELIM .. "gui_settings.lua")


function create_settings_dlg()
	settings.load()
	local dlg = dialog_create("dlg_settings", settings.get_formspec, settings.buttonhandler, settings.eventhandler)
	settings.is_dlg = true
	settings.page_id = settings.update_filtered_pages("")
	
	return dlg
end
