#!/bin/bash

set -e

echo "Performing fix format..."
if [ -z "${CLANG_FORMAT}" ]; then
	CLANG_FORMAT=clang-format
fi

echo "LINT: Using binary $CLANG_FORMAT"

CLANG_FORMAT_WHITELIST="util/clang-format-whitelist.txt"

files_to_lint="$(find src/ -name '*.cpp' -or -name '*.h')"

for f in ${files_to_lint}; do
	whitelisted=$(awk '$1 == "'$f'" { print 1 }' "$CLANG_FORMAT_WHITELIST")
	if [ -z "${whitelisted}" ]; then
		${CLANG_FORMAT} -i "$f"
	fi
done
