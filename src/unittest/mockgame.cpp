#include "mapblock.h"
#include "mapsector.h"
#include "mockgame.h"
#include "nodedef.h"

MapSector * MockMap::emergeSector(v2s16 p)
{
	MapSector *sector = getSectorNoGenerateNoLock(p);

	if (sector) return sector;

	sector = new MapSector(this, p, m_gamedef);
	m_sectors[p] = sector;
	return sector;
}

MapBlock * MockMap::emergeBlock(v3s16 p, bool create_blank)
{
	MapBlock *block = getBlockNoCreateNoEx(p);

	if(block != nullptr)
		return block;

	// Get the MapSector
	MapSector *sector = emergeSector(v2s16(p.X, p.Z));

	// Create the block and fill with air
	block = sector->createBlankBlock(p.Y);
	block->reallocate();
	MapNode *n = block->getData();
        for(u32 i=0; i < block->nodecount; i++, n++)
		n->setContent(CONTENT_AIR);
	return block;
}

void MockMap::setMockNode(v3s16 p, content_t c)
{
	emergeBlock(getNodeBlockPos(p));
	MapNode n(c);
	setNode(p, n);
}
