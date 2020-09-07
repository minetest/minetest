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


#include "wieldanimation.h"

v3f WieldAnimation::getTranslationAt(float time) const
{
	v3f translation;
	m_translationspline.interpolate(translation, time);
	return translation;
}

core::quaternion WieldAnimation::getRotationAt(float time) const
{
	core::quaternion rotation;
	m_rotationspline.interpolate(rotation, time);
	return rotation;
}

float WieldAnimation::getDuration() const
{
	return m_duration;
}

void WieldAnimation::setDuration(float duration)
{
	m_duration = duration;
	m_translationspline.normalizeDurations(duration);
	m_rotationspline.normalizeDurations(duration);
}

core::quaternion WieldAnimation::quatFromAngles(float pitch, float yaw, float roll)
{
	// the order of angles is important:
	core::quaternion res;
	res *= core::quaternion(pitch * core::DEGTORAD, 0, 0);
	res *= core::quaternion(0, yaw * core::DEGTORAD, 0);
	res *= core::quaternion(0, 0, roll * core::DEGTORAD);
	return res;
}

const WieldAnimation &WieldAnimation::getNamed(const std::string &name)
{
	if (repository.size() == 0)
		fillRepository();

	if (repository.find(name) == repository.end())
		return repository["punch"];

	return repository[name];
}

std::unordered_map<std::string, WieldAnimation> WieldAnimation::repository;

void WieldAnimation::fillRepository()
{
	// default: "punch"
	WieldAnimation &punch = repository["punch"];
	punch.m_translationspline.addNode(v3f(0, 0, 0))
			.addNode(v3f(-70, 50, 0))
			.addNode(v3f(-70, -50, 0))
			.addNode(v3f(0, 0, 0));
	punch.m_translationspline.addIndex(1.0, 0, 3);

	punch.m_rotationspline.addNode(quatFromAngles(0.0f, 0.0f, 0.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 90.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 0.0f));
	punch.m_rotationspline.addIndex(1.0, 0, 2);
	punch.setDuration(0.3f);

	WieldAnimation &dig = repository["dig"];
	dig.m_translationspline.addNode(v3f(0, 0, 0))
			.addNode(v3f(-70, -50, 0))
			.addNode(v3f(-70, 50, 0))
			.addNode(v3f(0, 0, 0));
	dig.m_translationspline.addIndex(1.0, 0, 3);

	dig.m_rotationspline.addNode(quatFromAngles(0.0f, 0.0f, 0.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 135.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 135.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 0.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, -80.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 0.0f));
	dig.m_rotationspline.addIndex(1.0, 0, 2).addIndex(1.0, 2, 3);
	dig.setDuration(0.3f);

	// eat (without chewing)
	WieldAnimation &eat = repository["eat"];
	eat.m_translationspline.addNode(v3f(0, 0, 0))
			.addNode(v3f(-35, 20, 0))
			.addNode(v3f(-55, 10, 0))
			.addNode(v3f(-55, 10, 0))
			.addNode(v3f(-55, 15, 0))
			.addNode(v3f(-55, 10, 0))
			.addNode(v3f(-55, 15, 0))
			.addNode(v3f(-55, 10, 0))
			.addNode(v3f(-30, 0, 0))
			.addNode(v3f(0, 0, 0))
			.addNode(v3f(0, 0, 0));
	eat.m_translationspline.addIndex(1.0, 0, 3)
			.addIndex(0.5, 3, 1)
			.addIndex(0.5, 4, 1)
			.addIndex(0.5, 5, 1)
			.addIndex(0.5, 6, 1)
			.addIndex(1.0, 7, 3);

	eat.m_rotationspline.addNode(quatFromAngles(0.0f, 0.0f, 0.0f))
			.addNode(quatFromAngles(-90.0f, 20.0f, -80.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 0.0f));
	eat.m_rotationspline.addIndex(1.0, 0, 1).addIndex(2.0, 1, 0).addIndex(1.0, 1, 1);
	eat.setDuration(1.0f);

	// "poke"
	WieldAnimation &poke = repository["poke"];
	poke.m_translationspline.addNode(v3f(0, 0, 0))
			.addNode(v3f(0, -10, 0))
			.addNode(v3f(-50, 20, 50))
			.addNode(v3f(0, -10, 0))
			.addNode(v3f(0, 0, 0));
	poke.m_translationspline.addIndex(1.0, 0, 2).addIndex(1.0, 2, 2);

	poke.m_rotationspline.addNode(quatFromAngles(0.0f, 0.0f, 0.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 90.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 90.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 90.0f))
			.addNode(quatFromAngles(0.0f, 0.0f, 0.0f));
	poke.m_rotationspline.addIndex(1.0, 0, 2).addIndex(1.0, 2, 2);
	poke.setDuration(0.5f);

}

void WieldAnimation::serialize(std::ostream &os, u16 protocol_version)
{
	os << serializeString(name);
	writeF32(os, m_duration);
	std::vector<v3f> nodes = m_translationspline.getNodes();
	writeU16(os, nodes.size());
	for (const auto &node : nodes) {
		writeV3F32(os, node);
	}
	std::vector<SplineIndex> indices = m_translationspline.getIndices();
	writeU16(os, indices.size());
	for (const auto &index : indices) {
		writeF32(os, index.duration);
		writeU16(os, index.offset);
		writeU16(os, index.length);
	}
	std::vector<core::quaternion> rnodes = m_rotationspline.getNodes();
	writeU16(os, rnodes.size());
	for (const auto &node : rnodes) {
		v3f out = v3f(node.X, node.Y, node.Z);
		writeV3F32(os, out);
	}
	std::vector<SplineIndex> rindices = m_rotationspline.getIndices();
	writeU16(os, rindices.size());
	for (const auto &index : rindices) {
		writeF32(os, index.duration);
		writeU16(os, index.offset);
		writeU16(os, index.length);
	}
}

WieldAnimation WieldAnimation::deSerialize(std::istream &is)
{
	std::string name = deSerializeString(is);
	WieldAnimation &anim = repository[name];
	anim.name = name;
	m_duration = readF32(is);
	u32 translationNodes_size = readU32(is);
	for (u32 i = 0; i < translationNodes_size; i++) {
		v3f node = readV3F32(is);
		anim.m_translationspline.addNode(node);
	}

	u32 translationIndex_size = readU32(is);
	for (u32 i = 0; i < translationIndex_size; i++) {
		float duration = readF32(is);
		u32 offset = readU16(is);
		u32 length = readU16(is);
		anim.m_translationspline.addIndex(duration, offset, length);
	}

	u32 rotationNodes_size = readU32(is);
	for (u32 i = 0; i < rotationNodes_size; i++) {
		v3f node = readV3F32(is);
		anim.m_rotationspline.addNode(quatFromAngles(node.X,node.Y, node.Z));
	}
	u32 rotationIndex_size = readU32(is);
	for (u32 i = 0; i < rotationIndex_size; i++) {
		float duration = readF32(is);
		u32 offset = readU16(is);
		u32 length = readU16(is);
		anim.m_rotationspline.addIndex(duration, offset, length);
	}
	anim.setDuration(m_duration);
	return anim;
}
