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

/////////////////// Schematic flags
#define SCHEM_CIDS_UPDATED 0x08


#define MTSCHEM_FILE_SIGNATURE 0x4d54534d // 'MTSM'
#define MTSCHEM_FILE_VER_HIGHEST_READ  3
#define MTSCHEM_FILE_VER_HIGHEST_WRITE 3

#define MTSCHEM_PROB_NEVER  0x00
#define MTSCHEM_PROB_ALWAYS 0xFF


class Schematic : public GenElement, public NodeResolver {
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
		std::map<std::string, std::string> &replace_names);
	void saveSchematicToFile(const char *filename, INodeDefManager *ndef);
	bool getSchematicFromMap(Map *map, v3s16 p1, v3s16 p2);

	void placeStructure(Map *map, v3s16 p, u32 flags,
		Rotation rot, bool force_placement, INodeDefManager *nef);
	void applyProbabilities(v3s16 p0,
		std::vector<std::pair<v3s16, u8> > *plist,
		std::vector<std::pair<s16, u8> > *splist);
};

class SchematicManager : public GenElementManager {
public:
	static const char *ELEMENT_TITLE;
	static const size_t ELEMENT_LIMIT = 0x10000;

	SchematicManager(IGameDef *gamedef);
	~SchematicManager() {}

	Schematic *create(int type)
	{
		return new Schematic;
	}
};

void build_nnlist_and_update_ids(MapNode *nodes, u32 nodecount,
	std::vector<content_t> *usednodes);


#endif
