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

// LIGHT_MAX is 14, 0-14 is 15 values
/*u8 light_decode_table[LIGHT_MAX+1] = 
{
0,
9,
12,
14,
16,
20,
26,
34,
45,
61,
81,
108,
143,
191,
255,
};*/
u8 light_decode_table[LIGHT_MAX+1] = 
{
0,
5,
12,
22,
35,
50,
65,
85,
100,
120,
140,
160,
185,
215,
255,
};

#if 0
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
#endif


