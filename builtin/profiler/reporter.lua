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

local DIR_DELIM, LINE_DELIM = DIR_DELIM, "\n"
local table, unpack, string, pairs, io, os = table, unpack, string, pairs, io, os
local rep, sprintf = string.rep, string.format
local tonumber, min = tonumber, math.min
local core, settings = core, core.settings
local reporter = {}

---
-- Shorten a string. End on an ellipsis if shortened.
--
local function shorten(str, length)
	if str and str:len() > length then
		return "..." .. str:sub(-(length-3))
	end
	return str
end

local function filter_matches(filter, text)
	return not filter or string.match(text, filter)
end

local function format_number(number, fmt)
	number = tonumber(number)
	if not number then
		return "N/A"
	end
	return sprintf(fmt or "%d", number)
end

local function tolabel(instrument)
	local class, label = instrument.class, instrument.label
	if class == "abm" or class == "lbm" then
		return sprintf("%s: %s", class:upper(), label)
	elseif class == "chatcommand" then
		return "/" .. label
	end
	return label
end

local Formatter = {
	new = function(self, object)
		object = object or {}
		object.out = {} -- output buffer
		self.__index = self
		return setmetatable(object, self)
	end,

	__tostring = function(self)
		return table.concat(self.out, LINE_DELIM)
	end,

	print = function(self, text, ...)
		if (...) then
			text = sprintf(text, ...)
		end

		if text then
			-- Avoid format unicode issues.
			text = text:gsub("Ms", "µs")
		end

		table.insert(self.out, text or LINE_DELIM)
	end,

	flush = function(self)
		table.insert(self.out, LINE_DELIM)
		local text = table.concat(self.out, LINE_DELIM)
		self.out = {}
		return text
	end
}

---
-- the maximal width the first column is going to expand to
local max_col1_width = 60

