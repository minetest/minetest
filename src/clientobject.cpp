/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "clientobject.h"
#include "debug.h"
#include "porting.h"
#include "constants.h"
#include "utility.h"
#include "environment.h"

/*
	ClientActiveObject
*/

core::map<u16, ClientActiveObject::Factory> ClientActiveObject::m_types;

ClientActiveObject::ClientActiveObject(u16 id):
	ActiveObject(id)
{
}

ClientActiveObject::~ClientActiveObject()
{
	removeFromScene();
}

ClientActiveObject* ClientActiveObject::create(u8 type)
{
	// Find factory function
	core::map<u16, Factory>::Node *n;
	n = m_types.find(type);
	if(n == NULL)
	{
		// If factory is not found, just return.
		dstream<<"WARNING: ClientActiveObject: No factory for type="
				<<type<<std::endl;
		return NULL;
	}

	Factory f = n->getValue();
	ClientActiveObject *object = (*f)();
	return object;
}

void ClientActiveObject::registerType(u16 type, Factory f)
{
	core::map<u16, Factory>::Node *n;
	n = m_types.find(type);
	if(n)
		return;
	m_types.insert(type, f);
}

/*
	TestCAO
*/

// Prototype
TestCAO proto_TestCAO;

TestCAO::TestCAO():
	ClientActiveObject(0),
	m_node(NULL),
	m_position(v3f(0,10*BS,0))
{
	ClientActiveObject::registerType(getType(), create);
}

TestCAO::~TestCAO()
{
}

ClientActiveObject* TestCAO::create()
{
	return new TestCAO();
}

void TestCAO::addToScene(scene::ISceneManager *smgr)
{
	if(m_node != NULL)
		return;
	
	video::IVideoDriver* driver = smgr->getVideoDriver();
	
	scene::SMesh *mesh = new scene::SMesh();
	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(-BS/2,-BS/4,0, 0,0,0, c, 0,1),
		video::S3DVertex(BS/2,-BS/4,0, 0,0,0, c, 1,1),
		video::S3DVertex(BS/2,BS/4,0, 0,0,0, c, 1,0),
		video::S3DVertex(-BS/2,BS/4,0, 0,0,0, c, 0,0),
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
	buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
	buf->getMaterial().setTexture
			(0, driver->getTexture(porting::getDataPath("rat.png").c_str()));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = smgr->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	updateNodePos();
}

void TestCAO::removeFromScene()
{
	if(m_node == NULL)
		return;

	m_node->remove();
	m_node = NULL;
}

void TestCAO::updateLight(u8 light_at_pos)
{
}

v3s16 TestCAO::getLightPosition()
{
	return floatToInt(m_position, BS);
}

void TestCAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	m_node->setPosition(m_position);
	//m_node->setRotation(v3f(0, 45, 0));
}

void TestCAO::step(float dtime, ClientEnvironment *env)
{
	if(m_node)
	{
		v3f rot = m_node->getRotation();
		//dstream<<"dtime="<<dtime<<", rot.Y="<<rot.Y<<std::endl;
		rot.Y += dtime * 180;
		m_node->setRotation(rot);
	}
}

void TestCAO::processMessage(const std::string &data)
{
	dstream<<"TestCAO: Got data: "<<data<<std::endl;
	std::istringstream is(data, std::ios::binary);
	u16 cmd;
	is>>cmd;
	if(cmd == 0)
	{
		v3f newpos;
		is>>newpos.X;
		is>>newpos.Y;
		is>>newpos.Z;
		m_position = newpos;
		updateNodePos();
	}
}

/*
	ItemCAO
*/

#include "inventory.h"

// Prototype
ItemCAO proto_ItemCAO;

ItemCAO::ItemCAO():
	ClientActiveObject(0),
	m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS*2./3.,BS/3.),
	m_node(NULL),
	m_position(v3f(0,10*BS,0))
{
	ClientActiveObject::registerType(getType(), create);
}

ItemCAO::~ItemCAO()
{
}

ClientActiveObject* ItemCAO::create()
{
	return new ItemCAO();
}

