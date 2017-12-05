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
grep -q -E 'versionCode [0-9]+$' build/android/build.gradle || die "error: Could not find Android version code"

VERSION_MAJOR=$(grep -E '^set\(VERSION_MAJOR [0-9]+\)$' CMakeLists.txt | tr -dC 0-9)
VERSION_MINOR=$(grep -E '^set\(VERSION_MINOR [0-9]+\)$' CMakeLists.txt | tr -dC 0-9)
VERSION_PATCH=$(grep -E '^set\(VERSION_PATCH [0-9]+\)$' CMakeLists.txt | tr -dC 0-9)
ANDROID_VERSION_CODE=$(grep -E 'versionCode [0-9]+$' build/android/build.gradle | tr -dC 0-9)

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

sed -i -re "s/^set\(DEVELOPMENT_BUILD TRUE\)$/set(DEVELOPMENT_BUILD FALSE)/" CMakeLists.txt || die "Failed to unset DEVELOPMENT_BUILD"

sed -i -re "s/versionCode [0-9]+$/versionCode $NEW_ANDROID_VERSION_CODE/" build/android/build.gradle || die "Failed to update Android version code"

sed -i -re "1s/[0-9]+\.[0-9]+\.[0-9]+/$NEW_VERSION/g" doc/lua_api.txt || die "Failed to update doc/lua_api.txt"

sed -i -re "1s/[0-9]+\.[0-9]+\.[0-9]+/$NEW_VERSION/g" doc/menu_lua_api.txt || die "Failed to update doc/menu_lua_api.txt"

git add -f CMakeLists.txt build/android/build.gradle doc/lua_api.txt doc/menu_lua_api.txt || die "git add failed"

git commit -m "Bump version to $NEW_VERSION" || die "git commit failed"

############
# Create tag
############

echo "Tagging $NEW_VERSION"

git tag -a "$NEW_VERSION" -m "$NEW_VERSION" || die 'Adding tag failed'

######################
# Create revert commit
######################

echo 'Creating "revert to development" commit'

sed -i -re 's/^set\(DEVELOPMENT_BUILD FALSE\)$/set(DEVELOPMENT_BUILD TRUE)/' CMakeLists.txt || die 'Failed to set DEVELOPMENT_BUILD'

git add -f CMakeLists.txt || die 'git add failed'

git commit -m "Continue with $NEW_VERSION-dev" || die 'git commit failed'

