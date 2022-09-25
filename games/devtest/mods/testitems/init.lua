local S = minetest.get_translator("testitems")

--
-- Texture overlays for items
--

-- For the global overlay color test
local GLOBAL_COLOR_ARG = "orange"

-- Punch handler to set random color with "color" argument in item metadata
local overlay_on_use = function(itemstack, user, pointed_thing)
	local meta = itemstack:get_meta()
	local color = math.random(0x0, 0xFFFFFF)
	local colorstr = string.format("#%06x", color)
	meta:set_string("color", colorstr)
	minetest.log("action", "[testitems] Color of "..itemstack:get_name().." changed to "..colorstr)
	return itemstack
end
-- Place handler to clear item metadata color
local overlay_on_place = function(itemstack, user, pointed_thing)
	local meta = itemstack:get_meta()
	meta:set_string("color", "")
	return itemstack
end

minetest.register_craftitem("testitems:overlay_meta", {
	description = S("Texture Overlay Test Item, Meta Color") .. "\n" ..
		S("Image must be a square with rainbow cross (inventory and wield)") .. "\n" ..
		S("Item meta color must only change square color") .. "\n" ..
		S("Punch: Set random color") .. "\n" ..
		S("Place: Clear color"),
	-- Base texture: A grayscale square (can be colorized)
	inventory_image = "testitems_overlay_base.png",
	wield_image = "testitems_overlay_base.png",
	-- Overlay: A rainbow cross (NOT to be colorized!)
	inventory_overlay = "testitems_overlay_overlay.png",
	wield_overlay = "testitems_overlay_overlay.png",

	on_use = overlay_on_use,
	on_place = overlay_on_place,
	on_secondary_use = overlay_on_place,
})
minetest.register_craftitem("testitems:overlay_global", {
	description = S("Texture Overlay Test Item, Global Color") .. "\n" ..
		S("Image must be an orange square with rainbow cross (inventory and wield)"),
	-- Base texture: A grayscale square (to be colorized)
	inventory_image = "testitems_overlay_base.png",
	wield_image = "testitems_overlay_base.png",
	-- Overlay: A rainbow cross (NOT to be colorized!)
	inventory_overlay = "testitems_overlay_overlay.png",
	wield_overlay = "testitems_overlay_overlay.png",
	color = GLOBAL_COLOR_ARG,
})


--
-- Item callbacks
--

minetest.register_craftitem("testitems:callback_1", {
	description = S("Callback test item 1"),
	inventory_image = "testitems_callback_1.png",
	wield_image = "testitems_callback_1.png",

	on_secondary_use = function(itemstack, user, pointed_thing)
		minetest.chat_send_all("[testitems:callback_1 on_secondary_use]")
		local ctrl = user and user:get_player_control() or {}
		if ctrl.sneak then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
			return itemstack
		end
	end,

	on_drop = function(itemstack, dropper, pos)
		minetest.chat_send_all("[testitems:callback_1 on_drop]")
		local ctrl = dropper and dropper:get_player_control() or {}
		if ctrl.sneak then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
		end
		return minetest.item_drop(itemstack, dropper, pos)
	end,

	on_pickup = function(itemstack, picker, obj, ...)
		minetest.chat_send_all("[testitems:callback_1 on_pickup]")
		assert(obj:get_luaentity().name == "__builtin:item")
		local ctrl = picker and picker:get_player_control() or {}
		if ctrl.left then
			minetest.chat_send_all(dump({...}))
		end
		if ctrl.up then
			return false
		elseif ctrl.sneak then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
			return itemstack
		else
			return true
		end
	end,

	on_use = function(itemstack, user, pointed_thing)
		minetest.chat_send_all("[testitems:callback_1 on_use]")
		local ctrl = user and user:get_player_control() or {}
		if ctrl.sneak then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
			return itemstack
		end
	end,

	after_use = function(itemstack, user, node, digparams) -- never called
		minetest.chat_send_all("[testitems:callback_1 after_use]")
		local ctrl = user and user:get_player_control() or {}
		if ctrl.up then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
			return itemstack
		end
	end,
})

minetest.register_craftitem("testitems:callback_2", {
	description = S("Callback test item 2"),
	inventory_image = "testitems_callback_2.png",
	wield_image = "testitems_callback_2.png",

	on_use = function(itemstack, user, pointed_thing)
		minetest.chat_send_all("[testitems:callback_2 on_use]")
		itemstack = ItemStack(itemstack)
		itemstack:set_name("testitems:callback_1")
		return itemstack
	end,
})

minetest.register_on_item_pickup(function(itemstack, picker, obj, time_from_last_punch,  ...)
	local item_name = itemstack:get_name()
	if item_name ~= "testitems:callback_1" and item_name ~= "testitems:callback_2" then
		return
	end
	minetest.chat_send_all("["..item_name.." register_on_item_pickup (1)]")
end)

minetest.register_on_item_pickup(function(itemstack, picker, obj, time_from_last_punch,  ...)
	local item_name = itemstack:get_name()
	if item_name ~= "testitems:callback_1" and item_name ~= "testitems:callback_2" then
		return
	end
	minetest.chat_send_all("["..item_name.." register_on_item_pickup (2)]")
	assert(obj:get_luaentity().name == "__builtin:item")
	local ctrl = picker and picker:get_player_control() or {}
	if ctrl.down then
		return true
	end
end)

minetest.register_on_item_pickup(function(itemstack, picker, obj, time_from_last_punch,  ...)
	local item_name = itemstack:get_name()
	if item_name ~= "testitems:callback_1" and item_name ~= "testitems:callback_2" then
		return
	end
	minetest.chat_send_all("["..item_name.." register_on_item_pickup (3)]")
end)
