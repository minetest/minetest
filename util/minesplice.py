#!/usr/bin/env python
# -*- coding: utf-8 -*-
'''
minesplice.py: copy, swap, and delete minetest blocks between map files
Copyright (C) 2012 Corey Edmunds (corey.edmunds@gmail.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
'''

from optparse import OptionParser
import re
import math
import sqlite3 as sql

def blockAsInt(x, y, z):
	return int(z * 16777216 + y * 4096 + x)
	
def swapBlocks(srcmap, srcvol, dstmap, dstpos):
	try:
		srcCon = sql.connect(srcmap);
		dstCon = sql.connect(dstmap);
		srcCurs = srcCon.cursor();
		dstCurs = dstCon.cursor();
		
		selectStmt = "SELECT `data` FROM `blocks` WHERE `pos` = ?1"
		insertStmt = "REPLACE INTO `blocks`(`pos`, `data`) VALUES (?1, ?2)"
		
		srcvol = srcvol.toBlockCoord();
		tX = 0;
		tY = 0;
		tZ = 0;
		if dstpos:
			dstpos = dstpos.toBlockCoord();
			tX = dstpos.X - srcvol.X 
			tY = dstpos.Y - srcvol.Y 
			tZ = dstpos.Z - srcvol.Z 
		
		swapcount = 0
		progresscount = 0
		totalvolume = srcvol.dX * srcvol.dY * srcvol.dZ
				
		for x in range(0, srcvol.dX):
			for y in range(0, srcvol.dY):
				for z in range(0, srcvol.dZ): 
					srcblockid = blockAsInt(srcvol.X + x, srcvol.Y + y, srcvol.Z + z)
					srcCurs.execute(selectStmt, (srcblockid,))
					srcdata = srcCurs.fetchone();
					dstblockid = blockAsInt(srcvol.X + x + tX, srcvol.Y + y + tY, srcvol.Z + z + tZ)
					dstCurs.execute(selectStmt, (dstblockid,))
					dstdata = dstCurs.fetchone();
					if srcdata:
						dstCurs.execute(insertStmt, (dstblockid, srcdata[0]))
					if dstdata:
						srcCurs.execute(insertStmt, (srcblockid, dstdata[0]))
					if ( srcdata and dstdata ):
						swapcount += 1;
					progresscount += 1
					if ( progresscount % (totalvolume/10) == 0):
						print 100 * progresscount / totalvolume, "% done";
		srcCon.commit()
		dstCon.commit()
		
		print "Swapped", swapcount, "/", totalvolume, "blocks,", (100 * swapcount) / totalvolume,  "% (anything else had void space)"
	
	except sql.Error, e:
		print "Error %s:" % e.args[0]
	finally:
		if srcCon:
			srcCon.close
		if dstCon:
			dstCon.close

def copyBlocks(srcmap, srcvol, dstmap, dstpos):
	try:
		srcCon = sql.connect(srcmap);
		dstCon = sql.connect(dstmap);
		srcCurs = srcCon.cursor();
		dstCurs = dstCon.cursor();
		
		selectStmt = "SELECT `data` FROM `blocks` WHERE `pos` = ?1"
		insertStmt = "REPLACE INTO `blocks`(`pos`, `data`) VALUES (?1, ?2)"
		
		srcvol = srcvol.toBlockCoord();
		tX = 0;
		tY = 0;
		tZ = 0;
		if dstpos:
			dstpos = dstpos.toBlockCoord();
			tX = dstpos.X - srcvol.X 
			tY = dstpos.Y - srcvol.Y 
			tZ = dstpos.Z - srcvol.Z 
		
		copycount = 0
		progresscount = 0
		totalvolume = srcvol.dX * srcvol.dY * srcvol.dZ
				
		for x in range(0, srcvol.dX):
			for y in range(0, srcvol.dY):
				for z in range(0, srcvol.dZ): 
					blockid = blockAsInt(srcvol.X + x, srcvol.Y + y, srcvol.Z + z)
					srcCurs.execute(selectStmt, (blockid,))
					data = srcCurs.fetchone();
					if data:
						blockid = blockAsInt(srcvol.X + x + tX, srcvol.Y + y + tY, srcvol.Z + z + tZ)
						dstCurs.execute(insertStmt, (blockid, data[0]))
						copycount += 1
					progresscount += 1
					if ( progresscount % (totalvolume/10) == 0):
						print 100 * progresscount / totalvolume, "% done";
		dstCon.commit()
		
		print "Copied", copycount, "/", totalvolume, "blocks,", (100 * copycount) / totalvolume,  "% (anything else was void space)"
	
	except sql.Error, e:
		print "Error %s:" % e.args[0]
	finally:
		if srcCon:
			srcCon.close
		if dstCon:
			dstCon.close
	
