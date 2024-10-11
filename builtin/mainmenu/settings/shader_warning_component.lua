--Minetest
--Copyright (C) 2024 grorp
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

return {
	query_text = "Shaders",
	requires = {
		shaders_support = true,
        shaders = false,
	},
    full_width = true,
	get_formspec = function(self, avail_w)
        local fs = {
            "box[0,0;", avail_w, ",1.2;", mt_color_orange, "]",
			"label[0.2,0.4;", fgettext("Shaders are disabled."), "]",
			"label[0.2,0.8;", fgettext("This is not a recommended configuration."), "]",
			"button[", avail_w - 2.2, ",0.2;2,0.8;fix_shader_warning;Enable]",
        }
		return table.concat(fs, ""), 1.2
	end,
	on_submit = function(self, fields)
		if fields.fix_shader_warning then
			core.settings:remove("enable_shaders") -- default value is true
			return true
		end
	end,
}
