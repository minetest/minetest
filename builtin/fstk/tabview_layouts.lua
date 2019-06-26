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


tabview_layouts = {}

tabview_layouts.tabs = {
	get = function(self, view)
		local formspec = ""
		if view.parent == nil then
			local tsize = view.tablist[view.last_tab_index].tabsize or
					{width=view.width, height=view.height}
			formspec = formspec ..
					string.format("size[%f,%f,%s]",tsize.width,tsize.height,
							dump(view.fixed_size))
		end
		formspec = formspec .. self:get_header(view)
		formspec = formspec ..
				view.tablist[view.last_tab_index].get_formspec(
						view,
						view.tablist[view.last_tab_index].name,
						view.tablist[view.last_tab_index].tabdata,
						view.tablist[view.last_tab_index].tabsize
				)

		return formspec
	end,

	get_header = function(self, view)
		local toadd = ""
		for i=1,#view.tablist do
			if toadd ~= "" then
				toadd = toadd .. ","
			end
			toadd = toadd .. view.tablist[i].caption
		end

		return string.format("tabheader[%f,%f;%s;%s;%i;true;false]",
				view.header_x, view.header_y, view.name, toadd, view.last_tab_index);
	end,

	handle_buttons = function(self, view, fields)
		--save tab selection to config file
		if fields[view.name] then
			local index = tonumber(fields[view.name])
			view:switch_to_tab(index)
			return true
		end

		return false
	end,
}


tabview_layouts.vertical = {
	get = function(self, view)
		local formspec = ""
		if view.parent == nil then
			local tsize = view.tablist[view.last_tab_index].tabsize or
					{width=view.width, height=view.height}
			formspec = formspec ..
					string.format("size[%f,%f,%s]real_coordinates[true]",tsize.width+3,tsize.height,
							dump(view.fixed_size))
		end
		formspec = formspec .. self:get_header(view)
		formspec = formspec .. "container[3,0]"
		formspec = formspec ..
				view.tablist[view.last_tab_index].get_formspec(
						view,
						view.tablist[view.last_tab_index].name,
						view.tablist[view.last_tab_index].tabdata,
						view.tablist[view.last_tab_index].tabsize
				)
		formspec = formspec .. "container_end[]"

		return formspec
	end,

	get_header = function(self, view)
		local icon     = view.icon
		local bgcolor  = view.bgcolor or "#ACACAC33"
		local selcolor = view.selcolor or "#ACACAC66"

		local tsize = view.tablist[view.last_tab_index].tabsize or
				{width=view.width, height=view.height}

		local fs = {
			("box[%f,%f;%f,%f;%s]"):format(0, 0, 3, tsize.height, bgcolor)
		}

		if icon then
			fs[#fs + 1] = "image[1,0.375;1,1;"
			fs[#fs + 1] = icon
			fs[#fs + 1] = "]"
		end

		for i = 1, #view.tablist do
			local tab = view.tablist[i]
			local name = "tab_" .. tab.name
			local y = (i - 1) * 0.8 + 2.25

			fs[#fs + 1] = "label[0.375,"
			fs[#fs + 1] = tonumber(y)
			fs[#fs + 1] = ";"
			fs[#fs + 1] = tab.caption
			fs[#fs + 1] = "]"

			fs[#fs + 1] = "style["
			fs[#fs + 1] = name
			fs[#fs + 1] = ";border=false]"

			fs[#fs + 1] = "button[0,"
			fs[#fs + 1] = tonumber(y - 0.4 )
			fs[#fs + 1] = ";3,0.8;"
			fs[#fs + 1] = name
			fs[#fs + 1] = ";]"

			if i == view.last_tab_index then
				fs[#fs + 1] = ("box[%f,%f;%f,%f;%s]"):format(0, y - 0.4, 3, 0.8, selcolor)
			end
		end

		print(defaulttexturedir .. "blank.png")

		return table.concat(fs, "")
	end,

	handle_buttons = function(self, view, fields)
		for i = 1, #view.tablist do
			local tab = view.tablist[i]
			if fields["tab_" .. tab.name] then
				view:switch_to_tab(i)
				return true
			end
		end

		return false
	end,
}


tabview_layouts.mainmenu = {
	backtxt = (defaulttexturedir or "") .. "back.png",

	get = function(self, view)
		local tab = view.tablist[view.last_tab_index]
		if tab then
			return self:get_with_back(view, tab)
		else
			return self:get_main_buttons(view)
		end
	end,

	get_with_back = function(self, view, tab)
		local backtxt = view.backtxt or self.backtxt

		local formspec = ""
		if view.parent == nil then
			local tsize = tab.tabsize or
					{width=view.width, height=view.height}
			formspec = formspec ..
					string.format("size[%f,%f,%s]real_coordinates[true]",tsize.width,tsize.height,
							dump(view.fixed_size))
		end
		formspec = formspec ..
				"style[main_back;noclip=true;border=false]image_button[-1.25,0;1,1;" .. backtxt .. ";main_back;]" ..
				tab.get_formspec(
						view,
						view.tablist[view.last_tab_index].name,
						view.tablist[view.last_tab_index].tabdata,
						view.tablist[view.last_tab_index].tabsize
				)

		return formspec
	end,

	get_main_buttons = function(self, view)
		local icon     = view.icon
		local bgcolor  = view.bgcolor or "#ACACAC33"
		local selcolor = view.selcolor or "#ACACAC66"

		local tsize = { width = 3, height = #view.tablist - 0.2}

		local fs = {
			("size[%f,%f]"):format(tsize.width, tsize.height),
			"real_coordinates[true]",
			"bgcolor[#00000000]",
			("box[%f,%f;%f,%f;%s]"):format(0, 0, 3, tsize.height, bgcolor)
		}

		for i = 1, #view.tablist do
			local tab = view.tablist[i]
			local name = "tab_" .. tab.name
			local y = i - 1

			fs[#fs + 1] = "button[0,"
			fs[#fs + 1] = tonumber(y)
			fs[#fs + 1] = ";3,0.8;"
			fs[#fs + 1] = name
			fs[#fs + 1] = ";"
			fs[#fs + 1] = minetest.formspec_escape(tab.caption)
			fs[#fs + 1] ="]"
		end

		return table.concat(fs, "")
	end,

	handle_buttons = function(self, view, fields)
		if fields.main_back then
			view:switch_to_tab(nil)
			return true
		end
		for i = 1, #view.tablist do
			local tab = view.tablist[i]
			if fields["tab_" .. tab.name] then
				view:switch_to_tab(i)
				return true
			end
		end

		return false
	end,
}
