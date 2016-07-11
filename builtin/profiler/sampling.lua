--Minetest
--Copyright (C) 2016 T4im
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
local pairs = pairs
local core = core

local profiler = ...

---
--localized refs to the dataset and stats of the current running profile state
local ins_stats, ins_total

---
-- the time that was logged in this sample
-- will be reseted and then updated during a sample
local sample_logged_time = 0

---
-- a set of all mod instruments that we found during sampling that had something logged to them
-- we store it as upvalue to the sample function
-- because we recycle the table instead of reinitializing it each time for performance
local logged_mods = {}

---
-- a set of all instruments that had something logged to them
-- and thus need to be sampled
-- filled via instrumentation
local logged_instruments = {}

local sampler = {
	logged_instruments = logged_instruments
}

---
-- reset sampler state
-- called on profiler.reset and during sampler init
function sampler.reset()
	sample_logged_time = 0

	-- use the most current profile state
	local data = profiler.data
	ins_stats = data.ins_stats
	ins_total = data.ins_total
end

---
-- Update a Stats table
-- @see data.lua/Stats
--
local function update_statistic(stats_table, time)
	stats_table.samples = stats_table.samples + 1

	-- Update absolute time (µs) spend by the subject
	if stats_table.time_max < time then
		stats_table.time_max = time
	end
	if stats_table.time_min > time then
		stats_table.time_min = time
	end
	stats_table.time_all = stats_table.time_all + time

	-- Update relative time (%) of this sample spend by the subject
	local current_part = (time / sample_logged_time) * 100
	if stats_table.part_max < current_part then
		stats_table.part_max = current_part
	end
	if stats_table.part_min > current_part then
		stats_table.part_min = current_part
	end
	stats_table.part_all = stats_table.part_all + current_part
end

---
-- Sample all logged measurements each server step.
-- Like any globalstep function, this should not be too heavy,
-- but it does not cause any instrumentation overhead.
--
local function sample(dtime)
	sample_logged_time = 0
	for instrument in pairs(logged_instruments) do
		local mod = instrument.mod
		logged_mods[mod] = true

		local time = instrument.logged_time
		mod.logged_time = mod.logged_time + time
		-- Accumulate total logged time of this sample for total stats calculations
		sample_logged_time = sample_logged_time + time

		-- Update time of this sample spend by the instrumented function.
		update_statistic(ins_stats[instrument], time)

		-- Reset logged data for the next sample.
		instrument.logged_time = 0
		logged_instruments[instrument] = nil
	end

	for mod in pairs(logged_mods) do
		-- Update time of this sample spend by this mod.
		update_statistic(ins_stats[mod], mod.logged_time)

		-- Reset logged data for the next sample.
		mod.logged_time = 0
		logged_mods[mod] = nil
	end

	-- Rare, but happens (for example when loading) and is of no value.
	-- so skip updating total stats when nothing was logged; particularly time_min and sample count
	if sample_logged_time == 0 then
		return
	end

	-- Update the total time spend over all mods.
	if ins_total.time_max < sample_logged_time then
		ins_total.time_max = sample_logged_time
	end
	if ins_total.time_min > sample_logged_time then
		ins_total.time_min = sample_logged_time
	end
	ins_total.time_all = ins_total.time_all + sample_logged_time

	ins_total.samples = ins_total.samples + 1
end

---
-- Setup empty profile and register the sampling function
--
function sampler.init()
	sampler.reset()

	if core.settings:get_bool("instrument.profiler") then
		core.register_globalstep(function()
			if sample_logged_time == 0 then
				return
			end
			return profiler.empty_instrument()
		end)
		core.register_globalstep(profiler.instrument {
			func = sample,
			mod = "*profiler*",
			label = "Sampler (update stats)",
		})
	else
		core.register_globalstep(sample)
	end
end

return sampler
