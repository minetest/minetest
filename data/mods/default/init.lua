-- default (Minetest 0.4 mod)
-- Most default stuff

-- Quick documentation about the API
-- =================================
--
-- Helper functions defined by builtin.lua:
-- dump2(obj, name="_", dumped={})
-- dump(obj, dumped={})
--
-- Mod load path
-- -------------
-- Generic:
-- $path_data/mods/
-- $path_userdata/usermods/
-- $mapdir/worldmods/
--
-- On a run-in-place version (eg. the distributed windows version):
-- minetest-0.4.x/data/mods/
-- minetest-0.4.x/usermods/
-- minetest-0.4.x/world/worldmods/
--
-- On an installed version on linux:
-- /usr/share/minetest/mods/
-- ~/.minetest/usermods
-- ~/.minetest/world/worldmods
--
-- Naming convention for registered textual names
-- ----------------------------------------------
-- "modname:<whatever>" (<whatever> can have characters a-zA-Z0-9_)
--
-- This is to prevent conflicting names from corrupting maps and is
-- enforced by the mod loader.
--
-- Example: mod "experimental", ideal item/node/entity name "tnt":
--          -> the name should be "experimental:tnt".
--
-- Enforcement can be overridden by prefixing the name with ":". This can
-- be used for overriding the registrations of some other mod.
--
-- Example: Any mod can redefine experimental:tnt by using the name
--          ":experimental:tnt" when registering it.
-- (also that mods is required to have "experimental" as a dependency)
--
-- The legacy mod uses ":" for maintaining backwards compatibility.
--
-- Textures
-- --------
-- Mods should generally prefix their textures with modname_, eg. given
-- the mod name "foomod", a texture could be called "default_foomod_superfurnace.png"
--
-- This is not crucial and a conflicting name will not corrupt maps.
--
-- Representations of simple things
-- --------------------------------
--
-- MapNode representation:
-- {name="name", param1=num, param2=num}
--
-- param1 and param2 are 8 bit and 4 bit integers, respectively. They
-- are reserved for certain automated functions. If you don't use these
-- functions, you can use them to store arbitrary values.
--
-- param1 is reserved for the engine when:
--   paramtype != "none"
-- param2 is reserved for the engine when any of these are used:
--   liquidtype == "flowing"
--   drawtype == "flowingliquid"
--   drawtype == "torchlike"
--   drawtype == "signlike"
--
-- Position representation:
-- {x=num, y=num, z=num}
--
-- stackstring/itemstring: A stack of items in serialized format.
-- eg. 'node "dirt" 5'
-- eg. 'tool "default:pick_wood" 21323'
-- eg. 'craft "apple" 2'
--
-- item: A single item in Lua table format.
-- eg. {type="node", name="dirt"} 
--     ^ a single dirt node
-- eg. {type="tool", name="default:pick_wood", wear=21323}
--     ^ a wooden pick about 1/3 weared out
-- eg. {type="craft", name="apple"}
--     ^ an apple.
--
-- Global functions:
-- minetest.register_entity(name, prototype table)
-- minetest.register_tool(name, tool definition)
-- minetest.register_node(name, node definition)
-- minetest.register_craftitem(name, craftitem definition)
-- minetest.register_craft(recipe)
-- minetest.register_abm(abm definition)
-- minetest.alias_node(name, convert_to)
-- minetest.alias_tool(name, convert_to)
-- minetest.alias_craftitem(name, convert_to)
-- minetest.register_globalstep(func(dtime))
-- minetest.register_on_placenode(func(pos, newnode, placer))
-- minetest.register_on_dignode(func(pos, oldnode, digger))
-- minetest.register_on_punchnode(func(pos, node, puncher))
-- minetest.register_on_generated(func(minp, maxp))
-- minetest.register_on_newplayer(func(ObjectRef))
-- minetest.register_on_respawnplayer(func(ObjectRef))
-- ^ return true in func to disable regular player placement
-- minetest.register_on_chat_message(func(name, message))
-- minetest.add_to_creative_inventory(itemstring)
-- minetest.setting_get(name) -> string or nil
-- minetest.setting_getbool(name) -> boolean value or nil
-- minetest.chat_send_all(text)
-- minetest.chat_send_player(name, text)
-- minetest.get_player_privs(name) -> set of privs
-- minetest.get_modpath(modname) -> eg. "/home/user/.minetest/usermods/modname"
--
-- stackstring_take_item(stackstring) -> stackstring, item
-- stackstring_put_item(stackstring, item) -> stackstring, success
-- stackstring_put_stackstring(stackstring, stackstring) -> stackstring, success
--
-- minetest.digprop_constanttime(time)
-- minetest.digprop_stonelike(toughness)
-- minetest.digprop_dirtlike(toughness)
-- minetest.digprop_gravellike(toughness)
-- minetest.digprop_woodlike(toughness)
-- minetest.digprop_leaveslike(toughness)
-- minetest.digprop_glasslike(toughness)
--
-- Global objects:
-- minetest.env - environment reference
--
-- Global tables:
-- minetest.registered_nodes
-- ^ List of registered node definitions, indexed by name
-- minetest.registered_craftitems
-- ^ List of registered craft item definitions, indexed by name
-- minetest.registered_entities
-- ^ List of registered entity prototypes, indexed by name
-- minetest.object_refs
-- ^ List of object references, indexed by active object id
-- minetest.luaentities
-- ^ List of lua entities, indexed by active object id
--
-- EnvRef is basically ServerEnvironment and ServerMap combined.
-- EnvRef methods:
-- - add_node(pos, node)
-- - remove_node(pos)
-- - get_node(pos)
--   ^ Returns {name="ignore", ...} for unloaded area
-- - get_node_or_nil(pos)
--   ^ Returns nil for unloaded area
-- - get_node_light(pos, timeofday) -> 0...15 or nil
--   ^ timeofday: nil = current time, 0 = night, 0.5 = day
-- - add_entity(pos, name)
-- - add_item(pos, itemstring)
-- - add_rat(pos)
-- - add_firefly(pos)
-- - get_meta(pos) -- Get a NodeMetaRef at that position
-- - get_player_by_name(name) -- Get an ObjectRef to a player
--
-- NodeMetaRef
-- - get_type()
-- - allows_text_input()
-- - set_text(text) -- eg. set the text of a sign
-- - get_text()
-- - get_owner()
-- Generic node metadata specific:
-- - set_infotext(infotext)
-- - inventory_set_list(name, {item1, item2, ...})
-- - inventory_get_list(name)
-- - set_inventory_draw_spec(string)
-- - set_allow_text_input(bool)
-- - set_allow_removal(bool)
-- - set_enforce_owner(bool)
-- - is_inventory_modified()
-- - reset_inventory_modified()
-- - is_text_modified()
-- - reset_text_modified()
-- - set_string(name, value)
-- - get_string(name)
--
-- ObjectRef is basically ServerActiveObject.
-- ObjectRef methods:
-- - remove(): remove object (after returning from Lua)
-- - getpos() -> {x=num, y=num, z=num}
-- - setpos(pos); pos={x=num, y=num, z=num}
-- - moveto(pos, continuous=false): interpolated move
-- - punch(puncher, time_from_last_punch)
--   ^ puncher = an another ObjectRef,
--   ^ time_from_last_punch = time since last punch action of the puncher
-- - right_click(clicker); clicker = an another ObjectRef
-- - get_wield_digging_properties() -> digging property table
-- - add_to_inventory_later(itemstring): like above, but after callback returns (only allowed for craftitem callbacks)
-- - get_hp(): returns number of hitpoints (2 * number of hearts)
-- - set_hp(hp): set number of hitpoints (2 * number of hearts)
-- LuaEntitySAO-only:
-- - setvelocity({x=num, y=num, z=num})
-- - setacceleration({x=num, y=num, z=num})
-- - getacceleration() -> {x=num, y=num, z=num}
-- - settexturemod(mod)
-- - setsprite(p={x=0,y=0}, num_frames=1, framelength=0.2,
-- -           select_horiz_by_yawpitch=false)
-- Player-only:
-- - get_player_name(): will return nil if is not a player
-- - inventory_set_list(name, {item1, item2, ...})
-- - inventory_get_list(name) -> {item1, item2, ...}
-- - damage_wielded_item(num) (item damage/wear range is 0-65535)
-- - add_to_inventory(itemstring): add an item to object inventory
--
-- Registered entities:
-- - Functions receive a "luaentity" as self:
--   - It has the member .object, which is an ObjectRef pointing to the object
--   - The original prototype stuff is visible directly via a metatable
-- - Callbacks:
--   - on_activate(self, staticdata)
--   - on_step(self, dtime)
--   - on_punch(self, hitter)
--   - on_rightclick(self, clicker)
--   - get_staticdata(self)
--     ^ return string that will be passed to on_activate when the object
--       is created next time
--
-- Entity prototype table:
-- {
--     physical = true,
--     collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
--     visual = "cube",
--     textures = {texture,texture,texture,texture,texture,texture},
--     on_activate = function(self, staticdata),
--     on_step = function(self, dtime),
--     on_punch = function(self, hitter),
--     on_rightclick = function(self, clicker),
--     get_staticdata = function(self),
--     # Also you can define arbitrary member variables here
--     myvariable = whatever,
-- }
--
-- Tool definition:
-- {
--     image = "default_tool_steelaxe.png",
--     full_punch_interval = 1.0,
--     basetime = 1.0,
--     dt_weight = 0.5,
--     dt_crackiness = -0.2,
--     dt_crumbliness = 1,
--     dt_cuttability = -0.5,
--     basedurability = 330,
--     dd_weight = 0,
--     dd_crackiness = 0,
--     dd_crumbliness = 0,
--     dd_cuttability = 0,
-- }
--
-- Node definition options:
-- {
--     name = "modname:somenode",
--     drawtype = "normal",
--     visual_scale = 1.0,
--     tile_images = {"default_unknown_block.png"},
--     inventory_image = "default_unknown_block.png",
--     special_materials = {
--         {image="", backface_culling=true},
--         {image="", backface_culling=true},
--     },
--     alpha = 255,
--     post_effect_color = {a=0, r=0, g=0, b=0},
--     paramtype = "none",
--     is_ground_content = false,
--     sunlight_propagates = false,
--     walkable = true,
--     pointable = true,
--     diggable = true,
--     climbable = false,
--     buildable_to = false,
--     wall_mounted = false,
--     often_contains_mineral = false,
--     dug_item = "",
--     extra_dug_item = "",
--     extra_dug_item_rarity = 2,
--     metadata_name = "",
--     liquidtype = "none",
--     liquid_alternative_flowing = "",
--     liquid_alternative_source = "",
--     liquid_viscosity = 0,
--     light_source = 0,
--     damage_per_second = 0,
--     selection_box = {type="regular"},
--     material = {
--         diggablity = "normal",
--         weight = 0,
--         crackiness = 0,
--         crumbliness = 0,
--         cuttability = 0,
--         flammability = 0,
--     },
--     cookresult_itemstring = "", -- Cannot be cooked
--     furnace_cooktime = 3.0,
--     furnace_burntime = -1, -- Cannot be used as fuel
-- }
--
-- Craftitem definition options:
-- minetest.register_craftitem("modname_name", {
--     image = "default_image.png",
--     stack_max = <maximum number of items in stack>,
--     cookresult_itemstring = itemstring (result of cooking),
--     furnace_cooktime = <cooking time>,
--     furnace_burntime = <time to burn as fuel in furnace>,
--     usable = <uh... some boolean value>,
--     dropcount = <amount of items to drop using drop action>
--     liquids_pointable = <whether can point liquids>,
--     on_drop = func(item, dropper, pos),
--     on_place_on_ground = func(item, placer, pos),
--     on_use = func(item, player, pointed_thing),
-- })
-- 
-- Recipe:
-- {
--     output = 'tool "default:pick_stone"',
--     recipe = {
--         {'node "cobble"', 'node "cobble"', 'node "cobble"'},
--         {'', 'craft "default:stick"', ''},
--         {'', 'craft "default:stick"', ''},
--     }
-- }
--
-- ABM (ActiveBlockModifier) definition:
-- {
--     nodenames = {"lava_source"},
--     neighbors = {"water_source", "water_flowing"}, -- (any of these)
--      ^ If left out or empty, any neighbor will do
--      ^ This might get removed in the future
--     interval = 1.0, -- (operation interval)
--     chance = 1, -- (chance of trigger is 1.0/this)
--     action = func(pos, node, active_object_count, active_object_count_wider),
-- }

