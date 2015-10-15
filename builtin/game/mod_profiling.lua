-- Minetest: builtin/game/mod_profiling.lua

local mod_statistics = {}
mod_statistics.step_total = 0
mod_statistics.data = {}
mod_statistics.stats = {}

local function new_stat_tbl()
	return {
		avg_count = 0,

		min_us = math.huge,
		max_us = 0,
		avg_us = 0,
		total_us = 0,

		min_factor = 1,
		max_factor = 0,
		avg_factor = 0,
		total_factor = 0,
	}
end

mod_statistics.stats["*total*"] = new_stat_tbl()


function mod_statistics.log_time(tp, mod_name, time_us)
	mod_name = mod_name or "*builtin*"
	local entry = mod_statistics.data[mod_name]
	if not entry then
		entry = {}
		mod_statistics.data[mod_name] = entry
	end
	if not entry[tp] then
		entry[tp] = 0
	end

	entry[tp] = entry[tp] + time_us
	mod_statistics.step_total = mod_statistics.step_total + time_us
end


function mod_statistics.update_statistics(dtime)
	local function update(t, time, no_update_factor)
		t.avg_count = t.avg_count + 1

		-- Update microsecond values
		t.min_us = math.min(time, t.min_us)
		t.max_us = math.max(time, t.max_us)
		t.total_us = t.total_us + time
		t.avg_us = t.total_us / t.avg_count

		-- Update factors
		if no_update_factor then return end
		local cur_f = 0
		if mod_statistics.step_total ~= 0 then
			cur_f = time / mod_statistics.step_total
		end
		t.min_factor = math.min(t.min_factor, cur_f)
		t.max_factor = math.max(t.max_factor, cur_f)
		t.total_factor = t.total_factor + cur_f
		t.avg_factor = t.total_factor / t.avg_count
	end
	for name, types in pairs(mod_statistics.data) do
		local stats = mod_statistics.stats[name]
		if not stats then
			stats = new_stat_tbl()
			mod_statistics.stats[name] = stats
		end
		
		local mod_time = 0
		for tp, time in pairs(types) do
			mod_time = mod_time + time
			if not stats.types then
				stats.types = {}
			end
			local type_tbl = stats.types[tp]
			if not type_tbl then
				type_tbl = new_stat_tbl()
				stats.types[tp] = type_tbl
			end

			update(type_tbl, time)

			types[tp] = 0
		end

		-- Per mod statistics
		update(stats, mod_time)
	end

	-- Update total
	update(mod_statistics.stats["*total*"], mod_statistics.step_total, true)

	mod_statistics.step_total = 0
end


local function elipsize(str, len)
	if #str > len then
		return "…"..str:sub(-len + 1)
	end
	return str
end


