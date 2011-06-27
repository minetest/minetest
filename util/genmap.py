#!/usr/bin/python2

# This is an example script that generates some valid map data.

import struct
import random
import os
import sys
import zlib
import array
from pnoise import pnoise

# Old directory format:
# world/sectors/XXXXZZZZ/YYYY
# XXXX,YYYY,ZZZZ = coordinates in hexadecimal
# fffe = -2
# ffff = -1
# 0000 =  0
# 0001 =  1
# 
# New directory format:
# world/sectors2/XXX/ZZZ/YYYY
# XXX,YYYY,ZZZ = coordinates in hexadecimal
# fffe = -2
# ffff = -1
# 0000 =  0
# 0001 =  1
# ffe = -2
# fff = -1
# 000 =  0
# 001 =  1
#
# For more proper file format documentation, refer to mapformat.txt
# For node type documentation, refer to mapnode.h
# NodeMetadata documentation is not complete, refer to nodemeta.cpp
#

# Seed for generating terrain
SEED = 0

# 0=old, 1=new
SECTOR_DIR_FORMAT = 1

mapdir = "../world"

def to4h(i):
	s = "";
	s += '{0:1x}'.format((i>>12) & 0x000f)
	s += '{0:1x}'.format((i>>8) & 0x000f)
	s += '{0:1x}'.format((i>>4) & 0x000f)
	s += '{0:1x}'.format((i>>0) & 0x000f)
	return s

def to3h(i):
	s = "";
	s += '{0:1x}'.format((i>>8) & 0x000f)
	s += '{0:1x}'.format((i>>4) & 0x000f)
	s += '{0:1x}'.format((i>>0) & 0x000f)
	return s

def get_sector_dir(px, pz):
	global SECTOR_DIR_FORMAT
	if SECTOR_DIR_FORMAT == 0:
		return "/sectors/"+to4h(px)+to4h(pz)
	elif SECTOR_DIR_FORMAT == 1:
		return "/sectors2/"+to3h(px)+"/"+to3h(pz)
	else:
		assert(0)

def getrand_air_stone():
	i = random.randrange(0,2)
	if i==0:
		return 0
	return 254

# 3-dimensional vector (position)
class v3:
	def __init__(self, x=0, y=0, z=0):
		self.X = x
		self.Y = y
		self.Z = z

class NodeMeta:
	def __init__(self, type_id, data):
		self.type_id = type_id
		self.data = data

class StaticObject:
	def __init__(self):
		self.type_id = 0
		self.data = ""

def ser_u16(i):
	return chr((i>>8)&0xff) + chr((i>>0)&0xff)
def ser_u32(i):
	return (chr((i>>24)&0xff) + chr((i>>16)&0xff)
			+ chr((i>>8)&0xff) + chr((i>>0)&0xff))

# A 16x16x16 chunk of map
class MapBlock:
	def __init__(self):
		self.content = array.array('B')
		self.param1 = array.array('B')
		self.param2 = array.array('B')
		for i in range(16*16*16):
			# Initialize to air
			self.content.append(254)
			# Full light on sunlight, none when no sunlight
			self.param1.append(15)
			# No additional parameters
			self.param2.append(0)
		
		# key = v3 pos
		# value = NodeMeta
		self.nodemeta = {}
	
		# key = v3 pos
		# value = StaticObject
		self.static_objects = {}
	
	def set_content(self, v3, b):
		self.content[v3.Z*16*16+v3.Y*16+v3.X] = b
	def set_param1(self, v3, b):
		self.param1[v3.Z*16*16+v3.Y*16+v3.X] = b
	def set_param2(self, v3, b):
		self.param2[v3.Z*16*16+v3.Y*16+v3.X] = b
	
	# Get data for serialization. Returns a string.
	def serialize_data(self):
		s = ""
		for i in range(16*16*16):
			s += chr(self.content[i])
		for i in range(16*16*16):
			s += chr(self.param1[i])
		for i in range(16*16*16):
			s += chr(self.param2[i])
		return s

	def serialize_nodemeta(self):
		s = ""
		s += ser_u16(1)
		s += ser_u16(len(self.nodemeta))
		for pos, meta in self.nodemeta.items():
			pos_i = pos.Z*16*16 + pos.Y*16 + pos.X
			s += ser_u16(pos_i)
			s += ser_u16(meta.type_id)
			s += ser_u16(len(meta.data))
			s += meta.data
		return s

	def serialize_staticobj(self):
		s = ""
		s += chr(0)
		s += ser_u16(len(self.static_objects))
		for pos, obj in self.static_objects.items():
			pos_i = pos.Z*16*16 + pos.Y*16 + pos.X
			s += ser_s32(pos.X*1000)
			s += ser_s32(pos.Y*1000)
			s += ser_s32(pos.Z*1000)
			s += ser_u16(obj.type_id)
			s += ser_u16(len(obj.data))
			s += obj.data
		return s

