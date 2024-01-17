
-- old non-method sound function

function core.sound_stop(handle, ...)
	return handle:stop(...)
end

-- On Android, closing the app via the "Recents" screen won't result in a clean
-- shutdown, discarding any setting changes made by the user.
-- To avoid that, we write the settings file in more cases on Android.
-- We don't do this on desktop platforms because you can have multiple
-- instances of Minetest running at the same time there.
function write_settings_early()
	if PLATFORM == "Android" then
		core.settings:write()
	end
end

local old_core_start = core.start
core.start = function()
	-- Necessary for saving the selected world, server address, player name, etc.
	write_settings_early()
	old_core_start()
end
