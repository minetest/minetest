#!/bin/bash -e

optimized=0

for file in $(git ls-files *.png); do
	 # Does only run a fuzzy check without -o7 -zm1-9 since it would be too slow otherwise
	output=$(optipng -nc -strip all -simulate $file 2>&1 >/dev/null)
	if ! grep -q "is already optimized" <<< $output; then
		echo -e "\033[0;31m$file\033[0m"
		optimized=1
	fi
done

if [ $optimized -eq 1 ] ; then
	echo "Found non-optimized png file, apply 'optipng -o7 -zm1-9 -nc -strip all -clobber <filename>'"
fi

exit $optimized
