local test_draw = {
	target = nil,
	fill = false,
	last_angle = {x=0, y=0, z=0},
	premade_ct = nil
}

minetest.register_chatcommand("test_draw", {
	params = "off | window | texture | toggle_fill | dump_driver | test_ref",
	description = "Toggle test drawing on and off",
	func = function(param)
		if param == "off" then
			test_draw.target = nil
		elseif param == "texture" then
			minetest.on_next_predraw(function(dtime)
				test_draw.target = Texture.new({x=1500, y=800})
			end)
		elseif param == "window" then
			test_draw.target = minetest.window
		elseif param == "toggle_fill" then
			test_draw.fill = not test_draw.fill
		elseif param == "dump_driver" then
			print(dump(minetest.get_driver_info()))
		elseif param == "test_ref" then
			size = Texture.ref("air.png"):get_size()
			print("\"air.png\" reference get_size: (" .. size.x .. ", " .. size.y .. ")")
			size = Texture.ref(minetest.window):get_size()
			print("`minetest.window` reference get_size: (" .. size.x .. ", " .. size.y .. ")")
		else
			print("Invalid parameter")
			return false
		end
		return true
	end
})

-- Uncomment for error: drawing outside drawing callback
-- test_draw.premade_ct:draw_pixel({x=0, y=0}, "red")

minetest.on_next_predraw(function(dtime)
	test_draw.premade_ct = Texture.new({x=32, y=32})
	local ct = test_draw.premade_ct

	ct:draw_rect({x=0, y=0, nx=0, ny=0}, "sienna")
	ct:draw_texture({x=4, y=4, nx=4, ny=4}, "default_tree.png")

	local ref = Texture.ref(ct)
	local size = ref:get_size()
	assert(size.x == 32 and size.y == 32)

	assert(minetest.window:is_writable() and not minetest.window:is_readable())
	assert(ct:is_writable() and ct:is_readable())
	assert(not ref:is_writable() and ref:is_readable())

	-- Uncomment for error: drawing to ref
	-- ref:fill("red")
	-- ref:draw_pixel({x=0, y=0}, "red")

	-- Uncomment for error: drawing to self
	-- ct:draw_texture({x=0, y=0, nx=0, ny=0}, ct)

	-- Uncomment for error: reading from write-only texture
	-- ct:draw_texture({x=0, y=0, nx=0, ny=0}, minetest.window)

	local max_size = minetest.get_driver_info().max_texture_size

	-- Uncomment for warning: unable to create texture
	-- assert(max_size.x ~= math.huge and max_size.y ~= math.huge)
	-- Texture.new({x=max_size.x + 1, y=max_size.y + 1})

	local function assert_eq(first, second)
		assert(first.x == second.x and first.y == second.y)
	end

	assert_eq(minetest.get_optimal_texture_size({x=0, y=0}), {x=1, y=1})
	assert_eq(minetest.get_optimal_texture_size({x=max_size.x + 1, y=max_size.y + 1}), max_size)
	assert_eq(minetest.get_optimal_texture_size({x=17, y=15}, true, false), {x=32, y=16})
	assert_eq(minetest.get_optimal_texture_size({x=15, y=31}, true, true), {x=32, y=32})
	assert_eq(minetest.get_optimal_texture_size({x=15, y=31}, false, true), {x=31, y=31})
	assert_eq(minetest.get_optimal_texture_size({x=9, y=31}, false, false, {x=10, y=10}), {x=9, y=10})
	assert_eq(minetest.get_optimal_texture_size({x=7, y=31}, true, false, {x=17, y=17}), {x=8, y=16})
	assert_eq(minetest.get_optimal_texture_size({x=100000, y=100000}, false, false, {x=math.huge, y=math.huge}), {x=100000, y=100000})
	assert_eq(minetest.get_optimal_texture_size({x=33, y=17}, true, false, nil, true), {x=32, y=16})
	assert_eq(minetest.get_optimal_texture_size({x=33, y=17}, true, true, nil, true), {x=16, y=16})
end)

