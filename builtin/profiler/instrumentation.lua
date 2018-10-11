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

local format = string.format
local pairs, type = pairs, type
local getmetatable, setmetatable = getmetatable, setmetatable
local core, settings, get_current_modname = core, core.settings, core.get_current_modname
local profiler, sampler = ...

local instrument_builtin = settings:get_bool("instrument.builtin", false)

local register_functions = {
	register_globalstep = 0,
	register_playerevent = 0,
	register_on_placenode = 0,
	register_on_dignode = 0,
	register_on_punchnode = 0,
	register_on_generated = 0,
	register_on_newplayer = 0,
	register_on_dieplayer = 0,
	register_on_respawnplayer = 0,
	register_on_prejoinplayer = 0,
	register_on_joinplayer = 0,
	register_on_leaveplayer = 0,
	register_on_cheat = 0,
	register_on_chat_message = 0,
	register_on_player_receive_fields = 0,
	register_on_craft = 0,
	register_craft_predict = 0,
	register_on_protection_violation = 0,
	register_on_item_eat = 0,
	register_on_punchplayer = 0,
	register_on_player_hpchange = 0,
}

local registered_instruments = {}

---
-- Represents and identifies the instrumenting component.
-- It associates metadata (originating mod, label, class) with it and holds
-- logged quantities gained from instrumentation to be picked up by the sampler.
--
-- An Instrument may cover a function or a collection of other instruments.
-- This effectively creates a flat hierachy of instruments, where the top often
-- represents the entirety of instruments deployed for a mod.
--
-- References are passed around the profiler to avoid deep table lookups in a
-- central tree structure when looking for the instruments to log to them.
-- It moves the search and sort away from instrumentation-time
-- to sampling time for reduced overhead.
local Instrument = {
	---
	-- @param parent the parent instrument (usually of the mod this belongs to)
	-- 			or nil for a top-level instrument
	-- @param class classify the instrument to change how the instrument is
	-- 			dealed with in other places (for example during reporting)
	-- 			e.g. mod, entity, abm, lbm, chatcommand
	-- @param label text to display and to differentiate between instruments
	new = function(self, parent, class, label)
		local modname = parent and parent.label or label or ""
		local name = format("%s:%s:%s", modname, class or "", label)
		if registered_instruments[name] then
			return registered_instruments[name]
		end
		local obj = self.reset({
			mod = parent,
			class = class,
			label = label,
		})
		registered_instruments[name] = obj
		return setmetatable(obj, self)
	end,

	---
	-- set all measured quantities to 0 that were logged for this instrument
	reset = function(self)
		self.logged_time = 0
		self.logged_calls = 0
		return self
	end,

	---
	-- @return the name of the mod that provided the instrumented function
	get_modname = function(self)
		return self.mod and self.mod.label or self.label
	end,

	__tostring = function(self)
		return format(
			"%s:%s:%s",
			self.mod and self.mod.label or "",
			self.class or "",
			self.label or ""
		)
	end,
}
Instrument.__index = Instrument

---
-- Keep `measure` and the closure in `instrument` lean, as these, and their
-- directly called functions are the overhead that is caused by instrumentation.
--
local get_time = core.get_us_time
local logged_instruments = sampler.logged_instruments
local function measure(ins, time_diff, ...)
	time_diff = get_time() - time_diff
	if time_diff >= 0 then
		ins.logged_time = ins.logged_time + time_diff
		ins.logged_calls = ins.logged_calls + 1
		logged_instruments[ins] = true
	end
	return ...
end

--- Automatically instrument a function to measure and log to the sampler.
-- def = {
-- 		func = function(...) ... end,
-- 		[mod = "",]
-- 		[class = "",]
-- 		label = "",
-- }
local function instrument(def)
	if not def or not def.func then
		return
	end

	local func = def.func
	local mod = def.mod or get_current_modname() or "*unknown*"

	if not instrument_builtin and mod == "*builtin*" then
		return func
	end

	mod = Instrument:new(nil, "mod", mod)
	def = Instrument:new(mod, def.class, def.label)
	def.func = func

	return function(...)
		-- This tail-call allows passing all return values of `func`
		-- also called https://en.wikipedia.org/wiki/Continuation_passing_style
		-- Compared to table creation and unpacking it won't lose `nil` returns
		-- and is expected to be faster
		-- `measure` will be executed after time() and func(...)
		return measure(def, get_time(), func(...))
	end
end

