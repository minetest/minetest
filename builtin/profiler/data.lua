--Minetest
--Copyright (C) 2016-2018 T4im
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

local profiler = ...

local setmetatable, rawset, rawget = setmetatable, rawset, rawget
local type, tostring = type, tostring
local huge, pairs = math.huge, pairs

--
-- Localized table ref to profiler.data.ins_total
-- the total statistics over all instruments of the current profile session
local ins_total
--------------------------------------------------------------------------------

---
-- a collection of statistics gained through instrument sampling
local Stats = {
	get_mean = function(self, key)
		return self[key] and self[key] / self.samples
	end,

	get_use = function(self)
		return self.samples / ins_total.samples
	end,

	---
	-- @return a serializable table copy
	export = function(self)
		local copy = {}
		for key, value in pairs(self) do
			-- Stats of mod-instruments can contain additional indexed "sub stats" references
			if type(key) == "number" then
				copy.includes = copy.includes or {}
				copy.includes[key] = tostring(value.instrument)
			else
				copy[key] = value
			end
		end
		if copy.instrument then
			-- the instrument can contain a reference to the instrumented function
			-- which cannot be serialized by json
			copy.instrument = tostring(copy.instrument)
		end
		return copy
	end
}
Stats.__index = Stats

--------------------------------------------------------------------------------

local Data = {}
Data.__index = Data

---
-- A set of Stats.
--
-- This is not a class, but a special purpose table in the structure of
-- dataset_instance = { [instrument] = stats }
-- With stats being automatically initialized on demand
--
-- By not using any methods in this object we can make assumptions and
-- skip some tests in __index for performance
local DataSet = {}

---
-- Return a requested Stats table.
-- Initialize if it doesn't exist yet.
DataSet.__index = function(table, ins)
	local new = setmetatable({
		instrument = ins,
		samples = 0,
		call_all = 0,
		time_min = huge,
		time_max = 0,
		time_all = 0,
	}, Stats)
	rawset(table, ins, new)

	local mod = rawget(ins, "mod")
	if mod then
		-- No rawget here, since we want it initialized.
		local mod_stat = table[mod]
		-- Then add to the mod's array.
		mod_stat[#mod_stat + 1] = new
	end
	return new
end

---
-- @return a serializable table copy of the entire data state, optionally filtered
function Data:export(filter)
	local dataset = {}
	for instrument, stats in pairs(self.ins_stats) do
		local name = tostring(instrument)
		if not filter or name:match(filter) then
			dataset[name] = stats:export()
		end
	end

	return {
		dataset = dataset,
		stats_total = self.ins_total:export()
	}
end

function Data.reset()
	local old_data = profiler.data

	if old_data then
		for instrument in pairs(old_data.ins_stats) do
			-- Instruments are recycled.
			instrument:reset()
		end
	end

	profiler.data = setmetatable({
		ins_stats = setmetatable({}, DataSet),
		-- Current stats over all mods.
		ins_total = setmetatable({
			samples = 0,
			time_min = huge,
			time_max = 0,
			time_all = 0,
		}, Stats),
	}, Data)
	ins_total = profiler.data.ins_total

	return old_data
end

return Data