def deleteBlocks(srcmap, srcvol):
	try:
		con = sql.connect(srcmap);
		curs = con.cursor();
		
		deleteStmt = "DELETE FROM `blocks` WHERE `pos` = ?1"
		srcvol = srcvol.toBlockCoord();
		
		progresscount = 0
		deletecount = 0
		totalvolume = srcvol.dX * srcvol.dY * srcvol.dZ

		
		for x in range(0, srcvol.dX):
			for y in range(0, srcvol.dY):
				for z in range(0, srcvol.dZ): 
					blockid = blockAsInt(srcvol.X + x, srcvol.Y + y, srcvol.Z + z)
					curs.execute(deleteStmt, (blockid,))
					if ( curs.rowcount > 0 ):
						deletecount += 1
					progresscount += 1
					if ( progresscount % (totalvolume/10) == 0):
						print 100 * progresscount / totalvolume, "% done";
		con.commit()
		
		print "Deleted", deletecount, "/", totalvolume, "blocks,", (100 * deletecount) / totalvolume, "% (anything else was void space)"
	except sql.Error, e:
		print "Error %s:" % e.args[0]
	finally:
		if con:
			con.close
			
class Coordinates:
	X = 0;
	Y = 0;
	Z = 0;
	dX = 0;
	dY = 0;
	dZ = 0;
	def toBlockCoord(self):
		coords = Coordinates();
		coords.X = int(math.floor(self.X / 16.0));
		coords.Y = int(math.floor(self.Y / 16.0));
		coords.Z = int(math.floor(self.Z / 16.0));
		coords.dX = int(math.ceil(self.dX / 16.0));
		coords.dY = int(math.ceil(self.dY / 16.0));
		coords.dZ = int(math.ceil(self.dZ / 16.0));
		return coords;		


def parseVolume(vol):
	p = re.compile("^src=(-?\\d{1,5}),(-?\\d{1,5}),(-?\\d{1,5})\\+(\\d{1,5})\\+(\\d{1,5})\\+(\\d{1,5})$")

	match = p.match(vol);

	if match == None:
		return None;
	
	o = Coordinates()
	o.X = int(match.group(1));
	o.Y = int(match.group(2));
	o.Z = int(match.group(3));
	o.dX = int(match.group(4));
	o.dY = int(match.group(5));
	o.dZ = int(match.group(6));
	
	return o;

def parseCoord(coord):
	p = re.compile("^dst=(-?\\d{1,5}),(-?\\d{1,5}),(-?\\d{1,5})$")
	match = p.match(coord);

	if match == None:
		return None;
	
	o = Coordinates()
	o.X = int(match.group(1));
	o.Y = int(match.group(2));
	o.Z = int(match.group(3));
	
	return o;


def main():
	parser = OptionParser("""Usage: %prog [-c|-s|-d] source.mt src=X,Y,Z+dX+dY+dZ [destination.mt] [dst=X',Y',Z']
	source.sqlite is the source world
	X,Y,Z are coordinates for the start of the operation volume
	+dX is the length (along X-axis) of the operation volume
	+dY is the height (along Y-axis) of the operation volume
	+dZ is the width (along Z-axis) of the operation volume
	destination.sqlite is the destination world (not needed when deleting)
	X',Y',Z' are the coordinates in the destination world (used when moving blocks)
	
	Ideally coordinates should be multiples of 16 (block boundaries).  If not:
	 X, Y, Z will round down 
	 dX, dY, dZ will round up""")
	parser.add_option("-c", "--copy", action="store_true", dest="copy",
				  help="Copy blocks from source-world to destination-world")
	parser.add_option("-s", "--swap", action="store_true", dest="swap",
				  help="Swap the specified blocks between the source world and destination world")
	parser.add_option("-d", "--delete", action="store_true", dest="delete",
				  help="Delete blocks from source world")
	(options, args) = parser.parse_args()

	print options;
	print args;

	optCount = 0;
	if options.copy == True:
		optCount += 1;
	if options.swap == True:
		optCount += 1;
	if options.delete == True:
		optCount += 1;

	if optCount > 1 or optCount < 1:
		print "Must specify one and only one option"
		return
	
	if len(args) < 2:
		print "Must at least specify source.mt X,Y,Z+dX+dY+dZ"
		return
	if len(args) > 4:
		print "Too many arguments"
		return
	srcmap = args[0];
	srcvol = parseVolume(args[1]);
	
	if srcvol == None:
		print "Invalid source volume: ", args[1];
		return;
	
	if len(args) >= 3:
		dstmap = args[2];
	dstpos = None
	if len(args) == 4:
		dstpos = parseCoord(args[3]);
		if dstpos == None:
			print "Invalid destination location: " , args[3];
			return

	if options.copy == True:
		if len(args) < 3:
			print "Must specify source.mt X,Y,Z+dX+dY+dZ destination.mt";
			return;
		copyBlocks(srcmap, srcvol, dstmap, dstpos);
	if options.swap == True:
		if len(args) < 3:
			print "Must specify source.mt X,Y,Z+dX+dY+dZ destination.mt";
			return;
		swapBlocks(srcmap, srcvol, dstmap, dstpos);	  
	if options.delete == True:
		if len(args) != 2:
			print "Must specify source.mt X,Y,Z+dX+dY+dZ";
			return;
		deleteBlocks(srcmap, srcvol);

	
if __name__ == '__main__':
	main();

