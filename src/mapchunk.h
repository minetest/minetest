/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MAPCHUNK_HEADER
#define MAPCHUNK_HEADER

/*
	MapChunk contains map-generation-time metadata for an area of
	some MapSectors. (something like 64x64)
*/

class MapChunk
{
public:
	MapChunk():
		m_is_volatile(true)
	{
	}

	/*
		If is_volatile is true, chunk can be modified when
		neighboring chunks are generated.

		It is set to false when all the 8 neighboring chunks have
		been generated.
	*/
	bool getIsVolatile(){ return m_is_volatile; }
	void setIsVolatile(bool is){ m_is_volatile = is; }

private:
	bool m_is_volatile;
};

#endif

