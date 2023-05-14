# Minetest Major Breakages List

This document contains a list of breaking changes to be made in the next major version.

* Remove attachment space multiplier (*10)
* Remove player gravity multiplier (*2)
* `get_sky()` returns a table (without arg)
* `game.conf` name/id mess
* Remove `depends.txt` / `description.txt` (would simplify ContentDB and Minetest code a little)
* ~~Rotate moon texture by 180°, making it coherent with the sun (see https://github.com/minetest/minetest/pull/11902)~~
* ~~Remove undocumented `set_physics_override(num, num, num)`~~
* Remove special handling of `${key}` syntax in metadata values
