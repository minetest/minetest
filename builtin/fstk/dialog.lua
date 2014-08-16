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

local function dialog_event_handler(self,event)
	if self.user_eventhandler == nil or
		self.user_eventhandler(event) == false then

		--close dialog on esc
		if event == "MenuQuit" then
			self:delete()
			return true
		end
	end
end

local dialog_metatable = {
	eventhandler = dialog_event_handler,
	get_formspec = function(self)
				if not self.hidden then return self.formspec(self.data) end
			end,
	handle_buttons = function(self,fields)
				if not self.hidden then return self.buttonhandler(self,fields) end
			end,
	handle_events  = function(self,event)
				if not self.hidden then return self.eventhandler(self,event) end
			end,
	hide = function(self) self.hidden = true end,
	show = function(self) self.hidden = false end,
	delete = function(self)
			if self.parent ~= nil then
				self.parent:show()
			end

			if self.parent_ui ~= nil then
				self.parent_ui:delete(self)
			end
		end,
	set_parent = function(self,parent) self.parent = parent end
}
dialog_metatable.__index = dialog_metatable

function dialog_create(name, get_formspec, buttonhandler, eventhandler,
		parent_ui)
	local self = {}

	self.name      = name
	self.type      = "toplevel"
	self.hidden    = true
	self.data      = {}
	self.parent_ui = parent_ui

	self.formspec           = get_formspec
	self.buttonhandler      = buttonhandler
	self.user_eventhandler  = eventhandler

	setmetatable(self,dialog_metatable)

	if self.parent_ui ~= nil then
		self.parent_ui:add(self)
	end

	return self
end
