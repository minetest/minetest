local last_target

core.register_globalstep(function(dtime)
	local target = core.get_target_dtime()
	-- dtime only changes in the next step
	assert(dtime == 0 or dtime >= target or dtime >= last_target)
	last_target = target
end)
