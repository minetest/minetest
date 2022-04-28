--Minetest
--Copyright (C) 2013 sapier
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
-- TODO improve doc                                                           --
-- TODO code cleanup                                                          --
-- Generic implementation of a filter/sortable list                           --
-- Usage:                                                                     --
-- Filterlist needs to be initialized on creation. To achieve this you need to --
-- pass following functions:                                                  --
-- raw_fct() (mandatory):                                                     --
--     function returning a table containing the elements to be filtered      --
-- compare_fct(element1,element2) (mandatory):                                --
--     function returning true/false if element1 is same element as element2  --
-- uid_match_fct(element1,uid) (optional)                                     --
--     function telling if uid is attached to element1                        --
-- filter_fct(element,filtercriteria) (optional)                              --
--     function returning true/false if filtercriteria met to element         --
-- fetch_param (optional)                                                     --
--     parameter passed to raw_fct to aquire correct raw data                 --
--                                                                            --
--------------------------------------------------------------------------------
filterlist = {}

--------------------------------------------------------------------------------
function filterlist.refresh(self)
	self.m_raw_list = self.m_raw_list_fct(self.m_fetch_param)
	filterlist.process(self)
end

--------------------------------------------------------------------------------
function filterlist.create(raw_fct,compare_fct,uid_match_fct,filter_fct,fetch_param)

	assert((raw_fct ~= nil) and (type(raw_fct) == "function"))
	assert((compare_fct ~= nil) and (type(compare_fct) == "function"))

	local self = {}

	self.m_raw_list_fct  = raw_fct
	self.m_compare_fct   = compare_fct
	self.m_filter_fct    = filter_fct
	self.m_uid_match_fct = uid_match_fct

	self.m_filtercriteria = nil
	self.m_fetch_param = fetch_param

	self.m_sortmode = "none"
	self.m_sort_list = {}

	self.m_processed_list = nil
	self.m_raw_list = self.m_raw_list_fct(self.m_fetch_param)

	self.add_sort_mechanism = filterlist.add_sort_mechanism
	self.set_filtercriteria = filterlist.set_filtercriteria
	self.get_filtercriteria = filterlist.get_filtercriteria
	self.set_sortmode       = filterlist.set_sortmode
	self.get_list           = filterlist.get_list
	self.get_raw_list       = filterlist.get_raw_list
	self.get_raw_element    = filterlist.get_raw_element
	self.get_raw_index      = filterlist.get_raw_index
	self.get_current_index  = filterlist.get_current_index
	self.size               = filterlist.size
	self.uid_exists_raw     = filterlist.uid_exists_raw
	self.raw_index_by_uid   = filterlist.raw_index_by_uid
	self.refresh            = filterlist.refresh

	filterlist.process(self)

	return self
end

--------------------------------------------------------------------------------
function filterlist.add_sort_mechanism(self,name,fct)
	self.m_sort_list[name] = fct
end

--------------------------------------------------------------------------------
function filterlist.set_filtercriteria(self,criteria)
	if criteria == self.m_filtercriteria and
		type(criteria) ~= "table" then
		return
	end
	self.m_filtercriteria = criteria
	filterlist.process(self)
end

--------------------------------------------------------------------------------
function filterlist.get_filtercriteria(self)
	return self.m_filtercriteria
end

--------------------------------------------------------------------------------
--supported sort mode "alphabetic|none"
function filterlist.set_sortmode(self,mode)
	if (mode == self.m_sortmode) then
		return
	end
	self.m_sortmode = mode
	filterlist.process(self)
end

--------------------------------------------------------------------------------
function filterlist.get_list(self)
	return self.m_processed_list
end

--------------------------------------------------------------------------------
function filterlist.get_raw_list(self)
	return self.m_raw_list
end

--------------------------------------------------------------------------------
function filterlist.get_raw_element(self,idx)
	if type(idx) ~= "number" then
		idx = tonumber(idx)
	end

	if idx ~= nil and idx > 0 and idx <= #self.m_raw_list then
		return self.m_raw_list[idx]
	end

	return nil
end

--------------------------------------------------------------------------------
function filterlist.get_raw_index(self,listindex)
	assert(self.m_processed_list ~= nil)

	if listindex ~= nil and listindex > 0 and
		listindex <= #self.m_processed_list then
		local entry = self.m_processed_list[listindex]

		for i,v in ipairs(self.m_raw_list) do

			if self.m_compare_fct(v,entry) then
				return i
			end
		end
	end

	return 0
end