local TxtFormatter = Formatter:new {
	update = function(self)
		-- initial widths of the columns in characters
		-- the first column will expand as needed up to max_fcol_width
		self.widths = self.widths or { 30, 5, 5, 9, 9, 9 }
		local widths = self.widths

		local col1_width = widths[1]
		if col1_width < max_col1_width then
			-- look through already collected stats
			-- and update column widths to meet the requirements
			for instrument in pairs(self.profile.ins_stats) do
				local label_len = tolabel(instrument):len() + 5
				if label_len > col1_width then
					widths[1] = min(label_len, max_col1_width)
				end
			end
		end

		self.txt_row_format = sprintf(" %%-%ds" .. rep(" | %%%ds", #widths - 1), unpack(widths))

		local HR = {}
		for i=1, #widths do
			HR[i]= rep("-", widths[i])
		end
		-- ' | ' should break less with github than '-+-', when people are pasting there
		self.HR = sprintf("-%s-", table.concat(HR, " | "))
	end,

	format_row = function(self, modname, stats, instrument)
		instrument = instrument or stats.instrument
		local label
		local fcol_width = self.widths[1]
		if instrument and instrument.mod then
			fcol_width = fcol_width - 3
			label = shorten(tolabel(instrument), fcol_width)
			local dot_length = fcol_width - label:len()
			local dots = rep(dot_length > 4 and "." or "", dot_length - 1)
			label = sprintf(sprintf(" - %%s%%%ds", dot_length), label, dots)
		else -- Print mod_stats
			label = shorten(modname, fcol_width - 2) .. ":"
		end

		local call_mean = stats:get_mean("call_all")
		self:print(self.txt_row_format, label,
			format_number(call_mean, call_mean and call_mean < 1000 and "%.1f"),
			format_number(stats:get_use() * 100, "%.1f"),
			format_number(stats:get_mean("time_all")),
			format_number(stats.time_min),
			format_number(stats.time_max)
		)
	end,

	format = function(self, filter)
		self:update()
		local profile = self.profile
		self:print("Values are per sample. A sample is taken every server step of measurements from instrumentation.")
		self:print(" calls: Mean calls of the instrumented function per sample during instrument activity.")
		self:print(" use %: Percentage of samples this instrument was active.")
		self:print(" avg/min/max µs: Average (mean), minimum and maximum time spent per sample during instrument activity.")
		self:print("A total of %d samples were taken", profile.ins_total.samples)

		if filter then
			self:print("The output is limited to '%s'", filter)
		end

		self:print()
		self:print(
			self.txt_row_format,
			"instrumentation", "calls", "use %", "avg Ms", "min Ms", "max Ms"
		)
		self:print(self.HR)

		for instrument, stat in pairs(profile.ins_stats) do
			local modname = instrument:get_modname()

			-- first print rows with children, then their children
			-- filter and skip any top level statistics without children,
			-- like non-mod instruments
			if filter_matches(filter, modname) and #stat > 0 then
				self:format_row(modname, stat, instrument)

				for i=1, #stat do
					self:format_row(modname, stat[i])
				end
			end
		end

		self:print(self.HR)
		if not filter then
			self:format_row("total", profile.ins_total)
		end
	end
}

local CsvFormatter = Formatter:new {
	format_row = function(self, modname, stats, instrument)
		self:print(
			"%q,%q,%d,%d,%f,%f,%f,%d,%d",
			modname,
			instrument.mod and tolabel(instrument) or "*",
			stats.samples,
			stats.call_all,
			stats:get_use(),
			stats.time_all,
			stats:get_mean("time_all"),
			stats.time_min,
			stats.time_max
		)
	end,

	format = function(self, filter)
		self:print(
			"%q,%q,%q,%q,%q,%q,%q,%q,%q",
			"modname",
			"instrumentation",
			"samples",
			"total calls",
			"relative use",
			"time all µs",
			"time avg µs",
			"time min µs",
			"time max µs"
		)
		for instrument, stats in pairs(self.profile.ins_stats) do
			local modname = instrument:get_modname()

			if filter_matches(filter, modname) then
				self:format_row(modname, stats, instrument)
			end
		end
	end
}

local function format_statistics(profile, format, filter)
	local formatter
	if format == "csv" then
		formatter = CsvFormatter:new {
			profile = profile
		}
	else
		formatter = TxtFormatter:new {
			profile = profile
		}
	end
	formatter:format(filter)
	return formatter:flush()
end

---
-- Format the profile ready for display and
-- @return string to be printed to the console
--
function reporter.print(profile, filter)
	if filter == "" then filter = nil end
	return format_statistics(profile, "txt", filter)
end

---
-- Serialize the profile data and
-- @return serialized data to be saved to a file
--
local function serialize_profile(profile, format, filter)
	if format == "lua" then
		return core.serialize(profile:export(filter))
	elseif format == "json" then
		return core.write_json(profile:export(filter))
	elseif format == "json_pretty" then
		return core.write_json(profile:export(filter), true)
	end
	-- Fall back to textual formats.
	return format_statistics(profile, format, filter)
end

local worldpath = core.get_worldpath()
local function get_save_path(format, filter)
	local report_path = settings:get("profiler.report_path") or ""
	if report_path ~= "" then
		core.mkdir(sprintf("%s%s%s", worldpath, DIR_DELIM, report_path))
	end
	return (sprintf(
		"%s/%s/profile-%s%s.%s",
		worldpath,
		report_path,
		os.date("%Y%m%dT%H%M%S"),
		filter and ("-" .. filter) or "",
		format
	):gsub("[/\\]+", DIR_DELIM))-- Clean up delims
end

---
-- Save the profile to the world path.
-- @return success, log message
--
function reporter.save(profile, format, filter)
	if not format or format == "" then
		format = settings:get("profiler.default_report_format") or "txt"
	end
	if filter == "" then
		filter = nil
	end

	local path = get_save_path(format, filter)

	local output, io_err = io.open(path, "w")
	if not output then
		return false, "Saving of profile failed with: " .. io_err
	end
	local content, err = serialize_profile(profile, format, filter)
	if not content then
		output:close()
		return false, "Saving of profile failed with: " .. err
	end
	output:write(content)
	output:close()

	local logmessage = "Profile saved to " .. path
	core.log("action", logmessage)
	return true, logmessage
end

return reporter
