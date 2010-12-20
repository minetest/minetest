/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "mapblock.h"
#include "map.h"
// For g_materials
#include "main.h"
#include "light.h"
#include <sstream>


/*
	MapBlock
*/

MapBlock::MapBlock(NodeContainer *parent, v3s16 pos, bool dummy):
		m_parent(parent),
		m_pos(pos),
		changed(true),
		is_underground(false),
		m_day_night_differs(false),
		m_objects(this)
{
	data = NULL;
	if(dummy == false)
		reallocate();

#ifndef SERVER
	m_mesh_expired = false;
	mesh_mutex.Init();
	mesh = NULL;
#endif
}

MapBlock::~MapBlock()
{
#ifndef SERVER
	{
		JMutexAutoLock lock(mesh_mutex);
		
		if(mesh)
		{
			mesh->drop();
			mesh = NULL;
		}
	}
#endif

	if(data)
		delete[] data;
}

bool MapBlock::isValidPositionParent(v3s16 p)
{
	if(isValidPosition(p))
	{
		return true;
	}
	else{
		return m_parent->isValidPosition(getPosRelative() + p);
	}
}

MapNode MapBlock::getNodeParent(v3s16 p)
{
	if(isValidPosition(p) == false)
	{
		return m_parent->getNode(getPosRelative() + p);
	}
	else
	{
		if(data == NULL)
			throw InvalidPositionException();
		return data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X];
	}
}

void MapBlock::setNodeParent(v3s16 p, MapNode & n)
{
	if(isValidPosition(p) == false)
	{
		m_parent->setNode(getPosRelative() + p, n);
	}
	else
	{
		if(data == NULL)
			throw InvalidPositionException();
		data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X] = n;
	}
}

MapNode MapBlock::getNodeParentNoEx(v3s16 p)
{
	if(isValidPosition(p) == false)
	{
		try{
			return m_parent->getNode(getPosRelative() + p);
		}
		catch(InvalidPositionException &e)
		{
			return MapNode(CONTENT_IGNORE);
		}
	}
	else
	{
		if(data == NULL)
		{
			return MapNode(CONTENT_IGNORE);
		}
		return data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X];
	}
}

/*
	Parameters must consist of air and !air.
	Order doesn't matter.

	If either of the nodes doesn't exist, light is 0.
	
	parameters:
		daynight_ratio: 0...1000
		n: getNodeParent(p)
		n2: getNodeParent(p + face_dir)
		face_dir: axis oriented unit vector from p to p2
*/
u8 MapBlock::getFaceLight(u32 daynight_ratio, MapNode n, MapNode n2,
		v3s16 face_dir)
{
	try{
		u8 light;
		u8 l1 = n.getLightBlend(daynight_ratio);
		u8 l2 = n2.getLightBlend(daynight_ratio);
		if(l1 > l2)
			light = l1;
		else
			light = l2;

		// Make some nice difference to different sides

		/*if(face_dir.X == 1 || face_dir.Z == 1 || face_dir.Y == -1)
			light = diminish_light(diminish_light(light));
		else if(face_dir.X == -1 || face_dir.Z == -1)
			light = diminish_light(light);*/

		if(face_dir.X == 1 || face_dir.X == -1 || face_dir.Y == -1)
			light = diminish_light(diminish_light(light));
		else if(face_dir.Z == 1 || face_dir.Z == -1)
			light = diminish_light(light);

		return light;
	}
	catch(InvalidPositionException &e)
	{
		return 0;
	}
}

#ifndef SERVER

