-- Minetest: builtin/constants.lua

--
-- Constants values for use with the Lua API
--


local lua_version = tonumber(_VERSION:match("%d+"))
if lua_version % 100 ~= 0 then
    lua_version = lua_version*10
end

if lua_version >= 540 then
    -- mapnode.h
    -- Built-in Content IDs (for use with VoxelManip API)
    core.CONTENT_UNKNOWN <const> = 125
    core.CONTENT_AIR     <const> = 126
    core.CONTENT_IGNORE  <const> = 127
    
    -- emerge.h
    -- Block emerge status constants (for use with core.emerge_area)
    core.EMERGE_CANCELLED   <const> = 0
    core.EMERGE_ERRORED     <const> = 1
    core.EMERGE_FROM_MEMORY <const> = 2
    core.EMERGE_FROM_DISK   <const> = 3
    core.EMERGE_GENERATED   <const> = 4

    -- constants.h
    -- Size of mapblocks in nodes
    core.MAP_BLOCKSIZE              <const> = 16
    -- Default maximal HP of a player
    core.PLAYER_MAX_HP_DEFAULT      <const> = 20
    -- Default maximal breath of a player
    core.PLAYER_MAX_BREATH_DEFAULT  <const> = 10

    -- light.h
    -- Maximum value for node 'light_source' parameter
    core.LIGHT_MAX <const> = 14
    
else            -- For supporting the backward compatibility
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

    -- light.h
    -- Maximum value for node 'light_source' parameter
    core.LIGHT_MAX = 14
end
