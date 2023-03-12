local function show_fullscreen_fs(name)
	local window = minetest.get_player_window_information(name)
	if not window then
		return false, "Unable to get window info"
	end

	print(dump(window))

	local size = { x = window.max_formspec_size.x * 1.1, y = window.max_formspec_size.y * 1.1 }
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

	minetest.show_formspec(name, "testfullscreenfs:fs", table.concat(fs))
	return true, ("Calculated size of %f, %f"):format(size.x, size.y)
end


minetest.register_chatcommand("testfullscreenfs", {
	func = show_fullscreen_fs,
})
