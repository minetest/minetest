--Minetest
--Copyright (C) 2014 sapier
--
--self program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--self program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with self program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

ui = {}
ui.childlist = {}
ui.default = nil

--------------------------------------------------------------------------------
function ui.add(child)
	--TODO check child
	ui.childlist[child.name] = child
	
	return child.name
end

--------------------------------------------------------------------------------
function ui.delete(child)

	if ui.childlist[child.name] == nil then
		return false
	end
	
	ui.childlist[child.name] = nil
	return true
end

--------------------------------------------------------------------------------
function ui.set_default(name)
	ui.default = name
end

--------------------------------------------------------------------------------
function ui.find_by_name(name)
	return ui.childlist[name]
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Internal functions not to be called from user
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
function ui.update()
	local formspec = ""

	-- handle errors
	if gamedata ~= nil and gamedata.errormessage ~= nil then
		formspec = "size[12,3.2]" ..
			"textarea[1,1;10,2;;ERROR: " ..
			core.formspec_escape(gamedata.errormessage) ..
			";]"..
			"button[4.5,2.5;3,0.5;btn_error_confirm;" .. fgettext("Ok") .. "]"
	else
		local active_toplevel_ui_elements = 0
		for key,value in pairs(ui.childlist) do
			if (value.type == "toplevel") then
				local retval = value:get_formspec()

				if retval ~= nil and retval ~= "" then
					active_toplevel_ui_elements = active_toplevel_ui_elements +1
					formspec = formspec .. retval
				end
			end
		end
		
		-- no need to show addons if there ain't a toplevel element
		if (active_toplevel_ui_elements > 0) then
			for key,value in pairs(ui.childlist) do
				if (value.type == "addon") then
					local retval = value:get_formspec()

					if retval ~= nil and retval ~= "" then
						formspec = formspec .. retval
					end
				end
			end
		end

		if (active_toplevel_ui_elements > 1) then
			print("WARNING: ui manager detected more then one active ui element, self most likely isn't intended")
		end

		if (active_toplevel_ui_elements == 0) then
			print("WARNING: not a single toplevel ui element active switching to default")
			ui.childlist[ui.default]:show()
			formspec = ui.childlist[ui.default]:get_formspec()
		end
	end
	core.update_formspec(formspec)
end

--------------------------------------------------------------------------------
function ui.handle_buttons(fields)

	if fields["btn_error_confirm"] then
		gamedata.errormessage = nil
		update_menu()
		return
	end

	for key,value in pairs(ui.childlist) do

		local retval = value:handle_buttons(fields)

		if retval then
			ui.update()
			return
		end
	end
end


--------------------------------------------------------------------------------
function ui.handle_events(event)
	
	for key,value in pairs(ui.childlist) do

		if value.handle_events ~= nil then
			local retval = value:handle_events(event)

			if retval then
				return retval
			end
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- initialize callbacks
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
core.button_handler = function(fields)
	if fields["btn_error_confirm"] then
		gamedata.errormessage = nil
		ui.update()
		return
	end

	if ui.handle_buttons(fields) then
		ui.update()
	end
end

--------------------------------------------------------------------------------
core.event_handler = function(event)
	if ui.handle_events(event) then
		ui.update()
		return
	end

	if event == "Refresh" then
		ui.update()
		return
	end
end
