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


