/*
Minetest WieldAnimation
Copyright (C) 2018 Ben Deutsch <ben@bendeutsch.de>
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

#include "irrlichttypes_extrabloated.h"
#include <iostream>
#include "util/serialize.h"
#include "splinesequence.h"
#include <unordered_map>

class WieldAnimation
{
public:
	std::string name;
	v3f getTranslationAt(float time) const;
	core::quaternion getRotationAt(float time) const;
	float getDuration() const;
	// call this *after* filling the splines
	void setDuration(float duration);
	static core::quaternion quatFromAngles(float pitch, float yaw, float roll);

	static const WieldAnimation &getNamed(const std::string &name);

	SplineSequence<v3f> m_translationspline;
	SplineSequence<core::quaternion> m_rotationspline;
	float m_duration;

	static std::unordered_map<std::string, WieldAnimation> repository;
	static void fillRepository();
	WieldAnimation deSerialize(std::istream &is);
	void serialize(std::ostream &os, u16 protocol_version);
};