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


--------------------------------------------------------------------------------
-- A tabview implementation                                                   --
-- Usage:                                                                     --
-- tabview.create: returns initialized tabview raw element                    --
-- element.add(tab): add a tab declaration                                    --
-- element.handle_buttons()                                                   --
-- element.handle_events()                                                    --
-- element.getFormspec() returns formspec of tabview                          --
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
local function add_tab(self,tab)
	assert(tab.size == nil or (type(tab.size) == table and
			tab.size.x ~= nil and tab.size.y ~= nil))
	assert(tab.show or tab.cbf_formspec ~= nil and type(tab.cbf_formspec) == "function")
	assert(tab.cbf_button_handler == nil or
			type(tab.cbf_button_handler) == "function")
	assert(tab.cbf_events == nil or type(tab.cbf_events) == "function")

	local newtab = {
		name = tab.name,
		caption = tab.caption,
		show = tab.show,
		button_handler = tab.cbf_button_handler,
		event_handler = tab.cbf_events,
		get_formspec = tab.cbf_formspec,
		tabsize = tab.tabsize,
		on_change = tab.on_change,
		tabdata = {},
	}

	self.tablist[#self.tablist + 1] = newtab

	if self.last_tab_index == #self.tablist then
		self.current_tab = tab.name
		if tab.on_activate ~= nil then
			tab.on_activate(nil,tab.name)
		end
	end
end

--------------------------------------------------------------------------------
local function get_formspec(self)
	if not self.hidden and (self.parent == nil or not self.parent.hidden) then
		return self.layout:get(self)
	else
		return ""
	end
end

--------------------------------------------------------------------------------
local function handle_buttons(self, fields)
	if self.hidden then
		return false
	end

	if self.layout:handle_buttons(self, fields) then
		return true
	end

	if self.glb_btn_handler ~= nil and
			self.glb_btn_handler(self,fields) then
		return true
	end

	local tab = self.tablist[self.last_tab_index]
	if tab and tab.button_handler ~= nil then
		return tab.button_handler(self, fields, tab.name, tab.tabdata)
	end

	return false
end

--------------------------------------------------------------------------------
local function handle_events(self, event)
	if self.hidden then
		return false
	end

	if self.glb_evt_handler ~= nil and
			self.glb_evt_handler(self,event) then
		return true
	end

	local tab = self.tablist[self.last_tab_index]
	if tab.evt_handler ~= nil then
		return tab.evt_handler(self, event, tab.name, tab.tabdata)
	end

	return false
end

--------------------------------------------------------------------------------
local function switch_to_tab(self, index)
	local old_tab = self.tablist[self.last_tab_index]
	local new_tab = self.tablist[index]

	if old_tab and old_tab.on_change ~= nil then
		old_tab.on_change("LEAVE", self.current_tab, new_tab and new_tab.name)
	end

	if new_tab and new_tab.show then
		new_tab.show(old_tab and old_tab.name, new_tab.name, self)
		return
	end

	--update tabview data
	self.last_tab_index = index
	self.current_tab = new_tab and new_tab.name

	if self.autosave_tab then
		core.settings:set(self.name .. "_LAST", self.current_tab or "")
	end

	if new_tab and new_tab.on_change then
		new_tab.on_change("ENTER", old_tab, self.current_tab)
	end
end

--------------------------------------------------------------------------------
local function set_tab_by_name(self, name)
	for i=1,#self.tablist,1 do
		if self.tablist[i].name == name then
			switch_to_tab(self, i)
			return true
		end
	end

	return false
end

--------------------------------------------------------------------------------
local function hide_tabview(self)
	self.hidden=true

	local tab = self.tablist[self.last_tab_index]
	--call on_change as we're not gonna show self tab any longer
	if tab and tab.on_change ~= nil then
		tab.on_change("LEAVE", self.current_tab, nil)
	end
end

--------------------------------------------------------------------------------
local function show_tabview(self)
	self.hidden=false

	-- call for tab to enter
	local tab = self.tablist[self.last_tab_index]
	if tab and tab.on_change ~= nil then
		tab.on_change("ENTER",
				nil,self.current_tab)
	end
end

local tabview_metatable = {
	add                       = add_tab,
	handle_buttons            = handle_buttons,
	handle_events             = handle_events,
	get_formspec              = get_formspec,
	show                      = show_tabview,
	hide                      = hide_tabview,
	delete                    = function(self) ui.delete(self) end,
	set_parent                = function(self,parent) self.parent = parent end,
	set_autosave_tab          =
	function(self,value) self.autosave_tab = value end,
	set_tab                   = set_tab_by_name,
	switch_to_tab             = switch_to_tab,
	set_global_button_handler =
	function(self,handler) self.glb_btn_handler = handler end,
	set_global_event_handler =
	function(self,handler) self.glb_evt_handler = handler end,
	set_fixed_size =
	function(self,state) self.fixed_size = state end,
}

tabview_metatable.__index = tabview_metatable

--------------------------------------------------------------------------------
function tabview_create(name, size, tabheaderpos, layout)
	local self = {}

	self.name     = name
	self.type     = "toplevel"
	self.width    = size.x
	self.height   = size.y
	self.header_x = tabheaderpos.x
	self.header_y = tabheaderpos.y
	self.bgcolor  = "#ACACAC33"
	self.selcolor = "#ACACAC66"
	self.layout   = layout or tabview_layouts.tabs

	setmetatable(self, tabview_metatable)

	self.fixed_size     = true
	self.hidden         = true
	self.current_tab    = nil
	self.last_tab_index = 1
	self.tablist        = {}

	self.autosave_tab   = false

	ui.add(self)
	return self
end