--------------------------------------------------------------------------------
function filterlist.get_current_index(self,listindex)
	assert(self.m_processed_list ~= nil)

	if listindex ~= nil and listindex > 0 and
		listindex <= #self.m_raw_list then
		local entry = self.m_raw_list[listindex]

		for i,v in ipairs(self.m_processed_list) do

			if self.m_compare_fct(v,entry) then
				return i
			end
		end
	end

	return 0
end

--------------------------------------------------------------------------------
function filterlist.process(self)
	assert(self.m_raw_list ~= nil)

	if self.m_sortmode == "none" and
		self.m_filtercriteria == nil then
		self.m_processed_list = self.m_raw_list
		return
	end

	self.m_processed_list = {}

	for k,v in pairs(self.m_raw_list) do
		if self.m_filtercriteria == nil or
			self.m_filter_fct(v,self.m_filtercriteria) then
			self.m_processed_list[#self.m_processed_list + 1] = v
		end
	end

	if self.m_sortmode == "none" then
		return
	end

	if self.m_sort_list[self.m_sortmode] ~= nil and
		type(self.m_sort_list[self.m_sortmode]) == "function" then

		self.m_sort_list[self.m_sortmode](self)
	end
end

--------------------------------------------------------------------------------
function filterlist.size(self)
	if self.m_processed_list == nil then
		return 0
	end

	return #self.m_processed_list
end

--------------------------------------------------------------------------------
function filterlist.uid_exists_raw(self,uid)
	for i,v in ipairs(self.m_raw_list) do
		if self.m_uid_match_fct(v,uid) then
			return true
		end
	end
	return false
end

--------------------------------------------------------------------------------
function filterlist.raw_index_by_uid(self, uid)
	local elementcount = 0
	local elementidx = 0
	for i,v in ipairs(self.m_raw_list) do
		if self.m_uid_match_fct(v,uid) then
			elementcount = elementcount +1
			elementidx = i
		end
	end


	-- If there are more elements than one with same name uid can't decide which
	-- one is meant. self shouldn't be possible but just for sure.
	if elementcount > 1 then
		elementidx=0
	end

	return elementidx
end

--------------------------------------------------------------------------------
-- COMMON helper functions                                                    --
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
function compare_worlds(world1,world2)
	if world1.path ~= world2.path then
		return false
	end

	if world1.name ~= world2.name then
		return false
	end

	if world1.gameid ~= world2.gameid then
		return false
	end

	return true
end

--------------------------------------------------------------------------------
function sort_worlds_alphabetic(self)

	table.sort(self.m_processed_list, function(a, b)
		--fixes issue #857 (crash due to sorting nil in worldlist)
		if a == nil or b == nil then
			if a == nil and b ~= nil then return false end
			if b == nil and a ~= nil then return true end
			return false
		end
		if a.name:lower() == b.name:lower() then
			return a.name < b.name
		end
		return a.name:lower() < b.name:lower()
	end)
end

--------------------------------------------------------------------------------
function sort_mod_list(self)
	-- Cache of lineages, each "lineage" being a list whose first item is
	-- a mod's highest parent and whose last item is the mod itself.
	local mod_lineages = {}
	for _, mod in ipairs(self.m_processed_list) do
		local lineage = {}
		mod_lineages[mod] = lineage
		repeat
			table.insert(lineage, 1, mod)
			mod = mod.modpack
		until mod == nil
	end

	-- Stabilize the sort to avoid issues with identically-named modpacks.
	local original_order = {}
	for i, mod in ipairs(self.m_processed_list) do
		original_order[mod] = i
	end

	table.sort(self.m_processed_list, function(a, b)
		if a == b then
			return false
		end

		-- Show game mods at bottom
		if a.type ~= b.type or a.loc ~= b.loc then
			if b.type == "game" then
				return a.loc ~= "game"
			end
			return b.loc == "game"
		end

		-- Find where the lineages diverge
		local a_lineage = mod_lineages[a]
		local b_lineage = mod_lineages[b]
		local a_divergence, b_divergence
		local i = 1
		repeat
			a_divergence = a_lineage[i]
			b_divergence = b_lineage[i]
			i = i + 1
		until a_divergence ~= b_divergence

		-- Compare the two where they diverge
		if not a_divergence then
			-- b is inside a
			return true
		elseif not b_divergence then
			-- a is inside b
			return false
		end
		if a_divergence.name:lower() < b_divergence.name:lower() then
			return true
		end
		if a_divergence.name < b_divergence.name then
			return true
		end
		if a_divergence.name > b_divergence.name then
			return false
		end
		return original_order[a_divergence] < original_order[b_divergence]
	end)
end
