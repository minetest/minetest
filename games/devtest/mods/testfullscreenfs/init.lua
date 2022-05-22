local function calculate_max_fs_size(window)
	return {
		x = window.size.x / (0.5555 * 96 * window.gui_scaling),
		y = window.size.y / (0.5555 * 96 * window.gui_scaling),
	}
end

local function show_fullscreen_fs(name)
	local window = minetest.get_player_window_information(name)
	if not window then
		return false, "Unable to get window info"
	end

	local size = calculate_max_fs_size(window)
	local fs = {
		"formspec_version[4]",
		("size[%f,%f]"):format(size.x, size.y),
		"padding[-0.01,-0.01]",
		("button[%f,%f;1,1;%s;%s]"):format(0, 0, "tl", "TL"),
		("button[%f,%f;1,1;%s;%s]"):format(size.x - 1, 0, "tr", "TR"),
		("button[%f,%f;1,1;%s;%s]"):format(size.x - 1, size.y - 1, "br", "BR"),
		("button[%f,%f;1,1;%s;%s]"):format(0, size.y - 1, "bl", "BL"),

		("label[%f,%f;%s]"):format(size.x / 2, size.y / 2, "Fullscreen")
	}

	print(table.concat(fs))

	minetest.show_formspec(name, "testfullscreenfs:fs", table.concat(fs))
	return true, ("Calculated size of %f, %f"):format(size.x, size.y)
end


minetest.register_chatcommand("testfullscreenfs", {
	func = show_fullscreen_fs,
})
