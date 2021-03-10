/*
Minetest
Copyright (C) 2021, DS

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

#include "irrlichttypes.h"
#include "util/basic_macros.h"
#include <random>
#include <string>

class ServerEnvironment;

/**
 * A global unique identifier.
 * It is global because it stays valid in a world forever.
 * It is unique because there are no collisions.
 */
typedef std::string GUId;

/**
 * Generates infinitely many guids.
 */
class GUIdGenerator {
public:
	/**
	 * Creates a new uninitialized generator.
	 * @param env ServerEnvironment where generator is running
	 * @param prefix Prefix of generated GUId
	 * @param next next value getted from loadMeta
	 */
	GUIdGenerator();

	~GUIdGenerator() = default;
	DISABLE_CLASS_COPY(GUIdGenerator)

	/**
	 * Generates the next guid, which it will never return again.
	 * @return the new guid, or "" if the generator is not yet initialized
	 */
	GUId next(const std::string &prefix);

private:
	void setServerEnvironment(ServerEnvironment *env) { m_env = env; }
  
	ServerEnvironment *m_env;
	std::random_device m_rand;
	std::uniform_int_distribution<u64> m_uniform;
	bool m_rand_usable;
	u32 m_next;
  
	friend class ServerEnvironment;
};
