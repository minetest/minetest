#include "extrusion.h"
#include <map>
#include <SMesh.h>
#include <SMeshBuffer.h>
#include "extruder.h"

static std::map<video::ITexture *, ExtrudedMesh> cache;

irr_ptr<scene::IMesh> createExtrusionMesh(video::ITexture *texture,
		video::ITexture *overlay_texture)
{
	ExtrudedMesh &extruded = cache[texture];
	if (!extruded) {
		Extruder extruder(texture);
		extruder.extrude();
		extruded = extruder.takeMesh();
	}
	irr_ptr<scene::SMesh> mesh(new scene::SMesh(), Grab::already_owned);
	irr_ptr<scene::SMeshBuffer> buf(new scene::SMeshBuffer(), Grab::already_owned);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	buf->getMaterial().setTexture(0, texture);
	buf->append(extruded.vertices.data(), extruded.vertices.size(),
			extruded.indices.data(), extruded.indices.size());
	mesh->addMeshBuffer(buf.get());
	if (!overlay_texture)
		return mesh;
	buf.reset(new scene::SMeshBuffer(), Grab::already_owned);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	buf->getMaterial().setTexture(0, overlay_texture);
	buf->append(extruded.overlay_vertices.data(), extruded.overlay_vertices.size(),
			extruded.overlay_indices.data(), extruded.overlay_indices.size());
	mesh->addMeshBuffer(buf.get());
	return mesh;
}

void purgeExtrusionMeshCache()
{
	cache.clear();
}
