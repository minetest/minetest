#!/bin/bash

function perform_lint() {
	echo "Performing lint..."
	local FORMAT_WHITELIST="util/lint-whitelist.txt"

	local OPTIONS=$(<util/astyle.conf)$'\n'
	while read line; do
		OPTIONS+="exclude='$line'"$'\n'
	done <$FORMAT_WHITELIST

	local RESULTS=$(astyle --formatted --recursive --suffix=none \
		--options=<(echo $OPTIONS) 'src/*.cpp' 'src/*.h')

	if [[ $? != 0 ]]; then
		echo "Lint failed to run."
		exit 1
	fi

	local FORMATTED=$(echo "$RESULTS" | grep Formatted)

	if [[ $FORMATTED != "" ]]; then
		echo "Lint failed: changes needed:"
		echo "$FORMATTED"
		git --no-pager diff --minimal
		exit 1
	fi

	echo "Lint succeeded."
}

perform_lint
