--[[
Minetest
Copyright (C) 2023 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

ui._elem_types = {}

function ui._new_type(base, type, type_id, id_required)
	local class = core.class(base)

	class._type = type
	class._type_id = type_id
	class._id_required = id_required

	ui._elem_types[type] = class

	return class
end

function ui.derive_elem(base, type)
	assert(not ui._elem_types[type], "Derived element name already used")
	return ui._new_type(base, type, base._type_id, base._id_required)
end

ui.Elem = ui._new_type(nil, "elem", 0x00, false)

ui.Label = ui.derive_elem(ui.Elem, "label")
ui.Image = ui.derive_elem(ui.Elem, "image")

function ui.Elem:new(props)
	if self._id_required then
		assert(ui.is_id(props.id), "ID is required for '" .. self._type .. "'")
	end

	self._id = props.id or ui.new_id()
	self._groups = {}
	self._boxes = {main = true}
	self._style = props.style or ui.Style{props = props}

	self._children = table.merge(props.children or props)

	-- Set by parent ui.Elem
	self._parent = nil
	self._index  = nil
	self._rindex = nil

	-- Set by ui.Window
	self._window = nil

	assert(ui.is_id(self._id), "Element ID must be an ID string")

	for _, group in ipairs(props.groups or {}) do
		assert(ui.is_id(group), "Element group must be an ID string")
		self._groups[group] = true
	end

	for i, child in ipairs(self._children) do
		assert(child._parent == nil, "Element already has a parent")
		assert(not core.is_instance(child, ui.Root),
				"ui.Root can only be a root element")

		child._parent = self
		child._index = i
		child._rindex = #self._children - i + 1
	end
end

function ui.Elem:_get_flat()
	local elems = {self}
	for _, child in ipairs(self._children) do
		table.insert_all(elems, child:_get_flat())
	end
	return elems
end

function ui.Elem:_encode()
	return ui._encode("Bz S", self._type_id, self._id, self:_encode_fields())
end

function ui.Elem:_encode_fields()
	local fl = ui._make_flags()

	if ui._shift_flag(fl, #self._children > 0) then
		local child_ids = {}
		for i, child in ipairs(self._children) do
			child_ids[i] = child._id
		end

		ui._encode_flag(fl, "Z", ui._encode_array("z", child_ids))
	end

	self:_encode_box(fl, self._boxes.main)

	return ui._encode_flags(fl)
end

function ui.Elem:_encode_box(fl, box)
	-- Element encoding always happens after styles are computed and boxes are
	-- populated with style indices. So, if this box has any styles applied to
	-- it, encode the relevant states.
	if not ui._shift_flag(fl, box.n > 0) then
		return
	end

	local box_fl = ui._make_flags()

	-- For each state, check if there is any styling. If there is, add it
	-- to the box's flags.
	for i = ui._STATE_NONE, ui._NUM_STATES - 1 do
		if ui._shift_flag(box_fl, box[i] ~= ui._NO_STYLE) then
			ui._encode_flag(box_fl, "I", box[i])
		end
	end

	ui._encode_flag(fl, "s", ui._encode_flags(box_fl))
end
