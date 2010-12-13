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

#ifndef MAPBLOCKOBJECT_HEADER
#define MAPBLOCKOBJECT_HEADER

#include "common_irrlicht.h"
#include <math.h>
#include <string>
#include "serialization.h"
#include "mapnode.h"
#include "constants.h"
#include "debug.h"

#define MAPBLOCKOBJECT_TYPE_TEST 0
#define MAPBLOCKOBJECT_TYPE_TEST2 1
#define MAPBLOCKOBJECT_TYPE_SIGN 2
#define MAPBLOCKOBJECT_TYPE_RAT 3
// Used for handling selecting special stuff
//#define MAPBLOCKOBJECT_TYPE_PSEUDO 4

class MapBlock;

class MapBlockObject
{
public:
	MapBlockObject(MapBlock *block, s16 id, v3f pos):
		m_collision_box(NULL),
		m_selection_box(NULL),
		m_block(block),
		m_id(id),
		m_pos(pos)
	{
	}
	virtual ~MapBlockObject()
	{
	}

	s16 getId()
	{
		return m_id;
	}
	MapBlock* getBlock()
	{
		return m_block;
	}
	
	// Writes id, pos and typeId
	void serializeBase(std::ostream &os, u8 version)
	{
		u8 buf[6];

		// id
		writeS16(buf, m_id);
		os.write((char*)buf, 2);
		
		// position
		// stored as x1000/BS v3s16
		v3s16 pos_i(m_pos.X*1000/BS, m_pos.Y*1000/BS, m_pos.Z*1000/BS);
		writeV3S16(buf, pos_i);
		os.write((char*)buf, 6);

		// typeId
		writeU16(buf, getTypeId());
		os.write((char*)buf, 2);
	}
	
	// Get floating point position on map
	v3f getAbsolutePos();

	void setBlockChanged();

	// Shootline is relative to block
	bool isSelected(core::line3d<f32> shootline)
	{
		if(m_selection_box == NULL)
			return false;

		core::aabbox3d<f32> offsetted_box(
				m_selection_box->MinEdge + m_pos,
				m_selection_box->MaxEdge + m_pos
		);

		return offsetted_box.intersectsWithLine(shootline);
	}

	core::aabbox3d<f32> getSelectionBoxOnMap()
	{
		v3f absolute_pos = getAbsolutePos();

		core::aabbox3d<f32> box(
				m_selection_box->MinEdge + absolute_pos,
				m_selection_box->MaxEdge + absolute_pos
		);

		return box;
	}
	
	/*
		Implementation interface
	*/

	virtual u16 getTypeId() const = 0;
	// Shall call serializeBase and then write the parameters
	virtual void serialize(std::ostream &os, u8 version) = 0;
	// Shall read parameters from stream
	virtual void update(std::istream &is, u8 version) = 0;

	virtual std::string getInventoryString() { return "None"; }
	
	// Reimplementation shall call this.
	virtual void updatePos(v3f pos)
	{
		m_pos = pos;
	}
	
	// Shall move the object around, modify it and possibly delete it.
	// Typical dtimes are 0.2 and 10000.
	// A return value of true requests deletion of the object by the caller.
	// NOTE: Only server calls this.
	virtual bool serverStep(float dtime) { return false; };
	// This should do slight animations only or so
	virtual void clientStep(float dtime) {};

	// NOTE: These functions should do nothing if the asked state is
	//       same as the current state
	// Shall add and remove relevant scene nodes for rendering the
	// object in the game world
	virtual void addToScene(scene::ISceneManager *smgr) {};
	// Shall remove stuff from the scene
	// Should return silently if there is nothing to remove
	// NOTE: This has to be called before calling destructor
	virtual void removeFromScene() {};

	virtual std::string infoText() { return ""; }
	
	// Shall be left NULL if doesn't collide
	// Position is relative to m_pos in block
	core::aabbox3d<f32> * m_collision_box;
	
	// Shall be left NULL if can't be selected
	core::aabbox3d<f32> * m_selection_box;

protected:
	MapBlock *m_block;
	// This differentiates the instance of the object
	// Not same as typeId.
	s16 m_id;
	// Position of the object inside the block
	// Units is node coordinates * BS
	v3f m_pos;

	friend class MapBlockObjectList;
};

