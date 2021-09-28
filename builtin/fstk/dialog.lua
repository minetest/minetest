--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--this program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
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
			ui.delete(self)
		end,
	set_parent = function(self,parent) self.parent = parent end
}
dialog_metatable.__index = dialog_metatable

function dialog_create(name,get_formspec,buttonhandler,eventhandler)
	local self = {}

	self.name = name
	self.type = "toplevel"
	self.hidden = true
	self.data = {}

	self.formspec      = get_formspec
	self.buttonhandler = buttonhandler
	self.user_eventhandler  = eventhandler

	setmetatable(self,dialog_metatable)

	ui.add(self)
	return self
end

function messagebox(name, message)
	return dialog_create(name,
			function()
				return ([[
					formspec_version[3]
					size[8,3]
					textarea[0.375,0.375;7.25,1.2;;;%s]
					button[3,1.825;2,0.8;ok;%s]
				]]):format(message, fgettext("OK"))
			end,
			function(this, fields)
				if fields.ok then
					this:delete()
					return true
				end
			end,
			nil)
end
