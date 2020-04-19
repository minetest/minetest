
-- stuff for core.register_on_player_interval
local time = 0.0
local time_next = math.huge
local step_sizes = {}
local expiring_times = {}

-- override core.register_on_player_interval to also store the step size in
-- step_sizes and update expiring_times and time_next
local old_register_on_player_interval = core.register_on_player_interval

function core.register_on_player_interval(step_size, func)
	old_register_on_player_interval(func)

	local index = #core.registered_on_player_intervals
	step_sizes[index] = step_size
	expiring_times[index] = step_size
	time_next = math.min(time_next, step_size)
end

core.register_globalstep(function(dtime)
	local players = core.get_connected_players()

	-- core.register_on_player_step
	for k = 1, #players do
		-- the callback mode does not matter
		core.run_callbacks(core.registered_on_player_steps, 0, players[k], dtime)
	end

	-- core.register_on_player_interval
	time = time + dtime

	if time < time_next then
		return
	end

	time_next = math.huge

	for i = 1, #expiring_times do
		local exp_time = expiring_times[i]
		if time >= exp_time then
			-- time expired
			-- call it
			local func = core.registered_on_player_intervals[i]
			core.set_last_run_mod(core.callback_origins[func])
			for i = 1, #players do
				func(players[i])
			end
			-- reset the timer
			exp_time = time + step_sizes[i]
			expiring_times[i] = exp_time
		end
		if time_next > exp_time then
			time_next = exp_time
		end
	end
end)