void MapBlock::makeFastFace(TileSpec tile, u8 light, v3f p,
		v3s16 dir, v3f scale, v3f posRelative_f,
		core::array<FastFace> &dest)
{
	FastFace face;
	
	// Position is at the center of the cube.
	v3f pos = p * BS;
	posRelative_f *= BS;

	v3f vertex_pos[4];
	// If looking towards z+, this is the face that is behind
	// the center point, facing towards z+.
	vertex_pos[0] = v3f( BS/2,-BS/2,BS/2);
	vertex_pos[1] = v3f(-BS/2,-BS/2,BS/2);
	vertex_pos[2] = v3f(-BS/2, BS/2,BS/2);
	vertex_pos[3] = v3f( BS/2, BS/2,BS/2);
	
	if(dir == v3s16(0,0,1))
	{
		for(u16 i=0; i<4; i++)
			vertex_pos[i].rotateXZBy(0);
	}
	else if(dir == v3s16(0,0,-1))
	{
		for(u16 i=0; i<4; i++)
			vertex_pos[i].rotateXZBy(180);
	}
	else if(dir == v3s16(1,0,0))
	{
		for(u16 i=0; i<4; i++)
			vertex_pos[i].rotateXZBy(-90);
	}
	else if(dir == v3s16(-1,0,0))
	{
		for(u16 i=0; i<4; i++)
			vertex_pos[i].rotateXZBy(90);
	}
	else if(dir == v3s16(0,1,0))
	{
		for(u16 i=0; i<4; i++)
			vertex_pos[i].rotateYZBy(-90);
	}
	else if(dir == v3s16(0,-1,0))
	{
		for(u16 i=0; i<4; i++)
			vertex_pos[i].rotateYZBy(90);
	}

	for(u16 i=0; i<4; i++)
	{
		vertex_pos[i].X *= scale.X;
		vertex_pos[i].Y *= scale.Y;
		vertex_pos[i].Z *= scale.Z;
		vertex_pos[i] += pos + posRelative_f;
	}

	f32 abs_scale = 1.;
	if     (scale.X < 0.999 || scale.X > 1.001) abs_scale = scale.X;
	else if(scale.Y < 0.999 || scale.Y > 1.001) abs_scale = scale.Y;
	else if(scale.Z < 0.999 || scale.Z > 1.001) abs_scale = scale.Z;

	v3f zerovector = v3f(0,0,0);
	
	u8 li = decode_light(light);
	//u8 li = 150;

	u8 alpha = 255;

	if(tile.id == TILE_WATER)
	{
		alpha = 128;
	}

	video::SColor c = video::SColor(alpha,li,li,li);

	face.vertices[0] = video::S3DVertex(vertex_pos[0], zerovector, c,
			core::vector2d<f32>(0,1));
	face.vertices[1] = video::S3DVertex(vertex_pos[1], zerovector, c,
			core::vector2d<f32>(abs_scale,1));
	face.vertices[2] = video::S3DVertex(vertex_pos[2], zerovector, c,
			core::vector2d<f32>(abs_scale,0));
	face.vertices[3] = video::S3DVertex(vertex_pos[3], zerovector, c,
			core::vector2d<f32>(0,0));

	face.tile = tile;
	//DEBUG
	//f->tile = TILE_STONE;
	
	dest.push_back(face);
	//return f;
}
	
/*
	Gets node tile from any place relative to block.
	Returns TILE_NODE if doesn't exist or should not be drawn.
*/
TileSpec MapBlock::getNodeTile(MapNode mn, v3s16 p, v3s16 face_dir)
{
	TileSpec spec;

	/*//DEBUG
	{
		spec.id = TILE_STONE;
		return spec;
	}*/

	spec.feature = TILEFEAT_NONE;
	//spec.id = TILE_STONE;
	spec.id = mn.getTile(face_dir);

	/*
		Check temporary modifications on this node
	*/
	core::map<v3s16, NodeMod>::Node *n;
	n = m_temp_mods.find(p);

	// If modified
	if(n != NULL)
	{
		struct NodeMod mod = n->getValue();
		if(mod.type == NODEMOD_CHANGECONTENT)
		{
			spec.id = content_tile(mod.param, face_dir);
		}
		if(mod.type == NODEMOD_CRACK)
		{
		}
	}
	
	return spec;
}

