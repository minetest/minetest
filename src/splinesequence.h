/*
Minetest SplineSequence
Copyright (C) 2016-2018 Ben Deutsch <ben@bendeutsch.de>
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
#include <vector>
#include <iostream>
#include <quaternion.h>

struct SplineIndex
{
	float duration;
	unsigned int offset;
	unsigned int length;
};

template <typename T> class SplineSequence
{
private:
	std::vector<T> nodes;
	std::vector<SplineIndex> indices;
	float m_totalDuration;

public:
	SplineSequence();
	~SplineSequence();

	std::vector<T> getNodes();
	std::vector<SplineIndex> getIndices();
	SplineSequence<T> &addNode(T node);
	SplineSequence<T> &addIndex(
			float duration, unsigned int offset, unsigned int length);
	// after this, all durations are non-negative, and sum to 1.0
	// or passed number
	SplineSequence<T> &normalizeDurations(float total = 1.0f);

	inline float getTotalDuration() { return m_totalDuration; };

	void interpolate(T &result, float alpha) const;

	T _interpolate(T &bottom, T &top, float alpha) const;
};

template <typename T> SplineSequence<T>::SplineSequence() : m_totalDuration(0.0f)
{
	// noop
}

template <typename T> SplineSequence<T>::~SplineSequence()
{
	// noop
}

template <typename T> inline std::vector<T> SplineSequence<T>::getNodes()
{
	return this->nodes;
}

template <typename T>
inline std::vector<SplineIndex> SplineSequence<T>::getIndices()
{
	return this->indices;
}

template <typename T> SplineSequence<T> &SplineSequence<T>::addNode(T node)
{
	nodes.push_back(node);
	return *this;
}

template <typename T>
SplineSequence<T> &SplineSequence<T>::addIndex(
		float duration, unsigned int offset, unsigned int length)
{
	// TODO: in-place constructor for structs?
	// TODO: sanity checks for durations
	SplineIndex index;
	index.duration = duration;
	index.offset = offset;
	index.length = length;
	indices.push_back(index);
	m_totalDuration += duration;
	return *this;
}

template <typename T>
SplineSequence<T> &SplineSequence<T>::normalizeDurations(float target)
{
	float sum = 0.0;
	for (std::vector<SplineIndex>::iterator i = indices.begin(); i != indices.end();
			i++) {
		float d = i->duration;
		if (d < 0.0)
			d = 0.0; // TODO: sanity check in addIndex
		sum += d;
	}

	if (sum == 0.0) {
		// TODO: throw exception or similar
	}
	float adjust = target / sum;

	for (std::vector<SplineIndex>::iterator i = indices.begin(); i != indices.end();
			i++) {
		float d = i->duration;
		if (d < 0.0)
			d = 0.0;
		d = d * adjust;
		i->duration = d;
	}
	m_totalDuration = target;
	return *this;
}

template <typename T> void SplineSequence<T>::interpolate(T &result, float alpha) const
{

	// find the index
	std::vector<SplineIndex>::const_iterator index = indices.begin();
	while (index != indices.end() && index->duration <= alpha) {
		alpha -= index->duration;
		index++;
	}
	if (index == indices.end()) {
		// search unsuccessful?
		return;
	}
	// std::cout << "Found index " << index->offset << ", " << index->length <<
	// std::endl;
	/*
	0.3 0.3 0.4 , 0.0 -> 1st, 0.0 rem -> 0.0
	0.3 0.3 0.4 , 0.5 -> 2nd, 0.2 rem -> 0.66
	*/
	alpha = alpha / index->duration;
	// std::cout << "Remaining alpha " << alpha << std::endl;

	typename std::vector<T>::const_iterator start = nodes.begin();
	start += index->offset;
	typename std::vector<T>::const_iterator end = start;
	end += index->length;
	// std::cout << "Start: " << start->X << " " << start->Y << " " << start->Z <<
	// std::endl; std::cout << "End:   " << end->X << " " << end->Y << " " << end->Z <<
	// std::endl; std::cout << "Ends: " << *start << " " << *end << std::endl;
	// these are both inclusive, but vector's range constructor
	// excludes the end -> advance by one
	end++;

	std::vector<T> workspace(start, end);
	for (unsigned int degree = index->length; degree > 0; degree--) {
		for (unsigned int i = 0; i < degree; i++) {
			// std::cout << "Interpolating alpha " << alpha << ", degree " <<
			// degree << ", step " << i << std::endl; std::cout << "Before " <<
			// workspace[i] << " onto " << workspace[i+1] << std::endl;
			workspace[i] = _interpolate(
					workspace[i], workspace[i + 1], alpha);
			//_interpolate(workspace[i], alpha, index->length);
			// workspace[i] = (1.0-alpha) * workspace[i] + alpha *
			// workspace[i+1]; std::cout << "After " << workspace[i] << " onto
			// " << workspace[i+1] << std::endl;
		}
	}
	result = workspace[0];

	// SplineIndex index;
	// for(
	//	std::vector<SplineIndex>::const_iterator i = indices.begin();
	//	i != indices.end(); i++
	//) {
	//	if (i->duration >= alpha) {
	//		index = *i;
	//		alpha -= i->duration;
	//	}
	//}
	// if (index->duration <= 0.0) {
	//	return;
	//}
}

template <typename T>
T SplineSequence<T>::_interpolate(T &bottom, T &top, float alpha) const
{
	return (1.0 - alpha) * bottom + alpha * top;
}

// declare specializations

// quaternions have a special interpolation
template <>
core::quaternion SplineSequence<core::quaternion>::_interpolate(
		core::quaternion &bottom, core::quaternion &top, float alpha) const;