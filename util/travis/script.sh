#!/bin/bash -e

if [ $WINDOWS = "no" ]; then
	mkdir -p travisbuild
	cd travisbuild
	cmake -DENABLE_GETTEXT=1 ..
	make -j2
else
	[ $CC = "clang" ] && exit 1 # Not supposed to happen
	# We need to have our build directory outside of the minetest directory because
	#  CMake will otherwise get very very confused with symlinks and complain that
	#  something is not a subdirectory of something even if it actually is.
	# e.g.:
	# /home/travis/minetest/minetest/travisbuild/minetest
	# \/  \/  \/
	# /home/travis/minetest/minetest/travisbuild/minetest/travisbuild/minetest
	# \/  \/  \/
	# /home/travis/minetest/minetest/travisbuild/minetest/travisbuild/minetest/travisbuild/minetest
	# You get the idea.
	OLDDIR=`pwd`
	cd ..
	[ $WINDOWS = "32" ] && EXISTING_MINETEST_DIR=$OLDDIR NO_MINETEST_GAME=1 $OLDDIR/util/buildbot/buildwin32.sh travisbuild && exit 0
	[ $WINDOWS = "64" ] && EXISTING_MINETEST_DIR=$OLDDIR NO_MINETEST_GAME=1 $OLDDIR/util/buildbot/buildwin64.sh travisbuild && exit 0
fi