function profiling_print_log(player_name, filter)
	local detailed_profiling = core.setting_getbool("detailed_profiling")
	local title_line = " Mod Name        |"..
		(detailed_profiling and " Type                      |" or "")..
		" Min µs    | Max µs    | Avg µs    | Min %     | Max %     | Avg %"
	local sep_line = "-----------------+"..
		(detailed_profiling and "---------------------------+" or "")..
		"-----------+-----------+-----------+-----------+-----------+-----------"
	local fmt_line = "%16s |"..
			(detailed_profiling and " %-25s |" or "")..
			" %9d | %9d | %9.2f | %8.4f%% | %8.4f%% | %8.4f%%"

	local need_sep = true
	local function log_line(...)
		local args = {...}
		if not detailed_profiling then
			table.remove(args, 2)
		end
		core.debug(string.format(fmt_line, unpack(args)))
		need_sep = true
	end

	local function log_sep_opt()
		if need_sep then
			core.debug(sep_line)
		end
		need_sep = false
	end

	local function log_stat_line(name, tp, stats)
		log_line(elipsize(name, 16), elipsize(tp, 25),
			stats.min_us,
			stats.max_us,
			stats.avg_us,
			stats.min_factor * 100,
			stats.max_factor * 100,
			stats.avg_factor * 100)
	end

	local function log_mod(name, stats)
		local log_name = name
		local types_logged = 0
		if stats.types and detailed_profiling then
			for tp, tp_stats in pairs(stats.types) do
				log_stat_line(log_name, tp, tp_stats)
				log_name = ""
				types_logged = types_logged + 1
			end
		end
		if types_logged ~= 1 then
			log_stat_line(log_name, "Total", stats)
		end
	end

	core.debug("Mod profile list filter: "..dump(filter))
	core.debug("Values below are per server step.")
	if detailed_profiling then
		core.debug("The following suffixes are used for entities:")
		core.debug("\t#oa := on_activate, #os := on_step, #op := on_punch, "..
				"#or := on_rightclick, #gs := get_staticdata")
	end
	core.debug(title_line)
	log_sep_opt()

	if filter == "" or filter == "*builtin*" then
		log_mod("Built-in", mod_statistics.stats["*builtin*"])
		log_sep_opt()
	end

	for name, stats in pairs(mod_statistics.stats) do
		if name:sub(1, 1) ~= "*" and filter == "" or name == filter then
			log_mod(name, stats)
			if detailed_profiling then
				log_sep_opt()
			end
		end
	end

	if filter == "" then
		log_sep_opt()
		local stats_t = mod_statistics.stats["*total*"]
		log_line("Total", "",
			stats_t.min_us,
			stats_t.max_us,
			stats_t.avg_us,
			100, 100, 100)
	end
	log_sep_opt()
	return true
end


local function init_profiling()
	core.log("info", "Initializing mod profiling")

	local function wrap_callback(cb, log_id, mod_name)
		return function(...)
			local start = core.get_us_time()
			local rets = {cb(...)}
			local delta = core.get_us_time() - start
			mod_statistics.log_time(log_id, mod_name, delta)
			return unpack(rets)
		end
	end

	local function wrap_callback_registrator(registrator, log_id)
		return function(to_register)
			local mod_name = core.get_current_modname()
			registrator(wrap_callback(to_register, log_id, mod_name))
		end
	end

	-- First register our own globalstep handler
	core.register_globalstep(mod_statistics.update_statistics)

	local old_register_entity = core.register_entity
	function core.register_entity(name, prototype)
		local mod_name = core.get_current_modname()

		local function wrap(fname, short_fname)
			if not prototype[fname] then return end
			prototype[fname] = wrap_callback(prototype[fname],
					name.."#"..short_fname, mod_name)
		end

		wrap("on_activate", "oa")
		wrap("on_step", "os")
		wrap("on_punch", "op")
		wrap("rightclick", "rc")
		wrap("get_staticdata", "gs")

		old_register_entity(name, prototype)
	end

	local to_replace = {
		"register_globalstep",
		"register_on_placenode",
		"register_on_dignode",
		"register_on_punchnode",
		"register_on_generated",
		"register_on_newplayer",
		"register_on_dieplayer",
		"register_on_respawnplayer",
		"register_on_prejoinplayer",
		"register_on_joinplayer",
		"register_on_leaveplayer",
		"register_on_cheat",
		"register_on_chat_message",
		"register_on_player_receive_fields",
		"register_on_mapgen_init",
		"register_on_craft",
		"register_craft_predict",
		"register_on_item_eat"
	}

	for _, name in ipairs(to_replace) do
		core[name] = wrap_callback_registrator(core[name], name)
	end

	local rp_register_abm = core.register_abm
	function core.register_abm(spec)
		local mod_name = core.get_current_modname()
		spec.label = spec.label or "Unknown ABM"
		spec.action = wrap_callback(spec.action, spec.label, mod_name)
		rp_register_abm(spec)
	end

	core.log("info", "Mod profiling initialized")
end

init_profiling()