void ItemCAO::addToScene(scene::ISceneManager *smgr)
{
	if(m_node != NULL)
		return;
	
	video::IVideoDriver* driver = smgr->getVideoDriver();
	
	scene::SMesh *mesh = new scene::SMesh();
	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[4] =
	{
		/*video::S3DVertex(-BS/2,-BS/4,0, 0,0,0, c, 0,1),
		video::S3DVertex(BS/2,-BS/4,0, 0,0,0, c, 1,1),
		video::S3DVertex(BS/2,BS/4,0, 0,0,0, c, 1,0),
		video::S3DVertex(-BS/2,BS/4,0, 0,0,0, c, 0,0),*/
		video::S3DVertex(BS/3.,0,0, 0,0,0, c, 0,1),
		video::S3DVertex(-BS/3.,0,0, 0,0,0, c, 1,1),
		video::S3DVertex(-BS/3.,0+BS*2./3.,0, 0,0,0, c, 1,0),
		video::S3DVertex(BS/3.,0+BS*2./3.,0, 0,0,0, c, 0,0),
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
	buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
	//buf->getMaterial().setTexture(0, NULL);
	// Initialize with the stick texture
	buf->getMaterial().setTexture
			(0, driver->getTexture(porting::getDataPath("stick.png").c_str()));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = smgr->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	// Set it to use the materials of the meshbuffers directly.
	// This is needed for changing the texture in the future
	m_node->setReadOnlyMaterials(true);
	updateNodePos();
}

void ItemCAO::removeFromScene()
{
	if(m_node == NULL)
		return;

	m_node->remove();
	m_node = NULL;
}

void ItemCAO::updateLight(u8 light_at_pos)
{
	if(m_node == NULL)
		return;

	u8 li = decode_light(light_at_pos);
	video::SColor color(255,li,li,li);

	scene::IMesh *mesh = m_node->getMesh();
	if(mesh == NULL)
		return;
	
	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			vertices[i].Color = color;
		}
	}
}

v3s16 ItemCAO::getLightPosition()
{
	return floatToInt(m_position, BS);
}

void ItemCAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	m_node->setPosition(m_position);
}

void ItemCAO::step(float dtime, ClientEnvironment *env)
{
	if(m_node)
	{
		/*v3f rot = m_node->getRotation();
		rot.Y += dtime * 120;
		m_node->setRotation(rot);*/
		LocalPlayer *player = env->getLocalPlayer();
		assert(player);
		v3f rot = m_node->getRotation();
		rot.Y = 180.0 - (player->getYaw());
		m_node->setRotation(rot);
	}
}

void ItemCAO::processMessage(const std::string &data)
{
	dstream<<"ItemCAO: Got message"<<std::endl;
	std::istringstream is(data, std::ios::binary);
	// command
	u8 cmd = readU8(is);
	if(cmd == 0)
	{
		// pos
		m_position = readV3F1000(is);
		updateNodePos();
	}
}

void ItemCAO::initialize(const std::string &data)
{
	dstream<<"ItemCAO: Got init data"<<std::endl;
	
	{
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0)
			return;
		// pos
		m_position = readV3F1000(is);
		// inventorystring
		m_inventorystring = deSerializeString(is);
	}
	
	updateNodePos();

	/*
		Update image of node
	*/

	if(m_node == NULL)
		return;

	scene::IMesh *mesh = m_node->getMesh();

	if(mesh == NULL)
		return;
	
	scene::IMeshBuffer *buf = mesh->getMeshBuffer(0);

	if(buf == NULL)
		return;

	// Create an inventory item to see what is its image
	std::istringstream is(m_inventorystring, std::ios_base::binary);
	video::ITexture *texture = NULL;
	try{
		InventoryItem *item = NULL;
		item = InventoryItem::deSerialize(is);
		dstream<<__FUNCTION_NAME<<": m_inventorystring=\""
				<<m_inventorystring<<"\" -> item="<<item
				<<std::endl;
		if(item)
		{
			texture = item->getImage();
			delete item;
		}
	}
	catch(SerializationError &e)
	{
		dstream<<"WARNING: "<<__FUNCTION_NAME
				<<": error deSerializing inventorystring \""
				<<m_inventorystring<<"\""<<std::endl;
	}
	
	// Set meshbuffer texture
	buf->getMaterial().setTexture(0, texture);
	
}

/*
	RatCAO
*/

#include "inventory.h"

// Prototype
RatCAO proto_RatCAO;

RatCAO::RatCAO():
	ClientActiveObject(0),
	m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS/2.,BS/3.),
	m_node(NULL),
	m_position(v3f(0,10*BS,0)),
	m_yaw(0)
{
	ClientActiveObject::registerType(getType(), create);
}

RatCAO::~RatCAO()
{
}

ClientActiveObject* RatCAO::create()
{
	return new RatCAO();
}

void RatCAO::addToScene(scene::ISceneManager *smgr)
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
	//buf->getMaterial().setTexture(0, NULL);
	buf->getMaterial().setTexture
			(0, driver->getTexture(porting::getDataPath("rat.png").c_str()));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = smgr->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	// Set it to use the materials of the meshbuffers directly.
	// This is needed for changing the texture in the future
	m_node->setReadOnlyMaterials(true);
	updateNodePos();
}

void RatCAO::removeFromScene()
{
	if(m_node == NULL)
		return;

	m_node->remove();
	m_node = NULL;
}

void RatCAO::updateLight(u8 light_at_pos)
{
	if(m_node == NULL)
		return;

	u8 li = decode_light(light_at_pos);
	video::SColor color(255,li,li,li);

	scene::IMesh *mesh = m_node->getMesh();
	if(mesh == NULL)
		return;
	
	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			vertices[i].Color = color;
		}
	}
}

