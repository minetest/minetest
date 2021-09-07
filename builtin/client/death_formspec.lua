-- CSM death formspec. Only used when clientside modding is enabled, otherwise
-- handled by the engine.

core.register_on_death(function()
	local formspec = "size[11,5.5]bgcolor[#320000b4;true]" ..
		"label[4.85,1.35;" .. fgettext("You died") ..
		"]button_exit[4,3;3,0.5;btn_respawn;".. fgettext("Respawn") .."]"
	core.show_formspec("bultin:death", formspec)
end)

core.register_on_formspec_input(function(formname, fields)
	if formname == "bultin:death" then
		core.send_respawn()
	end
end)
