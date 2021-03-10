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
#include <vector>
#include <string>

/**
 * A global unique identifier.
 * It is global because it stays valid in a world forever.
 * It is unique because there are no collisions.
 *
 * It is a string of the following form:
 * "v1guid<deci_num>"
 * where <deci_num> should be replaced by a number of decimal digits, with the
 * first digit != 0 ("0" and "" are invalid).
 * The number after the v is the version number (currently always 1).
 *
 * There is a partial order for guids:
 * guid A is newer than guid B iff
 * A's version number is higher than B's and B's version number is 1 or
 * A and B both have version number 1 and A's deci_num is higher
 */
typedef std::string GUID;

/**
 * Generates infinitely many guids.
 */
class GUIDGenerator {
public:
	/**
	 * Creates a new uninitialized generator.
	 */
	GUIDGenerator() = default;

	~GUIDGenerator() = default;

	/**
	 * The validity of a guid.
	 * Old means that it was a valid guid in an older version, this includes
	 * the empty string.
	 * Invalid guids are strings that have an invalid form, eg. "v1guid1a23", but
	 * also guids of newer versions.
	 * Everything else is valid.
	 */
	enum Validity {
		Valid,
		Old,
		Invalid,
	};

	/**
	 * Seeds the generator with the first guid to return next.
	 * Can fail for invalid seeds.
	 * Do only seed with guids of which you know that itself and newer guids are
	 * not used yet.
	 * @param first the first guid that should be returned by generateNext
	 * @return the validity of the given id; Valid iff the generator is seeded
	 */
	Validity seed(const GUID &first);

	/**
	 * Generates the next guid, which it will never return again.
	 * @return the new guid, or "" if the generator is not yet initialized
	 */
	GUID generateNext();

	/**
	 * Returns the guid that would be generated next.
	 * @return the next guid
	 */
	GUID peekNext() const;

	/**
	 * Returns the validity for a guid.
	 * @param id the guid
	 * @return the validity of id
	 */
	static Validity getValidity(const GUID &id);

private:
	/**
	 * Internal representation of the next guid.
	 * Each u64 stores 19 decimal digits of the next deci_num.
	 * The u64s are ordered little-endian-wise.
	 */
	std::vector<u64> m_next_v;
};
