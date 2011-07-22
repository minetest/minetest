#! /bin/bash
xgettext -n -o minetest.pot ./src/*.cpp ./src/*.h
msgmerge -U ./po/de/minetest.po minetest.pot
msgmerge -U ./po/fr/minetest.po minetest.pot
rm minetest.pot
