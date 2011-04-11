#!/usr/bin/python2

# This is an example script that generates some valid map data.

import struct
import random
import os
import sys
from pnoise import pnoise

"""
Map format:
map/sectors/XXXXZZZZ/YYYY

XXXX,YYYY,ZZZZ = coordinates in hexadecimal

fffe = -2
ffff = -1
0000 =  0
0001 =  1
"""

def to4h(i):
	s = "";
	s += '{0:1x}'.format((i>>12) & 0x000f)
	s += '{0:1x}'.format((i>>8) & 0x000f)
	s += '{0:1x}'.format((i>>4) & 0x000f)
	s += '{0:1x}'.format((i>>0) & 0x000f)
	return s

def getrand():
	i = random.randrange(0,2)
	if i==0:
		return 0
	return 254

def writeblock(mapdir, px,py,pz, version):
	sectordir = mapdir + "/sectors/" + to4h(px) + to4h(pz)
	
	try:
		os.makedirs(sectordir)
	except OSError:
		pass

	f = open(sectordir+"/"+to4h(py), "wb")

	if version == 0:
		# version
		f.write(struct.pack('B', 0))
		# is_underground
		f.write(struct.pack('B', 0))
	elif version == 2:
		# version
		f.write(struct.pack('B', 2))
		# is_underground
		f.write(struct.pack('B', 0))
	
	for z in range(0,16):
		for y in range(0,16):
			for x in range(0,16):
				b = 254
				r = 20.0*pnoise((px*16+x)/100.,(pz*16+z)/100.,0)
				r += 5.0*pnoise((px*16+x)/25.,(pz*16+z)/25.,0)
				#print("r="+str(r))
				y1 = py*16+y
				if y1 <= r-3:
					b = 0 #stone
				elif y1 <= r:
					b = 1 #grass
				elif y1 <= 1:
					b = 9 #water
				if version == 0:
					# Material content
					f.write(struct.pack('B', b))
				elif version == 2:
					# Material content
					f.write(struct.pack('B', b))
					# Brightness
					f.write(struct.pack('B', 15))

	f.close()

mapdir = "map"

for z in range(-2,3):
	for y in range(-1,2):
		for x in range(-2,3):
			print("generating block "+str(x)+","+str(y)+","+str(z))
			writeblock(mapdir, x,y,z, 0)

#END
