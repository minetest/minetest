#!/usr/bin/env python
# -*- coding: windows-1252 -*-

# Made by j0gge, modified by celeron55

# This program is free software. It comes without any warranty, to
# the extent permitted by applicable law. You can redistribute it
# and/or modify it under the terms of the Do What The Fuck You Want
# To Public License, Version 2, as published by Sam Hocevar. See
# http://sam.zoy.org/wtfpl/COPYING for more details.

# Requires Python Imaging Library: http://www.pythonware.com/products/pil/

# Some speed-up: ...lol, actually it slows it down.
#import psyco ; psyco.full() 
#from psyco.classes import *

import zlib
import Image, ImageDraw
import os
import string
import time

def hex_to_int(h):
	i = int(h,16)
	if(i > 2047):
		i-=4096
	return i

def hex4_to_int(h):
	i = int(h,16)
	if(i > 32767):
		i-=65536
	return i

def int_to_hex3(i):
	if(i < 0):
		return "%03X" % (i + 4096)
	else:
		return "%03X" % i

def int_to_hex4(i):
	if(i < 0):
		return "%04X" % (i + 65536)
	else:
		return "%04X" % i

def limit(i,l,h):
	if(i>h):
		i=h
	if(i<l):
		i=l
	return i
	
# Fix these!
path="../map/"
output="map.png"

sector_xmin = -1000/16
sector_xmax = 1000/16
sector_zmin = -1000/16
sector_zmax = 1000/16

# Load color information for the blocks.
colors = {}
f = file("colors.txt")
for line in f:
	values = string.split(line)
	colors[int(values[0])] = (int(values[1]), int(values[2]), int(values[3]))
f.close()

xlist = []
zlist = []

# List all sectors to memory and calculate the width and heigth of the resulting picture.
for filename in os.listdir(path + "sectors2"):
	for filename2 in os.listdir(path + "sectors2/" + filename):
		x = hex_to_int(filename)
		z = hex_to_int(filename2)
		if x < sector_xmin or x > sector_xmax:
			continue
		if z < sector_zmin or z > sector_zmax:
			continue
		xlist.append(x)
		zlist.append(z)
for filename in os.listdir(path + "sectors"):
	x = hex4_to_int(filename[:4])
	z = hex4_to_int(filename[-4:])
	if x < sector_xmin or x > sector_xmax:
		continue
	if z < sector_zmin or z > sector_zmax:
		continue
	xlist.append(x)
	zlist.append(z)

w = (max(xlist) - min(xlist)) * 16 + 16
h = (max(zlist) - min(zlist)) * 16 + 16

print "w="+str(w)+" h="+str(h)

im = Image.new("RGB", (w, h), "white")
impix = im.load()

stuff={}

starttime = time.time()

