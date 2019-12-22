-- Minetest: builtin/constants.lua

--
-- Constants values for use with the Lua API
--

-- mapnode.h
-- Built-in Content IDs (for use with VoxelManip API)
minetest.CONTENT_UNKNOWN = 125
minetest.CONTENT_AIR     = 126
minetest.CONTENT_IGNORE  = 127

-- emerge.h
-- Block emerge status constants (for use with minetest.emerge_area)
minetest.EMERGE_CANCELLED   = 0
minetest.EMERGE_ERRORED     = 1
minetest.EMERGE_FROM_MEMORY = 2
minetest.EMERGE_FROM_DISK   = 3
minetest.EMERGE_GENERATED   = 4

-- constants.h
-- Size of mapblocks in nodes
minetest.MAP_BLOCKSIZE = 16
-- Default maximal HP of a player
minetest.PLAYER_MAX_HP_DEFAULT = 20
-- Default maximal breath of a player
minetest.PLAYER_MAX_BREATH_DEFAULT = 11

-- light.h
-- Maximum value for node 'light_source' parameter
minetest.LIGHT_MAX = 14