u8 MapBlock::getNodeContent(v3s16 p, MapNode mn)
{
	/*
		Check temporary modifications on this node
	*/
	core::map<v3s16, NodeMod>::Node *n;
	n = m_temp_mods.find(p);

	// If modified
	if(n != NULL)
	{
		struct NodeMod mod = n->getValue();
		if(mod.type == NODEMOD_CHANGECONTENT)
		{
			// Overrides content
			return mod.param;
		}
		if(mod.type == NODEMOD_CRACK)
		{
			/*
				Content doesn't change.
				
				face_contents works just like it should, because
				there should not be faces between differently cracked
				nodes.

				If a semi-transparent node is cracked in front an
				another one, it really doesn't matter whether there
				is a cracked face drawn in between or not.
			*/
		}
	}

	return mn.d;
}

/*
	startpos:
	translate_dir: unit vector with only one of x, y or z
	face_dir: unit vector with only one of x, y or z
*/
void MapBlock::updateFastFaceRow(
		u32 daynight_ratio,
		v3f posRelative_f,
		v3s16 startpos,
		u16 length,
		v3s16 translate_dir,
		v3f translate_dir_f,
		v3s16 face_dir,
		v3f face_dir_f,
		core::array<FastFace> &dest)
{
	v3s16 p = startpos;
	
	u16 continuous_tiles_count = 0;
	
	MapNode n0 = getNodeParentNoEx(p);
	MapNode n1 = getNodeParentNoEx(p + face_dir);

	u8 light = getFaceLight(daynight_ratio, n0, n1, face_dir);
		
	TileSpec tile0 = getNodeTile(n0, p, face_dir);
	TileSpec tile1 = getNodeTile(n1, p + face_dir, -face_dir);

	for(u16 j=0; j<length; j++)
	{
		bool next_is_different = true;
		
		v3s16 p_next;
		MapNode n0_next;
		MapNode n1_next;
		TileSpec tile0_next;
		TileSpec tile1_next;
		u8 light_next = 0;

		if(j != length - 1)
		{
			p_next = p + translate_dir;
			n0_next = getNodeParentNoEx(p_next);
			n1_next = getNodeParentNoEx(p_next + face_dir);
			tile0_next = getNodeTile(n0_next, p_next, face_dir);
			tile1_next = getNodeTile(n1_next, p_next + face_dir, -face_dir);
			light_next = getFaceLight(daynight_ratio, n0_next, n1_next, face_dir);

			if(tile0_next == tile0
					&& tile1_next == tile1
					&& light_next == light)
			{
				next_is_different = false;
			}
		}

		continuous_tiles_count++;
		
		if(next_is_different)
		{
			/*
				Create a face if there should be one
			*/
			//u8 mf = face_contents(tile0, tile1);
			// This is hackish
			u8 content0 = getNodeContent(p, n0);
			u8 content1 = getNodeContent(p + face_dir, n1);
			u8 mf = face_contents(content0, content1);
			
			if(mf != 0)
			{
				// Floating point conversion of the position vector
				v3f pf(p.X, p.Y, p.Z);
				// Center point of face (kind of)
				v3f sp = pf - ((f32)continuous_tiles_count / 2. - 0.5) * translate_dir_f;
				v3f scale(1,1,1);
				if(translate_dir.X != 0){
					scale.X = continuous_tiles_count;
				}
				if(translate_dir.Y != 0){
					scale.Y = continuous_tiles_count;
				}
				if(translate_dir.Z != 0){
					scale.Z = continuous_tiles_count;
				}
				
				//FastFace *f;

				// If node at sp (tile0) is more solid
				if(mf == 1)
				{
					makeFastFace(tile0, light,
							sp, face_dir, scale,
							posRelative_f, dest);
				}
				// If node at sp is less solid (mf == 2)
				else
				{
					makeFastFace(tile1, light,
							sp+face_dir_f, -face_dir, scale,
							posRelative_f, dest);
				}
				//dest.push_back(f);
			}

			continuous_tiles_count = 0;
			n0 = n0_next;
			n1 = n1_next;
			tile0 = tile0_next;
			tile1 = tile1_next;
			light = light_next;
		}
		
		p = p_next;
	}
}

/*
	This is used because CMeshBuffer::append() is very slow
*/
struct PreMeshBuffer
{
	video::SMaterial material;
	core::array<u16> indices;
	core::array<video::S3DVertex> vertices;
};

