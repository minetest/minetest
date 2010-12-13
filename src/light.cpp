/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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
*/

#include "light.h"

/*

#!/usr/bin/python

from math import *
from sys import stdout

# We want 0 at light=0 and 255 at light=LIGHT_MAX
LIGHT_MAX = 15

L = []
for i in range(1,LIGHT_MAX+1):
    L.append(int(round(255.0 * 0.69 ** (i-1))))
	L.append(0)

L.reverse()
for i in L:
	stdout.write(str(i)+",\n")

*/

/*
	The first value should be 0, the last value should be 255.
*/
/*u8 light_decode_table[LIGHT_MAX+1] = 
{
0,
2,
3,
4,
6,
9,
13,
19,
28,
40,
58,
84,
121,
176,
255,
};*/

/*
#!/usr/bin/python

from math import *
from sys import stdout

# We want 0 at light=0 and 255 at light=LIGHT_MAX
LIGHT_MAX = 14
#FACTOR = 0.69
FACTOR = 0.75

L = []
for i in range(1,LIGHT_MAX+1):
    L.append(int(round(255.0 * FACTOR ** (i-1))))
L.append(0)

L.reverse()
for i in L:
    stdout.write(str(i)+",\n")
*/
u8 light_decode_table[LIGHT_MAX+1] = 
{
0,
6,
8,
11,
14,
19,
26,
34,
45,
61,
81,
108,
143,
191,
255,
};

/*
#!/usr/bin/python

from math import *
from sys import stdout

# We want 0 at light=0 and 255 at light=LIGHT_MAX
LIGHT_MAX = 14
#FACTOR = 0.69
FACTOR = 0.75

maxlight = 255
minlight = 8

L = []
for i in range(1,LIGHT_MAX+1):
    L.append(minlight+int(round((maxlight-minlight) * FACTOR ** (i-1))))
    #L.append(int(round(255.0 * FACTOR ** (i-1))))
L.append(minlight)

L.reverse()
for i in L:
    stdout.write(str(i)+",\n")
*/
/*u8 light_decode_table[LIGHT_MAX+1] = 
{
8,
14,
16,
18,
22,
27,
33,
41,
52,
67,
86,
112,
147,
193,
255,
};*/


