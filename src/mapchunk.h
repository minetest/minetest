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
	TODO: Remove
*/

#if 0
/*
	MapChunk contains map-generation-time metadata for an area of
	some MapSectors. (something like 16x16)
*/

// These should fit in 8 bits, as they are saved as such.
enum{
	GENERATED_FULLY = 0,
	GENERATED_PARTLY = 10,
	GENERATED_NOT = 20
};

class MapChunk
{
public:
	MapChunk():
		m_generation_level(GENERATED_NOT),
		m_modified(true)
	{
	}

	/*
		Generation level. Possible values:
		GENERATED_FULLY = 0 = fully generated
		GENERATED_PARTLY = partly generated
		GENERATED_NOT = not generated
	*/
	u16 getGenLevel(){ return m_generation_level; }
	void setGenLevel(u16 lev){ m_generation_level = lev; }

	void serialize(std::ostream &os, u8 version)
	{
		os.write((char*)&m_generation_level, 1);
	}
	void deSerialize(std::istream &is, u8 version)
	{
		is.read((char*)&m_generation_level, 1);
	}

	bool isModified(){ return m_modified; }
	void setModified(bool modified){ m_modified = modified; }

private:
	u8 m_generation_level;
	bool m_modified;
};
#endif

#endif

