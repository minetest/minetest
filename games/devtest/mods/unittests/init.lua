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

-- Calls invoke with a callback as argument
-- Suspends coroutine until that callback is called
-- Return values are passed through
local function await(invoke)
	local co = coroutine.running()
	assert(co)
	local called_early = true
	invoke(function(...)
		if called_early == true then
			called_early = {...}
		else
			coroutine.resume(co, ...)
		end
	end)
	if called_early ~= true then
		-- callback was already called before yielding
		return unpack(called_early)
	end
	called_early = nil
	return coroutine.yield()
end

function unittests.run_one(idx, counters, out_callback, player, pos)
	local def = unittests.list[idx]
	if not def.player then
		player = nil
	elseif player == nil then
		out_callback(false)
		return false
	end
	if not def.map then
		pos = nil
	elseif pos == nil then
		out_callback(false)
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
			out_callback(true)
		end, player, pos)
	else
		core.log("info", "[unittest] running " .. def.name)
		local status, err = pcall(def.func, player, pos)
		done(status, err)
		out_callback(true)
	end
	
	return true
end

local function wait_for_player(callback)
	if #core.get_connected_players() > 0 then
		return callback(core.get_connected_players()[1])
	end
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
	-- This runs in a coroutine so it uses await().
	local counters = { time = 0, total = 0, passed = 0 }

	-- Run standalone tests first
	for idx = 1, #unittests.list do
		local def = unittests.list[idx]
		def.done = await(function(cb)
			unittests.run_one(idx, counters, cb, nil, nil)
		end)
	end

	-- Wait for a player to join, run tests that require a player
	local player = await(wait_for_player)
	for idx = 1, #unittests.list do
		local def = unittests.list[idx]
		if not def.done then
			def.done = await(function(cb)
				unittests.run_one(idx, counters, cb, player, nil)
			end)
		end
	end

	-- Wait for the world to generate/load, run tests that require map access
	await(function(cb)
		wait_for_map(player, cb)
	end)
	local pos = vector.round(player:get_pos())
	for idx = 1, #unittests.list do
		local def = unittests.list[idx]
		if not def.done then
			def.done = await(function(cb)
				unittests.run_one(idx, counters, cb, player, pos)
			end)
		end
	end

	-- Print stats
	assert(#unittests.list == counters.total)
	print(string.rep("+", 80))
	print(string.format("Unit Test Results: %s",
	counters.total == counters.passed and "PASSED" or "FAILED"))
	print(string.format("    %d / %d failed tests.",
	counters.total - counters.passed, counters.total))
	print(string.format("    Testing took %dms total.", counters.time))
	print(string.rep("+", 80))
	unittests.on_finished(counters.total == counters.passed)
	return counters.total == counters.passed
end

--------------

local modpath = minetest.get_modpath("unittests")
dofile(modpath .. "/misc.lua")
dofile(modpath .. "/player.lua")
dofile(modpath .. "/crafting.lua")
dofile(modpath .. "/itemdescription.lua")

--------------

if core.settings:get_bool("devtest_unittests_autostart", false) then
	core.after(0, function()
		coroutine.wrap(unittests.run_all)()
	end)
else
	minetest.register_chatcommand("unittests", {
		privs = {basic_privs=true},
		description = "Runs devtest unittests (may modify player or map state)",
		func = function(name, param)
			unittests.on_finished = function(ok)
				core.chat_send_player(name,
					(ok and "All tests passed." or "There were test failures.") ..
					" Check the console for detailed output.")
			end
			coroutine.wrap(unittests.run_all)()
			return true, ""
		end,
	})
end
