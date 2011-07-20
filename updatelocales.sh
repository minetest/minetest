#! /bin/bash
xgettext -n -o minetest-c55.pot ./src/*.cpp ./src/*.h
msgmerge -U ./po/de/minetest-c55.po minetest-c55.pot
rm minetest-c55.pot
