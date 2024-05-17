# Minetest Major Breakages List

This document contains a list of breaking changes to be made in the next major version.
This list is largely advisory and items may be reevaluated once the time comes.

* Remove attachment space multiplier (*10)
* Remove space multiplier for models (*10)
* Remove player gravity multiplier (*2)
* `get_sky()` returns a table (without arg)
* `game.conf` name/id mess
* Remove `depends.txt` / `description.txt` (would simplify ContentDB and Minetest code a little)
* Rotate moon texture by 180°, making it coherent with the sun
    * https://github.com/minetest/minetest/pull/11902
* Remove undocumented `set_physics_override(num, num, num)`
* Remove special handling of `${key}` syntax in metadata values
* Remove `old_move`
* Change physics_override `sneak` to disable the speed change and not just the node clipping
    * https://github.com/minetest/minetest/issues/13699
* Migrate from player names to UUIDs, this would enable renaming of accounts and unicode player names (if desired)
* Harmonize `use_texture_alpha` between entities & nodes, change default to 'opaque' and remove bool compat code
* Merge `sound` and `sounds` table in itemdef
* Remove `DIR_DELIM` from Lua
* Stop reading initial properties from bare entity def.