#include "extrusion_mesh_cache.h"
#include "mesh.h"

#define MIN_EXTRUSION_MESH_RESOLUTION 16
#define MAX_EXTRUSION_MESH_RESOLUTION 512

scene::SMesh *ExtrusionMeshCache::create(int resolution_x, int resolution_y)
{
	const f32 r = 0.5;

	scene::SMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	v3f scale(1.0, 1.0, 0.1);

	// Front and back
	{
		video::S3DVertex vertices[8] = {
			// z-
			video::S3DVertex(-r,+r,-r, 0,0,-1, c, 0,0),
			video::S3DVertex(+r,+r,-r, 0,0,-1, c, 1,0),
			video::S3DVertex(+r,-r,-r, 0,0,-1, c, 1,1),
			video::S3DVertex(-r,-r,-r, 0,0,-1, c, 0,1),
			// z+
			video::S3DVertex(-r,+r,+r, 0,0,+1, c, 0,0),
			video::S3DVertex(-r,-r,+r, 0,0,+1, c, 0,1),
			video::S3DVertex(+r,-r,+r, 0,0,+1, c, 1,1),
			video::S3DVertex(+r,+r,+r, 0,0,+1, c, 1,0),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}

	f32 pixelsize_x = 1 / (f32) resolution_x;
	f32 pixelsize_y = 1 / (f32) resolution_y;

	for (int i = 0; i < resolution_x; ++i) {
		f32 pixelpos_x = i * pixelsize_x - 0.5;
		f32 x0 = pixelpos_x;
		f32 x1 = pixelpos_x + pixelsize_x;
		f32 tex0 = (i + 0.1) * pixelsize_x;
		f32 tex1 = (i + 0.9) * pixelsize_x;
		video::S3DVertex vertices[8] = {
			// x-
			video::S3DVertex(x0,-r,-r, -1,0,0, c, tex0,1),
			video::S3DVertex(x0,-r,+r, -1,0,0, c, tex1,1),
			video::S3DVertex(x0,+r,+r, -1,0,0, c, tex1,0),
			video::S3DVertex(x0,+r,-r, -1,0,0, c, tex0,0),
			// x+
			video::S3DVertex(x1,-r,-r, +1,0,0, c, tex0,1),
			video::S3DVertex(x1,+r,-r, +1,0,0, c, tex0,0),
			video::S3DVertex(x1,+r,+r, +1,0,0, c, tex1,0),
			video::S3DVertex(x1,-r,+r, +1,0,0, c, tex1,1),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}
	for (int i = 0; i < resolution_y; ++i) {
		f32 pixelpos_y = i * pixelsize_y - 0.5;
		f32 y0 = -pixelpos_y - pixelsize_y;
		f32 y1 = -pixelpos_y;
		f32 tex0 = (i + 0.1) * pixelsize_y;
		f32 tex1 = (i + 0.9) * pixelsize_y;
		video::S3DVertex vertices[8] = {
			// y-
			video::S3DVertex(-r,y0,-r, 0,-1,0, c, 0,tex0),
			video::S3DVertex(+r,y0,-r, 0,-1,0, c, 1,tex0),
			video::S3DVertex(+r,y0,+r, 0,-1,0, c, 1,tex1),
			video::S3DVertex(-r,y0,+r, 0,-1,0, c, 0,tex1),
			// y+
			video::S3DVertex(-r,y1,-r, 0,+1,0, c, 0,tex0),
			video::S3DVertex(-r,y1,+r, 0,+1,0, c, 0,tex1),
			video::S3DVertex(+r,y1,+r, 0,+1,0, c, 1,tex1),
			video::S3DVertex(+r,y1,-r, 0,+1,0, c, 1,tex0),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}

	// Create mesh object
	scene::SMesh *mesh = new scene::SMesh();
	mesh->addMeshBuffer(buf);
	buf->drop();
	scaleMesh(mesh, scale); // also recalculates bounding box
	return mesh;
}

ExtrusionMeshCache::ExtrusionMeshCache()
{
	for (int resolution = MIN_EXTRUSION_MESH_RESOLUTION;
			resolution <= MAX_EXTRUSION_MESH_RESOLUTION;
			resolution *= 2) {
		m_extrusion_meshes[resolution] = create(resolution, resolution);
	}
}

ExtrusionMeshCache::~ExtrusionMeshCache()
{
	for (auto &extrusion_meshe : m_extrusion_meshes) {
		extrusion_meshe.second->drop();
	}
}

scene::SMesh *ExtrusionMeshCache::createExtrusionMesh(
		video::ITexture *texture, video::ITexture *overlay_texture)
{
	scene::SMesh *mesh = getOrCreate(texture->getSize());
	scene::IMeshBuffer *buf = mesh->getMeshBuffer(0);
	buf->getMaterial().setTexture(0, texture);
	if (overlay_texture) {
		scene::IMeshBuffer *copy = cloneMeshBuffer(buf);
		copy->getMaterial().setTexture(0, overlay_texture);
		mesh->addMeshBuffer(copy);
		copy->drop();
	}
	return mesh;
}

scene::SMesh *ExtrusionMeshCache::createFlatMesh(
		video::ITexture *texture, video::ITexture *overlay_texture)
{
	static const video::S3DVertex vertices[4] = {
			video::S3DVertex(-0.5, -0.5, 0.0, 0, 0, -1,
					video::SColor(255, 255, 255, 255), 0, 1),
			video::S3DVertex(-0.5, 0.5, 0.0, 0, 0, -1,
					video::SColor(255, 255, 255, 255), 0, 0),
			video::S3DVertex(0.5, 0.5, 0.0, 0, 0, -1,
					video::SColor(255, 255, 255, 255), 1, 0),
			video::S3DVertex(0.5, -0.5, 0.0, 0, 0, -1,
					video::SColor(255, 255, 255, 255), 1, 1),
	};
	static const u16 indices[6] = {0, 1, 2, 2, 3, 0};
	scene::SMesh *mesh = new scene::SMesh();
	scene::SMeshBuffer *buf = new scene::SMeshBuffer();
	buf->append(vertices, 4, indices, 6);
	buf->getMaterial().setTexture(0, texture);
	mesh->addMeshBuffer(buf);
	buf->drop();
	if (overlay_texture) {
		buf = new scene::SMeshBuffer();
		buf->append(vertices, 4, indices, 6);
		buf->getMaterial().setTexture(0, overlay_texture);
		mesh->addMeshBuffer(buf);
		buf->drop();
	}
	return mesh;
}

irr::scene::SMesh *ExtrusionMeshCache::getOrCreate(irr::core::dimension2d<irr::u32> dim)
{
	// handle non-power of two textures inefficiently without cache
	if (!is_power_of_two(dim.Width) || !is_power_of_two(dim.Height))
		return create(dim.Width, dim.Height);
	auto it = m_extrusion_meshes.lower_bound(std::max(dim.Width, dim.Height));
	if (it == m_extrusion_meshes.end())
		it--;
	return cloneMesh(it->second);
}
