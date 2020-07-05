#include "mockgame.h"
#include "nodedef.h"

MockGameDef::MockGameDef(std::ostream &dout):
	dout(dout), m_ndef()
{
	ContentFeatures cf;
	cf.name = "X";
        m_ndef.set(cf.name, cf);
        cf.name = " ";
        cf.walkable = false;
        m_ndef.set(cf.name, cf);
}

bool CreateSector(v2s16 p2d, string definition)
{
	// Create a MapSector using the definition.
	const NodeDefManager  &ndef = *m_gamedef.ndef();
	MapSector *sector = new MapSector(this, p2d, m_gamedef);
	content_t solid, air;

	ndef.get("S", solid);
	ndef.get(" ", air);
	size_t len = definition.length();
        size_t p = 0;
        s16 y=0;
        while(p < len)
	{
		MapNode *n = sector->createBlankBlock(y++)->getData();
        	for(int i=0; i < MapBlock::MapBlock.nodecount; i++, p++, n++)
			switch(definition[p])
			{
			case 'S':
				n->setContent(solid);
				break;
			default:
				n->setContent(air);
			}
	}

	// Append it onto m_sectors
	m_sectors[p2d] = sector;
}

