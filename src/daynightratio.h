/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

inline u32 time_to_daynight_ratio(float time_of_day, bool smooth)
{
	float t = time_of_day;
	if (t < 0.0f)
		t += ((int)(-t) / 24000) * 24000.0f;
	if (t >= 24000.0f)
		t -= ((int)(t) / 24000) * 24000.0f;
	if (t > 12000.0f)
		t = 24000.0f - t;

	const float values[9][2] = {
		{4250.0f + 125.0f, 175.0f},
		{4500.0f + 125.0f, 175.0f},
		{4750.0f + 125.0f, 250.0f},
		{5000.0f + 125.0f, 350.0f},
		{5250.0f + 125.0f, 500.0f},
		{5500.0f + 125.0f, 675.0f},
		{5750.0f + 125.0f, 875.0f},
		{6000.0f + 125.0f, 1000.0f},
		{6250.0f + 125.0f, 1000.0f},
	};

	if (!smooth) {
		float lastt = values[0][0];
		for (u32 i = 1; i < 9; i++) {
			float t0 = values[i][0];
			float switch_t = (t0 + lastt) / 2.0f;
			lastt = t0;
			if (switch_t <= t)
				continue;

			return values[i][1];
		}
		return 1000;
	}

	if (t <= 4625.0f) // 4500 + 125
		return values[0][1];
	else if (t >= 6125.0f) // 6000 + 125
		return 1000;

	for (u32 i = 0; i < 9; i++) {
		if (values[i][0] <= t)
			continue;

		float td0 = values[i][0] - values[i - 1][0];
		float f = (t - values[i - 1][0]) / td0;
		return f * values[i][1] + (1.0f - f) * values[i - 1][1];
	}
	return 1000;
}
