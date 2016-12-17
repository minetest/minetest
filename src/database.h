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

#include <vector>
#include <string>
#include "irr_v3d.h"
#include "irrlichttypes.h"
#include "util/cpp11_container.h"

#ifndef PP
	#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"
#endif

class Database
{
public:
	virtual ~Database() {}

	virtual void beginSave() {}
	virtual void endSave() {}

	bool saveBlock(const v3s16 &pos, const std::string &data)
	{
		m_blocks_not_found.erase(pos);
		return saveBlockToDatabase(pos, data);
	}
	void loadBlock(const v3s16 &pos, std::string *block)
	{
		if (m_blocks_not_found.count(pos)) {
			block->clear();
		} else {
			loadBlockFromDatabase(pos, block);
			if (!block->length())
				m_blocks_not_found.insert(pos);
		}
	}
	bool deleteBlock(const v3s16 &pos)
	{
		bool deleted = deleteBlockFromDatabase(pos);
		if (deleted)
			m_blocks_not_found.insert(pos);
		return deleted;
	}
	bool blockNotFound(const v3s16 &pos) const
		{ return m_blocks_not_found.count(pos); }
	void forgetBlocksNotFound() { m_blocks_not_found.clear(); }

	static s64 getBlockAsInteger(const v3s16 &pos);
	static v3s16 getIntegerAsBlock(s64 i);

	virtual void listAllLoadableBlocks(std::vector<v3s16> &dst) = 0;

	virtual bool initialized() const { return true; }

protected:
	virtual bool saveBlockToDatabase(const v3s16 &pos, const std::string &data) = 0;
	virtual void loadBlockFromDatabase(const v3s16 &pos, std::string *block) = 0;
	virtual bool deleteBlockFromDatabase(const v3s16 &pos) = 0;

private:
	UNORDERED_SET<v3s16> m_blocks_not_found;
};

#endif

