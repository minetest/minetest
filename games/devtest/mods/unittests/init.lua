unittests = {}

unittests.list = {}

-- name: Name of the test
-- func:
--   for sync: function(player, pos), should error on failure
--   for async: function(callback, player, pos)
--     MUST call callback() or callback("error msg") in case of error once test is finished
--     this means you cannot use assert() in the test implementation
-- opts: {
--   player = false, -- Does test require a player?
--   map = false, -- Does test require map access?
--   async = false, -- Does the test run asynchronously? (read notes above!)
-- }
function unittests.register(name, func, opts)
	local def = table.copy(opts or {})
	def.name = name
	def.func = func
	table.insert(unittests.list, def)
end

function unittests.on_finished(all_passed)
	-- free to override
end

function unittests.run_one(idx, counters, out_callback, player, pos)
	local def = unittests.list[idx]
	if not def.player then
		player = nil
	elseif player == nil then
		core.after(0, out_callback) -- fuck
		return false
	end
	if not def.map then
		pos = nil
	elseif pos == nil then
		core.after(0, out_callback) -- fuck
		return false
	end

	local tbegin = core.get_us_time()
	local function done(status, err)
		local tend = core.get_us_time()
		local ms_taken = (tend - tbegin) / 1000

		if not status then
			core.log("error", err)
		end
		print(string.format("[%s] %s - %dms",
			status and "PASS" or "FAIL", def.name, ms_taken))
		counters.time = counters.time + ms_taken
		counters.total = counters.total + 1
		if status then
			counters.passed = counters.passed + 1
		end
	end

	if def.async then
		core.log("info", "[unittest] running " .. def.name .. " (async)")
		def.func(function(err)
			done(err == nil, err)
			core.after(0, out_callback) -- fuck
		end, player, pos)
	else
		core.log("info", "[unittest] running " .. def.name)
		local status, err = pcall(def.func, player, pos)
		done(status, err)
		core.after(0, out_callback) -- fuck
	end
	
	return true
end

local function wait_for_player(callback)
	local first = true
	core.register_on_joinplayer(function(player)
		if first then
			callback(player)
			first = false
		end
	end)
end

local function wait_for_map(player, callback)
	local check = function()
		if core.get_node_or_nil(player:get_pos()) ~= nil then
			callback()
		else
			minetest.after(0, check)
		end
	end
	check()
end

function unittests.run_all()
	-- I tried to make this code as good as it can be but async is still a mess in Lua
	local stage = 1
	local idx = 1
	local player, pos
	local counters = { time = 0, total = 0, passed = 0 }

	local function next()
		if stage == 1 then -- Stage 1: standalone tests
			if idx > #unittests.list then
				stage = 2
				idx = 1
				-- Wait for a player to join
				return wait_for_player(function(player_)
					player = player_
					next()
				end)
			end
			local def = unittests.list[idx]
			def.done = unittests.run_one(idx, counters, next, nil, nil)
			idx = idx + 1
		elseif stage == 2 then -- Stage 2: tests that require a player
			if idx > #unittests.list then
				stage = 3
				idx = 1
				-- Wait for world to generate/load
				return wait_for_map(player, function()
					pos = vector.round(player:get_pos())
					next()
				end)
			end
			local def = unittests.list[idx]
			if def.done then
				idx = idx + 1
				return next()
			end
			def.done = unittests.run_one(idx, counters, next, player, nil)
			idx = idx + 1
		elseif stage == 3 then -- Stage 3: tests that require map access
			if idx > #unittests.list then
				stage = 4
				return next()
			end
			local def = unittests.list[idx]
			if def.done then
				idx = idx + 1
				return next()
			end
			def.done = unittests.run_one(idx, counters, next, player, pos)
			idx = idx + 1
		elseif stage == 4 then -- Stage 4: print stats and exit, phew
			assert(#unittests.list == counters.total)
			print(string.rep("+", 80))
			print(string.format("Unit Test Results: %s",
				counters.total == counters.passed and "PASSED" or "FAILED"))
			print(string.format("    %d / %d failed tests.",
				counters.total - counters.passed, counters.total))
			print(string.format("    Testing took %dms total.", counters.time))
			print(string.rep("+", 80))
			unittests.on_finished(counters.total == counters.passed)
		end
	end

	next()
end

--------------

local modpath = minetest.get_modpath("unittests")
dofile(modpath .. "/random.lua")
dofile(modpath .. "/player.lua")
dofile(modpath .. "/crafting.lua")
dofile(modpath .. "/itemdescription.lua")

--------------

if core.settings:get_bool("devtest_unittests_autostart", false) then
	core.after(0, unittests.run_all)
end
