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
class ManualMapVoxelManipulator;
class PseudoRandom;
class NodeResolver;

/////////////////// Decoration flags
#define DECO_PLACE_CENTER_X     1
#define DECO_PLACE_CENTER_Y     2
#define DECO_PLACE_CENTER_Z     4
#define DECO_SCHEM_CIDS_UPDATED 8


#define MTSCHEM_FILE_SIGNATURE 0x4d54534d // 'MTSM'
#define MTSCHEM_FILE_VER_HIGHEST_READ  3
#define MTSCHEM_FILE_VER_HIGHEST_WRITE 3

#define MTSCHEM_PROB_NEVER  0x00
#define MTSCHEM_PROB_ALWAYS 0xFF

extern FlagDesc flagdesc_deco_schematic[];

class DecoSchematic : public Decoration {
public:
	std::string filename;

	std::vector<content_t> c_nodes;

	u32 flags;
	Rotation rotation;
	v3s16 size;
	MapNode *schematic;
	u8 *slice_probs;

	DecoSchematic();
	~DecoSchematic();

	void updateContentIds();
	virtual void generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p);
	virtual int getHeight();
	virtual std::string getName();

	void blitToVManip(v3s16 p, ManualMapVoxelManipulator *vm,
					Rotation rot, bool force_placement);

	bool loadSchematicFile(NodeResolver *resolver,
		std::map<std::string, std::string> &replace_names);
	void saveSchematicFile(INodeDefManager *ndef);

	bool getSchematicFromMap(Map *map, v3s16 p1, v3s16 p2);
	void placeStructure(Map *map, v3s16 p, bool force_placement);
	void applyProbabilities(v3s16 p0,
		std::vector<std::pair<v3s16, u8> > *plist,
		std::vector<std::pair<s16, u8> > *splist);
};

void build_nnlist_and_update_ids(MapNode *nodes, u32 nodecount,
					std::vector<content_t> *usednodes);


#endif
