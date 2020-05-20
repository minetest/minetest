#! /bin/bash

checkguard_output=$(python -m guardonce.checkguard -r ./src)
if [[ -n ${checkguard_output} ]]; then
	echo "The following header files are missing an include guard:"
	echo "${checkguard_output}"
	exit 1
fi
