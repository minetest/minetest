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

#include "smoothtranslator.h"
#include "numeric.h"
#include <algorithm>

template<typename T>
void SmoothTranslator<T>::init(T current)
{
	val_old = current;
	val_current = current;
	val_target = current;
	anim_time = 0;
	anim_time_counter = 0;
	aim_is_end = true;
}

template<typename T>
void SmoothTranslator<T>::update(T new_target, bool is_end_position, float update_interval)
{
	aim_is_end = is_end_position;
	val_old = val_current;
	val_target = new_target;
	if (update_interval > 0) {
		anim_time = update_interval;
	} else {
		if (anim_time < 0.001 || anim_time > 1.0)
			anim_time = anim_time_counter;
		else
			anim_time = anim_time * 0.9 + anim_time_counter * 0.1;
	}
	anim_time_counter = 0;
}

template<typename T>
void SmoothTranslator<T>::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;
	T val_diff = val_target - val_old;
	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
	val_current = val_old + val_diff * moveratio;
}

void SmoothTranslatorWrapped::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;
	f32 val_diff = std::abs(val_target - val_old);
	if (val_diff > 180.f)
		val_diff = 360.f - val_diff;

	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
	wrappedApproachShortest(val_current, val_target,
		val_diff * moveratio, 360.f);
}

void SmoothTranslatorWrappedv3f::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;

	v3f val_diff_v3f;
	val_diff_v3f.X = std::abs(val_target.X - val_old.X);
	val_diff_v3f.Y = std::abs(val_target.Y - val_old.Y);
	val_diff_v3f.Z = std::abs(val_target.Z - val_old.Z);

	if (val_diff_v3f.X > 180.f)
		val_diff_v3f.X = 360.f - val_diff_v3f.X;

	if (val_diff_v3f.Y > 180.f)
		val_diff_v3f.Y = 360.f - val_diff_v3f.Y;

	if (val_diff_v3f.Z > 180.f)
		val_diff_v3f.Z = 360.f - val_diff_v3f.Z;

	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
	wrappedApproachShortest(val_current.X, val_target.X,
		val_diff_v3f.X * moveratio, 360.f);

	wrappedApproachShortest(val_current.Y, val_target.Y,
		val_diff_v3f.Y * moveratio, 360.f);

	wrappedApproachShortest(val_current.Z, val_target.Z,
		val_diff_v3f.Z * moveratio, 360.f);
}

