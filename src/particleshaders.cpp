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

#include "particleshaders.h"
#include "util/numeric.h"
#include "util/serialize.h"

#include <sstream>

/*
	ParticleShaders
*/

std::string ParticleShaders::serialize() const
{
	std::stringstream output;

	writeU32(output, varCount);
	output << serializeString(pos) << serializeString(vel) << serializeString(acc)
		<< serializeString(exptime) << serializeString(size);
	return output.str();
}

void ParticleShaders::deSerialize(std::istream &is)
{
	varCount = readU32(is);
	pos = deSerializeString(is);
	vel = deSerializeString(is);
	acc = deSerializeString(is);
	exptime = deSerializeString(is);
	size = deSerializeString(is);
}
