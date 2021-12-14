unittests = {}

unittests.list = {}

-- name: Name of the test
-- func: function(player, pos), should error on failure
-- opts: {
--   player = false, -- Does test require a player?
--   map = false, -- Does test require map access?
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

function unittests.run_one(idx, counters, player, pos)
	local def = unittests.list[idx]
	if not def.player then
		player = nil
	elseif player == nil then
		return false
	end
	if not def.map then
		pos = nil
	elseif pos == nil then
		return false
	end

	local tbegin = core.get_us_time()
	local status, err = pcall(def.func, player, pos)
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
	local counters = { time = 0, total = 0, passed = 0 }
	-- Run standalone tests first
	for idx, def in ipairs(unittests.list) do
		def.done = unittests.run_one(idx, counters, nil, nil)
	end

	-- Wait for a player to join, run tests that require a player
	wait_for_player(function(player)
		for idx, def in ipairs(unittests.list) do
			if not def.done then
				def.done = unittests.run_one(idx, counters, player, nil)
			end
		end

		-- Wait for the world to generate/load, run tests that require map access
		wait_for_map(player, function()
			local pos = vector.round(player:get_pos())
			for idx, def in ipairs(unittests.list) do
				if not def.done then
					def.done = unittests.run_one(idx, counters, player, pos)
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
		end)
	end)
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

