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

local open_windows = {}

local function build_window(id, param)
	local info = open_windows[id]
	if not info then
		return nil, nil
	end

	local window = info.builder(id, info.player, info.context, param or {})
	assert(core.is_instance(window, ui.Window),
			"Expected ui.Window to be returned from builder function")
	assert(not window._id, "Window object has already been returned")

	window._id = id
	info.window = window

	return window, info.player
end

ui.Window = core.class()

ui._window_types = {
	bg = 0,
	mask = 1,
	hud = 2,
	message = 3,
	gui = 4,
	fg = 5,
}

function ui.Window:new(props)
	self._id = nil -- Set by build_window()
	self._type = props.type

	self._theme = props.theme or ui.get_default_theme()
	self._style = props.style or ui.Style{}

	self._root = props.root

	assert(ui._window_types[self._type], "Invalid window type")
	assert(core.is_instance(self._root, ui.Root),
			"Expected root of window to be ui.Root")

	self._elems = self._root:_get_flat()
	self._elems_by_id = {}

	for _, elem in ipairs(self._elems) do
		local id = elem._id

		assert(not self._elems_by_id[id], "Element has duplicate ID: '" .. id .. "'")
		self._elems_by_id[id] = elem

		assert(elem._window == nil, "Element already has window")
		elem._window = self
	end
end

function ui.Window:_encode(player, opening)
	local enc_styles = self:_encode_styles()
	local enc_elems = self:_encode_elems()

	local data = ui._encode("ZzZ", enc_elems, self._root._id, enc_styles)
	if opening then
		data = ui._encode("ZB", data, ui._window_types[self._type])
	end

	return data
end

function ui.Window:_encode_styles()
	-- Clear out all the boxes in every element.
	for _, elem in ipairs(self._elems) do
		for box in pairs(elem._boxes) do
			elem._boxes[box] = {n = 0}
		end
	end

	-- Get a cascaded and flattened list of all the styles for this window.
	local styles = self:_get_full_style():_get_flat()

	-- Take each style and apply its properties to every box and state matched
	-- by its selector.
	self:_apply_styles(styles)

	-- Take the styled boxes and encode their styles into a single table,
	-- replacing the boxes' style property tables with indices into this table.
	local enc_styles = self:_index_styles()

	return ui._encode_array("Z", enc_styles)
end

function ui.Window:_get_full_style()
	-- The full style contains the theme, global style, and inline element
	-- styles as sub-styles, in that order, to ensure the correct precedence.
	local styles = {self._theme, self._style}

	for _, elem in ipairs(self._elems) do
		-- Cascade the inline style with the element's ID, ensuring that the
		-- inline style globally refers to this element only.
		table.insert(styles, ui.Style{
			sel = "#" .. elem._id,
			nested = {elem._style},
		})
	end

	-- Return all these styles wrapped up into a single style.
	return ui.Style{
		nested = styles,
	}
end

local function apply_style(elem, boxes, style)
	-- Loop through each box, applying the styles accordingly. The table of
	-- boxes may be empty, in which case nothing happens.
	for _, box in pairs(boxes) do
		local name = box.name or "main"

		-- If this style resets all properties, find all states that are a
		-- subset of the state being styled and clear their property tables.
		if style._reset then
			for i = ui._STATE_NONE, ui._NUM_STATES - 1 do
				if bit.band(box.states, i) == box.states then
					elem._boxes[name][i] = nil
				end
			end
		end

		-- Get the existing style property table for this box if it exists.
		local props = elem._boxes[name][box.states] or {}

		-- Cascade the properties from this style onto the box.
		elem._boxes[name][box.states] = ui._cascade_props(style._props, props)
	end
end

function ui.Window:_apply_styles(styles)
	-- Loop through each style and element and see if the style properties can
	-- be applied to any boxes.
	for _, style in ipairs(styles) do
		for _, elem in ipairs(self._elems) do
			-- Check if the selector for this style. If it matches, apply the
			-- style to each of the applicable boxes.
			local matches, boxes = style._sel(elem)
			if matches then
				apply_style(elem, boxes, style)
			end
		end
	end
end

local function index_style(box, i, style_indices, enc_styles)
	-- If we have a style for this state, serialize it to a string. Identical
	-- styles have identical strings, so we use this to our advantage.
	local enc = ui._encode_props(box[i])

	-- If we haven't serialized a style identical to this one before, store
	-- this as the latest index in the list of style strings.
	if not style_indices[enc] then
		style_indices[enc] = #enc_styles
		table.insert(enc_styles, enc)
	end

	-- Set the index of our state to the index of its style string, and keep
	-- count of how many states with valid indices we have for this box so far.
	box[i] = style_indices[enc]
	box.n = box.n + 1
end

function ui.Window:_index_styles()
	local style_indices = {}
	local enc_styles = {}

	for _, elem in ipairs(self._elems) do
		for _, box in pairs(elem._boxes) do
			for i = ui._STATE_NONE, ui._NUM_STATES - 1 do
				if box[i] then
					-- If this box has a style, encode and index it.
					index_style(box, i, style_indices, enc_styles)
				else
					-- Otherwise, this state has no style, so set it as such.
					box[i] = ui._NO_STYLE
				end
			end
		end
	end

	return enc_styles
end

function ui.Window:_encode_elems()
	local enc_elems = {}

	for _, elem in ipairs(self._elems) do
		table.insert(enc_elems, elem:_encode())
	end

	return ui._encode_array("Z", enc_elems)
end

local OPEN_WINDOW   = 0x00
local REOPEN_WINDOW = 0x01
local UPDATE_WINDOW = 0x02
local CLOSE_WINDOW  = 0x03

local last_id = 0

function ui.open(builder, player, context, param)
	local id = last_id
	last_id = last_id + 1

	open_windows[id] = {
		builder = builder,
		player = player,
		context = context or {},
		window = nil, -- Set by build_window()
	}

	local window = build_window(id, param)
	local data = ui._encode("BL Z", OPEN_WINDOW, id,
			window:_encode(player, true))

	core.send_ui_message(player, data)
	return id
end

function ui.reopen(close_id, param)
	local new_id = last_id
	last_id = last_id + 1

	open_windows[new_id] = open_windows[close_id]
	open_windows[close_id] = nil

	local window, player = build_window(new_id, param)
	if not window then
		return nil
	end

	local data = ui._encode("BLL Z", REOPEN_WINDOW, new_id, close_id,
			window:_encode(player, true))

	core.send_ui_message(player, data)
	return new_id
end

function ui.update(id, param)
	local window, player = build_window(id, param)
	if not window then
		return
	end

	local data = ui._encode("BL Z", UPDATE_WINDOW, id,
			window:_encode(player, false))

	core.send_ui_message(player, data)
end

function ui.close(id)
	local info = open_windows[id]
	if not info then
		return
	end

	local data = ui._encode("BL", CLOSE_WINDOW, id)

	core.send_ui_message(info.player, data)
	open_windows[id] = nil
end

function ui.get_window_info(id)
	local info = open_windows[id]
	if not info then
		return nil
	end

	-- Only return a subset of the fields that are relevant for the caller.
	return {
		builder = info.builder,
		player = info.player,
		context = info.context,
	}
end

function ui.get_open_windows()
	local ids = {}
	for id in pairs(open_windows) do
		table.insert(ids, id)
	end
	return ids
end
