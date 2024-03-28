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

local next_id = 0

function ui.new_id()
	-- Just increment a monotonic counter and return it as hex. Even at
	-- unreasonably fast ID generation rates, it would take years for this
	-- counter to hit the 2^53 limit and start generating duplicates.
	next_id = next_id + 1
	return string.format("_%X", next_id)
end

ui._ID_CHARS = "a-zA-Z0-9_%-%:"

function ui.is_id(str)
	return type(str) == "string" and str == str:match("^[" .. ui._ID_CHARS .. "]+$")
end

-- This coordinate size calculation copies the one for fixed-size formspec
-- coordinates in guiFormSpecMenu.cpp.
function ui.get_coord_size()
	return math.floor(0.5555 * 96)
end

function ui._apply_bool(add, prop)
	if add ~= nil then
		return add
	end
	return prop
end

ui._encode = core.encode_network
ui._decode = core.decode_network

function ui._encode_array(format, arr)
	local formatted = {}
	for _, val in ipairs(arr) do
		table.insert(formatted, ui._encode(format, val))
	end

	return ui._encode("IZ", #formatted, table.concat(formatted))
end

function ui._pack_flags(...)
	local flags = 0
	for _, flag in ipairs({...}) do
		flags = bit.bor(bit.lshift(flags, 1), flag and 1 or 0)
	end
	return flags
end

function ui._make_flags()
	return {flags = 0, num_flags = 0, data = {}}
end

function ui._shift_flag(fl, flag)
	-- OR the LSB with the condition, and then right rotate it to the MSB.
	fl.flags = bit.ror(bit.bor(fl.flags, flag and 1 or 0), 1)
	fl.num_flags = fl.num_flags + 1

	return flag
end

function ui._encode_flag(fl, ...)
	table.insert(fl.data, ui._encode(...))
end

function ui._encode_flags(fl)
	-- We've been shifting into the right the entire time, so flags are in the
	-- upper bits; however, the protocol expects them to be in the lower bits.
	-- So, shift them the appropriate amount into the lower bits.
	local adjusted = bit.rshift(fl.flags, 32 - fl.num_flags)
	return ui._encode("I", adjusted) .. table.concat(fl.data)
end