class MeshCollector
{
public:
	void append(
			video::SMaterial material,
			const video::S3DVertex* const vertices,
			u32 numVertices,
			const u16* const indices,
			u32 numIndices
		)
	{
		PreMeshBuffer *p = NULL;
		for(u32 i=0; i<m_prebuffers.size(); i++)
		{
			PreMeshBuffer &pp = m_prebuffers[i];
			if(pp.material != material)
				continue;

			p = &pp;
			break;
		}

		if(p == NULL)
		{
			PreMeshBuffer pp;
			pp.material = material;
			m_prebuffers.push_back(pp);
			p = &m_prebuffers[m_prebuffers.size()-1];
		}

		u32 vertex_count = p->vertices.size();
		for(u32 i=0; i<numIndices; i++)
		{
			u32 j = indices[i] + vertex_count;
			if(j > 65535)
			{
				dstream<<"FIXME: Meshbuffer ran out of indices"<<std::endl;
				// NOTE: Fix is to just add an another MeshBuffer
			}
			p->indices.push_back(j);
		}
		for(u32 i=0; i<numVertices; i++)
		{
			p->vertices.push_back(vertices[i]);
		}
	}

	void fillMesh(scene::SMesh *mesh)
	{
		/*dstream<<"Filling mesh with "<<m_prebuffers.size()
				<<" meshbuffers"<<std::endl;*/
		for(u32 i=0; i<m_prebuffers.size(); i++)
		{
			PreMeshBuffer &p = m_prebuffers[i];

			/*dstream<<"p.vertices.size()="<<p.vertices.size()
					<<", p.indices.size()="<<p.indices.size()
					<<std::endl;*/
			
			// Create meshbuffer
			
			// This is a "Standard MeshBuffer",
			// it's a typedeffed CMeshBuffer<video::S3DVertex>
			scene::SMeshBuffer *buf = new scene::SMeshBuffer();
			// Set material
			buf->Material = p.material;
			//((scene::SMeshBuffer*)buf)->Material = p.material;
			// Use VBO
			//buf->setHardwareMappingHint(scene::EHM_STATIC);
			// Add to mesh
			mesh->addMeshBuffer(buf);
			// Mesh grabbed it
			buf->drop();

			buf->append(p.vertices.pointer(), p.vertices.size(),
					p.indices.pointer(), p.indices.size());
		}
	}

private:
	core::array<PreMeshBuffer> m_prebuffers;
};

