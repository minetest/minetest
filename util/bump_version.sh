#!/bin/bash

die() { echo "$@" 1>&2 ; exit 1; }

prompt_for_number() {
	local prompt_text=$1
	local default_value=$2
	local tmp=""
	while true; do
		read -p "$prompt_text [$default_value]: " tmp
		if [ "$tmp" = "" ]; then
			echo "$default_value"; return
		elif echo "$tmp" | grep -q -E '^[0-9]+$'; then
			echo "$tmp"; return
		fi
	done
}


##################################
# Switch to top minetest directory
##################################

cd ${0%/*}/..


#######################
# Determine old version
#######################

# Make sure all the files we need exist
grep -q -E '^set\(VERSION_MAJOR [0-9]+\)$' CMakeLists.txt || die "error: Could not find CMakeLists.txt"
grep -q -E '^set\(VERSION_MINOR [0-9]+\)$' CMakeLists.txt || die "error: Could not find CMakeLists.txt"
grep -q -E '^set\(VERSION_PATCH [0-9]+\)$' CMakeLists.txt || die "error: Could not find CMakeLists.txt"
grep -q -E '^ANDROID_VERSION_CODE = [0-9]+$' build/android/Makefile || die "error: Could not find build/android/Makefile"

VERSION_MAJOR=$(grep -E '^set\(VERSION_MAJOR [0-9]+\)$' CMakeLists.txt | tr -dC 0-9)
VERSION_MINOR=$(grep -E '^set\(VERSION_MINOR [0-9]+\)$' CMakeLists.txt | tr -dC 0-9)
VERSION_PATCH=$(grep -E '^set\(VERSION_PATCH [0-9]+\)$' CMakeLists.txt | tr -dC 0-9)
ANDROID_VERSION_CODE=$(grep -E '^ANDROID_VERSION_CODE = [0-9]+$' build/android/Makefile | tr -dC 0-9)

echo "Current Minetest version: $VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH"
echo "Current Android version code: $ANDROID_VERSION_CODE"


########################
# Prompt for new version
########################

NEW_VERSION_MAJOR=$VERSION_MAJOR
NEW_VERSION_MINOR=$VERSION_MINOR
NEW_VERSION_PATCH=$(expr $VERSION_PATCH + 1)

NEW_VERSION_MAJOR=$(prompt_for_number "Set major" $NEW_VERSION_MAJOR)

if [ "$NEW_VERSION_MAJOR" != "$VERSION_MAJOR" ]; then
	NEW_VERSION_MINOR=0
	NEW_VERSION_PATCH=0
fi

NEW_VERSION_MINOR=$(prompt_for_number "Set minor" $NEW_VERSION_MINOR)

if [ "$NEW_VERSION_MINOR" != "$VERSION_MINOR" ]; then
	NEW_VERSION_PATCH=0
fi

NEW_VERSION_PATCH=$(prompt_for_number "Set patch" $NEW_VERSION_PATCH)

NEW_ANDROID_VERSION_CODE=$(expr $ANDROID_VERSION_CODE + 1)
NEW_ANDROID_VERSION_CODE=$(prompt_for_number "Set android version code" $NEW_ANDROID_VERSION_CODE)

NEW_VERSION="$NEW_VERSION_MAJOR.$NEW_VERSION_MINOR.$NEW_VERSION_PATCH"


echo
echo "New version: $NEW_VERSION"
echo "New android version code: $NEW_ANDROID_VERSION_CODE"


#######################################
# Replace version everywhere and commit
#######################################

sed -i -re "s/^set\(VERSION_MAJOR [0-9]+\)$/set(VERSION_MAJOR $NEW_VERSION_MAJOR)/" CMakeLists.txt || die "Failed to update VERSION_MAJOR"

sed -i -re "s/^set\(VERSION_MINOR [0-9]+\)$/set(VERSION_MINOR $NEW_VERSION_MINOR)/" CMakeLists.txt || die "Failed to update VERSION_MINOR"

sed -i -re "s/^set\(VERSION_PATCH [0-9]+\)$/set(VERSION_PATCH $NEW_VERSION_PATCH)/" CMakeLists.txt || die "Failed to update VERSION_PATCH"

sed -i -re "s/^\tset\(VERSION_PATCH \\\$.VERSION_PATCH}-dev\)$/\t#set(VERSION_PATCH \${VERSION_PATCH}-dev)/" CMakeLists.txt || die "Failed to disable -dev suffix"

sed -i -re "s/^ANDROID_VERSION_CODE = [0-9]+$/ANDROID_VERSION_CODE = $NEW_ANDROID_VERSION_CODE/" build/android/Makefile || die "Failed to update ANDROID_VERSION_CODE"

sed -i -re "1s/[0-9]+\.[0-9]+\.[0-9]+/$NEW_VERSION/g" doc/lua_api.txt || die "Failed to update doc/lua_api.txt"

sed -i -re "1s/[0-9]+\.[0-9]+\.[0-9]+/$NEW_VERSION/g" doc/menu_lua_api.txt || die "Failed to update doc/menu_lua_api.txt"

git add -f CMakeLists.txt build/android/Makefile doc/lua_api.txt doc/menu_lua_api.txt || die "git add failed"

git commit -m "Bump version to $NEW_VERSION" || die "git commit failed"
