#! /bin/bash

function setup_for_format() {
	if [ -z "${CLANG_FORMAT}" ]; then
		CLANG_FORMAT=clang-format
	fi
	echo "LINT: Using binary $CLANG_FORMAT"
	CLANG_FORMAT_WHITELIST="util/ci/clang-format-whitelist.txt"

	files_to_lint="$(find src/ -name '*.cpp' -or -name '*.h')"
}

function check_format() {
	echo "Checking format..."

	setup_for_format

	local errorcount=0
	local fail=0
	for f in ${files_to_lint}; do
		d=$(diff -u "$f" <(${CLANG_FORMAT} "$f") || true)

		if ! [ -z "$d" ]; then
			whitelisted=$(awk '$1 == "'$f'" { print 1 }' "$CLANG_FORMAT_WHITELIST")

			# If file is not whitelisted, mark a failure
			if [ -z "${whitelisted}" ]; then
				errorcount=$((errorcount+1))

				printf "The file %s is not compliant with the coding style" "$f"
				if [ ${errorcount} -gt 50 ]; then
					printf "\nToo many errors encountered previously, this diff is hidden.\n"
				else
					printf ":\n%s\n" "$d"
				fi

				fail=1
			fi
		fi
	done

	if [ "$fail" = 1 ]; then
		echo "LINT reports failure."
		exit 1
	fi

	echo "LINT OK"
}



function fix_format() {
	echo "Fixing format..."

	setup_for_format

	for f in ${files_to_lint}; do
		whitelisted=$(awk '$1 == "'$f'" { print 1 }' "$CLANG_FORMAT_WHITELIST")
		if [ -z "${whitelisted}" ]; then
			echo "$f"
			$CLANG_FORMAT -i "$f"
		fi
	done
}
