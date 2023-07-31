# Minetest Major Breakages List

This document contains a list of breaking changes to be made in the next major version.

* Remove attachment space multiplier (*10)
* Remove player gravity multiplier (*2)
* `game.conf` name/id mess
* remove `depends.txt` / `description.txt` (would simplify ContentDB and Minetest code a little)
* `get_sky()` returns a table (without arg)
* rotate moon texture by 180°, making it coherent with the sun (see https://github.com/minetest/minetest/pull/11902)
* remove `shadow_sky_body_orbit_tilt` setting (see https://github.com/minetest/minetest/issues/13705#issuecomment-1658508029)
* remove undocumented `set_physics_override(num, num, num)`
* remove special handling of `${key}` syntax in metadata values
