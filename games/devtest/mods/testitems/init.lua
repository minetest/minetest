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
	description = "Callback test item 1 (Use/Drop + Sneak to switch to item 2)",
	inventory_image = "testitems_callback_1.png",
	wield_image = "testitems_callback_1.png",

	on_secondary_use = function(itemstack, user, pointed_thing)
		local ctrl = user and user:get_player_control() or {}
		if ctrl.sneak then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
		end

		minetest.chat_send_all("[testitems:callback_1 on_secondary_use] " .. itemstack:get_name())
		return itemstack
	end,

	on_drop = function(itemstack, dropper, pos)
		local ctrl = dropper and dropper:get_player_control() or {}
		if ctrl.sneak then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
		end

		minetest.chat_send_all("[testitems:callback_1 on_drop] " .. itemstack:get_name())
		return minetest.item_drop(itemstack, dropper, pos)
	end,

	on_pickup = function(itemstack, picker, pointed_thing, ...)
		minetest.chat_send_all("[testitems:callback_1 on_pickup]")
		assert(pointed_thing.ref:get_luaentity().name == "__builtin:item")
		local ctrl = picker and picker:get_player_control() or {}
		if ctrl.aux1 then
			-- Debug message
			minetest.chat_send_all(dump({...}))
		end
		if ctrl.sneak then
			-- Pick up one item of the other kind at once
			local taken = itemstack:take_item():set_name("testitems:callback_2")
			picker:get_inventory():add_item("main", taken)
			return itemstack
		end
		if ctrl.up then
			-- Eat it
			return minetest.do_item_eat(2, nil, itemstack, picker, pointed_thing)
		end

		-- Normal: pick up everything
		return minetest.item_pickup(itemstack, picker, pointed_thing, ...)
	end,

	on_use = function(itemstack, user, pointed_thing)
		local ctrl = user and user:get_player_control() or {}
		if ctrl.sneak then
			itemstack = ItemStack(itemstack)
			itemstack:set_name("testitems:callback_2")
		end

		minetest.chat_send_all("[testitems:callback_1 on_use] " .. itemstack:get_name())
		return itemstack
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
	description = "Callback test item 2 (Use to switch to item 1)",
	inventory_image = "testitems_callback_2.png",
	wield_image = "testitems_callback_2.png",

	on_use = function(itemstack, user, pointed_thing)
		minetest.chat_send_all("[testitems:callback_2 on_use]")
		itemstack = ItemStack(itemstack)
		itemstack:set_name("testitems:callback_1")
		return itemstack
	end,
})

minetest.register_on_item_pickup(function(itemstack, picker, pointed_thing, time_from_last_punch, ...)
	assert(pointed_thing.ref:get_luaentity().name == "__builtin:item")

	local item_name = itemstack:get_name()
	minetest.chat_send_all("["..item_name.." register_on_item_pickup (1)]")

	if item_name == "testitems:callback_2" then
		-- Same here. Pick up the other item type.
		itemstack:set_name("testitems:callback_1")
		return picker:get_inventory():add_item("main", itemstack)
	end
end)