#if 0
/*
	Used for handling selections of special stuff
*/
class PseudoMBObject : public MapBlockObject
{
public:
	// The constructor of every MapBlockObject should be like this
	PseudoMBObject(MapBlock *block, s16 id, v3f pos):
		MapBlockObject(block, id, pos)
	{
	}
	virtual ~PseudoMBObject()
	{
		if(m_selection_box)
			delete m_selection_box;
	}
	
	/*
		Implementation interface
	*/
	virtual u16 getTypeId() const
	{
		return MAPBLOCKOBJECT_TYPE_PSEUDO;
	}
	virtual void serialize(std::ostream &os, u8 version)
	{
		assert(0);
	}
	virtual void update(std::istream &is, u8 version)
	{
		assert(0);
	}
	virtual bool serverStep(float dtime)
	{
		assert(0);
	}

	/*
		Special methods
	*/
	
	void setSelectionBox(core::aabbox3d<f32> box)
	{
		m_selection_box = new core::aabbox3d<f32>(box);
	}
	
protected:
};
#endif

class TestObject : public MapBlockObject
{
public:
	// The constructor of every MapBlockObject should be like this
	TestObject(MapBlock *block, s16 id, v3f pos):
		MapBlockObject(block, id, pos),
		m_node(NULL)
	{
	}
	virtual ~TestObject()
	{
	}
	
	/*
		Implementation interface
	*/
	virtual u16 getTypeId() const
	{
		return MAPBLOCKOBJECT_TYPE_TEST;
	}
	virtual void serialize(std::ostream &os, u8 version)
	{
		serializeBase(os, version);

		// Write subpos_c * 100
		u8 buf[2];
		writeU16(buf, m_subpos_c * 100);
		os.write((char*)buf, 2);
	}
	virtual void update(std::istream &is, u8 version)
	{
		// Read subpos_c * 100
		u8 buf[2];
		is.read((char*)buf, 2);
		m_subpos_c = (f32)readU16(buf) / 100;
		
		updateNodePos();
	}
	virtual bool serverStep(float dtime)
	{
		m_subpos_c += dtime * 3.0;

		updateNodePos();

		return false;
	}
	virtual void addToScene(scene::ISceneManager *smgr)
	{
		if(m_node != NULL)
			return;
		
		//dstream<<"Adding to scene"<<std::endl;

		video::IVideoDriver* driver = smgr->getVideoDriver();
		
		scene::SMesh *mesh = new scene::SMesh();
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
			video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
			video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
			video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
		buf->getMaterial().setTexture
				(0, driver->getTexture("../data/player.png"));
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		m_node = smgr->addMeshSceneNode(mesh, NULL);
		mesh->drop();
		m_node->setPosition(getAbsolutePos());
	}
	virtual void removeFromScene()
	{
		//dstream<<"Removing from scene"<<std::endl;
		if(m_node != NULL)
		{
			m_node->remove();
			m_node = NULL;
		}
	}

	/*
		Special methods
	*/
	
	void updateNodePos()
	{
		m_subpos = BS*2.0 * v3f(sin(m_subpos_c), sin(m_subpos_c+1.0), sin(-m_subpos_c));
		
		if(m_node != NULL)
		{
			m_node->setPosition(getAbsolutePos() + m_subpos);
		}
	}
	
protected:
	scene::IMeshSceneNode *m_node;
	std::string m_text;

	v3f m_subpos;
	f32 m_subpos_c;
};

class MovingObject : public MapBlockObject
{
public:
	// The constructor of every MapBlockObject should be like this
	MovingObject(MapBlock *block, s16 id, v3f pos):
		MapBlockObject(block, id, pos),
		m_speed(0,0,0)
	{
		m_touching_ground = false;
	}
	virtual ~MovingObject()
	{
	}
	
	/*
		Implementation interface
	*/
	
	virtual u16 getTypeId() const = 0;

	virtual void serialize(std::ostream &os, u8 version)
	{
		serializeBase(os, version);

		u8 buf[6];

		// Write speed
		// stored as x100/BS v3s16
		v3s16 speed_i(m_speed.X*100/BS, m_speed.Y*100/BS, m_speed.Z*100/BS);
		writeV3S16(buf, speed_i);
		os.write((char*)buf, 6);
	}
	virtual void update(std::istream &is, u8 version)
	{
		u8 buf[6];
		
		// Read speed
		// stored as x100/BS v3s16
		is.read((char*)buf, 6);
		v3s16 speed_i = readV3S16(buf);
		v3f speed((f32)speed_i.X/100*BS,
				(f32)speed_i.Y/100*BS,
				(f32)speed_i.Z/100*BS);

		m_speed = speed;
	}

