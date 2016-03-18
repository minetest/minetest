#!/bin/sh

git diff master --check || exit 1

fail=false

print_commit_err() {
	echo "$1: commit message: $2" >&2
}

git rev-list master..HEAD | while read -r commitid; do
	commitmsg=`git cat-file commit $commitid | sed '1,/^$/d'`
	linenum=0
	printf '%s\n' "$commitmsg" | while read -r line; do
		case "$linenum" in
			0)	if [ "`printf \"$line\" | wc -L`" -gt "70" ]; then
					print_commit_err "$commitid" "First line is longer than 70 chars"; fail=true
				fi
				if ! printf "$line" | cut -c 1 | grep -q '[[:upper:]]'; then
					print_commit_err "$commitid" "First line doesn't start with capital letter"
					fail=true
				fi
				;;
			1)	if [ "`printf \"$line\" | wc -L`" -gt "0" ]; then
					print_commit_err "$commitid" "Second line is not blank"; fail=true
				fi
				;;
			*)
		esac
		linenum=$(($linenum+1))
	done
	# echo "$commitid is fine"
done

if [ $fail ]; then
	exit 1
fi