v3s16 RatCAO::getLightPosition()
{
	return floatToInt(m_position+v3f(0,BS*0.5,0), BS);
}

void RatCAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	//m_node->setPosition(m_position);
	m_node->setPosition(pos_translator.vect_show);

	v3f rot = m_node->getRotation();
	rot.Y = 180.0 - m_yaw;
	m_node->setRotation(rot);
}

void RatCAO::step(float dtime, ClientEnvironment *env)
{
	pos_translator.translate(dtime);
	updateNodePos();
}

void RatCAO::processMessage(const std::string &data)
{
	//dstream<<"RatCAO: Got message"<<std::endl;
	std::istringstream is(data, std::ios::binary);
	// command
	u8 cmd = readU8(is);
	if(cmd == 0)
	{
		// pos
		m_position = readV3F1000(is);
		pos_translator.update(m_position);
		// yaw
		m_yaw = readF1000(is);
		updateNodePos();
	}
}

void RatCAO::initialize(const std::string &data)
{
	//dstream<<"RatCAO: Got init data"<<std::endl;
	
	{
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0)
			return;
		// pos
		m_position = readV3F1000(is);
		pos_translator.init(m_position);
	}
	
	updateNodePos();
}

/*
	Oerkki1CAO
*/

#include "inventory.h"

// Prototype
Oerkki1CAO proto_Oerkki1CAO;

Oerkki1CAO::Oerkki1CAO():
	ClientActiveObject(0),
	m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS*2.,BS/3.),
	m_node(NULL),
	m_position(v3f(0,10*BS,0)),
	m_yaw(0)
{
	ClientActiveObject::registerType(getType(), create);
}

Oerkki1CAO::~Oerkki1CAO()
{
}

ClientActiveObject* Oerkki1CAO::create()
{
	return new Oerkki1CAO();
}

void Oerkki1CAO::addToScene(scene::ISceneManager *smgr)
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
		video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
		video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
	buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
	//buf->getMaterial().setTexture(0, NULL);
	buf->getMaterial().setTexture
			(0, driver->getTexture(porting::getDataPath("oerkki1.png").c_str()));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = smgr->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	// Set it to use the materials of the meshbuffers directly.
	// This is needed for changing the texture in the future
	m_node->setReadOnlyMaterials(true);
	updateNodePos();
}

void Oerkki1CAO::removeFromScene()
{
	if(m_node == NULL)
		return;

	m_node->remove();
	m_node = NULL;
}

void Oerkki1CAO::updateLight(u8 light_at_pos)
{
	if(m_node == NULL)
		return;

	u8 li = decode_light(light_at_pos);
	video::SColor color(255,li,li,li);

	scene::IMesh *mesh = m_node->getMesh();
	if(mesh == NULL)
		return;
	
	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			vertices[i].Color = color;
		}
	}
}

v3s16 Oerkki1CAO::getLightPosition()
{
	return floatToInt(m_position+v3f(0,BS*1.5,0), BS);
}

void Oerkki1CAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	//m_node->setPosition(m_position);
	m_node->setPosition(pos_translator.vect_show);

	v3f rot = m_node->getRotation();
	rot.Y = 180.0 - m_yaw + 90.0;
	m_node->setRotation(rot);
}

void Oerkki1CAO::step(float dtime, ClientEnvironment *env)
{
	pos_translator.translate(dtime);
	updateNodePos();

	LocalPlayer *player = env->getLocalPlayer();
	assert(player);
	
	v3f playerpos = player->getPosition();
	v2f playerpos_2d(playerpos.X,playerpos.Z);
	v2f objectpos_2d(m_position.X,m_position.Z);

	if(fabs(objectpos_2d.Y - playerpos_2d.Y) < 2.0*BS &&
			objectpos_2d.getDistanceFrom(playerpos_2d) < 1.0*BS)
	{
		if(m_attack_interval.step(dtime, 0.5))
		{
			env->damageLocalPlayer(2);
		}
	}
}

void Oerkki1CAO::processMessage(const std::string &data)
{
	//dstream<<"Oerkki1CAO: Got message"<<std::endl;
	std::istringstream is(data, std::ios::binary);
	// command
	u8 cmd = readU8(is);
	if(cmd == 0)
	{
		// pos
		m_position = readV3F1000(is);
		pos_translator.update(m_position);
		// yaw
		m_yaw = readF1000(is);
		updateNodePos();
	}
}

void Oerkki1CAO::initialize(const std::string &data)
{
	//dstream<<"Oerkki1CAO: Got init data"<<std::endl;
	
	{
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0)
			return;
		// pos
		m_position = readV3F1000(is);
		pos_translator.init(m_position);
	}
	
	updateNodePos();
}


