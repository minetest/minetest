/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#ifndef MG_SCHEMATIC_HEADER
#define MG_SCHEMATIC_HEADER

#include <map>
#include "mg_decoration.h"
#include "util/string.h"

class Map;
class Mapgen;
class MMVManip;
class PseudoRandom;
class NodeResolver;

/*
	Minetest Schematic File Format

	All values are stored in big-endian byte order.
	[u32] signature: 'MTSM'
	[u16] version: 3
	[u16] size X
	[u16] size Y
	[u16] size Z
	For each Y:
		[u8] slice probability value
	[Name-ID table] Name ID Mapping Table
		[u16] name-id count
		For each name-id mapping:
			[u16] name length
			[u8[]] name
	ZLib deflated {
	For each node in schematic:  (for z, y, x)
		[u16] content
	For each node in schematic:
		[u8] probability of occurance (param1)
	For each node in schematic:
		[u8] param2
	}

	Version changes:
	1 - Initial version
	2 - Fixed messy never/always place; 0 probability is now never, 0xFF is always
	3 - Added y-slice probabilities; this allows for variable height structures
*/

/////////////////// Schematic flags
#define SCHEM_CIDS_UPDATED 0x08

#define MTSCHEM_FILE_SIGNATURE 0x4d54534d // 'MTSM'
#define MTSCHEM_FILE_VER_HIGHEST_READ  3
#define MTSCHEM_FILE_VER_HIGHEST_WRITE 3

#define MTSCHEM_PROB_NEVER  0x00
#define MTSCHEM_PROB_ALWAYS 0xFF

enum SchematicType
{
	SCHEMATIC_NORMAL,
};

enum SchematicFormatType {
	SCHEM_FMT_HANDLE,
	SCHEM_FMT_MTS,
	SCHEM_FMT_LUA,
};

class Schematic : public ObjDef, public NodeResolver {
public:
	std::vector<content_t> c_nodes;

	u32 flags;
	v3s16 size;
	MapNode *schemdata;
	u8 *slice_probs;

	Schematic();
	virtual ~Schematic();

	virtual void resolveNodeNames(NodeResolveInfo *nri);

	void updateContentIds();

	void blitToVManip(v3s16 p, MMVManip *vm,
		Rotation rot, bool force_placement, INodeDefManager *ndef);

	bool loadSchematicFromFile(const char *filename, INodeDefManager *ndef,
		StringMap *replace_names);
	bool saveSchematicToFile(const char *filename, INodeDefManager *ndef);
	bool getSchematicFromMap(Map *map, v3s16 p1, v3s16 p2);

	bool deserializeFromMts(std::istream *is, INodeDefManager *ndef,
		std::vector<std::string> *names);
	bool serializeToMts(std::ostream *os, INodeDefManager *ndef);
	bool serializeToLua(std::ostream *os,
		INodeDefManager *ndef, bool use_comments);


	void placeStructure(Map *map, v3s16 p, u32 flags,
		Rotation rot, bool force_placement, INodeDefManager *nef);
	void applyProbabilities(v3s16 p0,
		std::vector<std::pair<v3s16, u8> > *plist,
		std::vector<std::pair<s16, u8> > *splist);

	std::string getAsLuaTable(INodeDefManager *ndef, bool use_comments);
};

class SchematicManager : public ObjDefManager {
public:
	SchematicManager(IGameDef *gamedef);
	~SchematicManager() {}

	const char *getObjectTitle() const
	{
		return "schematic";
	}

	static Schematic *create(SchematicType type)
	{
		return new Schematic;
	}
};

void build_nnlist_and_update_ids(MapNode *nodes, u32 nodecount,
	std::vector<content_t> *usednodes);


#endif
