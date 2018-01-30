#!/bin/sh

mydir="$(dirname "$(which "$0")")"
cd "$mydir/.."

lua5.1 "util/generate_from_settingtypes.lua"
