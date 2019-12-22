-- CSM death formspec. Only used when clientside modding is enabled, otherwise
-- handled by the engine.

minetest.register_on_death(function()
	minetest.display_chat_message("You died.")
	local formspec = "size[11,5.5]bgcolor[#320000b4;true]" ..
		"label[4.85,1.35;" .. fgettext("You died") ..
		"]button_exit[4,3;3,0.5;btn_respawn;".. fgettext("Respawn") .."]"
	minetest.show_formspec("bultin:death", formspec)
end)

minetest.register_on_formspec_input(function(formname, fields)
	if formname == "bultin:death" then
		minetest.send_respawn()
	end
end)