# Go through all sectors.
for n in range(len(xlist)):
	#if n > 500:
	#	break
	if n % 200 == 0:
		nowtime = time.time()
		dtime = nowtime - starttime
		n_per_second = 1.0 * n / dtime
		if n_per_second != 0:
			seconds_per_n = 1.0 / n_per_second
			time_guess = seconds_per_n * len(xlist)
			remaining_s = time_guess - dtime
			remaining_minutes = int(remaining_s / 60)
			remaining_s -= remaining_minutes * 60;
			print("Processing sector "+str(n)+" of "+str(len(xlist))
					+" ("+str(round(100.0*n/len(xlist), 1))+"%)"
					+" (ETA: "+str(remaining_minutes)+"m "
					+str(int(remaining_s))+"s)")

	xpos = xlist[n]
	zpos = zlist[n]
	
	xhex = int_to_hex3(xpos)
	zhex = int_to_hex3(zpos)
	xhex4 = int_to_hex4(xpos)
	zhex4 = int_to_hex4(zpos)
	
	sector1 = xhex4.lower() + zhex4.lower()
	sector2 = xhex.lower() + "/" + zhex.lower()
	
	ylist=[]
	
	sectortype = ""
	
	try:
		for filename in os.listdir(path + "sectors/" + sector1):
			if(filename != "meta"):
				pos = int(filename,16)
				if(pos > 32767):
					pos-=65536
				ylist.append(pos)
				sectortype = "old"
	except OSError:
		pass
	
	if sectortype != "old":
		try:
			for filename in os.listdir(path + "sectors2/" + sector2):
				if(filename != "meta"):
					pos = int(filename,16)
					if(pos > 32767):
						pos-=65536
					ylist.append(pos)
					sectortype = "new"
		except OSError:
			pass
	
	if sectortype == "":
		continue

	ylist.sort()
	
	# Make a list of pixels of the sector that are to be looked for.
	pixellist = []
	for x in range(16):
		for y in range(16):
			pixellist.append((x,y))
	
	# Go through the Y axis from top to bottom.
	for ypos in reversed(ylist):
		
		yhex = int_to_hex4(ypos)

		filename = ""
		if sectortype == "old":
			filename = path + "sectors/" + sector1 + "/" + yhex.lower()
		else:
			filename = path + "sectors2/" + sector2 + "/" + yhex.lower()

		f = file(filename, "rb")

		# Let's just memorize these even though it's not really necessary.
		version = f.read(1)
		flags = f.read(1)

		dec_o = zlib.decompressobj()
		try:
			mapdata = dec_o.decompress(f.read())
		except:
			mapdata = []
			
		f.close()
		
		if(len(mapdata)<4096):
			print "bad: " + xhex+zhex+"/"+yhex + " " + len(mapdata)
		else:
			chunkxpos=xpos*16
			chunkypos=ypos*16
			chunkzpos=zpos*16
			for (x,z) in reversed(pixellist):
				for y in reversed(range(16)):
					datapos=x+y*16+z*256
					if(ord(mapdata[datapos])!=254):
						try:
							pixellist.remove((x,z))
							# Memorize information on the type and height of the block and for drawing the picture.
							stuff[(chunkxpos+x,chunkzpos+z)]=(chunkypos+y,ord(mapdata[datapos]))
							break
						except:
							print "strange block: " + xhex+zhex+"/"+yhex + " x: " + str(x) + " y: " + str(y) + " z: " + str(z) + " block: " + str(ord(mapdata[datapos]))
		
		# After finding all the pixeld in the sector, we can move on to the next sector without having to continue the Y axis.
		if(len(pixellist)==0):
			break

print "Drawing image"
# Drawing the picture
starttime = time.time()
n = 0
minx = min(xlist)
minz = min(zlist)
for (x,z) in stuff.iterkeys():
	if n % 500000 == 0:
		nowtime = time.time()
		dtime = nowtime - starttime
		n_per_second = 1.0 * n / dtime
		if n_per_second != 0:
			listlen = len(stuff)
			seconds_per_n = 1.0 / n_per_second
			time_guess = seconds_per_n * listlen
			remaining_s = time_guess - dtime
			remaining_minutes = int(remaining_s / 60)
			remaining_s -= remaining_minutes * 60;
			print("Drawing pixel "+str(n)+" of "+str(listlen)
					+" ("+str(round(100.0*n/listlen, 1))+"%)"
					+" (ETA: "+str(remaining_minutes)+"m "
					+str(int(remaining_s))+"s)")
	n += 1

	(r,g,b)=colors[stuff[(x,z)][1]]
	
	# Comparing heights of a couple of adjacent blocks and changing brightness accordingly.
	try:
		y1=stuff[(x-1,z)][0]
		y2=stuff[(x,z-1)][0]
		y=stuff[(x,z)][0]
		
		d=(y-y1+y-y2)*12
		
		if(d>36):
			d=36
			
		r=limit(r+d,0,255)
		g=limit(g+d,0,255)
		b=limit(b+d,0,255)
	except:
		pass
	#impix[w-1-(x-minx*16),h-1-(z-minz*16)]=(r,g,b)
	impix[x-minx*16,h-1-(z-minz*16)]=(r,g,b)

# Flip the picture to make it right and save.
#print "Transposing"
#im=im.transpose(Image.FLIP_TOP_BOTTOM)	
print "Saving"
im.save(output)