WATER_ALPHA = 160
WATER_VISC = 1
LAVA_VISC = 7
LIGHT_MAX = 14

-- Definitions made by this mod that other mods can use too
default = {}

--
-- Tool definition
--

-- The hand
minetest.register_tool(":", {
	image = "",
	basetime = 0.5,
	dt_weight = 1,
	dt_crackiness = 0,
	dt_crumbliness = -1,
	dt_cuttability = 0,
	basedurability = 50,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})

minetest.register_tool("default:pick_wood", {
	image = "default_tool_woodpick.png",
	basetime = 2.0,
	dt_weight = 0,
	dt_crackiness = -0.5,
	dt_crumbliness = 2,
	dt_cuttability = 0,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:pick_stone", {
	image = "default_tool_stonepick.png",
	basetime = 1.5,
	dt_weight = 0,
	dt_crackiness = -0.5,
	dt_crumbliness = 2,
	dt_cuttability = 0,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:pick_steel", {
	image = "default_tool_steelpick.png",
	basetime = 1.0,
	dt_weight = 0,
	dt_crackiness = -0.5,
	dt_crumbliness = 2,
	dt_cuttability = 0,
	basedurability = 333,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:pick_mese", {
	image = "default_tool_mesepick.png",
	basetime = 0,
	dt_weight = 0,
	dt_crackiness = 0,
	dt_crumbliness = 0,
	dt_cuttability = 0,
	basedurability = 1337,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:shovel_wood", {
	image = "default_tool_woodshovel.png",
	basetime = 2.0,
	dt_weight = 0.5,
	dt_crackiness = 2,
	dt_crumbliness = -1.5,
	dt_cuttability = 0.3,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:shovel_stone", {
	image = "default_tool_stoneshovel.png",
	basetime = 1.5,
	dt_weight = 0.5,
	dt_crackiness = 2,
	dt_crumbliness = -1.5,
	dt_cuttability = 0.1,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:shovel_steel", {
	image = "default_tool_steelshovel.png",
	basetime = 1.0,
	dt_weight = 0.5,
	dt_crackiness = 2,
	dt_crumbliness = -1.5,
	dt_cuttability = 0.0,
	basedurability = 330,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:axe_wood", {
	image = "default_tool_woodaxe.png",
	basetime = 2.0,
	dt_weight = 0.5,
	dt_crackiness = -0.2,
	dt_crumbliness = 1,
	dt_cuttability = -0.5,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:axe_stone", {
	image = "default_tool_stoneaxe.png",
	basetime = 1.5,
	dt_weight = 0.5,
	dt_crackiness = -0.2,
	dt_crumbliness = 1,
	dt_cuttability = -0.5,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:axe_steel", {
	image = "default_tool_steelaxe.png",
	basetime = 1.0,
	dt_weight = 0.5,
	dt_crackiness = -0.2,
	dt_crumbliness = 1,
	dt_cuttability = -0.5,
	basedurability = 330,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:sword_wood", {
	image = "default_tool_woodsword.png",
	basetime = 3.0,
	dt_weight = 3,
	dt_crackiness = 0,
	dt_crumbliness = 1,
	dt_cuttability = -1,
	basedurability = 30,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:sword_stone", {
	image = "default_tool_stonesword.png",
	basetime = 2.5,
	dt_weight = 3,
	dt_crackiness = 0,
	dt_crumbliness = 1,
	dt_cuttability = -1,
	basedurability = 100,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})