void MapBlock::updateMesh(u32 daynight_ratio)
{
#if 0
	/*
		DEBUG: If mesh has been generated, don't generate it again
	*/
	{
		JMutexAutoLock meshlock(mesh_mutex);
		if(mesh != NULL)
			return;
	}
#endif
	
	// 4-21ms
	//TimeTaker timer1("updateMesh()", g_device);

	core::array<FastFace> fastfaces_new;
	
	v3f posRelative_f(getPosRelative().X, getPosRelative().Y,
			getPosRelative().Z); // floating point conversion
	
	/*
		We are including the faces of the trailing edges of the block.
		This means that when something changes, the caller must
		also update the meshes of the blocks at the leading edges.

		NOTE: This is the slowest part of this method.
	*/

	/*
		Go through every y,z and get top faces in rows of x+
	*/
	for(s16 y=0; y<MAP_BLOCKSIZE; y++){
		for(s16 z=0; z<MAP_BLOCKSIZE; z++){
			updateFastFaceRow(daynight_ratio, posRelative_f,
					v3s16(0,y,z), MAP_BLOCKSIZE,
					v3s16(1,0,0), //dir
					v3f  (1,0,0),
					v3s16(0,1,0), //face dir
					v3f  (0,1,0),
					fastfaces_new);
		}
	}
	/*
		Go through every x,y and get right faces in rows of z+
	*/
	for(s16 x=0; x<MAP_BLOCKSIZE; x++){
		for(s16 y=0; y<MAP_BLOCKSIZE; y++){
			updateFastFaceRow(daynight_ratio, posRelative_f,
					v3s16(x,y,0), MAP_BLOCKSIZE,
					v3s16(0,0,1),
					v3f  (0,0,1),
					v3s16(1,0,0),
					v3f  (1,0,0),
					fastfaces_new);
		}
	}
	/*
		Go through every y,z and get back faces in rows of x+
	*/
	for(s16 z=0; z<MAP_BLOCKSIZE; z++){
		for(s16 y=0; y<MAP_BLOCKSIZE; y++){
			updateFastFaceRow(daynight_ratio, posRelative_f,
					v3s16(0,y,z), MAP_BLOCKSIZE,
					v3s16(1,0,0),
					v3f  (1,0,0),
					v3s16(0,0,1),
					v3f  (0,0,1),
					fastfaces_new);
		}
	}

	// End of slow part

	/*
		Convert FastFaces to SMesh
	*/

	scene::SMesh *mesh_new = NULL;
	
	mesh_new = new scene::SMesh();
	
	if(fastfaces_new.size() > 0)
	{
		MeshCollector collector;

		for(u32 i=0; i<fastfaces_new.size(); i++)
		{
			FastFace &f = fastfaces_new[i];

			const u16 indices[] = {0,1,2,2,3,0};
			
			if(f.tile.feature == TILEFEAT_NONE)
			{
				/*collector.append(g_tile_materials[f.tile.id], f.vertices, 4,
						indices, 6);*/
				collector.append(tile_material_get(f.tile.id), f.vertices, 4,
						indices, 6);
			}
			else
			{
				// Not implemented
				assert(0);
			}
		}

		collector.fillMesh(mesh_new);

		// Use VBO for mesh (this just would set this for ever buffer)
		//mesh_new->setHardwareMappingHint(scene::EHM_STATIC);
		
		/*std::cout<<"MapBlock has "<<fastfaces_new->getSize()<<" faces "
				<<"and uses "<<mesh_new->getMeshBufferCount()
				<<" materials (meshbuffers)"<<std::endl;*/
	}

	/*
		Clear temporary FastFaces
	*/

	/*core::list<FastFace*>::Iterator i;
	i = fastfaces_new->begin();
	for(; i != fastfaces_new->end(); i++)
	{
		delete *i;
	}
	fastfaces_new->clear();
	delete fastfaces_new;*/

	/*
		Add special graphics:
		- torches
		
		TODO: Optimize by using same meshbuffer for same textures
	*/

	/*scene::ISceneManager *smgr = NULL;
	video::IVideoDriver* driver = NULL;
	if(g_device)
	{
		smgr = g_device->getSceneManager();
		driver = smgr->getVideoDriver();
	}*/
			
	for(s16 z=0; z<MAP_BLOCKSIZE; z++)
	for(s16 y=0; y<MAP_BLOCKSIZE; y++)
	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		v3s16 p(x,y,z);

		MapNode &n = getNodeRef(x,y,z);
		
		if(n.d == CONTENT_TORCH)
		{
			//scene::IMeshBuffer *buf = new scene::SMeshBuffer();
			scene::SMeshBuffer *buf = new scene::SMeshBuffer();
			video::SColor c(255,255,255,255);

			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c, 0,1),
				video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c, 1,1),
				video::S3DVertex(BS/2,BS/2,0, 0,0,0, c, 1,0),
				video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c, 0,0),
			};

			v3s16 dir = unpackDir(n.dir);

			for(s32 i=0; i<4; i++)
			{
				if(dir == v3s16(1,0,0))
					vertices[i].Pos.rotateXZBy(0);
				if(dir == v3s16(-1,0,0))
					vertices[i].Pos.rotateXZBy(180);
				if(dir == v3s16(0,0,1))
					vertices[i].Pos.rotateXZBy(90);
				if(dir == v3s16(0,0,-1))
					vertices[i].Pos.rotateXZBy(-90);
				if(dir == v3s16(0,-1,0))
					vertices[i].Pos.rotateXZBy(45);
				if(dir == v3s16(0,1,0))
					vertices[i].Pos.rotateXZBy(-45);

				vertices[i].Pos += intToFloat(p + getPosRelative());
			}

			u16 indices[] = {0,1,2,2,3,0};
			buf->append(vertices, 4, indices, 6);

			// Set material
			buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
			buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
			buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
			//buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			buf->getMaterial().MaterialType
					= video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			if(dir == v3s16(0,-1,0))
				buf->getMaterial().setTexture(0,
						g_irrlicht->getTexture("../data/torch_on_floor.png"));
						//g_texturecache.get("torch_on_floor"));
			else if(dir == v3s16(0,1,0))
				buf->getMaterial().setTexture(0,
						g_irrlicht->getTexture("../data/torch_on_ceiling.png"));
						//g_texturecache.get("torch_on_ceiling"));
			// For backwards compatibility
			else if(dir == v3s16(0,0,0))
				buf->getMaterial().setTexture(0,
						g_irrlicht->getTexture("../data/torch_on_floor.png"));
						//g_texturecache.get("torch_on_floor"));
			else
				buf->getMaterial().setTexture(0, 
						g_irrlicht->getTexture("../data/torch.png"));
				//buf->getMaterial().setTexture(0, g_texturecache.get("torch"));

			// Add to mesh
			mesh_new->addMeshBuffer(buf);
			buf->drop();
		}
	}
	
	/*
		Do some stuff to the mesh
	*/

	mesh_new->recalculateBoundingBox();

	/*
		Delete new mesh if it is empty
	*/

	if(mesh_new->getMeshBufferCount() == 0)
	{
		mesh_new->drop();
		mesh_new = NULL;
	}

	/*
		Replace the mesh
	*/

	mesh_mutex.Lock();

	//scene::SMesh *mesh_old = mesh[daynight_i];
	//mesh[daynight_i] = mesh_new;

	scene::SMesh *mesh_old = mesh;
	mesh = mesh_new;
	setMeshExpired(false);
	
	if(mesh_old != NULL)
	{
		// Remove hardware buffers of meshbuffers of mesh
		// NOTE: No way, this runs in a different thread and everything
		/*u32 c = mesh_old->getMeshBufferCount();
		for(u32 i=0; i<c; i++)
		{
			IMeshBuffer *buf = mesh_old->getMeshBuffer(i);
		}*/
		
		/*dstream<<"mesh_old->getReferenceCount()="
				<<mesh_old->getReferenceCount()<<std::endl;
		u32 c = mesh_old->getMeshBufferCount();
		for(u32 i=0; i<c; i++)
		{
			scene::IMeshBuffer *buf = mesh_old->getMeshBuffer(i);
			dstream<<"buf->getReferenceCount()="
					<<buf->getReferenceCount()<<std::endl;
		}*/

		// Drop the mesh
		mesh_old->drop();

		//delete mesh_old;
	}

	mesh_mutex.Unlock();
	
	//std::cout<<"added "<<fastfaces.getSize()<<" faces."<<std::endl;
}

