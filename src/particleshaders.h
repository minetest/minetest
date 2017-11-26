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

#pragma once

#include <iostream>
#include "irrlichttypes_extrabloated.h"

/* Operations specification
If args are not specified, then the operation is argless.

push_constant:
- Args: F1000 value
- Description: Pushes the given value onto the stack.

push_variable
- Args: U32 variable
- Description: Pushes the specified variable's value onto the stack.

write_variable:
- Args: U32 variable
- Description: Copies the value at the top of the stack to the specified
  variable, without popping.

make_vector:
- Description: Pops three values, and pushes a vector with those values. The
  values correspond to z, y, x in order of popping.

index_x, index_y, index_z:
- Description: Pops a vector and pushes the scalar in the specified index.

push_time:
- Description: Pushes the time passed since the particle spawner was spawned.

add, subtract, multiply, divide, power, random, lt, le, atan2
- Description: Pops two values and performs the specified operation between them.
  Random takes a low and high bound.

acos, asin, atan, ceil, cos, floor, log, sin, tan
- Description: Pops one value and performs the specified operation on it.
 */
struct ParticleShaders
{
	// Also update builtin/common/expression.lua
	enum Opcode : u8
	{
		op_push_constant = 0,
		op_push_variable = 1,
		op_write_variable = 2,
		op_make_vector = 3,
		op_index_x = 4,
		op_index_y = 5,
		op_index_z = 6,
		op_push_time = 7,
		op_add = 8,
		op_subtract = 9,
		op_multiply = 10,
		op_divide = 11,
		op_power = 12,
		op_random = 13,
		op_lt = 14,
		op_le = 15,
		op_atan2 = 16,
		op_acos = 17,
		op_asin = 18,
		op_atan = 19,
		op_ceil = 20,
		op_cos = 21,
		op_floor = 22,
		op_log = 23,
		op_sin = 24,
		op_tan = 25
	};

	std::string serialize() const;
	void deSerialize(std::istream &is);

	u32 varCount;
	std::string pos;
	std::string vel;
	std::string acc;
	std::string exptime;
	std::string size;
};
