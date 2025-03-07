#!/bin/bash

set -e

rm -rf public
mkdir public

redirect() {
    dir=$(dirname "public$1")
    mkdir -p $dir
    cp misc/redirect.html "public$1"

    url=${1/\/index.html/\/}
    sed -i "s|URL|$url|g" "public$1"
}

redirect /inventory/index.html
redirect /definition-tables/index.html
redirect /aliases/index.html
redirect /items/index.html
redirect /registered-definitions/index.html
redirect /colors/index.html
redirect /decoration-types/index.html
redirect /hud/index.html
redirect /index.html
redirect /spatial-vectors/index.html
redirect /metadata/index.html
redirect /perlin-noise/index.html
redirect /translations/index.html
redirect /tool-capabilities/index.html
redirect /l-system-trees/index.html
redirect /entity-damage-mechanism/index.html
redirect /escape-sequences/index.html
redirect /registered-entities/index.html
redirect /flag-specifier-format/index.html
redirect /minetest-namespace-reference/index.html
redirect /ores/index.html
redirect /search.html
redirect /representations-of-simple-things/index.html
redirect /nodes/index.html
redirect /lua-voxel-manipulator/index.html
redirect /helper-functions/index.html
redirect /formspec/index.html
redirect /games/index.html
redirect /sounds/index.html
redirect /textures/index.html
redirect /map-terminology-and-coordinates/index.html
redirect /schematics/index.html
redirect /groups/index.html
redirect /privileges/index.html
redirect /class-reference/index.html
redirect /mods/index.html
redirect /mapgen-objects/index.html