minetest.register_on_draw(function(dtime)
	local target = test_draw.target
	if not target then
		return
	end

	local dfont = Font()
	local mfont = Font{face="mono"}
	local dfont_table = dfont:to_table()
	local mfont_table = mfont:to_table()

	--[[ Fill ]]
	if test_draw.fill then
		target:fill("#DDDDDD55")
	end

	--[[ Dtime ]]
	target:draw_text({x=300, y=0}, "Time in milliseconds since last frame: " .. dtime * 1000)

	--[[ Size ]]
	local size = target:get_size()
	local metrics = dfont:get_metrics()
	target:draw_text({x=0, ny=metrics.height + 3}, "Screen size testing")
	target:draw_rect({x=0, y=size.y - 3, w=size.x, h=3}, "pink")
	target:draw_rect({x=1, y=size.y - 2, w=size.x - 2, h=1}, "red")

	--[[ Pixel ]]
	-- Basic
	target:draw_text({x=1, y=0}, "\\_ Green pixel here", nil, nil, "limegreen")
	target:draw_pixel({x=0, y=0}, "#00FF00")

	local text_width = dfont:get_text_width("Black pixel here -\\")
	target:draw_text({nx=text_width + 1, ny=metrics.height + 1}, "Black pixel here -\\", nil, nil, "black")
	target:draw_pixel({nx=0, ny=0}, "#000000")

	-- Pixel clipping
	target:draw_text({x=50, y=50 - metrics.height}, "Pixel clipping")
	target:draw_rect({x=50, y=50, w=100, h=100}, "#00FF0055")
	for i = 0, size.y - 1 do
		target:draw_pixel({x=i, y=i}, "#00FF00", {x=50, y=50, w=100, h=100})
	end

	--[[ Rect ]]
	-- Basic and Clipping
	target:draw_text({x=200, y=50 - metrics.height}, "Rect clipping")
	target:draw_rect({x=200, y=50, w=100, h=100}, "#0000FF55")
	target:draw_rect({x=200, y=50, w=100, h=100}, "#0000FF", {x=190, y=70, w=120, h=60})

	-- Gradient
	target:draw_text({x=350, y=50 - metrics.height}, "Gradient")
	target:draw_rect({x=350, y=50, w=100, h=100}, {"yellow", "orange", "orange", "red"})

	--[[ Texture ]]
	-- Basic
	target:draw_text({x=500, y=50 - metrics.height}, "Basic texture")
	target:draw_texture({x=500, y=50, w=100, h=100}, "default_dirt.png")

	-- Clipping
	target:draw_text({x=650, y=50 - metrics.height * 2}, "Clipping +")
	target:draw_text({x=650, y=50 - metrics.height}, "texture modifiers")
	target:draw_texture({x=650, y=50, w=100, h=100}, "default_dirt.png^default_grass_side.png", nil, nil, "#FFFFFF90")
	target:draw_texture({x=650, y=50, w=100, h=100}, "default_dirt.png^default_grass_side.png", {x=640, y=75, w=120, h=50})

	-- Recolor
	target:draw_text({x=800, y=50 - metrics.height}, "Recolorization")
	target:draw_texture({x=800, y=50, w=100, h=100}, "default_stone.png", nil, nil, "pink")

	-- Source rect
	target:draw_text({x=50, y=200 - metrics.height}, "Source rect")
	target:draw_rect({x=50, y=200, w=100, h=100}, "#00000055")
	target:draw_texture({x=50, y=200, w=100, h=100}, "default_apple.png", nil, {x=5, y=7, w=6, h=9})
	-- Source rect with clipping
	target:draw_text({x=200, y=200 - metrics.height}, "Source + clipping")
	target:draw_rect({x=200, y=200, w=100, h=100}, "#00000055")
	target:draw_texture({x=200, y=200, w=100, h=100}, "default_apple.png", nil, {nx=5, ny=0, w=6, h=9}, "#FFFFFF90")
	target:draw_texture({x=200, y=200, w=100, h=100}, "default_apple.png", {x=190, y=225, w=120, h=50}, {x=5, y=7, nx=5, ny=0})

	-- Translucency
	target:draw_text({x=350, y=200 - metrics.height}, "Translucency")
	target:draw_texture({x=350, y=200, w=100, h=100}, "testnodes_liquid.png")

	-- Custom textures
	target:draw_text({x=500, y=200 - metrics.height}, "Custom texture")
	local ct = Texture.new({x=100, y=100})
	ct:fill("#FFFFFF20")
	ct:draw_pixel({x=0, y=0}, "black")
	ct:draw_pixel({nx=0, y=0}, "black")
	ct:draw_pixel({x=0, ny=0}, "black")
	ct:draw_pixel({nx=0, ny=0}, "black")
	ct:draw_rect({x=16, y=16, nx=16, ny=16}, "#FFFFFFB0")
	ct:draw_text({x=16, y=16}, "Custom")
	ct:draw_text({x=16, y=16 + metrics.height}, "Texture")
	size = ct:get_size()
	ct:draw_text({x=16, y=16 + metrics.height * 2}, "(" .. size.x .. ", " .. size.y .. ")")
	-- Also tests Texture.copy
	target:draw_texture({x=500, y=200, w=100, h=100}, Texture.copy(ct))

	target:draw_text({x=650, y=200 - metrics.height}, "Premade custom")
	-- Also tests Texture.ref reading
	target:draw_texture({x=650, y=200, w=100, h=100}, Texture.ref(test_draw.premade_ct))

	--[[ Text ]]
	-- Plain text alignment (text size testing)
	target:draw_rect({x=50, y=350, w=200, h=metrics.height * 3 + 4}, "#00FF0055")
	---
	target:draw_rect({x=50, y=350, w=200, h=1}, "#00FF00")
	text_width = dfont:get_text_width("Left alignment")
	target:draw_rect({x=50, y=350 + 1, w=text_width, h=metrics.height}, "#00FF0055")
	target:draw_text({x=50, y=350 + 1}, "Left alignment")
	---
	target:draw_rect({x=50, y=350 + 1 + metrics.height, w=200, h=1}, "#00FF00")
	text_width = dfont:get_text_width("Center alignment")
	target:draw_rect({x=150 - text_width / 2, y=350 + 2 + metrics.height, w=text_width, h=metrics.height}, "#00FF0055")
	target:draw_text({x=150 - text_width / 2, y=350 + 2 + metrics.height}, "Center alignment")
	---
	target:draw_rect({x=50, y=350 + 2 + metrics.height * 2, w=200, h=1}, "#00FF00")
	text_width = dfont:get_text_width("Right alignment")
	target:draw_rect({x=250 - text_width, y=350 + 3 + metrics.height * 2, w=text_width, h=metrics.height}, "#00FF0055")
	target:draw_text({x=250 - text_width, y=350 + 3 + metrics.height * 2}, "Right alignment")
	---
	target:draw_rect({x=50, y=350 + 3 + metrics.height * 3, w=200, h=1}, "#00FF00")

	-- Different font alignment (text size testing)
	local ofont = Font{face="mono", size=mfont_table.size * 1.5, bold=true}
	local ometrics = ofont:get_metrics()
	target:draw_rect({x=300, y=350, w=350, h=ometrics.height * 3 + 4}, "#FFA50055")
	---
	target:draw_rect({x=300, y=350, w=350, h=1}, "#FFA500")
	text_width = ofont:get_text_width("Font left align", ofont)
	target:draw_rect({x=300, y=350 + 1, w=text_width, h=ometrics.height}, "#FFA50055")
	target:draw_text({x=300, y=350 + 1}, "Font left align", nil, ofont)
	---
	target:draw_rect({x=300, y=350 + 1 + ometrics.height, w=350, h=1}, "#FFA500")
	text_width = ofont:get_text_width("Font center align", ofont)
	target:draw_rect({x=475 - text_width / 2, y=350 + 2 + ometrics.height, w=text_width, h=ometrics.height}, "#FFA50055")
	target:draw_text({x=475 - text_width / 2, y=350 + 2 + ometrics.height}, "Font center align", nil, ofont)
	---
	target:draw_rect({x=300, y=350 + 2 + ometrics.height * 2, w=350, h=1}, "#FFA500")
	text_width = ofont:get_text_width("Font right align", ofont)
	target:draw_rect({x=650 - text_width, y=350 + 3 + ometrics.height * 2, w=text_width, h=ometrics.height}, "#FFA50055")
	target:draw_text({x=650 - text_width, y=350 + 3 + ometrics.height * 2}, "Font right align", nil, ofont)
	---
	target:draw_rect({x=300, y=350 + 3 + ometrics.height * 3, w=350, h=1}, "#FFA500")

	-- Clipping
	text_width = dfont:get_text_width("This text should be clipped this text should be clipped")
	target:draw_rect({x=50, y=490, w=200, h=metrics.height}, "#00000055")
	target:draw_text({x=150 - text_width / 2, y=490 - metrics.height / 2}, "This text should be clipped this text should be clipped", {x=50, y=490, w=200, h=metrics.height})
	target:draw_text({x=150 - text_width / 2, y=490 + metrics.height / 2}, "This text should be clipped this text should be clipped", {x=50, y=490, w=200, h=metrics.height})

	-- Fonts
	local mmetrics = mfont:get_metrics()

	target:draw_text({x=50, y=520}, "Bold", nil, Font{bold=true})
	target:draw_text({x=50, y=550}, "Italic", nil, Font{italic=true})
	target:draw_text({x=135, y=520}, "Mono (字形)", nil, mfont)
	target:draw_text({x=135, y=550}, "Fallback (字形)")
	target:draw_text({x=270, y=490}, "Underline")
	target:draw_rect({x=270, y=490 + metrics.underline_pos, w=dfont:get_text_width("Underline"), h=metrics.line_thickness}, "white")
	target:draw_text({x=270, y=520}, "Strike")
	target:draw_rect({x=270, y=520 + metrics.strike_pos, w=dfont:get_text_width("Strike"), h=metrics.line_thickness}, "white")
	target:draw_text({x=270, y=550}, "Mono Under", nil, mfont)
	target:draw_rect({x=270, y=550 + mmetrics.underline_pos, w=mfont:get_text_width("Mono Under"), h=mmetrics.line_thickness}, "white")
	target:draw_text({x=380, y=490}, "Mono Strike", nil, mfont)
	target:draw_rect({x=380, y=490 + mmetrics.strike_pos, w=mfont:get_text_width("Mono Strike"), h=mmetrics.line_thickness}, "white")
	target:draw_text({x=380, y=520}, "Small", nil, Font{face="mono", italic=true, size=dfont_table.size * 3/4})
	target:draw_text({x=380, y=540}, "Big", nil, Font{face="mono", bold=true, size=dfont_table.size * 2.5})
	target:draw_text({x=500, y=490}, "Red", nil, nil, "red")
	ofont = Font{bold=true, italic=true, size=mfont_table.size * 3/4}
	ometrics = ofont:get_metrics()
	target:draw_text({x=500, y=520}, "All, Small", nil, ofont, "red")
	target:draw_rect({x=500, y=520 + ometrics.underline_pos, w=ofont:get_text_width("All, Small"), h=ometrics.line_thickness}, "red")
	ofont = Font{bold=true, italic=true, size=mfont_table.size * 2.5}
	ometrics = ofont:get_metrics()
	target:draw_text({x=500, y=540}, "All, Big", nil, ofont, "red")
	target:draw_rect({x=500, y=540 + ometrics.underline_pos, w=ofont:get_text_width("All, Big"), h=ometrics.line_thickness}, "red")
	target:draw_text({x=650, y=490}, "Baseline")
	target:draw_rect({x=650, y=490 + metrics.baseline, w=dfont:get_text_width("Baseline"), h=1}, "white")
	target:draw_text({x=650, y=520}, "Line height: 1")
	target:draw_text({x=650, y=520 + metrics.line_height}, "Line height: 2")

	if target ~= minetest.window then
		minetest.window:draw_texture({x=0, y=0, nx=0, ny=0}, target)
		target:fill("#0000")
	end
end)
