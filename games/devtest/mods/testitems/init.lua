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
