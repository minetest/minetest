#!/bin/bash -e

optimized=$(git ls-files "*.png" | sort -u | while read file; do
	 # Does only run a fuzzy check without -o7 -zm1-9 since it would be too slow otherwise
	output=$(optipng -nc -strip all -simulate "$file" 2>&1 >/dev/null)
	if ! grep -q "is already optimized" <<< "$output"; then
		echo -e "\033[0;31m$file\033[0m"
	fi
done)

if [[ -n "$optimized" ]] ; then
	echo "$optimized"
	echo "Found non-optimized png file, apply 'optipng -o7 -zm1-9 -nc -strip all -clobber <filename>'"
	exit 1
fi
