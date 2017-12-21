#include "extrusion.h"
#include <map>
#include <SMeshBuffer.h>
#include "extruder.h"

static std::map<video::ITexture *, ExtrudedMesh> cache;

static void setupMaterial(video::SMaterial &m, video::ITexture *texture)
{
	m.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	m.setFlag(video::EMF_LIGHTING, false);
	m.setFlag(video::EMF_BILINEAR_FILTER, false);
	m.setFlag(video::EMF_TRILINEAR_FILTER, false);
	m.setFlag(video::EMF_BACK_FACE_CULLING, true);
#if (IRRLICHT_VERSION_MAJOR >= 1 && IRRLICHT_VERSION_MINOR >= 8) || IRRLICHT_VERSION_MAJOR >= 2
	m.setFlag(video::EMF_USE_MIP_MAPS, false);
#endif
	auto &l = m.TextureLayer[0];
	l.Texture = texture;
	l.TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	l.TextureWrapV = video::ETC_CLAMP_TO_EDGE;
}

scene::SMesh *createExtrusionMesh(video::ITexture *texture,
		video::ITexture *overlay_texture)
{
	ExtrudedMesh &extruded = cache[texture];
	if (!extruded) {
		Extruder extruder(texture);
		extruder.extrude();
		extruded = extruder.takeMesh();
	}
	scene::SMesh *mesh = new scene::SMesh();
	scene::SMeshBuffer *buf = new scene::SMeshBuffer();
	buf->append(extruded.vertices.data(), extruded.vertices.size(),
			extruded.indices.data(), extruded.indices.size());
	setupMaterial(buf->getMaterial(), texture);
	mesh->addMeshBuffer(buf);
	buf->drop();
	if (!overlay_texture)
		return mesh;
	buf = new scene::SMeshBuffer();
	buf->append(extruded.overlay_vertices.data(), extruded.overlay_vertices.size(),
			extruded.overlay_indices.data(), extruded.overlay_indices.size());
	setupMaterial(buf->getMaterial(), overlay_texture);
	mesh->addMeshBuffer(buf);
	buf->drop();
	return mesh;
}

void purgeExtrusionMeshCache()
{
	cache.clear();
}
