-- Minetest: builtin/constants.lua

--
-- Constants values for use with the Lua API
--

-- mapnode.h
-- Built-in Content IDs (for use with VoxelManip API)
core.CONTENT_UNKNOWN = 125
core.CONTENT_AIR     = 126
core.CONTENT_IGNORE  = 127

-- emerge.h
-- Block emerge status constants (for use with core.emerge_area)
core.EMERGE_CANCELLED   = 0
core.EMERGE_ERRORED     = 1
core.EMERGE_FROM_MEMORY = 2
core.EMERGE_FROM_DISK   = 3
core.EMERGE_GENERATED   = 4

-- constants.h
-- Size of mapblocks in nodes
core.MAP_BLOCKSIZE = 16
-- Default maximal HP of a player
core.PLAYER_MAX_HP_DEFAULT = 20
-- Default maximal breath of a player
core.PLAYER_MAX_BREATH_DEFAULT = 10
-- DEFAULT_PHYSICS value of a player
core.PLAYER_SPEED_DEFAULT = 1
core.PLAYER_JUMP_DEFAULT = 1
core.PLAYER_GRAVITY_DEFAULT = 1

core.PLAYER_SNEAK_DEFAULT = true
core.PLAYER_SNEAK_GLITCH_DEFAULT = false
core.PLAYER_NEW_MOVE_DEFAULT = true

core.PLAYER_SPEED_CLIMB_DEFAULT = 1
core.PLAYER_SPEED_CROUCH_DEFAULT = 1
core.PLAYER_LIQUID_FLUIDITY_DEFAULT = 1
core.PLAYER_LIQUID_FLUIDITY_SMOOTH_DEFAULT = 1
core.PLAYER_LIQUID_SINK_DEFAULT = 1
core.PLAYER_ACCELERATION_DEFAULT = 1
core.PLAYER_ACCELERATION_AIR_DEFAULT = 1
core.PLAYER_SPEED_FAST_DEFAULT = 1
core.PLAYER_ACCELERATION_FAST_DEFAULT = 1
core.PLAYER_SPEED_WALK_DEFAULT = 1

-- light.h
-- Maximum value for node 'light_source' parameter
core.LIGHT_MAX = 14
