#!/bin/sh
for i in `grep -L 'This program is free software' src/*.{h,cpp}`
do
	cat licensecomment.txt > tempfile
	cat $i >> tempfile
	cp tempfile $i
done
rm tempfile

