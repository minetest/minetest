/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "light.h"
#include <math.h>
#include "util/numeric.h"

#ifndef SERVER

// Length of LIGHT_MAX+1 means LIGHT_MAX is the last value.
// LIGHT_SUN is read as LIGHT_MAX from here.

u8 light_LUT[LIGHT_MAX+1] =
{
	/* Middle-raised variation of a_n+1 = a_n * 0.786
	 * Length of LIGHT_MAX+1 means LIGHT_MAX is the last value.
	 * LIGHT_SUN is read as LIGHT_MAX from here.
	 */
	8,
	11+2,
	14+7,
	18+10,
	22+15,
	29+20,
	37+20,
	47+15,
	60+10,
	76+7,
	97+5,
	123+2,
	157,
	200,
	255,
};

const u8 *light_decode_table = light_LUT;

/** Initialize or update the light value tables using the specified \p gamma.
 *  If \p gamma == 1.0 then the light table is linear.  Typically values for
 *  gamma range between 1.8 and 2.2.
 *
 *  @note The value for gamma will be restricted to the range 1.1 <= gamma <= 3.0.
 *
 *  @note This function is not, currently, a simple linear to gamma encoding
 *        because adjustments are made so that a gamma of 1.8 gives the same
 *        results as those hardcoded for use by the server.
 */
void set_light_table(float gamma)
{
	static const float brightness_step = 255.0f / (LIGHT_MAX + 1);

	/* These are adjustment values that are added to the calculated light value
	 * after gamma is applied.  Currently they are used so that given a gamma
	 * of 1.8 the light values set by this function are the same as those
	 * hardcoded in the initalizer list for the declaration of light_LUT.
	 */
	static const int adjustments[LIGHT_MAX + 1] = {
		 7,
		 7,
		 7,
		 5,
		 2,
		 0,
		-7,
		-20,
		-31,
		-39,
		-43,
		-45,
		-40,
		-25,
		0
	};

	gamma = rangelim(gamma, 1.1, 3.0);

	float brightness = brightness_step;

	for (size_t i = 0; i < LIGHT_MAX; i++) {
		light_LUT[i] = (u8)(255 * powf(brightness / 255.0f,  gamma));
		light_LUT[i] = rangelim(light_LUT[i] + adjustments[i], 0, 255);
		if (i > 1 && light_LUT[i] < light_LUT[i-1])
			light_LUT[i] = light_LUT[i-1] + 1;
		brightness += brightness_step;
	}
	light_LUT[LIGHT_MAX] = 255;
}
#endif



#if 0
/*
Made using this and:
- adding 220 as the second last one
- replacing the third last one (212) with 195

#!/usr/bin/python

from math import *
from sys import stdout

# We want 0 at light=0 and 255 at light=LIGHT_MAX
LIGHT_MAX = 14
#FACTOR = 0.69
#FACTOR = 0.75
FACTOR = 0.83
START_FROM_ZERO = False

L = []
if START_FROM_ZERO:
    for i in range(1,LIGHT_MAX+1):
        L.append(int(round(255.0 * FACTOR ** (i-1))))
    L.append(0)
else:
    for i in range(1,LIGHT_MAX+1):
        L.append(int(round(255.0 * FACTOR ** (i-1))))
    L.append(255)

L.reverse()
for i in L:
    stdout.write(str(i)+",\n")
*/
u8 light_decode_table[LIGHT_MAX+1] = 
{
23,
27,
33,
40,
48,
57,
69,
83,
100,
121,
146,
176,
195,
220,
255,
};
#endif

#if 0
// This is good
// a_n+1 = a_n * 0.786
// Length of LIGHT_MAX+1 means LIGHT_MAX is the last value.
// LIGHT_SUN is read as LIGHT_MAX from here.
u8 light_decode_table[LIGHT_MAX+1] = 
{
8,
11,
14,
18,
22,
29,
37,
47,
60,
76,
97,
123,
157,
200,
255,
};
#endif

#if 0
// Use for debugging in dark
u8 light_decode_table[LIGHT_MAX+1] = 
{
58,
64,
72,
80,
88,
98,
109,
121,
135,
150,
167,
185,
206,
229,
255,
};
#endif

// This is reasonable with classic lighting with a light source
/*u8 light_decode_table[LIGHT_MAX+1] = 
{
2,
3,
4,
6,
9,
13,
18,
25,
32,
35,
45,
57,
69,
79,
255
};*/


// As in minecraft, a_n+1 = a_n * 0.8
// NOTE: This doesn't really work that well because this defines
//       LIGHT_MAX as dimmer than LIGHT_SUN
// NOTE: Uh, this has had 34 left out; forget this.
/*u8 light_decode_table[LIGHT_MAX+1] = 
{
8,
11,
14,
17,
21,
27,
42,
53,
66,
83,
104,
130,
163,
204,
255,
};*/

// This was a quick try of more light, manually quickly made
/*u8 light_decode_table[LIGHT_MAX+1] = 
{
0,
7,
11,
15,
21,
29,
42,
53,
69,
85,
109,
135,
167,
205,
255,
};*/

// This was used for a long time, manually made
/*u8 light_decode_table[LIGHT_MAX+1] = 
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
};*/

/*u8 light_decode_table[LIGHT_MAX+1] = 
{
0,
3,
6,
10,
18,
25,
35,
50,
75,
95,
120,
150,
185,
215,
255,
};*/
/*u8 light_decode_table[LIGHT_MAX+1] = 
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
};*/
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


