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

#ifndef DATABASE_HEADER
#define DATABASE_HEADER

#include <list>
#include "irr_v3d.h"
#include "irrlichttypes.h"

class MapBlock;

class Database
{
public:
	virtual void beginSave()=0;
	virtual void endSave()=0;

	virtual void saveBlock(MapBlock *block)=0;
	virtual MapBlock* loadBlock(v3s16 blockpos)=0;
	s64 getBlockAsInteger(const v3s16 pos);
	v3s16 getIntegerAsBlock(s64 i);
	virtual void listAllLoadableBlocks(std::list<v3s16> &dst)=0;
	virtual int Initialized(void)=0;
	virtual ~Database() {};
};
#endif
