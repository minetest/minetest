-- Minetest: builtin/game/mod_profiling.lua

local mod_statistics = {}
mod_statistics.step_total = 0
mod_statistics.data = {}
mod_statistics.stats = {}
mod_statistics.stats["total"] = {
	min_us = math.huge,
	max_us = 0,
	avg_us = 0,
	min_per = 100,
	max_per = 100,
	avg_per = 100
}

local replacement_table = {
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

--------------------------------------------------------------------------------
function mod_statistics.log_time(type, modname, time_in_us)

	if modname == nil then
		modname = "builtin"
	end
	
	if mod_statistics.data[modname] == nil then
		mod_statistics.data[modname] = {}
	end
	
	if mod_statistics.data[modname][type] == nil then
		mod_statistics.data[modname][type] = 0
	end
	
	mod_statistics.data[modname][type] =
		mod_statistics.data[modname][type] + time_in_us
	mod_statistics.step_total = mod_statistics.step_total + time_in_us
end

--------------------------------------------------------------------------------
function mod_statistics.update_statistics(dtime)
	for modname,types in pairs(mod_statistics.data) do
		
		if mod_statistics.stats[modname] == nil then
			mod_statistics.stats[modname] = {
				min_us = math.huge,
				max_us = 0,
				avg_us = 0,
				min_per = 100,
				max_per = 0,
				avg_per = 0
			}
		end
		
		local modtime = 0
		for type,time in pairs(types) do
			modtime = modtime + time
			if mod_statistics.stats[modname].types == nil then
				mod_statistics.stats[modname].types = {}
			end
			if mod_statistics.stats[modname].types[type] == nil then
				mod_statistics.stats[modname].types[type] = {
					min_us = math.huge,
					max_us = 0,
					avg_us = 0,
					min_per = 100,
					max_per = 0,
					avg_per = 0
				}
			end
			
			local toupdate = mod_statistics.stats[modname].types[type]
			
			--update us values
			toupdate.min_us = math.min(time, toupdate.min_us)
			toupdate.max_us = math.max(time, toupdate.max_us)
			--TODO find better algorithm
			toupdate.avg_us = toupdate.avg_us * 0.99 + modtime * 0.01
				
			--update percentage values
			local cur_per = (time/mod_statistics.step_total) * 100
			toupdate.min_per = math.min(toupdate.min_per, cur_per)
				
			toupdate.max_per = math.max(toupdate.max_per, cur_per)
				
			--TODO find better algorithm
			toupdate.avg_per = toupdate.avg_per * 0.99 + cur_per * 0.01
			
			mod_statistics.data[modname][type] = 0
		end
		
		--per mod statistics
		--update us values
		mod_statistics.stats[modname].min_us =
			math.min(modtime, mod_statistics.stats[modname].min_us)
		mod_statistics.stats[modname].max_us =
			math.max(modtime, mod_statistics.stats[modname].max_us)
		--TODO find better algorithm
		mod_statistics.stats[modname].avg_us =
			mod_statistics.stats[modname].avg_us * 0.99 + modtime * 0.01
			
		--update percentage values
		local cur_per = (modtime/mod_statistics.step_total) * 100
		mod_statistics.stats[modname].min_per =
			math.min(mod_statistics.stats[modname].min_per, cur_per)
			
		mod_statistics.stats[modname].max_per =
			math.max(mod_statistics.stats[modname].max_per, cur_per)
			
		--TODO find better algorithm
		mod_statistics.stats[modname].avg_per =
			mod_statistics.stats[modname].avg_per * 0.99 + cur_per * 0.01
	end
	
	--update "total"
	mod_statistics.stats["total"].min_us =
		math.min(mod_statistics.step_total, mod_statistics.stats["total"].min_us)
	mod_statistics.stats["total"].max_us =
		math.max(mod_statistics.step_total, mod_statistics.stats["total"].max_us)
	--TODO find better algorithm
	mod_statistics.stats["total"].avg_us =
		mod_statistics.stats["total"].avg_us * 0.99 +
		mod_statistics.step_total * 0.01
	
	mod_statistics.step_total = 0
end

--------------------------------------------------------------------------------
local function build_callback(log_id, fct)
	return function( toregister )
		local modname = core.get_current_modname()
		
		fct(function(...)
			local starttime = core.get_us_time()
			-- note maximum 10 return values are supported unless someone finds
			-- a way to store a variable lenght return value list
			local r0, r1, r2, r3, r4, r5, r6, r7, r8, r9 = toregister(...)
			local delta = core.get_us_time() - starttime
			mod_statistics.log_time(log_id, modname, delta)
			return r0, r1, r2, r3, r4, r5, r6, r7, r8, r9
			end
		)
	end
end

--------------------------------------------------------------------------------
function profiling_print_log(cmd, filter)

	print("Filter:" .. dump(filter))

	core.log("action", "Values below show times/percentages per server step.")
	core.log("action", "Following suffixes are used for entities:")
	core.log("action", "\t#oa := on_activate, #os := on_step, #op := on_punch, #or := on_rightclick, #gs := get_staticdata")
	core.log("action",
		string.format("%16s | %25s | %10s | %10s | %10s | %9s | %9s | %9s",
		"modname", "type" , "min µs", "max µs", "avg µs", "min %", "max %", "avg %")
	)
	core.log("action",
		"-----------------+---------------------------+-----------+" ..
		"-----------+-----------+-----------+-----------+-----------")
	for modname,statistics in pairs(mod_statistics.stats) do
		if modname ~= "total" then
		
			if (filter == "") or (modname == filter) then
				if modname:len() > 16 then
					modname = "..." .. modname:sub(-13)
				end
			
				core.log("action",
					string.format("%16s | %25s | %9d | %9d | %9d | %9d | %9d | %9d",
					modname,
					" ",
					statistics.min_us,
					statistics.max_us,
					statistics.avg_us,
					statistics.min_per,
					statistics.max_per,
					statistics.avg_per)
				)
				if core.setting_getbool("detailed_profiling") then
					if statistics.types ~= nil then
						for type,typestats in pairs(statistics.types) do
						
							if type:len() > 25 then
								type = "..." .. type:sub(-22)
							end
						
							core.log("action",
								string.format(
								"%16s | %25s | %9d | %9d | %9d | %9d | %9d | %9d",
								" ",
								type,
								typestats.min_us,
								typestats.max_us,
								typestats.avg_us,
								typestats.min_per,
								typestats.max_per,
								typestats.avg_per)
							)
						end
					end
				end
			end
		end
	end
		core.log("action",
			"-----------------+---------------------------+-----------+" ..
			"-----------+-----------+-----------+-----------+-----------")
			
	if filter == "" then
		core.log("action",
			string.format("%16s | %25s | %9d | %9d | %9d | %9d | %9d | %9d",
			"total",
			" ",
			mod_statistics.stats["total"].min_us,
			mod_statistics.stats["total"].max_us,
			mod_statistics.stats["total"].avg_us,
			mod_statistics.stats["total"].min_per,
			mod_statistics.stats["total"].max_per,
			mod_statistics.stats["total"].avg_per)
		)
	end
	core.log("action", " ")
	
	return true
end

--------------------------------------------------------------------------------
local function initialize_profiling()
	core.log("action", "Initialize tracing")
	
	mod_statistics.entity_callbacks = {}
	
	-- first register our own globalstep handler
	core.register_globalstep(mod_statistics.update_statistics)
	
	local rp_register_entity = core.register_entity
	core.register_entity = function(name, prototype)
		local modname = core.get_current_modname()
		local new_on_activate = nil
		local new_on_step = nil
		local new_on_punch = nil
		local new_rightclick = nil
		local new_get_staticdata = nil
		
		if prototype.on_activate ~= nil then
			local cbid = name .. "#oa"
			mod_statistics.entity_callbacks[cbid] = prototype.on_activate
			new_on_activate = function(self, staticdata, dtime_s)
				local starttime = core.get_us_time()
				mod_statistics.entity_callbacks[cbid](self, staticdata, dtime_s)
				local delta = core.get_us_time() -starttime
				mod_statistics.log_time(cbid, modname, delta)
			end
		end
		
		if prototype.on_step ~= nil then
			local cbid = name .. "#os"
			mod_statistics.entity_callbacks[cbid] = prototype.on_step
			new_on_step = function(self, dtime)
				local starttime = core.get_us_time()
				mod_statistics.entity_callbacks[cbid](self, dtime)
				local delta = core.get_us_time() -starttime
				mod_statistics.log_time(cbid, modname, delta)
			end
		end
	
		if prototype.on_punch ~= nil then
			local cbid = name .. "#op"
			mod_statistics.entity_callbacks[cbid] = prototype.on_punch
			new_on_punch = function(self, hitter)
				local starttime = core.get_us_time()
				mod_statistics.entity_callbacks[cbid](self, hitter)
				local delta = core.get_us_time() -starttime
				mod_statistics.log_time(cbid, modname, delta)
			end
		end
		
		if prototype.rightclick ~= nil then
			local cbid = name .. "#rc"
			mod_statistics.entity_callbacks[cbid] = prototype.rightclick
			new_rightclick = function(self, clicker)
				local starttime = core.get_us_time()
				mod_statistics.entity_callbacks[cbid](self, clicker)
				local delta = core.get_us_time() -starttime
				mod_statistics.log_time(cbid, modname, delta)
			end
		end
		
		if prototype.get_staticdata ~= nil then
			local cbid = name .. "#gs"
			mod_statistics.entity_callbacks[cbid] = prototype.get_staticdata
			new_get_staticdata = function(self)
				local starttime = core.get_us_time()
				local retval = mod_statistics.entity_callbacks[cbid](self)
				local delta = core.get_us_time() -starttime
				mod_statistics.log_time(cbid, modname, delta)
				return retval
			end
		end
	
		prototype.on_activate = new_on_activate
		prototype.on_step = new_on_step
		prototype.on_punch = new_on_punch
		prototype.rightclick = new_rightclick
		prototype.get_staticdata = new_get_staticdata
		
		rp_register_entity(name,prototype)
	end
	
	for i,v in ipairs(replacement_table) do
		local to_replace = core[v]
		core[v] = build_callback(v, to_replace)
	end
	
	local rp_register_abm = core.register_abm
	core.register_abm = function(spec)
	
		local modname = core.get_current_modname()
	
		local replacement_spec = {
			nodenames = spec.nodenames,
			neighbors = spec.neighbors,
			interval  = spec.interval,
			chance    = spec.chance,
			action = function(pos, node, active_object_count, active_object_count_wider)
				local starttime = core.get_us_time()
				spec.action(pos, node, active_object_count, active_object_count_wider)
				local delta = core.get_us_time() - starttime
				mod_statistics.log_time("abm", modname, delta)
			end
		}
		rp_register_abm(replacement_spec)
	end
	
	core.log("action", "Mod profiling initialized")
end

initialize_profiling()
