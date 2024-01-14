
local postponed_cache_size = tonumber(minetest.settings:get("abm_postponed_cache_size"))

local abm_first_run = false;

minetest.register_chatcommand("uncancelableABM", {
	params = "enable | disable | interval_1 | interval_5",
	description = "Active/Deactivate ABM which can be postponed.",
	func = function(name, param)
		if param == "enable" then
			minetest.change_abm("testuncancelableabm:abm", {
					interval = 30.0,
				})
			abm_first_run = true;
			if postponed_cache_size == 0 then
				return true, "ABM has been enabled, but ABM postponed cache is disabled."
			else
				return true, "ABM has been enabled and should be postponed."
			end
		elseif param == "interval_1" then
			minetest.change_abm("testuncancelableabm:abm", {
					interval = 1.0,
				})
			abm_first_run = true;
			if postponed_cache_size == 0 then
				return true, "ABM has been enabled with interval 1, but ABM postponed cache is disabled."
			else
				return true, "ABM has been enabled with interval 1 and should be postponed."
			end
		elseif param == "interval_5" then
			minetest.change_abm("testuncancelableabm:abm", {
					interval = 5.0,
				})
			abm_first_run = true;
			if postponed_cache_size == 0 then
				return true, "ABM has been enabled with interval 5, but ABM postponed cache is disabled."
			else
				return true, "ABM has been enabled with interval 5 and should be postponed."
			end
		elseif param == "disable" then
			minetest.change_abm("testuncancelableabm:abm", {
					interval = -30.0,
				})
			return true, "ABM has been disabled."
		else
			return false, "Check /help uncancelableABB for allowed parameters."
		end
	end,
})

minetest.register_abm({
		name = "testuncancelableabm:abm",
		nodenames = "basenodes:stone",
		interval = -30.0,
		chance = 1,
		cancelable = false,
		action = function(pos)
			local time = minetest.get_us_time()
			-- waste some time, to make ABM long
			while time == minetest.get_us_time() do
			end
			if abm_first_run then
				minetest.chat_send_all("Uncancelable ABM runs first time after enable.")
				abm_first_run = false
			end
		end,
	})