---
-- Create a label, either by parsing a name
-- or generating with a running index number.
--
local counts = {}
local function generate_label(class, name, func_name)
	local modname = get_current_modname() or "*unknown*"

	-- remove the register_ prefix for shorter and more descriptive names
	-- since we instrument the otherwise func_name-less callbacks
	-- and not their registration functions
	func_name = func_name and func_name:gsub("^register_", "", 1) or ""

	-- with an instance name available, we only need to clean and format it
	if name then
		-- remove mod: and :mod: prefixes and replace underscores with spaces
		name = name:gsub("^:?[_%w]*:", "", 1):gsub("_", " ")
		return format("'%s' %s", name, func_name):trim()
	end

	-- generate a running index number to differentiate between individual
	-- nameless callbacks/functions
	local index_id = format("%s:%s:%s:%s", modname, class or "", name or "", func_name)
	local index = counts[index_id] or 1
	counts[index_id] = index + 1
	return format("%s[%d]", func_name, index)
end

local function can_be_called(func)
	-- It has to be a function or callable table
	return type(func) == "function" or
		((type(func) == "table" or type(func) == "userdata") and
		getmetatable(func) and getmetatable(func).__call)
end

local function assert_can_be_called(func, func_name, level)
	if not can_be_called(func) then
		-- Then throw an *helpful* error, by pointing on our caller instead of us.
		error(format("Invalid argument to %s. Expected function-like type instead of '%s'.", func_name, type(func)), level + 1)
	end
end

---
-- Wraps a registration function `func` in such a way,
-- that it will automatically instrument any callback function passed as first argument.
--
local function instrument_register(func, func_name)
	return function(callback, ...)
		assert_can_be_called(callback, func_name, 2)
		register_functions[func_name] = register_functions[func_name] + 1
		return func(instrument {
			func = callback,
			label = generate_label("callback", nil, func_name),
		}, ...)
	end
end

local function init_chatcommand()
	if settings:get_bool("instrument.chatcommand", true) then
		local orig_register_chatcommand = core.register_chatcommand
		core.register_chatcommand = function(cmd, def)
			def.func = instrument {
				class = "chatcommand",
				func = def.func,
				label = cmd,
			}
			return orig_register_chatcommand(cmd, def)
		end
	end
end

---
-- Start instrumenting selected functions
--
local function init()
	if settings:get_bool("instrument.entity", true) then
		-- Explicitly declare entity api-methods.
		-- Simple iteration would ignore lookup via __index.
		local entity_instrumentation = {
			"on_activate",
			"on_step",
			"on_punch",
			"rightclick",
			"get_staticdata",
		}
		-- Wrap register_entity() to instrument them on registration.
		local orig_register_entity = core.register_entity
		core.register_entity = function(name, prototype)
			local modname = get_current_modname() or "*unknown*"
			for _, func_name in pairs(entity_instrumentation) do
				prototype[func_name] = instrument {
					mod = modname,
					class = "entity",
					func = prototype[func_name],
					label = generate_label("entity", name, func_name),
				}
			end
			return orig_register_entity(name,prototype)
		end
	end

	if settings:get_bool("instrument.abm", true) then
		-- Wrap register_abm() to automatically instrument abms.
		local orig_register_abm = core.register_abm
		core.register_abm = function(spec)
			spec.action = instrument {
				func = spec.action,
				class = "abm",
				label = spec.label or generate_label("abm"),
			}
			return orig_register_abm(spec)
		end
	end

	if settings:get_bool("instrument.lbm", true) then
		-- Wrap register_lbm() to automatically instrument lbms.
		local orig_register_lbm = core.register_lbm
		core.register_lbm = function(spec)
			spec.action = instrument {
				func = spec.action,
				class = "lbm",
				label = spec.label or generate_label("lbm", spec.name),
			}
			return orig_register_lbm(spec)
		end
	end

	if settings:get_bool("instrument.global_callback", true) then
		for func_name, _ in pairs(register_functions) do
			core[func_name] = instrument_register(core[func_name], func_name)
		end
	end

	if settings:get_bool("instrument.profiler", false) then
		-- Measure overhead of instrumentation
		-- but also measures an additional function call to an empty function
		-- that technically is not part of the overhead anymore
		profiler.empty_instrument = instrument {
			func = function() return end,
			mod = "*profiler*",
			label = "Instrumentation overhead",
		}
	end
end

return {
	registered_instruments = registered_instruments,
	register_functions = register_functions,
	instrument = instrument,
	init = init,
	init_chatcommand = init_chatcommand,
}
