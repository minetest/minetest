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
-- eg. 'node "default:dirt" 5'
-- eg. 'tool "default:pick_wood" 21323'
-- eg. 'craft "default:apple" 2'
--
-- item: A stack of items in Lua table format.
-- eg. {name="default:dirt", count=1, wear=0, metadata=""} 
--     ^ a single dirt node
-- eg. {name="default:pick_wood", count=1, wear=21323, metadata=""}
--     ^ a wooden pick about 1/3 weared out
-- eg. {name="default:apple", count=1, wear=0, metadata=""}
--     ^ an apple.
--
-- Any time an item must be passed to a function, it can be an
-- ItemStack (see below), an itemstring or a table in the above format.
--
-- Global functions:
-- minetest.register_entity(name, prototype table)
-- minetest.register_abm(abm definition)
-- minetest.register_node(name, node definition)
-- minetest.register_tool(name, item definition)
-- minetest.register_craftitem(name, item definition)
-- minetest.register_alias(name, convert_to)
-- minetest.register_craft(recipe)
-- minetest.register_globalstep(func(dtime))
-- minetest.register_on_placenode(func(pos, newnode, placer))
-- minetest.register_on_dignode(func(pos, oldnode, digger))
-- minetest.register_on_punchnode(func(pos, node, puncher))
-- minetest.register_on_generated(func(minp, maxp))
-- minetest.register_on_newplayer(func(ObjectRef))
-- minetest.register_on_dieplayer(func(ObjectRef))
-- minetest.register_on_respawnplayer(func(ObjectRef))
-- ^ return true in func to disable regular player placement
-- ^ currently called _before_ repositioning of player occurs
-- minetest.register_on_chat_message(func(name, message))
-- minetest.add_to_creative_inventory(itemstring)
-- minetest.setting_get(name) -> string or nil
-- minetest.setting_getbool(name) -> boolean value or nil
-- minetest.chat_send_all(text)
-- minetest.chat_send_player(name, text)
-- minetest.get_player_privs(name) -> set of privs
-- minetest.get_inventory(location) -> InvRef
-- ^ location = eg. {type="player", name="celeron55"}
--                  {type="node", pos={x=, y=, z=}}
-- minetest.get_current_modname() -> string
-- minetest.get_modpath(modname) -> eg. "/home/user/.minetest/usermods/modname"
--
-- minetest.debug(line)
-- ^ Goes to dstream
-- minetest.log(line)
-- minetest.log(loglevel, line)
-- ^ loglevel one of "error", "action", "info", "verbose"
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
-- minetest.registered_items
-- ^ List of registered items, indexed by name
-- minetest.registered_nodes
-- ^ List of registered node definitions, indexed by name
-- minetest.registered_craftitems
-- ^ List of registered craft item definitions, indexed by name
-- minetest.registered_tools
-- ^ List of registered tool definitions, indexed by name
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
-- - add_entity(pos, name): Returns ObjectRef or nil if failed
-- - add_item(pos, itemstring)
-- - add_rat(pos)
-- - add_firefly(pos)
-- - get_meta(pos) -- Get a NodeMetaRef at that position
-- - get_player_by_name(name) -- Get an ObjectRef to a player
-- - get_objects_inside_radius(pos, radius)
-- - set_timeofday(val): val: 0...1; 0 = midnight, 0.5 = midday
-- - get_timeofday()
--
-- NodeMetaRef (this stuff is subject to change in a future version)
-- - get_type()
-- - allows_text_input()
-- - set_text(text) -- eg. set the text of a sign
-- - get_text()
-- - get_owner()
-- - set_owner(string)
-- Generic node metadata specific:
-- - set_infotext(infotext)
-- - get_inventory() -> InvRef
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
-- - get_hp(): returns number of hitpoints (2 * number of hearts)
-- - set_hp(hp): set number of hitpoints (2 * number of hearts)
-- - get_inventory() -> InvRef
-- - get_wield_list(): returns the name of the inventory list the wielded item is in
-- - get_wield_index(): returns the index of the wielded item
-- - get_wielded_item() -> ItemStack
-- - set_wielded_item(item): replaces the wielded item, returns true if successful
-- LuaEntitySAO-only: (no-op for other objects)
-- - setvelocity({x=num, y=num, z=num})
-- - getvelocity() -> {x=num, y=num, z=num}
-- - setacceleration({x=num, y=num, z=num})
-- - getacceleration() -> {x=num, y=num, z=num}
-- - setyaw(radians)
-- - getyaw() -> radians
-- - settexturemod(mod)
-- - setsprite(p={x=0,y=0}, num_frames=1, framelength=0.2,
-- -           select_horiz_by_yawpitch=false)
-- - ^ Select sprite from spritesheet with optional animation and DM-style
-- -   texture selection based on yaw relative to camera
-- - get_entity_name() (DEPRECATED: Will be removed in a future version)
-- - get_luaentity()
-- Player-only: (no-op for other objects)
-- - get_player_name(): will return nil if is not a player
-- - get_look_dir(): get camera direction as a unit vector
-- - get_look_pitch(): pitch in radians
-- - get_look_yaw(): yaw in radians (wraps around pretty randomly as of now)
--
-- InvRef methods:
-- - get_size(listname): get size of a list
-- - set_size(listname, size): set size of a list
-- - get_stack(listname, i): get a copy of stack index i in list
-- - set_stack(listname, i, stack): copy stack to index i in list
-- - get_list(listname): return full list
-- - set_list(listname, list): set full list (size will not change)
-- - add_item(listname, stack): add item somewhere in list, returns leftover ItemStack
-- - room_for_item(listname, stack): returns true if the stack of items
--     can be fully added to the list
-- - contains_item(listname, stack): returns true if the stack of items
--     can be fully taken from the list
--   remove_item(listname, stack): take as many items as specified from the list,
--     returns the items that were actually removed (as an ItemStack)
--
-- ItemStack methods:
-- - is_empty(): return true if stack is empty
-- - get_name(): returns item name (e.g. "default:stone")
-- - get_count(): returns number of items on the stack
-- - get_wear(): returns tool wear (0-65535), 0 for non-tools
-- - get_metadata(): returns metadata (a string attached to an item stack)
-- - clear(): removes all items from the stack, making it empty
-- - replace(item): replace the contents of this stack (item can also
--     be an itemstring or table)
-- - to_string(): returns the stack in itemstring form
-- - to_table(): returns the stack in Lua table form
-- - get_stack_max(): returns the maximum size of the stack (depends on the item)
-- - get_free_space(): returns get_stack_max() - get_count()
-- - is_known(): returns true if the item name refers to a defined item type
-- - get_definition(): returns the item definition table
-- - get_tool_digging_properties(): returns the digging properties of the item,
--   ^ or those of the hand if none are defined for this item type
-- - add_wear(amount): increases wear by amount if the item is a tool
-- - add_item(item): put some item or stack onto this stack,
--   ^ returns leftover ItemStack
-- - item_fits(item): returns true if item or stack can be fully added to this one
-- - take_item(n): take (and remove) up to n items from this stack
--   ^ returns taken ItemStack
--   ^ if n is omitted, n=1 is used
-- - peek_item(n): copy (don't remove) up to n items from this stack
--   ^ returns copied ItemStack
--   ^ if n is omitted, n=1 is used
--
-- Registered entities:
-- - Functions receive a "luaentity" as self:
--   - It has the member .name, which is the registered name ("mod:thing")
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
--     visual = "cube"/"sprite",
--     visual_size = {x=1, y=1},
--     textures = {texture,texture,texture,texture,texture,texture},
--     spritediv = {x=1, y=1},
--     initial_sprite_basepos = {x=0, y=0},
--     on_activate = function(self, staticdata),
--     on_step = function(self, dtime),
--     on_punch = function(self, hitter),
--     on_rightclick = function(self, clicker),
--     get_staticdata = function(self),
--     # Also you can define arbitrary member variables here
--     myvariable = whatever,
-- }
--
-- Item definition options (register_node, register_craftitem, register_tool)
-- {
--     description = "Steel Axe",
--     groups = {}, -- key=name, value=rating; rating=1..3.
--                     if rating not applicable, use 1.
--                     eg. {wool=1, fluffy=3}
--                         {soil=2, outerspace=1, crumbly=1}
--                         {hard=3, brittle=3, spikes=2
--                         {hard=1, metal=1, spikes=1}
--     inventory_image = "default_tool_steelaxe.png",
--     wield_image = "",
--     wield_scale = {x=1,y=1,z=1},
--     stack_max = 99,
--     liquids_pointable = false,
--     tool_digging_properties = {
--         full_punch_interval = 1.0,
--         basetime = 1.0,
--         dt_weight = 0.5,
--         dt_crackiness = -0.2,
--         dt_crumbliness = 1,
--         dt_cuttability = -0.5,
--         basedurability = 330,
--         dd_weight = 0,
--         dd_crackiness = 0,
--         dd_crumbliness = 0,
--         dd_cuttability = 0,
--     }
--     on_drop = func(item, dropper, pos),
--     on_place = func(item, placer, pointed_thing),
--     on_use = func(item, user, pointed_thing),
-- }
--
-- Node definition options (register_node):
-- {
--     <all fields allowed in item definitions>,
--     drawtype = "normal",
--     visual_scale = 1.0,
--     tile_images = {"default_unknown_block.png"},
--     special_materials = {
--         {image="", backface_culling=true},
--         {image="", backface_culling=true},
--     },
--     alpha = 255,
--     post_effect_color = {a=0, r=0, g=0, b=0},
--     paramtype = "none",
--     paramtype2 = "none",
--     is_ground_content = false,
--     sunlight_propagates = false,
--     walkable = true,
--     pointable = true,
--     diggable = true,
--     climbable = false,
--     buildable_to = false,
--     drop = "",
--     -- alternatively drop = { max_items = ..., items = { ... } }
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
--     legacy_facedir_simple = false, -- Support maps made in and before January 2012
--     legacy_wallmounted = false, -- Support maps made in and before January 2012
-- }
--
-- Recipe:
-- {
--     output = 'default:pick_stone',
--     recipe = {
--         {'default:cobble', 'default:cobble', 'default:cobble'},
--         {'', 'default:stick', ''},
--         {'', 'default:stick', ''},
--     },
--     replacements = <optional list of item pairs,
--                     replace one input item with another item on crafting>
-- }
--
-- Recipe (shapeless):
-- {
--     type = "shapeless",
--     output = 'mushrooms:mushroom_stew',
--     recipe = {
--         "mushrooms:bowl",
--         "mushrooms:mushroom_brown",
--         "mushrooms:mushroom_red",
--     },
--     replacements = <optional list of item pairs,
--                     replace one input item with another item on crafting>
-- }
--
-- Recipe (tool repair):
-- {
--     type = "toolrepair",
--     additional_wear = -0.02,
-- }
--
-- Recipe (cooking):
-- {
--     type = "cooking",
--     output = "default:glass",
--     recipe = "default:sand",
--     cooktime = 3,
-- }
--
-- Recipe (furnace fuel):
-- {
--     type = "fuel",
--     recipe = "default:leaves",
--     burntime = 1,
-- }
--
-- ABM (ActiveBlockModifier) definition:
-- {
--     nodenames = {"default:lava_source"},
--     neighbors = {"default:water_source", "default:water_flowing"}, -- (any of these)
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

minetest.register_tool("default:pick_wood", {
	description = "Wooden Pickaxe",
	inventory_image = "default_tool_woodpick.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:pick_stone", {
	description = "Stone Pickaxe",
	inventory_image = "default_tool_stonepick.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:pick_steel", {
	description = "Steel Pickaxe",
	inventory_image = "default_tool_steelpick.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:pick_mese", {
	description = "Mese Pickaxe",
	inventory_image = "default_tool_mesepick.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:shovel_wood", {
	description = "Wooden Shovel",
	inventory_image = "default_tool_woodshovel.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:shovel_stone", {
	description = "Stone Shovel",
	inventory_image = "default_tool_stoneshovel.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:shovel_steel", {
	description = "Steel Shovel",
	inventory_image = "default_tool_steelshovel.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:axe_wood", {
	description = "Wooden Axe",
	inventory_image = "default_tool_woodaxe.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:axe_stone", {
	description = "Stone Axe",
	inventory_image = "default_tool_stoneaxe.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:axe_steel", {
	description = "Steel Axe",
	inventory_image = "default_tool_steelaxe.png",
	tool_digging_properties = {
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
	},
})
minetest.register_tool("default:sword_wood", {
	description = "Wooden Sword",
	inventory_image = "default_tool_woodsword.png",
	tool_digging_properties = {
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
	}
})
minetest.register_tool("default:sword_stone", {
	description = "Stone Sword",
	inventory_image = "default_tool_stonesword.png",
	tool_digging_properties = {
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
	}
})
minetest.register_tool("default:sword_steel", {
	description = "Steel Sword",
	inventory_image = "default_tool_steelsword.png",
	tool_digging_properties = {
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
	}
})

--
-- Crafting definition
--

minetest.register_craft({
	output = 'default:wood 4',
	recipe = {
		{'default:tree'},
	}
})

minetest.register_craft({
	output = 'default:stick 4',
	recipe = {
		{'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:fence_wood 2',
	recipe = {
		{'default:stick', 'default:stick', 'default:stick'},
		{'default:stick', 'default:stick', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sign_wall',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:wood', 'default:wood', 'default:wood'},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:torch 4',
	recipe = {
		{'default:coal_lump'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:pick_wood',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:pick_stone',
	recipe = {
		{'default:cobble', 'default:cobble', 'default:cobble'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:pick_steel',
	recipe = {
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:pick_mese',
	recipe = {
		{'default:mese', 'default:mese', 'default:mese'},
		{'', 'default:stick', ''},
		{'', 'default:stick', ''},
	}
})

minetest.register_craft({
	output = 'default:shovel_wood',
	recipe = {
		{'default:wood'},
		{'default:stick'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:shovel_stone',
	recipe = {
		{'default:cobble'},
		{'default:stick'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:shovel_steel',
	recipe = {
		{'default:steel_ingot'},
		{'default:stick'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:axe_wood',
	recipe = {
		{'default:wood', 'default:wood'},
		{'default:wood', 'default:stick'},
		{'', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:axe_stone',
	recipe = {
		{'default:cobble', 'default:cobble'},
		{'default:cobble', 'default:stick'},
		{'', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:axe_steel',
	recipe = {
		{'default:steel_ingot', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:stick'},
		{'', 'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sword_wood',
	recipe = {
		{'default:wood'},
		{'default:wood'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sword_stone',
	recipe = {
		{'default:cobble'},
		{'default:cobble'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:sword_steel',
	recipe = {
		{'default:steel_ingot'},
		{'default:steel_ingot'},
		{'default:stick'},
	}
})

minetest.register_craft({
	output = 'default:rail 15',
	recipe = {
		{'default:steel_ingot', '', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:stick', 'default:steel_ingot'},
		{'default:steel_ingot', '', 'default:steel_ingot'},
	}
})

minetest.register_craft({
	output = 'default:chest',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:wood', '', 'default:wood'},
		{'default:wood', 'default:wood', 'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:chest_locked',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:wood', 'default:steel_ingot', 'default:wood'},
		{'default:wood', 'default:wood', 'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:furnace',
	recipe = {
		{'default:cobble', 'default:cobble', 'default:cobble'},
		{'default:cobble', '', 'default:cobble'},
		{'default:cobble', 'default:cobble', 'default:cobble'},
	}
})

minetest.register_craft({
	output = 'default:steelblock',
	recipe = {
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
		{'default:steel_ingot', 'default:steel_ingot', 'default:steel_ingot'},
	}
})

minetest.register_craft({
	output = 'default:sandstone',
	recipe = {
		{'default:sand', 'default:sand'},
		{'default:sand', 'default:sand'},
	}
})

minetest.register_craft({
	output = 'default:clay',
	recipe = {
		{'default:clay_lump', 'default:clay_lump'},
		{'default:clay_lump', 'default:clay_lump'},
	}
})

minetest.register_craft({
	output = 'default:brick',
	recipe = {
		{'default:clay_brick', 'default:clay_brick'},
		{'default:clay_brick', 'default:clay_brick'},
	}
})

minetest.register_craft({
	output = 'default:paper',
	recipe = {
		{'default:papyrus', 'default:papyrus', 'default:papyrus'},
	}
})

minetest.register_craft({
	output = 'default:book',
	recipe = {
		{'default:paper'},
		{'default:paper'},
		{'default:paper'},
	}
})

minetest.register_craft({
	output = 'default:bookshelf',
	recipe = {
		{'default:wood', 'default:wood', 'default:wood'},
		{'default:book', 'default:book', 'default:book'},
		{'default:wood', 'default:wood', 'default:wood'},
	}
})

minetest.register_craft({
	output = 'default:ladder',
	recipe = {
		{'default:stick', '', 'default:stick'},
		{'default:stick', 'default:stick', 'default:stick'},
		{'default:stick', '', 'default:stick'},
	}
})

--
-- Crafting (tool repair)
--
minetest.register_craft({
	type = "toolrepair",
	additional_wear = -0.02,
})

--
-- Cooking recipes
--

minetest.register_craft({
	type = "cooking",
	output = "default:glass",
	recipe = "default:sand",
})

minetest.register_craft({
	type = "cooking",
	output = "default:coal_lump",
	recipe = "default:tree",
})

minetest.register_craft({
	type = "cooking",
	output = "default:coal_lump",
	recipe = "default:jungletree",
})

minetest.register_craft({
	type = "cooking",
	output = "default:stone",
	recipe = "default:cobble",
})

minetest.register_craft({
	type = "cooking",
	output = "default:steel_ingot",
	recipe = "default:iron_lump",
})

minetest.register_craft({
	type = "cooking",
	output = "default:clay_brick",
	recipe = "default:clay_lump",
})

--
-- Fuels
--

minetest.register_craft({
	type = "fuel",
	recipe = "default:tree",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:jungletree",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:junglegrass",
	burntime = 2,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:leaves",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:cactus",
	burntime = 15,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:papyrus",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:bookshelf",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:fence_wood",
	burntime = 15,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:ladder",
	burntime = 5,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:wood",
	burntime = 7,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:mese",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:lava_source",
	burntime = 60,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:torch",
	burntime = 4,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:sign_wall",
	burntime = 10,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:chest",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:chest_locked",
	burntime = 30,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:nyancat",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:nyancat_rainbow",
	burntime = 1,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:sapling",
	burntime = 10,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:apple",
	burntime = 3,
})

minetest.register_craft({
	type = "fuel",
	recipe = "default:coal_lump",
	burntime = 40,
})

--
-- Node definitions
--

minetest.register_node("default:stone", {
	description = "Stone",
	tile_images = {"default_stone.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(1.0),
	drop = 'default:cobble',
	legacy_mineral = true,
})

minetest.register_node("default:stone_with_coal", {
	description = "Stone with coal",
	tile_images = {"default_stone.png^default_mineral_coal.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(1.0),
	drop = 'default:coal_lump',
})

minetest.register_node("default:stone_with_iron", {
	description = "Stone with iron",
	tile_images = {"default_stone.png^default_mineral_iron.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(1.0),
	drop = 'default:iron_lump',
})

minetest.register_node("default:dirt_with_grass", {
	description = "Dirt with grass",
	tile_images = {"default_grass.png", "default_dirt.png", "default_dirt.png^default_grass_side.png"},
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	drop = 'default:dirt',
})

minetest.register_node("default:dirt_with_grass_footsteps", {
	description = "Dirt with grass and footsteps",
	tile_images = {"default_grass_footsteps.png", "default_dirt.png", "default_dirt.png^default_grass_side.png"},
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	drop = 'default:dirt',
})

minetest.register_node("default:dirt", {
	description = "Dirt",
	tile_images = {"default_dirt.png"},
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
})

minetest.register_node("default:sand", {
	description = "Sand",
	tile_images = {"default_sand.png"},
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
})

minetest.register_node("default:gravel", {
	description = "Gravel",
	tile_images = {"default_gravel.png"},
	is_ground_content = true,
	material = minetest.digprop_gravellike(1.0),
})

minetest.register_node("default:sandstone", {
	description = "Sandstone",
	tile_images = {"default_sandstone.png"},
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),  -- FIXME should this be stonelike?
	drop = 'default:sand',
})

minetest.register_node("default:clay", {
	description = "Clay",
	tile_images = {"default_clay.png"},
	is_ground_content = true,
	material = minetest.digprop_dirtlike(1.0),
	drop = 'default:clay_lump 4',
})

minetest.register_node("default:brick", {
	description = "Brick",
	tile_images = {"default_brick.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(1.0),
	drop = 'default:clay_brick 4',
})

minetest.register_node("default:tree", {
	description = "Tree",
	tile_images = {"default_tree_top.png", "default_tree_top.png", "default_tree.png"},
	is_ground_content = true,
	material = minetest.digprop_woodlike(1.0),
})

minetest.register_node("default:jungletree", {
	description = "Jungle Tree",
	tile_images = {"default_jungletree_top.png", "default_jungletree_top.png", "default_jungletree.png"},
	is_ground_content = true,
	material = minetest.digprop_woodlike(1.0),
})

minetest.register_node("default:junglegrass", {
	description = "Jungle Grass",
	drawtype = "plantlike",
	visual_scale = 1.3,
	tile_images = {"default_junglegrass.png"},
	inventory_image = "default_junglegrass.png",
	wield_image = "default_junglegrass.png",
	paramtype = "light",
	walkable = false,
	material = minetest.digprop_leaveslike(1.0),
})

minetest.register_node("default:leaves", {
	description = "Leaves",
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	tile_images = {"default_leaves.png"},
	paramtype = "light",
	material = minetest.digprop_leaveslike(1.0),
	drop = {
		max_items = 1,
		items = {
			{
				-- player will get sapling with 1/20 chance
				items = {'default:sapling'},
				rarity = 20,
			},
			{
				-- player will get leaves only if he get no saplings,
				-- this is because max_items is 1
				items = {'default:leaves'},
			}
		}
	},
})

minetest.register_node("default:cactus", {
	description = "Cactus",
	tile_images = {"default_cactus_top.png", "default_cactus_top.png", "default_cactus_side.png"},
	is_ground_content = true,
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node("default:papyrus", {
	description = "Papyrus",
	drawtype = "plantlike",
	tile_images = {"default_papyrus.png"},
	inventory_image = "default_papyrus.png",
	wield_image = "default_papyrus.png",
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	material = minetest.digprop_leaveslike(0.5),
})

minetest.register_node("default:bookshelf", {
	description = "Bookshelf",
	tile_images = {"default_wood.png", "default_wood.png", "default_bookshelf.png"},
	is_ground_content = true,
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node("default:glass", {
	description = "Glass",
	drawtype = "glasslike",
	tile_images = {"default_glass.png"},
	inventory_image = minetest.inventorycube("default_glass.png"),
	paramtype = "light",
	sunlight_propagates = true,
	is_ground_content = true,
	material = minetest.digprop_glasslike(1.0),
})

minetest.register_node("default:fence_wood", {
	description = "Wooden Fence",
	drawtype = "fencelike",
	tile_images = {"default_wood.png"},
	inventory_image = "default_fence.png",
	wield_image = "default_fence.png",
	paramtype = "light",
	is_ground_content = true,
	selection_box = {
		type = "fixed",
		fixed = {-1/7, -1/2, -1/7, 1/7, 1/2, 1/7},
	},
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node("default:rail", {
	description = "Rail",
	drawtype = "raillike",
	tile_images = {"default_rail.png", "default_rail_curved.png", "default_rail_t_junction.png", "default_rail_crossing.png"},
	inventory_image = "default_rail.png",
	wield_image = "default_rail.png",
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
	description = "Ladder",
	drawtype = "signlike",
	tile_images = {"default_ladder.png"},
	inventory_image = "default_ladder.png",
	wield_image = "default_ladder.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	is_ground_content = true,
	walkable = false,
	climbable = true,
	selection_box = {
		type = "wallmounted",
		--wall_top = = <default>
		--wall_bottom = = <default>
		--wall_side = = <default>
	},
	material = minetest.digprop_woodlike(0.5),
	legacy_wallmounted = true,
})

minetest.register_node("default:wood", {
	description = "Wood",
	tile_images = {"default_wood.png"},
	is_ground_content = true,
	material = minetest.digprop_woodlike(0.75),
})

minetest.register_node("default:mese", {
	description = "Mese",
	tile_images = {"default_mese.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(0.5),
})

minetest.register_node("default:cloud", {
	description = "Cloud",
	tile_images = {"default_cloud.png"},
	is_ground_content = true,
})

minetest.register_node("default:water_flowing", {
	description = "Water (flowing)",
	inventory_image = minetest.inventorycube("default_water.png"),
	drawtype = "flowingliquid",
	tile_images = {"default_water.png"},
	alpha = WATER_ALPHA,
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "flowing",
	liquid_alternative_flowing = "default:water_flowing",
	liquid_alternative_source = "default:water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	special_materials = {
		{image="default_water.png", backface_culling=false},
		{image="default_water.png", backface_culling=true},
	},
})

minetest.register_node("default:water_source", {
	description = "Water",
	inventory_image = minetest.inventorycube("default_water.png"),
	drawtype = "liquid",
	tile_images = {"default_water.png"},
	alpha = WATER_ALPHA,
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "source",
	liquid_alternative_flowing = "default:water_flowing",
	liquid_alternative_source = "default:water_source",
	liquid_viscosity = WATER_VISC,
	post_effect_color = {a=64, r=100, g=100, b=200},
	special_materials = {
		-- New-style water source material (mostly unused)
		{image="default_water.png", backface_culling=false},
	},
})

minetest.register_node("default:lava_flowing", {
	description = "Lava (flowing)",
	inventory_image = minetest.inventorycube("default_lava.png"),
	drawtype = "flowingliquid",
	tile_images = {"default_lava.png"},
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "flowing",
	liquid_alternative_flowing = "default:lava_flowing",
	liquid_alternative_source = "default:lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		{image="default_lava.png", backface_culling=false},
		{image="default_lava.png", backface_culling=true},
	},
})

minetest.register_node("default:lava_source", {
	description = "Lava",
	inventory_image = minetest.inventorycube("default_lava.png"),
	drawtype = "liquid",
	tile_images = {"default_lava.png"},
	paramtype = "light",
	light_source = LIGHT_MAX - 1,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	liquidtype = "source",
	liquid_alternative_flowing = "default:lava_flowing",
	liquid_alternative_source = "default:lava_source",
	liquid_viscosity = LAVA_VISC,
	damage_per_second = 4*2,
	post_effect_color = {a=192, r=255, g=64, b=0},
	special_materials = {
		-- New-style lava source material (mostly unused)
		{image="default_lava.png", backface_culling=false},
	},
})

minetest.register_node("default:torch", {
	description = "Torch",
	drawtype = "torchlike",
	tile_images = {"default_torch_on_floor.png", "default_torch_on_ceiling.png", "default_torch.png"},
	inventory_image = "default_torch_on_floor.png",
	wield_image = "default_torch_on_floor.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	sunlight_propagates = true,
	walkable = false,
	light_source = LIGHT_MAX-1,
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, 0.5-0.6, -0.1, 0.1, 0.5, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.5, -0.3, -0.1, -0.5+0.3, 0.3, 0.1},
	},
	material = minetest.digprop_constanttime(0.0),
	legacy_wallmounted = true,
})

minetest.register_node("default:sign_wall", {
	description = "Sign",
	drawtype = "signlike",
	tile_images = {"default_sign_wall.png"},
	inventory_image = "default_sign_wall.png",
	wield_image = "default_sign_wall.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	sunlight_propagates = true,
	walkable = false,
	metadata_name = "sign",
	selection_box = {
		type = "wallmounted",
		--wall_top = <default>
		--wall_bottom = <default>
		--wall_side = <default>
	},
	material = minetest.digprop_constanttime(0.5),
	legacy_wallmounted = true,
})

minetest.register_node("default:chest", {
	description = "Chest",
	tile_images = {"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_front.png"},
	paramtype2 = "facedir",
	metadata_name = "chest",
	material = minetest.digprop_woodlike(1.0),
	legacy_facedir_simple = true,
})

minetest.register_node("default:chest_locked", {
	description = "Locked Chest",
	tile_images = {"default_chest_top.png", "default_chest_top.png", "default_chest_side.png",
		"default_chest_side.png", "default_chest_side.png", "default_chest_lock.png"},
	paramtype2 = "facedir",
	metadata_name = "locked_chest",
	material = minetest.digprop_woodlike(1.0),
	legacy_facedir_simple = true,
})

minetest.register_node("default:furnace", {
	description = "Furnace",
	tile_images = {"default_furnace_side.png", "default_furnace_side.png", "default_furnace_side.png",
		"default_furnace_side.png", "default_furnace_side.png", "default_furnace_front.png"},
	paramtype2 = "facedir",
	metadata_name = "furnace",
	material = minetest.digprop_stonelike(3.0),
	legacy_facedir_simple = true,
})

minetest.register_node("default:cobble", {
	description = "Cobble",
	tile_images = {"default_cobble.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(0.9),
})

minetest.register_node("default:mossycobble", {
	description = "Mossy Cobble",
	tile_images = {"default_mossycobble.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(0.8),
})

minetest.register_node("default:steelblock", {
	description = "Steel Block",
	tile_images = {"default_steel_block.png"},
	is_ground_content = true,
	material = minetest.digprop_stonelike(5.0),
})

minetest.register_node("default:nyancat", {
	description = "Nyancat",
	tile_images = {"default_nc_side.png", "default_nc_side.png", "default_nc_side.png",
		"default_nc_side.png", "default_nc_back.png", "default_nc_front.png"},
	inventory_image = "default_nc_front.png",
	paramtype2 = "facedir",
	material = minetest.digprop_stonelike(3.0),
	legacy_facedir_simple = true,
})

minetest.register_node("default:nyancat_rainbow", {
	description = "Nyancat Rainbow",
	tile_images = {"default_nc_rb.png"},
	inventory_image = "default_nc_rb.png",
	material = minetest.digprop_stonelike(3.0),
})

minetest.register_node("default:sapling", {
	description = "Sapling",
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"default_sapling.png"},
	inventory_image = "default_sapling.png",
	wield_image = "default_sapling.png",
	paramtype = "light",
	walkable = false,
	material = minetest.digprop_constanttime(0.0),
})

minetest.register_node("default:apple", {
	description = "Apple",
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"default_apple.png"},
	inventory_image = "default_apple.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	material = minetest.digprop_constanttime(0.0),
	on_use = minetest.item_eat(4),
})

--
-- Crafting items
--

minetest.register_craftitem("default:stick", {
	description = "Stick",
	inventory_image = "default_stick.png",
})

minetest.register_craftitem("default:paper", {
	description = "Paper",
	inventory_image = "default_paper.png",
})

minetest.register_craftitem("default:book", {
	description = "Book",
	inventory_image = "default_book.png",
})

minetest.register_craftitem("default:coal_lump", {
	description = "Lump of coal",
	inventory_image = "default_coal_lump.png",
})

minetest.register_craftitem("default:iron_lump", {
	description = "Lump of iron",
	inventory_image = "default_iron_lump.png",
})

minetest.register_craftitem("default:clay_lump", {
	description = "Lump of clay",
	inventory_image = "default_clay_lump.png",
})

minetest.register_craftitem("default:steel_ingot", {
	description = "Steel ingot",
	inventory_image = "default_steel_ingot.png",
})

minetest.register_craftitem("default:clay_brick", {
	description = "Clay brick",
	inventory_image = "default_steel_ingot.png",
	inventory_image = "default_clay_brick.png",
})

minetest.register_craftitem("default:scorched_stuff", {
	description = "Scorched stuff",
	inventory_image = "default_scorched_stuff.png",
})

--
-- Creative inventory
--

minetest.add_to_creative_inventory('default:pick_mese')
minetest.add_to_creative_inventory('default:pick_steel')
minetest.add_to_creative_inventory('default:axe_steel')
minetest.add_to_creative_inventory('default:shovel_steel')

minetest.add_to_creative_inventory('default:torch')
minetest.add_to_creative_inventory('default:cobble')
minetest.add_to_creative_inventory('default:dirt')
minetest.add_to_creative_inventory('default:stone')
minetest.add_to_creative_inventory('default:sand')
minetest.add_to_creative_inventory('default:sandstone')
minetest.add_to_creative_inventory('default:clay')
minetest.add_to_creative_inventory('default:brick')
minetest.add_to_creative_inventory('default:tree')
minetest.add_to_creative_inventory('default:wood')
minetest.add_to_creative_inventory('default:leaves')
minetest.add_to_creative_inventory('default:cactus')
minetest.add_to_creative_inventory('default:papyrus')
minetest.add_to_creative_inventory('default:bookshelf')
minetest.add_to_creative_inventory('default:glass')
minetest.add_to_creative_inventory('default:fence')
minetest.add_to_creative_inventory('default:rail')
minetest.add_to_creative_inventory('default:mese')
minetest.add_to_creative_inventory('default:chest')
minetest.add_to_creative_inventory('default:furnace')
minetest.add_to_creative_inventory('default:sign_wall')
minetest.add_to_creative_inventory('default:water_source')
minetest.add_to_creative_inventory('default:lava_source')
minetest.add_to_creative_inventory('default:ladder')

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

local function handle_give_command(cmd, giver, receiver, stackstring)
	if not minetest.get_player_privs(giver)["give"] then
		minetest.chat_send_player(giver, "error: you don't have permission to give")
		return
	end
	minetest.debug("DEBUG: "..cmd..' invoked, stackstring="'..stackstring..'"')
	minetest.log(cmd..' invoked, stackstring="'..stackstring..'"')
	local itemstack = ItemStack(stackstring)
	if itemstack:is_empty() then
		minetest.chat_send_player(giver, 'error: cannot give an empty item')
		return
	elseif not itemstack:is_known() then
		minetest.chat_send_player(giver, 'error: cannot give an unknown item')
		return
	end
	local receiverref = minetest.env:get_player_by_name(receiver)
	if receiverref == nil then
		minetest.chat_send_player(giver, receiver..' is not a known player')
		return
	end
	local leftover = receiverref:get_inventory():add_item("main", itemstack)
	if leftover:is_empty() then
		partiality = ""
	elseif leftover:get_count() == itemstack:get_count() then
		partiality = "could not be "
	else
		partiality = "partially "
	end
	if giver == receiver then
		minetest.chat_send_player(giver, '"'..stackstring
			..'" '..partiality..'added to inventory.');
	else
		minetest.chat_send_player(giver, '"'..stackstring
			..'" '..partiality..'added to '..receiver..'\'s inventory.');
		minetest.chat_send_player(receiver, '"'..stackstring
			..'" '..partiality..'added to inventory.');
	end
end

minetest.register_on_chat_message(function(name, message)
	--print("default on_chat_message: name="..dump(name).." message="..dump(message))
	local cmd = "/giveme"
	if message:sub(0, #cmd) == cmd then
		local stackstring = string.match(message, cmd.." (.*)")
		if stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' stackstring')
			return true -- Handled chat message
		end
		handle_give_command(cmd, name, name, stackstring)
		return true
	end
	local cmd = "/give"
	if message:sub(0, #cmd) == cmd then
		local receiver, stackstring = string.match(message, cmd.." ([%a%d_-]+) (.*)")
		if receiver == nil or stackstring == nil then
			minetest.chat_send_player(name, 'usage: '..cmd..' name stackstring')
			return true -- Handled chat message
		end
		handle_give_command(cmd, name, receiver, stackstring)
		return true
	end
	local cmd = "/spawnentity"
	if message:sub(0, #cmd) == cmd then
		if not minetest.get_player_privs(name)["give"] then
			minetest.chat_send_player(name, "you don't have permission to spawn (give)")
			return true -- Handled chat message
		end
		if not minetest.get_player_privs(name)["interact"] then
			minetest.chat_send_player(name, "you don't have permission to interact")
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
	local cmd = "/pulverize"
	if message:sub(0, #cmd) == cmd then
		local player = minetest.env:get_player_by_name(name)
		if player == nil then
			print("Unable to pulverize, player is nil")
			return true -- Handled chat message
		end
		if player:get_wielded_item():is_empty() then
			minetest.chat_send_player(name, 'Unable to pulverize, no item in hand.')
		else
			player:set_wielded_item(nil)
			minetest.chat_send_player(name, 'An item was pulverized.')
		end
		return true
	end
end)

--
-- Done, print some random stuff
--

--print("minetest.registered_entities:")
--dump2(minetest.registered_entities)

-- END
