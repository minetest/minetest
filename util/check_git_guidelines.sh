#!/bin/sh

git diff master --check || exit 1

https://git-scm.com/book/uz/v2/Customizing-Git-An-Example-Git-Enforced-Policy
for commitid in `git rev-list master..HEAD`; do
	commitmsg=`git cat-file commit $commitid | sed sed '1,/^$/d'`
	linenum=0
	for line in $commitmsg; do
		case "$linenum" in
			0)	if [ "`echo \"$line\" | wc -L`" -gt "70" ]; then
					echo "$commitid: commit message: First line is longer than 70 chars"; exit 1
				fi
				;;
			1)	if [ "`echo \"$line\" | wc -L`" -gt "0" ]; then
					echo "$commitid: commit message: Second line is not blank"; exit 1
				fi
				;;
			*)
		esac
		let linenum=$linenum+1
	done
	$commitmsg
done