	virtual bool serverStep(float dtime) { return false; };
	virtual void clientStep(float dtime) {};
	
	virtual void addToScene(scene::ISceneManager *smgr) = 0;
	virtual void removeFromScene() = 0;

	/*
		Special methods
	*/
	
	// Moves with collision detection
	void move(float dtime, v3f acceleration);
	
protected:
	v3f m_speed;
	bool m_touching_ground;
};

class Test2Object : public MovingObject
{
public:
	// The constructor of every MapBlockObject should be like this
	Test2Object(MapBlock *block, s16 id, v3f pos):
		MovingObject(block, id, pos),
		m_node(NULL)
	{
		m_collision_box = new core::aabbox3d<f32>
				(-BS*0.3,0,-BS*0.3, BS*0.3,BS*1.7,BS*0.3);
	}
	virtual ~Test2Object()
	{
		delete m_collision_box;
	}
	
	/*
		Implementation interface
	*/
	virtual u16 getTypeId() const
	{
		return MAPBLOCKOBJECT_TYPE_TEST2;
	}
	virtual void serialize(std::ostream &os, u8 version)
	{
		MovingObject::serialize(os, version);
	}
	virtual void update(std::istream &is, u8 version)
	{
		MovingObject::update(is, version);
		
		updateNodePos();
	}

	virtual bool serverStep(float dtime)
	{
		m_speed.X = 2*BS;
		m_speed.Z = 0;

		if(m_touching_ground)
		{
			static float count = 0;
			count -= dtime;
			if(count < 0.0)
			{
				count += 1.0;
				m_speed.Y = 6.5*BS;
			}
		}

		move(dtime, v3f(0, -9.81*BS, 0));

		updateNodePos();

		return false;
	}
	
	virtual void clientStep(float dtime)
	{
		m_pos += m_speed * dtime;

		updateNodePos();
	}
	
	virtual void addToScene(scene::ISceneManager *smgr)
	{
		if(m_node != NULL)
			return;
		
		//dstream<<"Adding to scene"<<std::endl;

		video::IVideoDriver* driver = smgr->getVideoDriver();
		
		scene::SMesh *mesh = new scene::SMesh();
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
			video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
			video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
			video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
		buf->getMaterial().setTexture
				(0, driver->getTexture("../data/player.png"));
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		m_node = smgr->addMeshSceneNode(mesh, NULL);
		mesh->drop();
		m_node->setPosition(getAbsolutePos());
	}
	virtual void removeFromScene()
	{
		//dstream<<"Removing from scene"<<std::endl;
		if(m_node != NULL)
		{
			m_node->remove();
			m_node = NULL;
		}
	}

	/*
		Special methods
	*/
	
	void updateNodePos()
	{
		//m_subpos = BS*2.0 * v3f(sin(m_subpos_c), sin(m_subpos_c+1.0), sin(-m_subpos_c));
		
		if(m_node != NULL)
		{
			//m_node->setPosition(getAbsolutePos() + m_subpos);
			m_node->setPosition(getAbsolutePos());
		}
	}
	
protected:
	scene::IMeshSceneNode *m_node;
};

class RatObject : public MovingObject
{
public:
	RatObject(MapBlock *block, s16 id, v3f pos):
		MovingObject(block, id, pos),
		m_node(NULL)
	{
		m_collision_box = new core::aabbox3d<f32>
				(-BS*0.3,0,-BS*0.3, BS*0.3,BS*0.5,BS*0.3);
		m_selection_box = new core::aabbox3d<f32>
				(-BS*0.3,0,-BS*0.3, BS*0.3,BS*0.5,BS*0.3);

		m_counter1 = 0;
		m_counter2 = 0;
	}
	virtual ~RatObject()
	{
		delete m_collision_box;
		delete m_selection_box;
	}
	