def writeblock(mapdir, px,py,pz, block):

	sectordir = mapdir + get_sector_dir(px, pz);
	
	try:
		os.makedirs(sectordir)
	except OSError:
		pass
	
	path = sectordir+"/"+to4h(py)

	print("writing block file "+path)

	f = open(sectordir+"/"+to4h(py), "wb")

	if f == None:
		return

	# version
	version = 17
	f.write(struct.pack('B', version))

	# flags
	# 0x01=is_undg, 0x02=dn_diff, 0x04=lighting_expired
	flags = 0 + 0x02 + 0x04
	f.write(struct.pack('B', flags))
	
	# data
	c_obj = zlib.compressobj()
	c_obj.compress(block.serialize_data())
	f.write(struct.pack('BB', 0x78, 0x9c)) # zlib magic number
	f.write(c_obj.flush())

	# node metadata
	c_obj = zlib.compressobj()
	c_obj.compress(block.serialize_nodemeta())
	f.write(struct.pack('BB', 0x78, 0x9c)) # zlib magic number
	f.write(c_obj.flush())

	# mapblockobject count
	f.write(ser_u16(0))

	# static objects
	f.write(block.serialize_staticobj())

	# timestamp
	f.write(ser_u32(0xffffffff))

	f.close()

for z0 in range(-1,3):
	for x0 in range(-1,3):
		for y0 in range(-1,3):
			print("generating block "+str(x0)+","+str(y0)+","+str(z0))
			#v3 blockp = v3(x0,y0,z0)
			
			# Create a MapBlock
			block = MapBlock()
			
			# Generate stuff in it
			for z in range(0,16):
				for x in range(0,16):
					h = 20.0*pnoise((x0*16+x)/100.,(z0*16+z)/100.,SEED+0)
					h += 5.0*pnoise((x0*16+x)/25.,(z0*16+z)/25.,SEED+0)
					if pnoise((x0*16+x)/25.,(z0*16+z)/25.,SEED+92412) > 0.05:
						h += 10
					#print("r="+str(r))
					# This enables comparison by ==
					h = int(h)
					for y in range(0,16):
						p = v3(x,y,z)
						b = 254
						y1 = y0*16+y
						if y1 <= h-3:
							b = 0 #stone
						elif y1 <= h and y1 <= 0:
							b = 8 #mud
						elif y1 == h:
							b = 1 #grass
						elif y1 < h:
							b = 8 #mud
						elif y1 <= 1:
							b = 9 #water

						# Material content
						block.set_content(p, b)

					# Place a sign at the center at surface level.
					# Placing a sign means placing the sign node and
					# adding node metadata to the mapblock.
					if x == 8 and z == 8 and y0*16 <= h-1 and (y0+1)*16-1 > h:
						p = v3(8,h+1-y0*16,8)
						# 14 = Sign
						content_type = 14
						block.set_content(p, content_type)
						# This places the sign to the bottom of the cube.
						# Working values: 0x01, 0x02, 0x04, 0x08, 0x10, 0x20
						block.set_param2(p, 0x08)
						# Then add metadata to hold the text of the sign
						s = "Hello at sector ("+str(x0)+","+str(z0)+")"
						meta = NodeMeta(content_type, ser_u16(len(s))+s)
						block.nodemeta[p] = meta

			# Write it on disk
			writeblock(mapdir, x0,y0,z0, block)

#END
