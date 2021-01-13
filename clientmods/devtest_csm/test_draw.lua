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
		print(param)
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
	assert(size.x == 32 and size.y == 32, "Incorrect texture size for custom premade texture")

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

	-- Uncomment for warning: unable to create texture
	-- local size = minetest.get_driver_info().max_texture_size
	-- Texture.new({x=size.x + 1, y=size.y + 1})
end)

minetest.register_on_draw(function(dtime)
	local target = test_draw.target
	if not target then
		return
	end

	--[[ Fill ]]
	if test_draw.fill then
		target:fill("#FFFFFF55")
	end

	--[[ Dtime ]]
	target:draw_text({x=700, y=0}, "Time in milliseconds since last frame: " .. dtime * 1000)

	--[[ Size ]]
	local size = target:get_size()
	local text_size = minetest.get_text_size("Screen size testing")
	target:draw_text({x=0, ny=text_size.y + 3}, "Screen size testing")
	target:draw_rect({x=0, y=size.y - 3, w=size.x, h=3}, "pink")
	target:draw_rect({x=1, y=size.y - 2, w=size.x - 2, h=1}, "red")

	--[[ Pixel ]]
	-- Basic
	target:draw_text({x=1, y=0}, "\\_ Green pixel here", nil, {color="limegreen"})
	target:draw_pixel({x=0, y=0}, "#00FF00")

	text_size = minetest.get_text_size("Black pixel here -\\")
	target:draw_text({nx=text_size.x + 1, ny=text_size.y + 1}, "Black pixel here -\\", nil, {color="black"})
	target:draw_pixel({nx=1, ny=1}, "#000000")

	-- Clipping
	target:draw_text({x=100, y=100 - text_size.y}, "Pixel clipping")
	target:draw_rect({x=100, y=100, w=100, h=100}, "#00FF0055")
	for i = 0, size.y - 1 do
		target:draw_pixel({x=i, y=i}, "#00FF00", {x=100, y=100, w=100, h=100})
	end

	--[[ Rect ]]
	-- Basic and Clipping
	target:draw_text({x=250, y=100 - text_size.y}, "Rect clipping")
	target:draw_rect({x=250, y=100, w=100, h=100}, "#0000FF55")
	target:draw_rect({x=250, y=100, w=100, h=100}, "#0000FF", {x=240, y=120, w=120, h=60})

	-- Gradient
	target:draw_text({x=400, y=100 - text_size.y}, "Gradient")
	target:draw_rect({x=400, y=100, w=100, h=100}, {"yellow", "orange", "orange", "red"})

	--[[ Texture ]]
	-- Basic
	target:draw_text({x=550, y=100 - text_size.y}, "Basic texture")
	target:draw_texture({x=550, y=100, w=100, h=100}, "default_dirt.png")

	-- Clipping
	target:draw_text({x=700, y=100 - text_size.y * 2}, "Clipping +")
	target:draw_text({x=700, y=100 - text_size.y}, "texture modifiers")
	target:draw_texture({x=700, y=100, w=100, h=100}, "default_dirt.png^default_grass_side.png", nil, nil, nil, "#FFFFFF90")
	target:draw_texture({x=700, y=100, w=100, h=100}, "default_dirt.png^default_grass_side.png", {x=690, y=125, w=120, h=50})

	-- Reolor
	target:draw_text({x=850, y=100 - text_size.y}, "Recolorization")
	target:draw_texture({x=850, y=100, w=100, h=100}, "default_stone.png", nil, nil, nil, "pink")

	-- Source rect
	target:draw_text({x=1000, y=100 - text_size.y}, "Source rect")
	target:draw_rect({x=1000, y=100, w=100, h=100}, "#00000055")
	target:draw_texture({x=1000, y=100, w=100, h=100}, "default_apple.png", nil, {x=5, y=7, w=6, h=9})
	-- Source rect with clipping
	target:draw_text({x=1150, y=100 - text_size.y}, "Source + clipping")
	target:draw_rect({x=1150, y=100, w=100, h=100}, "#00000055")
	target:draw_texture({x=1150, y=100, w=100, h=100}, "default_apple.png", nil, {nx=5, ny=0, w=6, h=9}, nil, "#FFFFFF90")
	target:draw_texture({x=1150, y=100, w=100, h=100}, "default_apple.png", {x=1140, y=125, w=120, h=50}, {x=5, y=7, nx=5, ny=0})

	-- Middle rect
	target:draw_text({x=100, y=250 - text_size.y}, "Middle rect")
	target:draw_texture({x=100, y=250, w=100, h=100}, "testnodes_slippery.png", nil, nil, {x=2, y=2, nx=2, ny=2})
	-- Middle rect with clipping
	target:draw_text({x=250, y=250 - text_size.y}, "Middle + clipping")
	target:draw_texture({x=250, y=250, w=100, h=100}, "testnodes_slippery.png", nil, nil, {x=2, y=2, nx=2, ny=2}, "#FFFFFF90")
	target:draw_texture({x=250, y=250, w=100, h=100}, "testnodes_slippery.png", {x=240, y=275, w=120, h=50}, nil, {nx=2, ny=2, w=12, h=12})
	-- Middle and source rects
	target:draw_text({x=400, y=250 - text_size.y}, "Middle + source")
	target:draw_texture({x=400, y=250, w=100, h=100}, "testnodes_slippery.png", nil, {x=1, y=1, nx=1, ny=1}, {x=1, y=1, w=12, ny=1})
	-- Middle and source rects with clipping
	target:draw_text({x=550, y=250 - text_size.y * 2}, "Middle + source +")
	target:draw_text({x=550, y=250 - text_size.y}, "clipping")
	target:draw_texture({x=550, y=250, w=100, h=100}, "testnodes_slippery.png", nil, {x=1, y=1, nx=1, ny=1}, {x=1, y=1, nx=1, ny=1}, "#FFFFFF90")
	target:draw_texture({x=550, y=250, w=100, h=100}, "testnodes_slippery.png", {x=540, y=275, w=120, h=50}, {x=1, y=1, nx=1, ny=1}, {x=1, y=1, nx=1, ny=1})

	-- Translucency
	target:draw_text({x=700, y=250 - text_size.y}, "Translucency")
	target:draw_texture({x=700, y=250, w=100, h=100}, "testnodes_liquid.png")
	-- No alpha
	target:draw_text({x=850, y=250 - text_size.y * 2}, "Translucency +")
	target:draw_text({x=850, y=250 - text_size.y}, "no alpha")
	target:draw_texture({x=850, y=250, w=100, h=100}, "testnodes_liquid.png", nil, nil, nil, nil, true)
	target:draw_text({x=1000, y=250 - text_size.y * 2}, "Transparency +")
	target:draw_text({x=1000, y=250 - text_size.y}, "no alpha")
	target:draw_texture({x=1000, y=250, w=100, h=100}, "default_apple.png", nil, nil, nil, nil, true)

	-- Custom textures
	target:draw_text({x=1150, y=250 - text_size.y}, "Custom texture")
	local ct = Texture.new({x=100, y=100})
	ct:draw_pixel({x=0, y=0}, "black")
	ct:draw_pixel({nx=1, y=0}, "black")
	ct:draw_pixel({x=0, ny=1}, "black")
	ct:draw_pixel({nx=1, ny=1}, "black")
	ct:draw_rect({x=16, y=16, nx=16, ny=16}, "#FFFFFFB0")
	ct:draw_text({x=16, y=16}, "Custom")
	ct:draw_text({x=16, y=16 + text_size.y}, "Texture")
	size = ct:get_size()
	ct:draw_text({x=16, y=16 + text_size.y * 2}, "(" .. size.x .. ", " .. size.y .. ")")
	-- Also tests Texture.copy
	target:draw_texture({x=1150, y=250, w=100, h=100}, Texture.copy(ct))

	target:draw_text({x=1150, y=400 - text_size.y}, "Premade custom")
	-- Also tests Texture.ref reading
	target:draw_texture({x=1150, y=400, w=100, h=100}, Texture.ref(test_draw.premade_ct))

	--[[ Item ]]
	-- Basic
	target:draw_text({x=100, y=400 - text_size.y}, "Tool")
	target:draw_rect({x=100, y=400, w=100, h=100}, "#00000055")
	target:draw_item({x=100, y=400, w=100, h=100}, "basetools:pick_mese")

	-- No item count (item counts should not appear)
	target:draw_text({x=250, y=400 - text_size.y * 2}, "Node, count 10")
	target:draw_text({x=250, y=400 - text_size.y}, "(count should not appear)", nil, {size=minetest.get_font_size() * 2/3})
	target:draw_rect({x=250, y=400, w=100, h=100}, "#00000055")
	target:draw_item({x=250, y=400, w=100, h=100}, "basenodes:dirt_with_grass 10")

	-- Clipping
	target:draw_text({x=400, y=400 - text_size.y}, "Clipping")
	target:draw_rect({x=400, y=400, w=100, h=100}, "#00000055")
	target:draw_rect({x=390, y=425, w=120, h=50}, "#00000055")
	target:draw_item({x=400, y=400, w=100, h=100}, "basenodes:dirt_with_grass", {x=390, y=425, w=120, h=50})

	-- Angle
	target:draw_text({x=550, y=400 - text_size.y}, "Front angle")
	target:draw_rect({x=550, y=400, w=100, h=100}, "#00000055")
	target:draw_item({x=550, y=400, w=100, h=100}, "chest_of_everything:chest 10", nil, {x=30, y=-45, z=0})

	-- Multiple angles
	target:draw_text({x=700, y=400 - text_size.y}, "Moving angle")
	test_draw.last_angle.x = test_draw.last_angle.x + 1
	test_draw.last_angle.y = test_draw.last_angle.y + 2
	test_draw.last_angle.z = test_draw.last_angle.z + 3
	target:draw_rect({x=700, y=400, w=100, h=100}, "#00000055")
	target:draw_item({x=700, y=400, w=100, h=100}, "basetools:pick_mese", nil, test_draw.last_angle)

	--[[ Text ]]
	-- Plain text alignment (text size testing)
	target:draw_rect({x=100, y=550, w=200, h=text_size.y * 3 + 4}, "#00FF0055")
	---
	target:draw_rect({x=100, y=550, w=200, h=1}, "#00FF00")
	text_size = minetest.get_text_size("Left alignment")
	target:draw_rect({x=100, y=551, w=text_size.x, h=text_size.y}, "#00FF0055")
	target:draw_text({x=100, y=551}, "Left alignment")
	---
	target:draw_rect({x=100, y=551 + text_size.y, w=200, h=1}, "#00FF00")
	text_size = minetest.get_text_size("Center alignment")
	target:draw_rect({x=200 - text_size.x / 2, y=552 + text_size.y, w=text_size.x, h=text_size.y}, "#00FF0055")
	target:draw_text({x=200 - text_size.x / 2, y=550 + text_size.y + 1}, "Center alignment")
	---
	target:draw_rect({x=100, y=552 + text_size.y * 2, w=200, h=1}, "#00FF00")
	text_size = minetest.get_text_size("Right alignment")
	target:draw_rect({x=300 - text_size.x, y=553 + text_size.y * 2, w=text_size.x, h=text_size.y}, "#00FF0055")
	target:draw_text({x=300 - text_size.x, y=550 + text_size.y * 2 + 2}, "Right alignment")
	---
	target:draw_rect({x=100, y=553 + text_size.y * 3, w=200, h=1}, "#00FF00")

	-- Different font alignment (text size testing)
	local font = {font="mono", size=minetest.get_font_size("mono") * 1.5, bold=true}
	text_size = minetest.get_text_size("", font)
	target:draw_rect({x=350, y=550, w=350, h=text_size.y * 3 + 4}, "#FFA50055")
	---
	target:draw_rect({x=350, y=550, w=350, h=1}, "#FFA500")
	text_size = minetest.get_text_size("Font left align", font)
	target:draw_rect({x=350, y=551, w=text_size.x, h=text_size.y}, "#FFA50055")
	target:draw_text({x=350, y=551}, "Font left align", nil, font)
	---
	target:draw_rect({x=350, y=551 + text_size.y, w=350, h=1}, "#FFA500")
	text_size = minetest.get_text_size("Font center align", font)
	target:draw_rect({x=525 - text_size.x / 2, y=552 + text_size.y, w=text_size.x, h=text_size.y}, "#FFA50055")
	target:draw_text({x=525 - text_size.x / 2, y=550 + text_size.y + 1}, "Font center align", nil, font)
	---
	target:draw_rect({x=350, y=552 + text_size.y * 2, w=350, h=1}, "#FFA500")
	text_size = minetest.get_text_size("Font right align", font)
	target:draw_rect({x=700 - text_size.x, y=553 + text_size.y * 2, w=text_size.x, h=text_size.y}, "#FFA50055")
	target:draw_text({x=700 - text_size.x, y=550 + text_size.y * 2 + 2}, "Font right align", nil, font)
	---
	target:draw_rect({x=350, y=553 + text_size.y * 3, w=350, h=1}, "#FFA500")

	-- Clipping
	text_size = minetest.get_text_size("This text should be clipped this text should be clipped")
	target:draw_rect({x=750, y=550, w=200, h=text_size.y}, "#00000055")
	target:draw_text({x=850 - text_size.x / 2, y=550 - text_size.y / 2}, "This text should be clipped this text should be clipped", {x=750, y=550, w=200, h=text_size.y})
	target:draw_text({x=850 - text_size.x / 2, y=550 + text_size.y / 2}, "This text should be clipped this text should be clipped", {x=750, y=550, w=200, h=text_size.y})

	-- Fonts
	target:draw_text({x=750, y=580}, "Bold", nil, {bold=true})
	target:draw_text({x=750, y=610}, "Italic", nil, {italic=true})
	target:draw_text({x=835, y=580}, "Underline", nil, {underline=true})
	target:draw_text({x=835, y=610}, "Strikethrough", nil, {strike=true})
	target:draw_text({x=980, y=550}, "Mono", nil, {font="mono"})
	target:draw_text({x=980, y=580}, "Small", nil, {size=minetest.get_font_size() * 3/4})
	target:draw_text({x=980, y=600}, "Big", nil, {size=minetest.get_font_size() * 2.5})
	target:draw_text({x=1060, y=550}, "Red", nil, {color="red"})
	target:draw_text({x=1060, y=580}, "All, Small", nil, {font="mono", bold=true, italic=true, underline=true, strike=true, color="red", size=minetest.get_font_size() * 3/4})
	target:draw_text({x=1060, y=600}, "All, Big", nil, {font="mono", bold=true, italic=true, underline=true, strike=true, color="red", size=minetest.get_font_size() * 2.5})

	if target ~= minetest.window then
		minetest.window:draw_texture({x=0, y=0, nx=0, ny=0}, target)
		target:fill("#0000")
	end
end)
