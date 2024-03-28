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

ui.Style = core.class()

function ui.Style:new(props)
	self._sel = ui._parse_sel(props.sel or "*")
	self._props = ui._cascade_props(props.props or props, {})
	self._nested = table.merge(props.nested or props)
	self._reset = props.reset
end

function ui.Style:_get_flat()
	local flat_styles = {}
	self:_get_flat_impl(flat_styles, ui._parse_sel("*"))
	return flat_styles
end

function ui.Style:_get_flat_impl(flat_styles, parent_sel)
	-- Intersect our selector with our parent selector, resulting in a fully
	-- qualified selector.
	local full_sel = ui._intersect_sels({parent_sel, self._sel})

	-- Copy this style's properties into a new style with the full selector.
	local flat = ui.Style{
		reset = self._reset,
		props = self._props,
	}
	flat._sel = full_sel

	table.insert(flat_styles, flat)

	-- For each sub-style of this style, cascade it with our full selector and
	-- add it to the list of flat styles.
	for _, nested in ipairs(self._nested) do
		nested:_get_flat_impl(flat_styles, full_sel)
	end
end

local function cascade_layer(new, add, props, p)
	new[p.."_image"] = add[p.."_image"] or props[p.."_image"]
	new[p.."_fill"] = add[p.."_fill"] or props[p.."_fill"]
	new[p.."_tint"] = add[p.."_tint"] or props[p.."_tint"]

	new[p.."_source"] = add[p.."_source"] or props[p.."_source"]
	new[p.."_middle"] = add[p.."_middle"] or props[p.."_middle"]
	new[p.."_middle_scale"] = add[p.."_middle_scale"] or props[p.."_middle_scale"]

	new[p.."_frames"] = add[p.."_frames"] or props[p.."_frames"]
	new[p.."_frame_time"] = add[p.."_frame_time"] or props[p.."_frame_time"]
end

function ui._cascade_props(add, props)
	local new = {}

	new.size = add.size or props.size

	new.rel_pos = add.rel_pos or props.rel_pos
	new.rel_anchor = add.rel_anchor or props.rel_anchor
	new.rel_size = add.rel_size or props.rel_size

	new.margin = add.margin or props.margin
	new.padding = add.padding or props.padding

	cascade_layer(new, add, props, "bg")
	cascade_layer(new, add, props, "fg")

	new.fg_scale = add.fg_scale or props.fg_scale
	new.fg_halign = add.fg_halign or props.fg_halign
	new.fg_valign = add.fg_valign or props.fg_valign

	new.visible = ui._apply_bool(add.visible, props.visible)
	new.noclip = ui._apply_bool(add.noclip, props.noclip)

	return new
end

local halign_map = {left = 0, center = 1, right = 2}
local valign_map = {top = 0, center = 1, bottom = 2}
local spacing_map = {
	before = 0,
	after = 1,
	outside = 2,
	around = 3,
	between = 4,
	evenly = 5,
	remove = 6,
}

local function encode_layer(props, p)
	local fl = ui._make_flags()

	if ui._shift_flag(fl, props[p.."_image"]) then
		ui._encode_flag(fl, "z", props[p.."_image"])
	end
	if ui._shift_flag(fl, props[p.."_fill"]) then
		ui._encode_flag(fl, "I", core.colorspec_to_colorint(props[p.."_fill"]))
	end
	if ui._shift_flag(fl, props[p.."_tint"]) then
		ui._encode_flag(fl, "I", core.colorspec_to_colorint(props[p.."_tint"]))
	end

	if ui._shift_flag(fl, props[p.."_source"]) then
		ui._encode_flag(fl, "ffff", unpack(props[p.."_source"]))
	end
	if ui._shift_flag(fl, props[p.."_middle"]) then
		ui._encode_flag(fl, "ffff", unpack(props[p.."_middle"]))
	end
	if ui._shift_flag(fl, props[p.."_middle_scale"]) then
		ui._encode_flag(fl, "f", props[p.."_middle_scale"])
	end

	if ui._shift_flag(fl, props[p.."_frames"]) then
		ui._encode_flag(fl, "I", props[p.."_frames"])
	end
	if ui._shift_flag(fl, props[p.."_frame_time"]) then
		ui._encode_flag(fl, "I", props[p.."_frame_time"])
	end

	return fl
end

function ui._encode_props(props)
	local fl = ui._make_flags()

	if ui._shift_flag(fl, props.size) then
		ui._encode_flag(fl, "ff", unpack(props.size))
	end

	if ui._shift_flag(fl, props.rel_pos) then
		ui._encode_flag(fl, "ff", unpack(props.rel_pos))
	end
	if ui._shift_flag(fl, props.rel_anchor) then
		ui._encode_flag(fl, "ff", unpack(props.rel_anchor))
	end
	if ui._shift_flag(fl, props.rel_size) then
		ui._encode_flag(fl, "ff", unpack(props.rel_size))
	end

	if ui._shift_flag(fl, props.margin) then
		ui._encode_flag(fl, "ffff", unpack(props.margin))
	end
	if ui._shift_flag(fl, props.padding) then
		ui._encode_flag(fl, "ffff", unpack(props.padding))
	end

	local bg_fl = encode_layer(props, "bg")
	if ui._shift_flag(fl, bg_fl.flags ~= 0) then
		ui._encode_flag(fl, "s", ui._encode_flags(bg_fl))
	end
	local fg_fl = encode_layer(props, "fg")
	if ui._shift_flag(fl, fg_fl.flags ~= 0) then
		ui._encode_flag(fl, "s", ui._encode_flags(fg_fl))
	end

	if ui._shift_flag(fl, props.fg_scale) then
		ui._encode_flag(fl, "f", props.fg_scale)
	end
	if ui._shift_flag(fl, props.fg_halign) then
		ui._encode_flag(fl, "B", halign_map[props.fg_halign])
	end
	if ui._shift_flag(fl, props.fg_valign) then
		ui._encode_flag(fl, "B", valign_map[props.fg_valign])
	end

	if ui._shift_flag(fl, props.visible ~= nil) then
		ui._shift_flag(fl, props.visible)
	end
	if ui._shift_flag(fl, props.noclip ~= nil) then
		ui._shift_flag(fl, props.noclip)
	end

	return ui._encode("s", ui._encode_flags(fl))
end

local default_theme = ui.Style{}

function ui.get_default_theme()
	return default_theme
end

function ui.set_default_theme(theme)
	default_theme = theme
end