/*void MapBlock::updateMeshes(s32 first_i)
{
	assert(first_i >= 0 && first_i <= DAYNIGHT_CACHE_COUNT);
	updateMesh(first_i);
	for(s32 i=0; i<DAYNIGHT_CACHE_COUNT; i++)
	{
		if(i == first_i)
			continue;
		updateMesh(i);
	}
}*/

#endif // !SERVER

/*
	Propagates sunlight down through the block.
	Doesn't modify nodes that are not affected by sunlight.
	
	Returns false if sunlight at bottom block is invalid
	Returns true if bottom block doesn't exist.

	If there is a block above, continues from it.
	If there is no block above, assumes there is sunlight, unless
	is_underground is set.

	At the moment, all sunlighted nodes are added to light_sources.
	TODO: This could be optimized.
*/
bool MapBlock::propagateSunlight(core::map<v3s16, bool> & light_sources)
{
	// Whether the sunlight at the top of the bottom block is valid
	bool block_below_is_valid = true;
	
	v3s16 pos_relative = getPosRelative();
	
	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		for(s16 z=0; z<MAP_BLOCKSIZE; z++)
		{
			bool no_sunlight = false;
			bool no_top_block = false;
			// Check if node above block has sunlight
			try{
				MapNode n = getNodeParent(v3s16(x, MAP_BLOCKSIZE, z));
				if(n.getLight(LIGHTBANK_DAY) != LIGHT_SUN)
				{
					/*if(is_underground)
					{
						no_sunlight = true;
					}*/
					no_sunlight = true;
				}
			}
			catch(InvalidPositionException &e)
			{
				no_top_block = true;
				
				// TODO: This makes over-ground roofed places sunlighted
				// Assume sunlight, unless is_underground==true
				if(is_underground)
				{
					no_sunlight = true;
				}
				
				// TODO: There has to be some way to allow this behaviour
				// As of now, it just makes everything dark.
				// No sunlight here
				//no_sunlight = true;
			}

			/*std::cout<<"("<<x<<","<<z<<"): "
					<<"no_top_block="<<no_top_block
					<<", is_underground="<<is_underground
					<<", no_sunlight="<<no_sunlight
					<<std::endl;*/
		
			s16 y = MAP_BLOCKSIZE-1;
			
			if(no_sunlight == false)
			{
				// Continue spreading sunlight downwards through transparent
				// nodes
				for(; y >= 0; y--)
				{
					v3s16 pos(x, y, z);
					
					MapNode &n = getNodeRef(pos);

					if(n.sunlight_propagates())
					{
						n.setLight(LIGHTBANK_DAY, LIGHT_SUN);

						light_sources.insert(pos_relative + pos, true);
					}
					else{
						break;
					}
				}
			}

			bool sunlight_should_go_down = (y==-1);

			// Fill rest with black (only transparent ones)
			for(; y >= 0; y--){
				v3s16 pos(x, y, z);
				
				MapNode &n = getNodeRef(pos);

				if(n.light_propagates())
				{
					n.setLight(LIGHTBANK_DAY, 0);
				}
				else{
					break;
				}
			}

			/*
				If the block below hasn't already been marked invalid:

				Check if the node below the block has proper sunlight at top.
				If not, the block below is invalid.
				
				Ignore non-transparent nodes as they always have no light
			*/
			try
			{
			if(block_below_is_valid)
			{
				MapNode n = getNodeParent(v3s16(x, -1, z));
				if(n.light_propagates())
				{
					if(n.getLight(LIGHTBANK_DAY) == LIGHT_SUN
							&& sunlight_should_go_down == false)
						block_below_is_valid = false;
					else if(n.getLight(LIGHTBANK_DAY) != LIGHT_SUN
							&& sunlight_should_go_down == true)
						block_below_is_valid = false;
				}
			}//if
			}//try
			catch(InvalidPositionException &e)
			{
				/*std::cout<<"InvalidBlockException for bottom block node"
						<<std::endl;*/
				// Just no block below, no need to panic.
			}
		}
	}

	return block_below_is_valid;
}