	/*
		Implementation interface
	*/
	virtual u16 getTypeId() const
	{
		return MAPBLOCKOBJECT_TYPE_RAT;
	}
	virtual void serialize(std::ostream &os, u8 version)
	{
		MovingObject::serialize(os, version);
		u8 buf[2];

		// Write yaw * 10
		writeS16(buf, m_yaw * 10);
		os.write((char*)buf, 2);

	}
	virtual void update(std::istream &is, u8 version)
	{
		MovingObject::update(is, version);
		u8 buf[2];
		
		// Read yaw * 10
		is.read((char*)buf, 2);
		s16 yaw_i = readS16(buf);
		m_yaw = (f32)yaw_i / 10;

		updateNodePos();
	}

	virtual bool serverStep(float dtime)
	{
		v3f dir(cos(m_yaw/180*PI),0,sin(m_yaw/180*PI));

		f32 speed = 2*BS;

		m_speed.X = speed * dir.X;
		m_speed.Z = speed * dir.Z;

		if(m_touching_ground && (m_oldpos - m_pos).getLength() < dtime*speed/2)
		{
			m_counter1 -= dtime;
			if(m_counter1 < 0.0)
			{
				m_counter1 += 1.0;
				m_speed.Y = 5.0*BS;
			}
		}

		{
			m_counter2 -= dtime;
			if(m_counter2 < 0.0)
			{
				m_counter2 += (float)(rand()%100)/100*3.0;
				m_yaw += ((float)(rand()%200)-100)/100*180;
				m_yaw = wrapDegrees(m_yaw);
			}
		}

		m_oldpos = m_pos;

		//m_yaw += dtime*90;

		move(dtime, v3f(0, -9.81*BS, 0));

		updateNodePos();

		return false;
	}
	
	virtual void clientStep(float dtime)
	{
		m_pos += m_speed * dtime;

		updateNodePos();
	}
	
	virtual void addToScene(scene::ISceneManager *smgr)
	{
		if(m_node != NULL)
			return;
		
		video::IVideoDriver* driver = smgr->getVideoDriver();
		
		scene::SMesh *mesh = new scene::SMesh();
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
			video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
			video::S3DVertex(BS/2,BS/2,0, 0,0,0, c, 1,0),
			video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
		buf->getMaterial().setTexture
				(0, driver->getTexture("../data/rat.png"));
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		m_node = smgr->addMeshSceneNode(mesh, NULL);
		mesh->drop();
		m_node->setPosition(getAbsolutePos());
	}
	virtual void removeFromScene()
	{
		if(m_node != NULL)
		{
			m_node->remove();
			m_node = NULL;
		}
	}

	virtual std::string getInventoryString()
	{
		// There must be a space after the name
		// Or does there?
		return std::string("Rat ");
	}

	/*
		Special methods
	*/
	
	void updateNodePos()
	{
		if(m_node != NULL)
		{
			m_node->setPosition(getAbsolutePos());
			m_node->setRotation(v3f(0, -m_yaw+180, 0));
		}
	}
	
protected:
	scene::IMeshSceneNode *m_node;
	float m_yaw;

	float m_counter1;
	float m_counter2;
	v3f m_oldpos;
};

class SignObject : public MapBlockObject
{
public:
	// The constructor of every MapBlockObject should be like this
	SignObject(MapBlock *block, s16 id, v3f pos):
		MapBlockObject(block, id, pos),
		m_node(NULL)
	{
		m_selection_box = new core::aabbox3d<f32>
				(-BS*0.4,-BS*0.5,-BS*0.4, BS*0.4,BS*0.5,BS*0.4);
	}
	virtual ~SignObject()
	{
		delete m_selection_box;
	}
	
