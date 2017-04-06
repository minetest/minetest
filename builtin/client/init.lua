-- Minetest: builtin/client/init.lua
local scriptpath = core.get_builtin_path()..DIR_DELIM
local clientpath = scriptpath.."client"..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM

dofile(clientpath .. "register.lua")
dofile(commonpath .. "after.lua")
dofile(commonpath .. "chatcommands.lua")
dofile(clientpath .. "chatcommands.lua")
dofile(commonpath .. "vector.lua")

core.register_on_death(function()
	core.display_chat_message("You died.")
	local formspec = "size[11,5.5]bgcolor[#320000b4;true]" ..
	"label[4.85,1.35;" .. fgettext("You died.") .. "]button_exit[4,3;3,0.5;btn_respawn;".. fgettext("Respawn") .."]"
	core.show_formspec("bultin:death", formspec)
end)

core.register_on_formspec_input(function(formname, fields)
	if formname == "bultin:death" then
		core.send_respawn()
	end
end)