void MapBlock::copyTo(VoxelManipulator &dst)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));
	
	dst.copyFrom(data, data_area, v3s16(0,0,0),
			getPosRelative(), data_size);
}

/*void getPseudoObjects(v3f origin, f32 max_d,
		core::array<DistanceSortedObject> &dest)
{
}*/


void MapBlock::updateDayNightDiff()
{
	if(data == NULL)
	{
		m_day_night_differs = false;
		return;
	}

	bool differs = false;

	/*
		Check if any lighting value differs
	*/
	for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
	{
		MapNode &n = data[i];
		if(n.getLight(LIGHTBANK_DAY) != n.getLight(LIGHTBANK_NIGHT))
		{
			differs = true;
			break;
		}
	}

	/*
		If some lighting values differ, check if the whole thing is
		just air. If it is, differ = false
	*/
	if(differs)
	{
		bool only_air = true;
		for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
		{
			MapNode &n = data[i];
			if(n.d != CONTENT_AIR)
			{
				only_air = false;
				break;
			}
		}
		if(only_air)
			differs = false;
	}

	// Set member variable
	m_day_night_differs = differs;
}

/*
	Serialization
*/

void MapBlock::serialize(std::ostream &os, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");
	
	if(data == NULL)
	{
		throw SerializationError("ERROR: Not writing dummy block.");
	}
	
	// These have no compression
	if(version <= 3 || version == 5 || version == 6)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;
		
		u32 buflen = 1 + nodecount * MapNode::serializedLength(version);
		SharedBuffer<u8> dest(buflen);

		dest[0] = is_underground;
		for(u32 i=0; i<nodecount; i++)
		{
			u32 s = 1 + i * MapNode::serializedLength(version);
			data[i].serialize(&dest[s], version);
		}
		
		os.write((char*)*dest, dest.getSize());
	}
	else if(version <= 10)
	{
		/*
			With compression.
			Compress the materials and the params separately.
		*/
		
		// First byte
		os.write((char*)&is_underground, 1);

		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		// Get and compress materials
		SharedBuffer<u8> materialdata(nodecount);
		for(u32 i=0; i<nodecount; i++)
		{
			materialdata[i] = data[i].d;
		}
		compress(materialdata, os, version);

		// Get and compress lights
		SharedBuffer<u8> lightdata(nodecount);
		for(u32 i=0; i<nodecount; i++)
		{
			lightdata[i] = data[i].param;
		}
		compress(lightdata, os, version);
		
		if(version >= 10)
		{
			// Get and compress pressure
			SharedBuffer<u8> pressuredata(nodecount);
			for(u32 i=0; i<nodecount; i++)
			{
				pressuredata[i] = data[i].pressure;
			}
			compress(pressuredata, os, version);
		}
	}
	// All other versions (newest)
	else
	{
		// First byte
		u8 flags = 0;
		if(is_underground)
			flags |= 1;
		if(m_day_night_differs)
			flags |= 2;
		os.write((char*)&flags, 1);

		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		/*
			Get data
		*/

		SharedBuffer<u8> databuf(nodecount*3);

		// Get contents
		for(u32 i=0; i<nodecount; i++)
		{
			databuf[i] = data[i].d;
		}

		// Get params
		for(u32 i=0; i<nodecount; i++)
		{
			databuf[i+nodecount] = data[i].param;
		}

		// Get pressure
		for(u32 i=0; i<nodecount; i++)
		{
			databuf[i+nodecount*2] = data[i].pressure;
		}

		/*
			Compress data to output stream
		*/

		compress(databuf, os, version);
	}
}

