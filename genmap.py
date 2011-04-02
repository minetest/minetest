#!/usr/bin/python

"
This is an example script that generates some valid map data.
"

import struct
import random

def getrand():
	i = random.randrange(0,2)
	if i==0:
		return 0
	return 254

"""
Map format:
map/sectors/XXXXZZZZ/YYYY

XXXX,YYYY,ZZZZ = coordinates in hexadecimal

fffe = -2
ffff = -1
0000 =  0
0001 =  1
"""

f = open("map/sectors/00000000/ffff", "wb")

# version
f.write(struct.pack('B', 2))
# is_underground
f.write(struct.pack('B', 0))

for i in range(0,16*16*16):
	# Material content
	f.write(struct.pack('B', getrand()))
	# Brightness
	f.write(struct.pack('B', 15))

f.close()

