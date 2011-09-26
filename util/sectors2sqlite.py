#!/usr/bin/python3

# Loads block files from sectors folders into map.sqlite database.
# The sectors folder should be safe to remove after this prints "Finished."

import time, os, sys

try:
	import sqlite3
except:
	exit('You need to have the Python sqlite3 module.')

path = "../world/"

paths = []

# sectors2 gets to try first
if os.path.isdir(path + 'sectors2/'):
	paths.append('sectors2')
if os.path.isdir(path + 'sectors/'):
	paths.append('sectors')

if not paths:
	exit('Could not find sectors folder at ' + path + 'sectors2/ or ' + path + 'sectors/')

def parseSigned12bit(u):
	u = int('0x'+u, 16)
	return (u if u < 2**11 else u - 2**12)

def parseSigned16bit(u):
	u = int('0x'+u, 16)
	return (u if u < 2**15 else u - 2**16)

def int64(u):
	while u >= 2**63:
		u -= 2**64
	while u <= -2**63:
		u += 2**64
	return u

# Convert sector folder(s) to integer
def getSectorPos(dirname):
	if len(dirname) == 8:
		# Old layout
		x = parseSigned16bit(dirname[:4])
		z = parseSigned16bit(dirname[4:])
	elif len(dirname) == 7:
		# New layout
		x = parseSigned12bit(dirname[:3])
		z = parseSigned12bit(dirname[4:])
	else:
		print('Terrible sector at ' + dirname)
		return
	
	return x, z

# Convert block file to integer position
def getBlockPos(sectordir, blockfile):
	p2d = getSectorPos(sectordir)
	
	if not p2d:
		return
	
	if len(blockfile) != 4:
		print("Invalid block filename: " + blockfile)
	
	y = parseSigned16bit(blockfile)
	
	return p2d[0], y, p2d[1]

# Convert location to integer
def getBlockAsInteger(p):
	return int64(p[2]*16777216 + p[1]*4096 + p[0])

# Init

create = False
if not os.path.isfile(path + 'map.sqlite'):
	create = True

conn = sqlite3.connect(path + 'map.sqlite')

if not conn:
	exit('Could not open database.')

cur = conn.cursor()

if create:
	cur.execute("CREATE TABLE IF NOT EXISTS `blocks` (`pos` INT NOT NULL PRIMARY KEY, `data` BLOB);")
	conn.commit()
	print('Created database at ' + path + 'map.sqlite')

# Crawl the folders

count = 0
t = time.time()
for base in paths:
	v = 0
	if base == 'sectors':
		v = 1
	elif base == 'sectors2':
		v= 2
	else:
		print('Ignoring base ' + base)
		continue
	
	for root, dirs, files in os.walk(path + base):
		if files:
			for block in files:
				pos = getBlockAsInteger(getBlockPos(root[(-8 if v == 1 else -7 if v == 2 else 0):], block))
				
				if pos is None:
					print('Ignoring broken path ' + root + '/' + block)
					continue
				
				f = open(root+'/'+block, 'rb')
				blob = f.read()
				f.close()
				if sys.version_info.major == 2:
					blob = buffer(blob)
				else:
					blob = memoryview(blob)
				cur.execute('INSERT OR IGNORE INTO `blocks` VALUES(?, ?)', (pos, blob))
				count += 1
		
		if(time.time() - t > 3):
			t = time.time()
			print(str(count)+' blocks processed...')

conn.commit()

print('Finished. (' + str(count) + ' blocks)')