void MapBlock::deSerialize(std::istream &is, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	// These have no compression
	if(version <= 3 || version == 5 || version == 6)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;
		char tmp;
		is.read(&tmp, 1);
		if(is.gcount() != 1)
			throw SerializationError
					("MapBlock::deSerialize: no enough input data");
		is_underground = tmp;
		for(u32 i=0; i<nodecount; i++)
		{
			s32 len = MapNode::serializedLength(version);
			SharedBuffer<u8> d(len);
			is.read((char*)*d, len);
			if(is.gcount() != len)
				throw SerializationError
						("MapBlock::deSerialize: no enough input data");
			data[i].deSerialize(*d, version);
		}
	}
	else if(version <= 10)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		u8 t8;
		is.read((char*)&t8, 1);
		is_underground = t8;

		{
			// Uncompress and set material data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].d = s[i];
			}
		}
		{
			// Uncompress and set param data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].param = s[i];
			}
		}
	
		if(version >= 10)
		{
			// Uncompress and set pressure data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].pressure = s[i];
			}
		}
	}
	// All other versions (newest)
	else
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		u8 flags;
		is.read((char*)&flags, 1);
		is_underground = (flags & 1) ? true : false;
		m_day_night_differs = (flags & 2) ? true : false;

		// Uncompress data
		std::ostringstream os(std::ios_base::binary);
		decompress(is, os, version);
		std::string s = os.str();
		if(s.size() != nodecount*3)
			throw SerializationError
					("MapBlock::deSerialize: invalid format");

		// Set contents
		for(u32 i=0; i<nodecount; i++)
		{
			data[i].d = s[i];
		}
		// Set params
		for(u32 i=0; i<nodecount; i++)
		{
			data[i].param = s[i+nodecount];
		}
		// Set pressure
		for(u32 i=0; i<nodecount; i++)
		{
			data[i].pressure = s[i+nodecount*2];
		}
	}
}


//END
