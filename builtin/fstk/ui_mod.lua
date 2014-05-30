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


local ui_registry = {}

--------------------------------------------------------------------------------
local function add(self, child)
	assert(child ~= nil)

	if (self.childlist[child.name] ~= nil) then
		return false
	end
	self.childlist[child.name] = child

	return child.name
end

--------------------------------------------------------------------------------
local function delete(self, child)
	if self.childlist[child.name] == nil then
		return false
	end

	self.childlist[child.name] = nil
	return true
end

--------------------------------------------------------------------------------
local function set_default(self, name)
	self.default = name
end

--------------------------------------------------------------------------------
local function find_by_name(self, name)
	return self.childlist[name]
end

--------------------------------------------------------------------------------
local function hide(self)
	for key,value in pairs(self.childlist) do
		if type(value.hide) == "function" then
			value:hide()
		end
	end
end

--------------------------------------------------------------------------------
local function update(self)
	local formspec = ""
	local active_toplevel_ui_elements = 0

	for key,value in pairs(self.childlist) do
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
		for key,value in pairs(self.childlist) do
			if (value.type == "addon") then
				local retval = value:get_formspec()

				if retval ~= nil and retval ~= "" then
					formspec = formspec .. retval
				end
			end
		end
	end

	if (active_toplevel_ui_elements == 0) then
		core.log("WARNING: not a single toplevel ui element active switching " ..
				"to default")
		self.childlist[self.default]:show()
		formspec = self.childlist[self.default]:get_formspec()
	end

	core.show_formspec(self.playername, self.formname, formspec)
end

--------------------------------------------------------------------------------
local ui_metatable = {
	add          = add,
	delete       = delete,
	set_default  = set_default,
	find_by_name = find_by_name,
	hide         = hide,
	update       = update
}

ui_metatable.__index = ui_metatable

--------------------------------------------------------------------------------
function create_ui(playername, unique_id)

	if (ui_registry[playername] == nil) then
		ui_registry[playername] = {}
	end

	if ui_registry[playername][unique_id] ~= nil then
		return nil
	end

	local self = {}

	setmetatable(self, ui_metatable)

	ui_registry[playername][unique_id] = self

	self.childlist  = {}
	self.default    = nil
	self.formname   = unique_id
	self.playername = playername

	return self
end

--------------------------------------------------------------------------------
function get_ui_by_unique_id(playername, unique_id)

	if (ui_registry[playername] == nil) then
		return nil
	end

	return ui_registry[playername][unique_id]
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Internal functions not to be called from user
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
local function handle_buttons(player, formname, fields)

	if not player:is_player() then
		return
	end

	local playername = player:get_player_name()

	if playername == nil or
		ui_registry[playername] == nil then
		return
	end

	if ui_registry[playername][formname] == nil then
		return
	end

	for key,value in pairs(ui_registry[playername][formname].childlist) do

		local retval = value:handle_buttons(fields)

		if retval then
			ui_registry[playername][formname]:update()
			return
		end
	end
end


--------------------------------------------------------------------------------
local function player_leave(player)

	if not player:is_player() then
		return
	end

	local playername = player:get_player_name()

	if playername == nil then
		return
	end

	ui_registry[playername] = nil
end

--------------------------------------------------------------------------------

minetest.register_on_player_receive_fields(handle_buttons)
minetest.register_on_leaveplayer(player)