	/*
		Implementation interface
	*/
	virtual u16 getTypeId() const
	{
		return MAPBLOCKOBJECT_TYPE_SIGN;
	}
	virtual void serialize(std::ostream &os, u8 version)
	{
		serializeBase(os, version);
		u8 buf[2];

		// Write yaw * 10
		writeS16(buf, m_yaw * 10);
		os.write((char*)buf, 2);

		// Write text length
		writeU16(buf, m_text.size());
		os.write((char*)buf, 2);
		
		// Write text
		os.write(m_text.c_str(), m_text.size());
	}
	virtual void update(std::istream &is, u8 version)
	{
		u8 buf[2];

		// Read yaw * 10
		is.read((char*)buf, 2);
		s16 yaw_i = readS16(buf);
		m_yaw = (f32)yaw_i / 10;

		// Read text length
		is.read((char*)buf, 2);
		u16 size = readU16(buf);

		// Read text
		m_text.clear();
		for(u16 i=0; i<size; i++)
		{
			is.read((char*)buf, 1);
			m_text += buf[0];
		}

		updateSceneNode();
	}
	virtual bool serverStep(float dtime)
	{
		return false;
	}
	virtual void addToScene(scene::ISceneManager *smgr)
	{
		if(m_node != NULL)
			return;
		
		video::IVideoDriver* driver = smgr->getVideoDriver();
		
		scene::SMesh *mesh = new scene::SMesh();
		{ // Front
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c, 0,1),
			video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c, 1,1),
			video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c, 1,0),
			video::S3DVertex(BS/2,BS/2,0, 0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		//buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
		buf->getMaterial().setTexture
				(0, driver->getTexture("../data/sign.png"));
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		}
		{ // Back
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c, 0,1),
			video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c, 1,1),
			video::S3DVertex(BS/2,BS/2,0, 0,0,0, c, 1,0),
			video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		//buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
		buf->getMaterial().setTexture
				(0, driver->getTexture("../data/sign_back.png"));
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		}
		m_node = smgr->addMeshSceneNode(mesh, NULL);
		mesh->drop();

		updateSceneNode();
	}
	virtual void removeFromScene()
	{
		if(m_node != NULL)
		{
			m_node->remove();
			m_node = NULL;
		}
	}

	virtual std::string infoText()
	{
		return std::string("\"") + m_text + "\"";
	}

	virtual std::string getInventoryString()
	{
		return std::string("Sign ")+m_text;
	}

	/*
		Special methods
	*/

	void updateSceneNode()
	{
		if(m_node != NULL)
		{
			m_node->setPosition(getAbsolutePos());
			m_node->setRotation(v3f(0, m_yaw, 0));
		}
	}

	void setText(std::string text)
	{
		if(text.size() > SIGN_TEXT_MAX_LENGTH)
			text = text.substr(0, SIGN_TEXT_MAX_LENGTH);
		m_text = text;

		setBlockChanged();
	}

	void setYaw(f32 yaw)
	{
		m_yaw = yaw;

		setBlockChanged();
	}
	
protected:
	scene::IMeshSceneNode *m_node;
	std::string m_text;
	f32 m_yaw;
};

struct DistanceSortedObject
{
	DistanceSortedObject(MapBlockObject *a_obj, f32 a_d)
	{
		obj = a_obj;
		d = a_d;
	}
	
	MapBlockObject *obj;
	f32 d;

	bool operator < (DistanceSortedObject &other)
	{
		return d < other.d;
	}
};

class MapBlockObjectList
{
public:
	MapBlockObjectList(MapBlock *block);
	~MapBlockObjectList();
	// Writes the count, id, the type id and the parameters of all objects
	void serialize(std::ostream &os, u8 version);
	// Reads ids, type_ids and parameters.
	// Creates, updates and deletes objects.
	// If smgr!=NULL, new objects are added to the scene
	void update(std::istream &is, u8 version, scene::ISceneManager *smgr);
	// Finds a new unique id
	s16 getFreeId() throw(ContainerFullException);
	/*
		Adds an object.
		Set id to -1 to have this set it to a suitable one.
		The block pointer member is set to this block.
	*/
	void add(MapBlockObject *object)
			throw(ContainerFullException, AlreadyExistsException);

	// Deletes and removes all objects
	void clear();

	/*
		Removes an object.
		Ignores inexistent objects
	*/
	void remove(s16 id);
	/*
		References an object.
		The object will not be valid after step() or of course if
		it is removed.
		Grabbing the lock is recommended while processing.
	*/
	MapBlockObject * get(s16 id);

	// You'll want to grab this in a SharedPtr
	JMutexAutoLock * getLock()
	{
		return new JMutexAutoLock(m_mutex);
	}

	// Steps all objects and if server==true, removes those that
	// want to be removed
	void step(float dtime, bool server);

	// Wraps an object that wants to move onto this block from an another
	// Returns true if wrapping was impossible
	bool wrapObject(MapBlockObject *object);
	
	// origin is relative to block
	void getObjects(v3f origin, f32 max_d,
			core::array<DistanceSortedObject> &dest);
	
	// Number of objects
	s32 getCount()
	{
		return m_objects.size();
	}

private:
	JMutex m_mutex;
	// Key is id
	core::map<s16, MapBlockObject*> m_objects;
	MapBlock *m_block;
};


#endif

