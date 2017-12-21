#include "extrusion.h"
#include <map>
#include <SMesh.h>
#include <SMeshBuffer.h>
#include "extruder.h"

static std::map<video::ITexture *, irr_ptr<scene::SMeshBuffer>> cache;

irr_ptr<scene::IMesh> createExtrusionMesh(video::ITexture *texture,
		video::ITexture *overlay_texture = nullptr)
{
	irr_ptr<scene::SMeshBuffer> &buf = cache[texture];
	if (!buf) {
		Extruder extruder(texture);
		extruder.extrude();
		buf.reset(extruder.createMesh(), Grab::already_owned);
	}
	irr_ptr<scene::SMeshBuffer> buf2(new scene::SMeshBuffer(), Grab::already_owned);
	buf2->append(buf->getVertices(), buf->getVertexCount(),
			buf->getIndices(), buf->getIndexCount());
	irr_ptr<scene::SMesh> mesh(new scene::SMesh(), Grab::already_owned);
	mesh->addMeshBuffer(buf2.get());
	return mesh;
}

void purgeExtrusionMeshCache()
{
	cache.clear();
}
