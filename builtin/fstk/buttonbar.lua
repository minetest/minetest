--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


local function buttonbar_formspec(self)

	if self.hidden then
		return ""
	end

	local formspec = string.format("box[%f,%f;%f,%f;%s]",
			self.pos.x,self.pos.y ,self.size.x,self.size.y,self.bgcolor)

	for i=self.startbutton,#self.buttons,1 do
		local btn_name = self.buttons[i].name
		local btn_pos = {}

		if self.orientation == "horizontal" then
			btn_pos.x = self.pos.x + --base pos
			(i - self.startbutton) * self.btn_size +       --button offset
			self.btn_initial_offset
		else
			btn_pos.x = self.pos.x + (self.btn_size * 0.05)
		end

		if self.orientation == "vertical" then
			btn_pos.y = self.pos.y + --base pos
			(i - self.startbutton) * self.btn_size +       --button offset
			self.btn_initial_offset
		else
			btn_pos.y = self.pos.y + (self.btn_size * 0.05)
		end

		if (self.orientation == "vertical" and
			(btn_pos.y + self.btn_size <= self.pos.y + self.size.y)) or
			(self.orientation == "horizontal" and
			(btn_pos.x + self.btn_size <= self.pos.x + self.size.x)) then

		local borders="true"

		if self.buttons[i].image ~= nil then
			borders="false"
		end

		formspec = formspec ..
			string.format("image_button[%f,%f;%f,%f;%s;%s;%s;true;%s]tooltip[%s;%s]",
					btn_pos.x, btn_pos.y, self.btn_size, self.btn_size,
					self.buttons[i].image, btn_name, self.buttons[i].caption,
					borders, btn_name, self.buttons[i].tooltip)
		else
			--print("end of displayable buttons: orientation: " .. self.orientation)
			--print( "button_end: " .. (btn_pos.y + self.btn_size - (self.btn_size * 0.05)))
			--print( "bar_end: " .. (self.pos.x + self.size.x))
			break
		end
	end

	if (self.have_move_buttons) then
		local btn_dec_pos = {}
		btn_dec_pos.x = self.pos.x + (self.btn_size * 0.05)
		btn_dec_pos.y = self.pos.y + (self.btn_size * 0.05)
		local btn_inc_pos = {}
		local btn_size = {}

		if self.orientation == "horizontal" then
			btn_size.x = 0.5
			btn_size.y = self.btn_size
			btn_inc_pos.x = self.pos.x + self.size.x - 0.5
			btn_inc_pos.y = self.pos.y + (self.btn_size * 0.05)
		else
			btn_size.x = self.btn_size
			btn_size.y = 0.5
			btn_inc_pos.x = self.pos.x + (self.btn_size * 0.05)
			btn_inc_pos.y = self.pos.y + self.size.y - 0.5
		end

		local text_dec = "<"
		local text_inc = ">"
		if self.orientation == "vertical" then
			text_dec = "^"
			text_inc = "v"
		end

		formspec = formspec ..
			string.format("image_button[%f,%f;%f,%f;;btnbar_dec_%s;%s;true;true]",
					btn_dec_pos.x, btn_dec_pos.y, btn_size.x, btn_size.y,
					self.name, text_dec)

		formspec = formspec ..
			string.format("image_button[%f,%f;%f,%f;;btnbar_inc_%s;%s;true;true]",
					btn_inc_pos.x, btn_inc_pos.y, btn_size.x, btn_size.y,
					 self.name, text_inc)
	end

	return formspec
end

local function buttonbar_buttonhandler(self, fields)

	if fields["btnbar_inc_" .. self.name] ~= nil and
		self.startbutton < #self.buttons then

		self.startbutton = self.startbutton + 1
		return true
	end

	if fields["btnbar_dec_" .. self.name] ~= nil and self.startbutton > 1 then
		self.startbutton = self.startbutton - 1
		return true
	end

	for i=1,#self.buttons,1 do
		if fields[self.buttons[i].name] ~= nil then
			return self.userbuttonhandler(fields)
		end
	end
end

local buttonbar_metatable = {
	handle_buttons = buttonbar_buttonhandler,
	handle_events  = function(self, event) end,
	get_formspec   = buttonbar_formspec,

	hide = function(self) self.hidden = true end,
	show = function(self) self.hidden = false end,

	delete = function(self) ui.delete(self) end,

	add_button = function(self, name, caption, image, tooltip)
			if caption == nil then caption = "" end
			if image == nil then image = "" end
			if tooltip == nil then tooltip = "" end

			self.buttons[#self.buttons + 1] = {
				name = name,
				caption = caption,
				image = image,
				tooltip = tooltip
			}
			if self.orientation == "horizontal" then
				if ( (self.btn_size * #self.buttons) + (self.btn_size * 0.05 *2)
					> self.size.x ) then

					self.btn_initial_offset = self.btn_size * 0.05 + 0.5
					self.have_move_buttons = true
				end
			else
				if ((self.btn_size * #self.buttons) + (self.btn_size * 0.05 *2)
					> self.size.y ) then

					self.btn_initial_offset = self.btn_size * 0.05 + 0.5
					self.have_move_buttons = true
				end
			end
		end,

	set_bgparams = function(self, bgcolor)
			if (type(bgcolor) == "string") then
				self.bgcolor = bgcolor
			end
		end,
}

buttonbar_metatable.__index = buttonbar_metatable

function buttonbar_create(name, cbf_buttonhandler, pos, orientation, size)
	assert(name ~= nil)
	assert(cbf_buttonhandler ~= nil)
	assert(orientation == "vertical" or orientation == "horizontal")
	assert(pos ~= nil and type(pos) == "table")
	assert(size ~= nil and type(size) == "table")

	local self = {}
	self.name = name
	self.type = "addon"
	self.bgcolor = "#000000"
	self.pos = pos
	self.size = size
	self.orientation = orientation
	self.startbutton = 1
	self.have_move_buttons = false
	self.hidden = false

	if self.orientation == "horizontal" then
			self.btn_size = self.size.y
	else
			self.btn_size = self.size.x
	end

	if (self.btn_initial_offset == nil) then
		self.btn_initial_offset = self.btn_size * 0.05
	end

	self.userbuttonhandler = cbf_buttonhandler
	self.buttons = {}

	setmetatable(self,buttonbar_metatable)

	ui.add(self)
	return self
end