minetest.register_tool("default:sword_steel", {
	image = "default_tool_steelsword.png",
	basetime = 2.0,
	dt_weight = 3,
	dt_crackiness = 0,
	dt_crumbliness = 1,
	dt_cuttability = -1,
	basedurability = 330,
	dd_weight = 0,
	dd_crackiness = 0,
	dd_crumbliness = 0,
	dd_cuttability = 0,
})

--
-- Crafting definition
--

minetest.register_craft({
	output = 'node "default:wood" 4',
	recipe = {
		{'node "tree"'},
	}
})

minetest.register_craft({
	output = 'craft "default:stick" 4',
	recipe = {
		{'node "default:wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:fence_wood" 2',
	recipe = {
		{'craft "default:stick"', 'craft "default:stick"', 'craft "default:stick"'},
		{'craft "default:stick"', 'craft "default:stick"', 'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'node "default:sign_wall" 1',
	recipe = {
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
		{'', 'craft "default:stick"', ''},
	}
})

minetest.register_craft({
	output = 'node "default:torch" 4',
	recipe = {
		{'craft "default:coal_lump"'},
		{'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_wood"',
	recipe = {
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
		{'', 'craft "default:stick"', ''},
		{'', 'craft "default:stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_stone"',
	recipe = {
		{'node "cobble"', 'node "cobble"', 'node "cobble"'},
		{'', 'craft "default:stick"', ''},
		{'', 'craft "default:stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_steel"',
	recipe = {
		{'craft "default:steel_ingot"', 'craft "default:steel_ingot"', 'craft "default:steel_ingot"'},
		{'', 'craft "default:stick"', ''},
		{'', 'craft "default:stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:pick_mese"',
	recipe = {
		{'node "mese"', 'node "mese"', 'node "mese"'},
		{'', 'craft "default:stick"', ''},
		{'', 'craft "default:stick"', ''},
	}
})

minetest.register_craft({
	output = 'tool "default:shovel_wood"',
	recipe = {
		{'node "default:wood"'},
		{'craft "default:stick"'},
		{'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:shovel_stone"',
	recipe = {
		{'node "cobble"'},
		{'craft "default:stick"'},
		{'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:shovel_steel"',
	recipe = {
		{'craft "default:steel_ingot"'},
		{'craft "default:stick"'},
		{'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:axe_wood"',
	recipe = {
		{'node "default:wood"', 'node "default:wood"'},
		{'node "default:wood"', 'craft "default:stick"'},
		{'', 'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:axe_stone"',
	recipe = {
		{'node "cobble"', 'node "cobble"'},
		{'node "cobble"', 'craft "default:stick"'},
		{'', 'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:axe_steel"',
	recipe = {
		{'craft "default:steel_ingot"', 'craft "default:steel_ingot"'},
		{'craft "default:steel_ingot"', 'craft "default:stick"'},
		{'', 'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:sword_wood"',
	recipe = {
		{'node "default:wood"'},
		{'node "default:wood"'},
		{'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:sword_stone"',
	recipe = {
		{'node "cobble"'},
		{'node "cobble"'},
		{'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'tool "default:sword_steel"',
	recipe = {
		{'craft "default:steel_ingot"'},
		{'craft "default:steel_ingot"'},
		{'craft "default:stick"'},
	}
})

minetest.register_craft({
	output = 'node "default:rail" 15',
	recipe = {
		{'craft "default:steel_ingot"', '', 'craft "default:steel_ingot"'},
		{'craft "default:steel_ingot"', 'craft "default:stick"', 'craft "default:steel_ingot"'},
		{'craft "default:steel_ingot"', '', 'craft "default:steel_ingot"'},
	}
})

minetest.register_craft({
	output = 'node "default:chest" 1',
	recipe = {
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
		{'node "default:wood"', '', 'node "default:wood"'},
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:chest_locked" 1',
	recipe = {
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
		{'node "default:wood"', 'craft "default:steel_ingot"', 'node "default:wood"'},
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:furnace" 1',
	recipe = {
		{'node "cobble"', 'node "cobble"', 'node "cobble"'},
		{'node "cobble"', '', 'node "cobble"'},
		{'node "cobble"', 'node "cobble"', 'node "cobble"'},
	}
})

minetest.register_craft({
	output = 'node "default:steelblock" 1',
	recipe = {
		{'craft "default:steel_ingot"', 'craft "default:steel_ingot"', 'craft "default:steel_ingot"'},
		{'craft "default:steel_ingot"', 'craft "default:steel_ingot"', 'craft "default:steel_ingot"'},
		{'craft "default:steel_ingot"', 'craft "default:steel_ingot"', 'craft "default:steel_ingot"'},
	}
})

minetest.register_craft({
	output = 'node "default:sandstone" 1',
	recipe = {
		{'node "sand"', 'node "sand"'},
		{'node "sand"', 'node "sand"'},
	}
})

minetest.register_craft({
	output = 'node "default:clay" 1',
	recipe = {
		{'craft "default:clay_lump"', 'craft "default:clay_lump"'},
		{'craft "default:clay_lump"', 'craft "default:clay_lump"'},
	}
})

minetest.register_craft({
	output = 'node "default:brick" 1',
	recipe = {
		{'craft "default:clay_brick"', 'craft "default:clay_brick"'},
		{'craft "default:clay_brick"', 'craft "default:clay_brick"'},
	}
})

minetest.register_craft({
	output = 'craft "default:paper" 1',
	recipe = {
		{'node "papyrus"', 'node "papyrus"', 'node "papyrus"'},
	}
})

minetest.register_craft({
	output = 'craft "default:book" 1',
	recipe = {
		{'craft "default:paper"'},
		{'craft "default:paper"'},
		{'craft "default:paper"'},
	}
})

minetest.register_craft({
	output = 'node "default:bookshelf" 1',
	recipe = {
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
		{'craft "default:book"', 'craft "default:book"', 'craft "default:book"'},
		{'node "default:wood"', 'node "default:wood"', 'node "default:wood"'},
	}
})

minetest.register_craft({
	output = 'node "default:ladder" 1',
	recipe = {
		{'craft "default:stick"', '', 'craft "default:stick"'},
		{'craft "default:stick"', 'craft "default:stick"', 'craft "default:stick"'},
		{'craft "default:stick"', '', 'craft "default:stick"'},
	}
})

--
-- Node definitions
--

minetest.register_node("default:stone", {
	tile_images = {"default_stone.png"},
	inventory_image = minetest.inventorycube("default_stone.png"),
	paramtype = "mineral",
	is_ground_content = true,
	often_contains_mineral = true, -- Texture atlas hint
	material = minetest.digprop_stonelike(1.0),
	dug_item = 'node "cobble" 1',
})

minetest.register_node("default:dirt_with_grass", {
	tile_images = {"default_grass.png", "default_dirt.png", "default_dirt.png^default_grass_side.png"},
	inventory_image = minetest.inventorycube("default_dirt.png^default_grass_side.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	dug_item = 'node "dirt" 1',
})

minetest.register_node("default:dirt_with_grass_footsteps", {
	tile_images = {"default_grass_footsteps.png", "default_dirt.png", "default_dirt.png^default_grass_side.png"},
	inventory_image = "default_grass_footsteps.png",
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	dug_item = 'node "dirt" 1',
})

minetest.register_node("default:dirt", {
	tile_images = {"default_dirt.png"},
	inventory_image = minetest.inventorycube("default_dirt.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
})

minetest.register_node("default:sand", {
	tile_images = {"default_sand.png"},
	inventory_image = minetest.inventorycube("default_sand.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	cookresult_itemstring = 'node "glass" 1',
})

minetest.register_node("default:gravel", {
	tile_images = {"default_gravel.png"},
	inventory_image = minetest.inventorycube("default_gravel.png"),
	is_ground_content = true,
	material = minetest.digprop_gravellike(1.0),
})

minetest.register_node("default:sandstone", {
	tile_images = {"default_sandstone.png"},
	inventory_image = minetest.inventorycube("default_sandstone.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),  -- FIXME should this be stonelike?
	dug_item = 'node "sand" 1',  -- FIXME is this intentional?
})

minetest.register_node("default:clay", {
	tile_images = {"default_clay.png"},
	inventory_image = minetest.inventorycube("default_clay.png"),
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	dug_item = 'craft "default:clay_lump" 4',
})

minetest.register_node("default:brick", {
	tile_images = {"default_brick.png"},
	inventory_image = minetest.inventorycube("default_brick.png"),
	is_ground_content = true,
	material = minetest.digprop_stonelike(1.0),
	dug_item = 'craft "default:clay_brick" 4',
})

minetest.register_node("default:tree", {
	tile_images = {"default_tree_top.png", "default_tree_top.png", "default_tree.png"},
	inventory_image = minetest.inventorycube("default_tree_top.png", "default_tree.png", "default_tree.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(1.0),
	cookresult_itemstring = 'craft "default:coal_lump" 1',
	furnace_burntime = 30,
})

minetest.register_node("default:jungletree", {
	tile_images = {"default_jungletree_top.png", "default_jungletree_top.png", "default_jungletree.png"},
	inventory_image = minetest.inventorycube("default_jungletree_top.png", "default_jungletree.png", "default_jungletree.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(1.0),
	cookresult_itemstring = 'craft "default:coal_lump" 1',
	furnace_burntime = 30,
})

minetest.register_node("default:junglegrass", {
	drawtype = "plantlike",
	visual_scale = 1.3,
	tile_images = {"default_junglegrass.png"},
	inventory_image = "default_junglegrass.png",
	paramtype = "light",
	walkable = false,
	material = minetest.digprop_leaveslike(1.0),
	furnace_burntime = 2,
})

minetest.register_node("default:leaves", {
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	tile_images = {"default_leaves.png"},
	inventory_image = minetest.inventorycube("default_leaves.png"),
	paramtype = "light",
	material = minetest.digprop_leaveslike(1.0),
	extra_dug_item = 'node "sapling" 1',
	extra_dug_item_rarity = 20,
	furnace_burntime = 1,
})

minetest.register_node("default:cactus", {
	tile_images = {"default_cactus_top.png", "default_cactus_top.png", "default_cactus_side.png"},
	inventory_image = minetest.inventorycube("default_cactus_top.png", "default_cactus_side.png", "default_cactus_side.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(0.75),
	furnace_burntime = 15,
})

minetest.register_node("default:papyrus", {
	drawtype = "plantlike",
	tile_images = {"default_papyrus.png"},
	inventory_image = "default_papyrus.png",
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	material = minetest.digprop_leaveslike(0.5),
	furnace_burntime = 1,
})

minetest.register_node("default:bookshelf", {
	tile_images = {"default_wood.png", "default_wood.png", "default_bookshelf.png"},
	inventory_image = minetest.inventorycube("default_wood.png", "default_bookshelf.png", "default_bookshelf.png"),
	is_ground_content = true,
	material = minetest.digprop_woodlike(0.75),
	furnace_burntime = 30,
})

minetest.register_node("default:glass", {
	drawtype = "glasslike",
	tile_images = {"default_glass.png"},
	inventory_image = minetest.inventorycube("default_glass.png"),
	paramtype = "light",
	sunlight_propagates = true,
	is_ground_content = true,
	material = minetest.digprop_glasslike(1.0),
})

minetest.register_node("default:fence_wood", {
	drawtype = "fencelike",
	tile_images = {"default_wood.png"},
	inventory_image = "default_fence.png",
	paramtype = "light",
	is_ground_content = true,
	selection_box = {
		type = "fixed",
		fixed = {-1/7, -1/2, -1/7, 1/7, 1/2, 1/7},
	},
	furnace_burntime = 15,
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node("default:rail", {
	drawtype = "raillike",
	tile_images = {"default_rail.png", "default_rail_curved.png", "default_rail_t_junction.png", "default_rail_crossing.png"},
	inventory_image = "default_rail.png",
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
		--fixed = <default>
	},
	material = minetest.digprop_dirtlike(0.75),
})

minetest.register_node("default:ladder", {
	drawtype = "signlike",
	tile_images = {"default_ladder.png"},
	inventory_image = "default_ladder.png",
	paramtype = "light",
	is_ground_content = true,
	wall_mounted = true,
	walkable = false,
	climbable = true,
	selection_box = {
		type = "wallmounted",
		--wall_top = = <default>
		--wall_bottom = = <default>
		--wall_side = = <default>
	},
	furnace_burntime = 5,
	material = minetest.digprop_woodlike(0.5),
})

minetest.register_node("default:wood", {
	tile_images = {"default_wood.png"},
	inventory_image = minetest.inventorycube("default_wood.png"),
	is_ground_content = true,
	furnace_burntime = 7,
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node("default:mese", {
	tile_images = {"default_mese.png"},
	inventory_image = minetest.inventorycube("default_mese.png"),
	is_ground_content = true,
	furnace_burntime = 30,
	material = minetest.digprop_stonelike(0.5),
})

minetest.register_node("default:cloud", {
	tile_images = {"default_cloud.png"},
	inventory_image = minetest.inventorycube("default_cloud.png"),
	is_ground_content = true,
})

minetest.register_node("default:water_flowing", {
	drawtype = "flowingliquid",
	tile_images = {"default_water.png"},
	alpha = WATER_ALPHA,
	inventory_image = minetest.inventorycube("default_water.png"),
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "flowing",
	liquid_alternative_flowing = "water_flowing",
	liquid_alternative_source = "water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	special_materials = {
		{image="default_water.png", backface_culling=false},
		{image="default_water.png", backface_culling=true},
	},
})

minetest.register_node("default:water_source", {
	drawtype = "liquid",
	tile_images = {"default_water.png"},
	alpha = WATER_ALPHA,
	inventory_image = minetest.inventorycube("default_water.png"),
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "source",
	liquid_alternative_flowing = "water_flowing",
	liquid_alternative_source = "water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	special_materials = {
		-- New-style water source material (mostly unused)
		{image="default_water.png", backface_culling=false},
	},
})

minetest.register_node("default:lava_flowing", {
	drawtype = "flowingliquid",
	tile_images = {"default_lava.png"},
	inventory_image = minetest.inventorycube("default_lava.png"),
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "flowing",
	liquid_alternative_flowing = "lava_flowing",
	liquid_alternative_source = "lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		{image="default_lava.png", backface_culling=false},
		{image="default_lava.png", backface_culling=true},
	},
})

minetest.register_node("default:lava_source", {
	drawtype = "liquid",
	tile_images = {"default_lava.png"},
	inventory_image = minetest.inventorycube("default_lava.png"),
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "source",
	liquid_alternative_flowing = "lava_flowing",
	liquid_alternative_source = "lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		-- New-style lava source material (mostly unused)
		{image="default_lava.png", backface_culling=false},
	},
	furnace_burntime = 60,
})

minetest.register_node("default:torch", {
	drawtype = "torchlike",
	tile_images = {"default_torch_on_floor.png", "default_torch_on_ceiling.png", "default_torch.png"},
	inventory_image = "default_torch_on_floor.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	wall_mounted = true,
	light_source = LIGHT_MAX-1,
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, 0.5-0.6, -0.1, 0.1, 0.5, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.5, -0.3, -0.1, -0.5+0.3, 0.3, 0.1},
	},
	material = minetest.digprop_constanttime(0.0),
	furnace_burntime = 4,
})

minetest.register_node("default:sign_wall", {
	drawtype = "signlike",
	tile_images = {"default_sign_wall.png"},
	inventory_image = "default_sign_wall.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	wall_mounted = true,
	metadata_name = "sign",
	selection_box = {
		type = "wallmounted",
		--wall_top = <default>
		--wall_bottom = <default>
		--wall_side = <default>
	},
	material = minetest.digprop_constanttime(0.5),
	furnace_burntime = 10,
})

minetest.register_node("default:chest", {
	tile_images = {"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_front.png"},
	inventory_image = minetest.inventorycube("default_chest_top.png", "default_chest_front.png", "default_chest_side.png"),
	paramtype = "facedir_simple",
	metadata_name = "chest",
	material = minetest.digprop_woodlike(1.0),
	furnace_burntime = 30,
})

minetest.register_node("default:chest_locked", {
	tile_images = {"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_lock.png"},
	inventory_image = minetest.inventorycube("default_chest_top.png", "default_chest_lock.png", "default_chest_side.png"),
	paramtype = "facedir_simple",
	metadata_name = "locked_chest",
	material = minetest.digprop_woodlike(1.0),
	furnace_burntime = 30,
})

minetest.register_node("default:furnace", {
	tile_images = {"default_furnace_side.png", "default_furnace_side.png", "default_furnace_side.png",
		"default_furnace_side.png", "default_furnace_side.png", "default_furnace_front.png"},
	inventory_image = minetest.inventorycube("default_furnace_side.png", "default_furnace_front.png", "default_furnace_side.png"),
	paramtype = "facedir_simple",
	metadata_name = "furnace",
	material = minetest.digprop_stonelike(3.0),
})

minetest.register_node("default:cobble", {
	tile_images = {"default_cobble.png"},
	inventory_image = minetest.inventorycube("default_cobble.png"),
	is_ground_content = true,
	cookresult_itemstring = 'node "stone" 1',
	material = minetest.digprop_stonelike(0.9),
})

minetest.register_node("default:mossycobble", {
	tile_images = {"default_mossycobble.png"},
	inventory_image = minetest.inventorycube("default_mossycobble.png"),
	is_ground_content = true,
	material = minetest.digprop_stonelike(0.8),
})

minetest.register_node("default:steelblock", {
	tile_images = {"default_steel_block.png"},
	inventory_image = minetest.inventorycube("default_steel_block.png"),
	is_ground_content = true,
	material = minetest.digprop_stonelike(5.0),
})

minetest.register_node("default:nyancat", {
	tile_images = {"default_nc_side.png", "default_nc_side.png", "default_nc_side.png",
		"default_nc_side.png", "default_nc_back.png", "default_nc_front.png"},
	inventory_image = "default_nc_front.png",
	paramtype = "facedir_simple",
	material = minetest.digprop_stonelike(3.0),
	furnace_burntime = 1,
})

minetest.register_node("default:nyancat_rainbow", {
	tile_images = {"default_nc_rb.png"},
	inventory_image = "default_nc_rb.png",
	material = minetest.digprop_stonelike(3.0),
	furnace_burntime = 1,
})

minetest.register_node("default:sapling", {
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"default_sapling.png"},
	inventory_image = "default_sapling.png",
	paramtype = "light",
	walkable = false,
	material = minetest.digprop_constanttime(0.0),
	furnace_burntime = 10,
})

minetest.register_node("default:apple", {
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"default_apple.png"},
	inventory_image = "default_apple.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	dug_item = 'craft "apple" 1',
	material = minetest.digprop_constanttime(0.0),
	furnace_burntime = 3,
})

--
-- Crafting items
--

minetest.register_craftitem("default:stick", {
	image = "default_stick.png",
	--furnace_burntime = ...,
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:paper", {
	image = "default_paper.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:book", {
	image = "default_book.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:coal_lump", {
	image = "default_coal_lump.png",
	furnace_burntime = 40;
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:iron_lump", {
	image = "default_iron_lump.png",
	cookresult_itemstring = 'craft "default:steel_ingot" 1',
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:clay_lump", {
	image = "default_clay_lump.png",
	cookresult_itemstring = 'craft "default:clay_brick" 1',
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:steel_ingot", {
	image = "default_steel_ingot.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:clay_brick", {
	image = "default_clay_brick.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:scorched_stuff", {
	image = "default_scorched_stuff.png",
	on_place_on_ground = minetest.craftitem_place_item,
})

minetest.register_craftitem("default:apple", {
	image = "default_apple.png",
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = minetest.craftitem_eat(4),
})

--
-- Creative inventory
--

minetest.add_to_creative_inventory('tool "default:pick_mese" 0')
minetest.add_to_creative_inventory('tool "default:pick_steel" 0')
minetest.add_to_creative_inventory('tool "default:axe_steel" 0')
minetest.add_to_creative_inventory('tool "default:shovel_steel" 0')

minetest.add_to_creative_inventory('node "default:torch" 0')
minetest.add_to_creative_inventory('node "default:cobble" 0')
minetest.add_to_creative_inventory('node "default:dirt" 0')
minetest.add_to_creative_inventory('node "default:stone" 0')
minetest.add_to_creative_inventory('node "default:sand" 0')
minetest.add_to_creative_inventory('node "default:sandstone" 0')
minetest.add_to_creative_inventory('node "default:clay" 0')
minetest.add_to_creative_inventory('node "default:brick" 0')
minetest.add_to_creative_inventory('node "default:tree" 0')
minetest.add_to_creative_inventory('node "default:leaves" 0')
minetest.add_to_creative_inventory('node "default:cactus" 0')
minetest.add_to_creative_inventory('node "default:papyrus" 0')
minetest.add_to_creative_inventory('node "default:bookshelf" 0')
minetest.add_to_creative_inventory('node "default:glass" 0')
minetest.add_to_creative_inventory('node "default:fence" 0')
minetest.add_to_creative_inventory('node "default:rail" 0')
minetest.add_to_creative_inventory('node "default:mese" 0')
minetest.add_to_creative_inventory('node "default:chest" 0')
minetest.add_to_creative_inventory('node "default:furnace" 0')
minetest.add_to_creative_inventory('node "default:sign_wall" 0')
minetest.add_to_creative_inventory('node "default:water_source" 0')
minetest.add_to_creative_inventory('node "default:lava_source" 0')
minetest.add_to_creative_inventory('node "default:ladder" 0')

--
-- Some common functions
--

default.falling_node_names = {}

function nodeupdate_single(p)
	n = minetest.env:get_node(p)
	if default.falling_node_names[n.name] ~= nil then
		p_bottom = {x=p.x, y=p.y-1, z=p.z}
		n_bottom = minetest.env:get_node(p_bottom)
		if n_bottom.name == "air" then
			minetest.env:remove_node(p)
			minetest.env:add_entity(p, "default:falling_"..n.name)
			nodeupdate(p)
		end
	end
end

function nodeupdate(p)
	for x = -1,1 do
	for y = -1,1 do
	for z = -1,1 do
		p2 = {x=p.x+x, y=p.y+y, z=p.z+z}
		nodeupdate_single(p2)
	end
	end
	end
end

--
-- Falling stuff
--

function default.register_falling_node(nodename, texture)
	default.falling_node_names[nodename] = true
	-- Override naming conventions for stuff like :default:falling_default:sand
	minetest.register_entity(":default:falling_"..nodename, {
		-- Static definition
		physical = true,
		collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
		visual = "cube",
		textures = {texture,texture,texture,texture,texture,texture},
		-- State
		-- Methods
		on_step = function(self, dtime)
			-- Set gravity
			self.object:setacceleration({x=0, y=-10, z=0})
			-- Turn to actual sand when collides to ground or just move
			local pos = self.object:getpos()
			local bcp = {x=pos.x, y=pos.y-0.7, z=pos.z} -- Position of bottom center point
			local bcn = minetest.env:get_node(bcp)
			if bcn.name ~= "air" then
				-- Turn to a sand node
				local np = {x=bcp.x, y=bcp.y+1, z=bcp.z}
				minetest.env:add_node(np, {name=nodename})
				self.object:remove()
			else
				-- Do nothing
			end
		end
	})
end

default.register_falling_node("default:sand", "default_sand.png")
default.register_falling_node("default:gravel", "default_gravel.png")

--
-- Global callbacks
--

-- Global environment step function
function on_step(dtime)
	-- print("on_step")
end
minetest.register_globalstep(on_step)

function on_placenode(p, node)
	--print("on_placenode")
	nodeupdate(p)
end
minetest.register_on_placenode(on_placenode)

function on_dignode(p, node)
	--print("on_dignode")
	nodeupdate(p)
end
minetest.register_on_dignode(on_dignode)

function on_punchnode(p, node)
end
minetest.register_on_punchnode(on_punchnode)

minetest.register_on_chat_message(function(name, message)
	--print("default on_chat_message: name="..dump(name).." message="..dump(message))
	local cmd = "/giveme"
	if message:sub(0, #cmd) == cmd then
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to give")
			return true -- Handled chat message
		end
		local stackstring = string.match(message, cmd.." (.*)")
		if stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' stackstring')
			return true -- Handled chat message
		end
		print(cmd..' invoked, stackstring="'..stackstring..'"')
		local player = minetest.env:get_player_by_name(name)
		if player == nil then
			minetest.chat_send_player(name, name2..' is not a known player')
			return true -- Handled chat message
		end
		local added, error_msg = player:add_to_inventory(stackstring)
		if added then
			minetest.chat_send_player(name, '"'..stackstring
					..'" added to inventory.');
		else
			minetest.chat_send_player(name, 'Could not give "'..stackstring
					..'": '..error_msg);
		end
		return true -- Handled chat message
	end
	local cmd = "/give"
	if message:sub(0, #cmd) == cmd then
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to give")
			return true -- Handled chat message
		end
		local name2, stackstring = string.match(message, cmd.." ([%a%d_-]+) (.*)")
		if name == nil or stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' name stackstring')
			return true -- Handled chat message
		end
		print(cmd..' invoked, name2="'..name2
				..'" stackstring="'..stackstring..'"')
		local player = minetest.env:get_player_by_name(name2)
		if player == nil then
			minetest.chat_send_player(name, name2..' is not a known player')
			return true -- Handled chat message
		end
		local added, error_msg = player:add_to_inventory(stackstring)
		if added then
			minetest.chat_send_player(name, '"'..stackstring
					..'" added to '..name2..'\'s inventory.');
			minetest.chat_send_player(name2, '"'..stackstring
					..'" added to inventory.');
		else
			minetest.chat_send_player(name, 'Could not give "'..stackstring
					..'": '..error_msg);
		end
		return true -- Handled chat message
	end
	local cmd = "/spawnentity"
	if message:sub(0, #cmd) == cmd then
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to spawn (give)")
			return true -- Handled chat message
		end
		local entityname = string.match(message, cmd.." (.*)")
		if entityname == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' entityname')
			return true -- Handled chat message
		end
		print(cmd..' invoked, entityname="'..entityname..'"')
		local player = minetest.env:get_player_by_name(name)
		if player == nil then
			print("Unable to spawn entity, player is nil")
			return true -- Handled chat message
		end
		local p = player:getpos()
		p.y = p.y + 1
		minetest.env:add_entity(p, entityname)
		minetest.chat_send_player(name, '"'..entityname
				..'" spawned.');
		return true -- Handled chat message
	end
end)

--
-- Done, print some random stuff
--

--print("minetest.registered_entities:")
--dump2(minetest.registered_entities)

-- END
