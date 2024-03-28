--[[
Minetest
Copyright (C) 2024 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
--]]

ui.Root = ui._new_type(ui.Elem, "root", 0x01, false)

function ui.Root:new(props)
	ui.Elem.new(self, props)

	self._boxes.backdrop = true
end

function ui.Root:_encode_fields()
	local fl = ui._make_flags()

	self:_encode_box(fl, self._boxes.backdrop)

	return ui._encode("SZ", ui.Elem._encode_fields(self), ui._encode_flags(fl))
end
