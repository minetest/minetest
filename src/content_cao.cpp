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

#include "content_cao.h"
#include "tile.h"
#include "environment.h"
#include "collision.h"
#include "settings.h"
#include <ICameraSceneNode.h>
#include <ITextSceneNode.h>
#include <IBillboardSceneNode.h>
#include "serialization.h" // For decompressZlib
#include "gamedef.h"
#include "clientobject.h"
#include "content_object.h"
#include "mesh.h"
#include "utility.h" // For IntervalLimiter
class Settings;

core::map<u16, ClientActiveObject::Factory> ClientActiveObject::m_types;

/*
	SmoothTranslator
*/

struct SmoothTranslator
{
	v3f vect_old;
	v3f vect_show;
	v3f vect_aim;
	f32 anim_counter;
	f32 anim_time;
	f32 anim_time_counter;
	bool aim_is_end;

	SmoothTranslator():
		vect_old(0,0,0),
		vect_show(0,0,0),
		vect_aim(0,0,0),
		anim_counter(0),
		anim_time(0),
		anim_time_counter(0),
		aim_is_end(true)
	{}

	void init(v3f vect)
	{
		vect_old = vect;
		vect_show = vect;
		vect_aim = vect;
		anim_counter = 0;
		anim_time = 0;
		anim_time_counter = 0;
		aim_is_end = true;
	}

	void sharpen()
	{
		init(vect_show);
	}

	void update(v3f vect_new, bool is_end_position=false, float update_interval=-1)
	{
		aim_is_end = is_end_position;
		vect_old = vect_show;
		vect_aim = vect_new;
		if(update_interval > 0){
			anim_time = update_interval;
		} else {
			if(anim_time < 0.001 || anim_time > 1.0)
				anim_time = anim_time_counter;
			else
				anim_time = anim_time * 0.9 + anim_time_counter * 0.1;
		}
		anim_time_counter = 0;
		anim_counter = 0;
	}

	void translate(f32 dtime)
	{
		anim_time_counter = anim_time_counter + dtime;
		anim_counter = anim_counter + dtime;
		v3f vect_move = vect_aim - vect_old;
		f32 moveratio = 1.0;
		if(anim_time > 0.001)
			moveratio = anim_time_counter / anim_time;
		// Move a bit less than should, to avoid oscillation
		moveratio = moveratio * 0.8;
		float move_end = 1.5;
		if(aim_is_end)
			move_end = 1.0;
		if(moveratio > move_end)
			moveratio = move_end;
		vect_show = vect_old + vect_move * moveratio;
	}

	bool is_moving()
	{
		return ((anim_time_counter / anim_time) < 1.4);
	}
};


/*
	TestCAO
*/

class TestCAO : public ClientActiveObject
{
public:
	TestCAO(IGameDef *gamedef, ClientEnvironment *env);
	virtual ~TestCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_TEST;
	}
	
	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env);

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

private:
	scene::IMeshSceneNode *m_node;
	v3f m_position;
};

/*
	ItemCAO
*/

class ItemCAO : public ClientActiveObject
{
public:
	ItemCAO(IGameDef *gamedef, ClientEnvironment *env);
	virtual ~ItemCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_ITEM;
	}
	
	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env);

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();
	void updateInfoText();
	void updateTexture();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return m_position;}
	
	std::string infoText()
		{return m_infotext;}

private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	std::string m_itemstring;
	std::string m_infotext;
};

/*
	RatCAO
*/

class RatCAO : public ClientActiveObject
{
public:
	RatCAO(IGameDef *gamedef, ClientEnvironment *env);
	virtual ~RatCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_RAT;
	}
	
	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env);

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return pos_translator.vect_show;}
		//{return m_position;}

private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
};

/*
	Oerkki1CAO
*/

class Oerkki1CAO : public ClientActiveObject
{
public:
	Oerkki1CAO(IGameDef *gamedef, ClientEnvironment *env);
	virtual ~Oerkki1CAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_OERKKI1;
	}
	
	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env);

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return pos_translator.vect_show;}
		//{return m_position;}

	// If returns true, punch will not be sent to the server
	bool directReportPunch(const std::string &toolname, v3f dir);

private:
	IntervalLimiter m_attack_interval;
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
	float m_damage_visual_timer;
	bool m_damage_texture_enabled;
};

/*
	FireflyCAO
*/

class FireflyCAO : public ClientActiveObject
{
public:
	FireflyCAO(IGameDef *gamedef, ClientEnvironment *env);
	virtual ~FireflyCAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_FIREFLY;
	}
	
	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env);

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return m_position;}

private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
};

static void setBillboardTextureMatrix(scene::IBillboardSceneNode *bill,
		float txs, float tys, int col, int row)
{
	video::SMaterial& material = bill->getMaterial(0);
	core::matrix4& matrix = material.getTextureMatrix(0);
	matrix.setTextureTranslate(txs*col, tys*row);
	matrix.setTextureScale(txs, tys);
}

/*
	MobV2CAO
*/

class MobV2CAO : public ClientActiveObject
{
public:
	MobV2CAO(IGameDef *gamedef, ClientEnvironment *env);
	virtual ~MobV2CAO();
	
	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_MOBV2;
	}
	
	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env);

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr);
	void removeFromScene();
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	void initialize(const std::string &data);
	
	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}
	v3f getPosition()
		{return pos_translator.vect_show;}
		//{return m_position;}
	bool doShowSelectionBox(){return false;}

	// If returns true, punch will not be sent to the server
	bool directReportPunch(const std::string &toolname, v3f dir);

private:
	void setLooks(const std::string &looks);
	
	IntervalLimiter m_attack_interval;
	core::aabbox3d<f32> m_selection_box;
	scene::IBillboardSceneNode *m_node;
	v3f m_position;
	std::string m_texture_name;
	float m_yaw;
	SmoothTranslator pos_translator;
	bool m_walking;
	float m_walking_unset_timer;
	float m_walk_timer;
	int m_walk_frame;
	float m_damage_visual_timer;
	u8 m_last_light;
	bool m_shooting;
	float m_shooting_unset_timer;
	v2f m_sprite_size;
	float m_sprite_y;
	bool m_bright_shooting;
	std::string m_sprite_type;
	int m_simple_anim_frames;
	float m_simple_anim_frametime;
	bool m_lock_full_brightness;
	int m_player_hit_damage;
	float m_player_hit_distance;
	float m_player_hit_interval;
	float m_player_hit_timer;

	Settings *m_properties;
};

/*
	TestCAO
*/

// Prototype
TestCAO proto_TestCAO(NULL, NULL);

TestCAO::TestCAO(IGameDef *gamedef, ClientEnvironment *env):
	ClientActiveObject(0, gamedef, env),
	m_node(NULL),
	m_position(v3f(0,10*BS,0))
{
	ClientActiveObject::registerType(getType(), create);
}

TestCAO::~TestCAO()
{
}

ClientActiveObject* TestCAO::create(IGameDef *gamedef, ClientEnvironment *env)
{
	return new TestCAO(gamedef, env);
}

void TestCAO::addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
{
	if(m_node != NULL)
		return;
	
	//video::IVideoDriver* driver = smgr->getVideoDriver();
	
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
	buf->getMaterial().setTexture(0, tsrc->getTextureRaw("rat.png"));
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
		//infostream<<"dtime="<<dtime<<", rot.Y="<<rot.Y<<std::endl;
		rot.Y += dtime * 180;
		m_node->setRotation(rot);
	}
}

void TestCAO::processMessage(const std::string &data)
{
	infostream<<"TestCAO: Got data: "<<data<<std::endl;
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
ItemCAO proto_ItemCAO(NULL, NULL);

ItemCAO::ItemCAO(IGameDef *gamedef, ClientEnvironment *env):
	ClientActiveObject(0, gamedef, env),
	m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS*2./3.,BS/3.),
	m_node(NULL),
	m_position(v3f(0,10*BS,0))
{
	if(!gamedef && !env)
	{
		ClientActiveObject::registerType(getType(), create);
	}
}

ItemCAO::~ItemCAO()
{
}

ClientActiveObject* ItemCAO::create(IGameDef *gamedef, ClientEnvironment *env)
{
	return new ItemCAO(gamedef, env);
}

void ItemCAO::addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
{
	if(m_node != NULL)
		return;
	
	//video::IVideoDriver* driver = smgr->getVideoDriver();
	
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
	// Initialize with a generated placeholder texture
	buf->getMaterial().setTexture(0, tsrc->getTextureRaw(""));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = smgr->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	updateNodePos();

	/*
		Update image of node
	*/

	updateTexture();
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
	setMeshColor(m_node->getMesh(), color);
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

void ItemCAO::updateInfoText()
{
	try{
		IItemDefManager *idef = m_gamedef->idef();
		ItemStack item;
		item.deSerialize(m_itemstring, idef);
		if(item.isKnown(idef))
			m_infotext = item.getDefinition(idef).description;
		else
			m_infotext = "Unknown item: '" + m_itemstring + "'";
		if(item.count >= 2)
			m_infotext += " (" + itos(item.count) + ")";
	}
	catch(SerializationError &e)
	{
		m_infotext = "Unknown item: '" + m_itemstring + "'";
	}
}

void ItemCAO::updateTexture()
{
	if(m_node == NULL)
		return;

	// Create an inventory item to see what is its image
	std::istringstream is(m_itemstring, std::ios_base::binary);
	video::ITexture *texture = NULL;
	try{
		IItemDefManager *idef = m_gamedef->idef();
		ItemStack item;
		item.deSerialize(is, idef);
		texture = item.getDefinition(idef).inventory_texture;
	}
	catch(SerializationError &e)
	{
		infostream<<"WARNING: "<<__FUNCTION_NAME
				<<": error deSerializing itemstring \""
				<<m_itemstring<<std::endl;
	}
	
	// Set meshbuffer texture
	m_node->getMaterial(0).setTexture(0, texture);
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
	//infostream<<"ItemCAO: Got message"<<std::endl;
	std::istringstream is(data, std::ios::binary);
	// command
	u8 cmd = readU8(is);
	if(cmd == 0)
	{
		// pos
		m_position = readV3F1000(is);
		updateNodePos();
	}
	if(cmd == 1)
	{
		// itemstring
		m_itemstring = deSerializeString(is);
		updateInfoText();
		updateTexture();
	}
}

void ItemCAO::initialize(const std::string &data)
{
	infostream<<"ItemCAO: Got init data"<<std::endl;
	
	{
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0)
			return;
		// pos
		m_position = readV3F1000(is);
		// itemstring
		m_itemstring = deSerializeString(is);
	}
	
	updateNodePos();
	updateInfoText();
}

/*
	RatCAO
*/

#include "inventory.h"

// Prototype
RatCAO proto_RatCAO(NULL, NULL);

RatCAO::RatCAO(IGameDef *gamedef, ClientEnvironment *env):
	ClientActiveObject(0, gamedef, env),
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

ClientActiveObject* RatCAO::create(IGameDef *gamedef, ClientEnvironment *env)
{
	return new RatCAO(gamedef, env);
}

void RatCAO::addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
{
	if(m_node != NULL)
		return;
	
	//video::IVideoDriver* driver = smgr->getVideoDriver();
	
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
	buf->getMaterial().setTexture(0, tsrc->getTextureRaw("rat.png"));
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
	setMeshColor(m_node->getMesh(), color);
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
	//infostream<<"RatCAO: Got message"<<std::endl;
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
	//infostream<<"RatCAO: Got init data"<<std::endl;
	
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
Oerkki1CAO proto_Oerkki1CAO(NULL, NULL);

Oerkki1CAO::Oerkki1CAO(IGameDef *gamedef, ClientEnvironment *env):
	ClientActiveObject(0, gamedef, env),
	m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS*2.,BS/3.),
	m_node(NULL),
	m_position(v3f(0,10*BS,0)),
	m_yaw(0),
	m_damage_visual_timer(0),
	m_damage_texture_enabled(false)
{
	ClientActiveObject::registerType(getType(), create);
}

Oerkki1CAO::~Oerkki1CAO()
{
}

ClientActiveObject* Oerkki1CAO::create(IGameDef *gamedef, ClientEnvironment *env)
{
	return new Oerkki1CAO(gamedef, env);
}

void Oerkki1CAO::addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
{
	if(m_node != NULL)
		return;
	
	//video::IVideoDriver* driver = smgr->getVideoDriver();
	
	scene::SMesh *mesh = new scene::SMesh();
	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(-BS/2-BS,0,0, 0,0,0, c, 0,1),
		video::S3DVertex(BS/2+BS,0,0, 0,0,0, c, 1,1),
		video::S3DVertex(BS/2+BS,BS*2,0, 0,0,0, c, 1,0),
		video::S3DVertex(-BS/2-BS,BS*2,0, 0,0,0, c, 0,0),
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
	buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
	//buf->getMaterial().setTexture(0, NULL);
	buf->getMaterial().setTexture(0, tsrc->getTextureRaw("oerkki1.png"));
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
	
	if(light_at_pos <= 2)
	{
		m_node->setVisible(false);
		return;
	}

	m_node->setVisible(true);

	u8 li = decode_light(light_at_pos);
	video::SColor color(255,li,li,li);
	setMeshColor(m_node->getMesh(), color);
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
	ITextureSource *tsrc = m_gamedef->tsrc();

	pos_translator.translate(dtime);
	updateNodePos();

	LocalPlayer *player = env->getLocalPlayer();
	assert(player);
	
	v3f playerpos = player->getPosition();
	v2f playerpos_2d(playerpos.X,playerpos.Z);
	v2f objectpos_2d(m_position.X,m_position.Z);

	if(fabs(m_position.Y - playerpos.Y) < 1.5*BS &&
			objectpos_2d.getDistanceFrom(playerpos_2d) < 1.5*BS)
	{
		if(m_attack_interval.step(dtime, 0.5))
		{
			env->damageLocalPlayer(2);
		}
	}

	if(m_damage_visual_timer > 0)
	{
		if(!m_damage_texture_enabled)
		{
			// Enable damage texture
			if(m_node)
			{
				/*video::IVideoDriver* driver =
					m_node->getSceneManager()->getVideoDriver();*/
				
				scene::IMesh *mesh = m_node->getMesh();
				if(mesh == NULL)
					return;
				
				u16 mc = mesh->getMeshBufferCount();
				for(u16 j=0; j<mc; j++)
				{
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
					buf->getMaterial().setTexture(0,
							tsrc->getTextureRaw("oerkki1_damaged.png"));
				}
			}
			m_damage_texture_enabled = true;
		}
		m_damage_visual_timer -= dtime;
	}
	else
	{
		if(m_damage_texture_enabled)
		{
			// Disable damage texture
			if(m_node)
			{
				/*video::IVideoDriver* driver =
					m_node->getSceneManager()->getVideoDriver();*/
				
				scene::IMesh *mesh = m_node->getMesh();
				if(mesh == NULL)
					return;
				
				u16 mc = mesh->getMeshBufferCount();
				for(u16 j=0; j<mc; j++)
				{
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
					buf->getMaterial().setTexture(0,
							tsrc->getTextureRaw("oerkki1.png"));
				}
			}
			m_damage_texture_enabled = false;
		}
	}
}

void Oerkki1CAO::processMessage(const std::string &data)
{
	//infostream<<"Oerkki1CAO: Got message"<<std::endl;
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
	else if(cmd == 1)
	{
		//u16 damage = readU8(is);
		m_damage_visual_timer = 1.0;
	}
}

void Oerkki1CAO::initialize(const std::string &data)
{
	//infostream<<"Oerkki1CAO: Got init data"<<std::endl;
	
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

bool Oerkki1CAO::directReportPunch(const std::string &toolname, v3f dir)
{
	m_damage_visual_timer = 1.0;

	m_position += dir * BS;
	pos_translator.sharpen();
	pos_translator.update(m_position);
	updateNodePos();
	
	return false;
}

/*
	FireflyCAO
*/

// Prototype
FireflyCAO proto_FireflyCAO(NULL, NULL);

FireflyCAO::FireflyCAO(IGameDef *gamedef, ClientEnvironment *env):
	ClientActiveObject(0, gamedef, env),
	m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS/2.,BS/3.),
	m_node(NULL),
	m_position(v3f(0,10*BS,0)),
	m_yaw(0)
{
	ClientActiveObject::registerType(getType(), create);
}

FireflyCAO::~FireflyCAO()
{
}

ClientActiveObject* FireflyCAO::create(IGameDef *gamedef, ClientEnvironment *env)
{
	return new FireflyCAO(gamedef, env);
}

void FireflyCAO::addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
{
	if(m_node != NULL)
		return;
	
	//video::IVideoDriver* driver = smgr->getVideoDriver();
	
	scene::SMesh *mesh = new scene::SMesh();
	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(0,0,0, 0,0,0, c, 0,1),
		video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
		video::S3DVertex(BS/2,BS/2,0, 0,0,0, c, 1,0),
		video::S3DVertex(0,BS/2,0, 0,0,0, c, 0,0),
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
	buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
	//buf->getMaterial().setTexture(0, NULL);
	buf->getMaterial().setTexture(0, tsrc->getTextureRaw("firefly.png"));
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

void FireflyCAO::removeFromScene()
{
	if(m_node == NULL)
		return;

	m_node->remove();
	m_node = NULL;
}

void FireflyCAO::updateLight(u8 light_at_pos)
{
	if(m_node == NULL)
		return;

	u8 li = 255;
	video::SColor color(255,li,li,li);
	setMeshColor(m_node->getMesh(), color);
}

v3s16 FireflyCAO::getLightPosition()
{
	return floatToInt(m_position+v3f(0,BS*0.5,0), BS);
}

void FireflyCAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	//m_node->setPosition(m_position);
	m_node->setPosition(pos_translator.vect_show);

	v3f rot = m_node->getRotation();
	rot.Y = 180.0 - m_yaw;
	m_node->setRotation(rot);
}

void FireflyCAO::step(float dtime, ClientEnvironment *env)
{
	pos_translator.translate(dtime);
	updateNodePos();
}

void FireflyCAO::processMessage(const std::string &data)
{
	//infostream<<"FireflyCAO: Got message"<<std::endl;
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

void FireflyCAO::initialize(const std::string &data)
{
	//infostream<<"FireflyCAO: Got init data"<<std::endl;
	
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
	MobV2CAO
*/

// Prototype
MobV2CAO proto_MobV2CAO(NULL, NULL);

MobV2CAO::MobV2CAO(IGameDef *gamedef, ClientEnvironment *env):
	ClientActiveObject(0, gamedef, env),
	m_selection_box(-0.4*BS,-0.4*BS,-0.4*BS, 0.4*BS,0.8*BS,0.4*BS),
	m_node(NULL),
	m_position(v3f(0,10*BS,0)),
	m_yaw(0),
	m_walking(false),
	m_walking_unset_timer(0),
	m_walk_timer(0),
	m_walk_frame(0),
	m_damage_visual_timer(0),
	m_last_light(0),
	m_shooting(0),
	m_shooting_unset_timer(0),
	m_sprite_size(BS,BS),
	m_sprite_y(0),
	m_bright_shooting(false),
	m_lock_full_brightness(false),
	m_player_hit_timer(0)
{
	ClientActiveObject::registerType(getType(), create);

	m_properties = new Settings;
}

MobV2CAO::~MobV2CAO()
{
	delete m_properties;
}

ClientActiveObject* MobV2CAO::create(IGameDef *gamedef, ClientEnvironment *env)
{
	return new MobV2CAO(gamedef, env);
}

void MobV2CAO::addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
{
	if(m_node != NULL)
		return;
	
	/*infostream<<"MobV2CAO::addToScene using texture_name="<<
			m_texture_name<<std::endl;*/
	std::string texture_string = m_texture_name +
			"^[makealpha:128,0,0^[makealpha:128,128,0";
	
	scene::IBillboardSceneNode *bill = smgr->addBillboardSceneNode(
			NULL, v2f(1, 1), v3f(0,0,0), -1);
	bill->setMaterialTexture(0, tsrc->getTextureRaw(texture_string));
	bill->setMaterialFlag(video::EMF_LIGHTING, false);
	bill->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
	bill->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
	bill->setMaterialFlag(video::EMF_FOG_ENABLE, true);
	bill->setColor(video::SColor(255,0,0,0));
	bill->setVisible(false); /* Set visible when brightness is known */
	bill->setSize(m_sprite_size);
	if(m_sprite_type == "humanoid_1"){
		const float txp = 1./192;
		const float txs = txp*32;
		const float typ = 1./240;
		const float tys = typ*48;
		setBillboardTextureMatrix(bill, txs, tys, 0, 0);
	} else if(m_sprite_type == "simple"){
		const float txs = 1.0;
		const float tys = 1.0 / m_simple_anim_frames;
		setBillboardTextureMatrix(bill, txs, tys, 0, 0);
	} else {
		infostream<<"MobV2CAO: Unknown sprite type \""<<m_sprite_type<<"\""
				<<std::endl;
	}

	m_node = bill;

	updateNodePos();
}

void MobV2CAO::removeFromScene()
{
	if(m_node == NULL)
		return;

	m_node->remove();
	m_node = NULL;
}

void MobV2CAO::updateLight(u8 light_at_pos)
{
	if(m_lock_full_brightness)
		light_at_pos = 15;
	
	m_last_light = light_at_pos;

	if(m_node == NULL)
		return;
	
	if(m_damage_visual_timer > 0)
		return;
	
	if(m_shooting && m_bright_shooting)
		return;
	
	/*if(light_at_pos <= 2){
		m_node->setVisible(false);
		return;
	}*/

	m_node->setVisible(true);

	u8 li = decode_light(light_at_pos);
	video::SColor color(255,li,li,li);
	m_node->setColor(color);
}

v3s16 MobV2CAO::getLightPosition()
{
	return floatToInt(m_position+v3f(0,0,0), BS);
}

void MobV2CAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	m_node->setPosition(pos_translator.vect_show + v3f(0,m_sprite_y,0));
}

void MobV2CAO::step(float dtime, ClientEnvironment *env)
{
	scene::IBillboardSceneNode *bill = m_node;
	if(!bill)
		return;

	pos_translator.translate(dtime);
	
	if(m_sprite_type == "humanoid_1"){
		scene::ICameraSceneNode* camera = m_node->getSceneManager()->getActiveCamera();
		if(!camera)
			return;
		v3f cam_to_mob = m_node->getAbsolutePosition() - camera->getAbsolutePosition();
		cam_to_mob.normalize();
		int col = 0;
		if(cam_to_mob.Y > 0.75)
			col = 5;
		else if(cam_to_mob.Y < -0.75)
			col = 4;
		else{
			float mob_dir = atan2(cam_to_mob.Z, cam_to_mob.X) / PI * 180.;
			float dir = mob_dir - m_yaw;
			dir = wrapDegrees_180(dir);
			//infostream<<"id="<<m_id<<" dir="<<dir<<std::endl;
			if(fabs(wrapDegrees_180(dir - 0)) <= 45.1)
				col = 2;
			else if(fabs(wrapDegrees_180(dir - 90)) <= 45.1)
				col = 3;
			else if(fabs(wrapDegrees_180(dir - 180)) <= 45.1)
				col = 0;
			else if(fabs(wrapDegrees_180(dir + 90)) <= 45.1)
				col = 1;
			else
				col = 4;
		}

		int row = 0;
		if(m_shooting){
			row = 3;
		} else if(m_walking){
			m_walk_timer += dtime;
			if(m_walk_timer >= 0.5){
				m_walk_frame = (m_walk_frame + 1) % 2;
				m_walk_timer = 0;
			}
			if(m_walk_frame == 0)
				row = 1;
			else
				row = 2;
		}

		const float txp = 1./192;
		const float txs = txp*32;
		const float typ = 1./240;
		const float tys = typ*48;
		setBillboardTextureMatrix(bill, txs, tys, col, row);
	} else if(m_sprite_type == "simple"){
		m_walk_timer += dtime;
		if(m_walk_timer >= m_simple_anim_frametime){
			m_walk_frame = (m_walk_frame + 1) % m_simple_anim_frames;
			m_walk_timer = 0;
		}
		int col = 0;
		int row = m_walk_frame;
		const float txs = 1.0;
		const float tys = 1.0 / m_simple_anim_frames;
		setBillboardTextureMatrix(bill, txs, tys, col, row);
	} else {
		infostream<<"MobV2CAO::step(): Unknown sprite type \""
				<<m_sprite_type<<"\""<<std::endl;
	}

	updateNodePos();

	/* Damage local player */
	if(m_player_hit_damage && m_player_hit_timer <= 0.0){
		LocalPlayer *player = env->getLocalPlayer();
		assert(player);
		
		v3f playerpos = player->getPosition();
		v2f playerpos_2d(playerpos.X,playerpos.Z);
		v2f objectpos_2d(m_position.X,m_position.Z);

		if(fabs(m_position.Y - playerpos.Y) < m_player_hit_distance*BS &&
		objectpos_2d.getDistanceFrom(playerpos_2d) < m_player_hit_distance*BS)
		{
			env->damageLocalPlayer(m_player_hit_damage);
			m_player_hit_timer = m_player_hit_interval;
		}
	}

	/* Run timers */

	m_player_hit_timer -= dtime;

	if(m_damage_visual_timer >= 0){
		m_damage_visual_timer -= dtime;
		if(m_damage_visual_timer <= 0){
			infostream<<"id="<<m_id<<" damage visual ended"<<std::endl;
		}
	}

	m_walking_unset_timer += dtime;
	if(m_walking_unset_timer >= 1.0){
		m_walking = false;
	}

	m_shooting_unset_timer -= dtime;
	if(m_shooting_unset_timer <= 0.0){
		if(m_bright_shooting){
			u8 li = decode_light(m_last_light);
			video::SColor color(255,li,li,li);
			bill->setColor(color);
			m_bright_shooting = false;
		}
		m_shooting = false;
	}

}

void MobV2CAO::processMessage(const std::string &data)
{
	//infostream<<"MobV2CAO: Got message"<<std::endl;
	std::istringstream is(data, std::ios::binary);
	// command
	u8 cmd = readU8(is);

	// Move
	if(cmd == 0)
	{
		// pos
		m_position = readV3F1000(is);
		pos_translator.update(m_position);
		// yaw
		m_yaw = readF1000(is);

		m_walking = true;
		m_walking_unset_timer = 0;

		updateNodePos();
	}
	// Damage
	else if(cmd == 1)
	{
		//u16 damage = readU16(is);

		/*u8 li = decode_light(m_last_light);
		if(li >= 100)
			li = 30;
		else
			li = 255;*/

		/*video::SColor color(255,255,0,0);
		m_node->setColor(color);

		m_damage_visual_timer = 0.2;*/
	}
	// Trigger shooting
	else if(cmd == 2)
	{
		// length
		m_shooting_unset_timer = readF1000(is);
		// bright?
		m_bright_shooting = readU8(is);
		if(m_bright_shooting){
			u8 li = 255;
			video::SColor color(255,li,li,li);
			m_node->setColor(color);
		}

		m_shooting = true;
	}
}

void MobV2CAO::initialize(const std::string &data)
{
	//infostream<<"MobV2CAO: Got init data"<<std::endl;
	
	{
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0){
			infostream<<__FUNCTION_NAME<<": Invalid version"<<std::endl;
			return;
		}
		
		std::ostringstream tmp_os(std::ios::binary);
		decompressZlib(is, tmp_os);
		std::istringstream tmp_is(tmp_os.str(), std::ios::binary);
		m_properties->parseConfigLines(tmp_is, "MobArgsEnd");

		infostream<<"MobV2CAO::initialize(): got properties:"<<std::endl;
		m_properties->writeLines(infostream);
		
		m_properties->setDefault("looks", "dummy_default");
		m_properties->setDefault("yaw", "0");
		m_properties->setDefault("pos", "(0,0,0)");
		m_properties->setDefault("player_hit_damage", "0");
		m_properties->setDefault("player_hit_distance", "1.5");
		m_properties->setDefault("player_hit_interval", "1.5");
		
		setLooks(m_properties->get("looks"));
		m_yaw = m_properties->getFloat("yaw");
		m_position = m_properties->getV3F("pos");
		m_player_hit_damage = m_properties->getS32("player_hit_damage");
		m_player_hit_distance = m_properties->getFloat("player_hit_distance");
		m_player_hit_interval = m_properties->getFloat("player_hit_interval");

		pos_translator.init(m_position);
	}
	
	updateNodePos();
}

bool MobV2CAO::directReportPunch(const std::string &toolname, v3f dir)
{
	video::SColor color(255,255,0,0);
	m_node->setColor(color);

	m_damage_visual_timer = 0.05;

	m_position += dir * BS;
	pos_translator.sharpen();
	pos_translator.update(m_position);
	updateNodePos();
	
	return false;
}

void MobV2CAO::setLooks(const std::string &looks)
{
	v2f selection_size = v2f(0.4, 0.4) * BS;
	float selection_y = 0 * BS;

	if(looks == "dungeon_master"){
		m_texture_name = "dungeon_master.png";
		m_sprite_type = "humanoid_1";
		m_sprite_size = v2f(2, 3) * BS;
		m_sprite_y = 0.85 * BS;
		selection_size = v2f(0.4, 2.6) * BS;
		selection_y = -0.4 * BS;
	}
	else if(looks == "fireball"){
		m_texture_name = "fireball.png";
		m_sprite_type = "simple";
		m_sprite_size = v2f(1, 1) * BS;
		m_simple_anim_frames = 3;
		m_simple_anim_frametime = 0.1;
		m_lock_full_brightness = true;
	}
	else{
		m_texture_name = "stone.png";
		m_sprite_type = "simple";
		m_sprite_size = v2f(1, 1) * BS;
		m_simple_anim_frames = 3;
		m_simple_anim_frametime = 0.333;
		selection_size = v2f(0.4, 0.4) * BS;
		selection_y = 0 * BS;
	}

	m_selection_box = core::aabbox3d<f32>(
			-selection_size.X, selection_y, -selection_size.X,
			selection_size.X, selection_y+selection_size.Y,
			selection_size.X);
}

/*
	LuaEntityCAO
*/

#include "luaentity_common.h"

class LuaEntityCAO : public ClientActiveObject
{
private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_meshnode;
	scene::IBillboardSceneNode *m_spritenode;
	v3f m_position;
	v3f m_velocity;
	v3f m_acceleration;
	float m_yaw;
	struct LuaEntityProperties *m_prop;
	SmoothTranslator pos_translator;
	// Spritesheet/animation stuff
	v2f m_tx_size;
	v2s16 m_tx_basepos;
	bool m_tx_select_horiz_by_yawpitch;
	int m_anim_frame;
	int m_anim_num_frames;
	float m_anim_framelength;
	float m_anim_timer;

public:
	LuaEntityCAO(IGameDef *gamedef, ClientEnvironment *env):
		ClientActiveObject(0, gamedef, env),
		m_selection_box(-BS/3.,-BS/3.,-BS/3., BS/3.,BS/3.,BS/3.),
		m_meshnode(NULL),
		m_spritenode(NULL),
		m_position(v3f(0,10*BS,0)),
		m_velocity(v3f(0,0,0)),
		m_acceleration(v3f(0,0,0)),
		m_yaw(0),
		m_prop(new LuaEntityProperties),
		m_tx_size(1,1),
		m_tx_basepos(0,0),
		m_tx_select_horiz_by_yawpitch(false),
		m_anim_frame(0),
		m_anim_num_frames(1),
		m_anim_framelength(0.2),
		m_anim_timer(0)
	{
		if(gamedef == NULL)
			ClientActiveObject::registerType(getType(), create);
	}

	void initialize(const std::string &data)
	{
		infostream<<"LuaEntityCAO: Got init data"<<std::endl;
		
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0)
			return;
		// pos
		m_position = readV3F1000(is);
		// yaw
		m_yaw = readF1000(is);
		// properties
		std::istringstream prop_is(deSerializeLongString(is), std::ios::binary);
		m_prop->deSerialize(prop_is);

		infostream<<"m_prop: "<<m_prop->dump()<<std::endl;

		m_selection_box = m_prop->collisionbox;
		m_selection_box.MinEdge *= BS;
		m_selection_box.MaxEdge *= BS;
			
		pos_translator.init(m_position);

		m_tx_size.X = 1.0 / m_prop->spritediv.X;
		m_tx_size.Y = 1.0 / m_prop->spritediv.Y;
		m_tx_basepos.X = m_tx_size.X * m_prop->initial_sprite_basepos.X;
		m_tx_basepos.Y = m_tx_size.Y * m_prop->initial_sprite_basepos.Y;
		
		updateNodePos();
	}

	~LuaEntityCAO()
	{
		delete m_prop;
	}

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
	{
		return new LuaEntityCAO(gamedef, env);
	}

	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_LUAENTITY;
	}
	core::aabbox3d<f32>* getSelectionBox()
	{
		return &m_selection_box;
	}
	v3f getPosition()
	{
		return pos_translator.vect_show;
	}
		
	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
	{
		if(m_meshnode != NULL || m_spritenode != NULL)
			return;
		
		//video::IVideoDriver* driver = smgr->getVideoDriver();

		if(m_prop->visual == "sprite"){
			infostream<<"LuaEntityCAO::addToScene(): single_sprite"<<std::endl;
			m_spritenode = smgr->addBillboardSceneNode(
					NULL, v2f(1, 1), v3f(0,0,0), -1);
			m_spritenode->setMaterialTexture(0,
					tsrc->getTextureRaw("unknown_block.png"));
			m_spritenode->setMaterialFlag(video::EMF_LIGHTING, false);
			m_spritenode->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
			m_spritenode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
			m_spritenode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
			m_spritenode->setColor(video::SColor(255,0,0,0));
			m_spritenode->setVisible(false); /* Set visible when brightness is known */
			m_spritenode->setSize(m_prop->visual_size*BS);
			{
				const float txs = 1.0 / 1;
				const float tys = 1.0 / 1;
				setBillboardTextureMatrix(m_spritenode,
						txs, tys, 0, 0);
			}
		} else if(m_prop->visual == "cube"){
			infostream<<"LuaEntityCAO::addToScene(): cube"<<std::endl;
			scene::IMesh *mesh = createCubeMesh(v3f(BS,BS,BS));
			m_meshnode = smgr->addMeshSceneNode(mesh, NULL);
			mesh->drop();
			
			m_meshnode->setScale(v3f(1));
			// Will be shown when we know the brightness
			m_meshnode->setVisible(false);
		} else {
			infostream<<"LuaEntityCAO::addToScene(): \""<<m_prop->visual
					<<"\" not supported"<<std::endl;
		}
		updateTextures("");
		updateNodePos();
	}

	void removeFromScene()
	{
		if(m_meshnode){
			m_meshnode->remove();
			m_meshnode = NULL;
		}
		if(m_spritenode){
			m_spritenode->remove();
			m_spritenode = NULL;
		}
	}

	void updateLight(u8 light_at_pos)
	{
		u8 li = decode_light(light_at_pos);
		video::SColor color(255,li,li,li);
		if(m_meshnode){
			setMeshColor(m_meshnode->getMesh(), color);
			m_meshnode->setVisible(true);
		}
		if(m_spritenode){
			m_spritenode->setColor(color);
			m_spritenode->setVisible(true);
		}
	}

	v3s16 getLightPosition()
	{
		return floatToInt(m_position, BS);
	}

	void updateNodePos()
	{
		if(m_meshnode){
			m_meshnode->setPosition(pos_translator.vect_show);
		}
		if(m_spritenode){
			m_spritenode->setPosition(pos_translator.vect_show);
		}
	}

	void step(float dtime, ClientEnvironment *env)
	{
		if(m_prop->physical){
			core::aabbox3d<f32> box = m_prop->collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			collisionMoveResult moveresult;
			f32 pos_max_d = BS*0.25; // Distance per iteration
			v3f p_pos = m_position;
			v3f p_velocity = m_velocity;
			IGameDef *gamedef = env->getGameDef();
			moveresult = collisionMovePrecise(&env->getMap(), gamedef,
					pos_max_d, box, dtime, p_pos, p_velocity);
			// Apply results
			m_position = p_pos;
			m_velocity = p_velocity;
			
			bool is_end_position = moveresult.collides;
			pos_translator.update(m_position, is_end_position, dtime);
			pos_translator.translate(dtime);
			updateNodePos();

			m_velocity += dtime * m_acceleration;
		} else {
			m_position += dtime * m_velocity + 0.5 * dtime * dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
			pos_translator.update(m_position, pos_translator.aim_is_end, pos_translator.anim_time);
			pos_translator.translate(dtime);
			updateNodePos();
		}

		m_anim_timer += dtime;
		if(m_anim_timer >= m_anim_framelength){
			m_anim_timer -= m_anim_framelength;
			m_anim_frame++;
			if(m_anim_frame >= m_anim_num_frames)
				m_anim_frame = 0;
		}

		updateTexturePos();
	}

	void updateTexturePos()
	{
		if(m_spritenode){
			scene::ICameraSceneNode* camera =
					m_spritenode->getSceneManager()->getActiveCamera();
			if(!camera)
				return;
			v3f cam_to_entity = m_spritenode->getAbsolutePosition()
					- camera->getAbsolutePosition();
			cam_to_entity.normalize();

			int row = m_tx_basepos.Y;
			int col = m_tx_basepos.X;
			
			if(m_tx_select_horiz_by_yawpitch)
			{
				if(cam_to_entity.Y > 0.75)
					col += 5;
				else if(cam_to_entity.Y < -0.75)
					col += 4;
				else{
					float mob_dir = atan2(cam_to_entity.Z, cam_to_entity.X) / PI * 180.;
					float dir = mob_dir - m_yaw;
					dir = wrapDegrees_180(dir);
					//infostream<<"id="<<m_id<<" dir="<<dir<<std::endl;
					if(fabs(wrapDegrees_180(dir - 0)) <= 45.1)
						col += 2;
					else if(fabs(wrapDegrees_180(dir - 90)) <= 45.1)
						col += 3;
					else if(fabs(wrapDegrees_180(dir - 180)) <= 45.1)
						col += 0;
					else if(fabs(wrapDegrees_180(dir + 90)) <= 45.1)
						col += 1;
					else
						col += 4;
				}
			}
			
			// Animation goes downwards
			row += m_anim_frame;

			float txs = m_tx_size.X;
			float tys = m_tx_size.Y;
			setBillboardTextureMatrix(m_spritenode,
					txs, tys, col, row);
		}
	}

	void updateTextures(const std::string &mod)
	{
		ITextureSource *tsrc = m_gamedef->tsrc();

		if(m_spritenode){
			std::string texturestring = "unknown_block.png";
			if(m_prop->textures.size() >= 1)
				texturestring = m_prop->textures[0];
			texturestring += mod;
			m_spritenode->setMaterialTexture(0,
					tsrc->getTextureRaw(texturestring));
		}
		if(m_meshnode){
			for (u32 i = 0; i < 6; ++i)
			{
				std::string texturestring = "unknown_block.png";
				if(m_prop->textures.size() > i)
					texturestring = m_prop->textures[i];
				texturestring += mod;
				AtlasPointer ap = tsrc->getTexture(texturestring);

				// Get the tile texture and atlas transformation
				video::ITexture* atlas = ap.atlas;
				v2f pos = ap.pos;
				v2f size = ap.size;

				// Set material flags and texture
				video::SMaterial& material = m_meshnode->getMaterial(i);
				material.setFlag(video::EMF_LIGHTING, false);
				material.setFlag(video::EMF_BILINEAR_FILTER, false);
				material.setTexture(0, atlas);
				material.getTextureMatrix(0).setTextureTranslate(pos.X, pos.Y);
				material.getTextureMatrix(0).setTextureScale(size.X, size.Y);
			}
		}
	}

	void processMessage(const std::string &data)
	{
		//infostream<<"LuaEntityCAO: Got message"<<std::endl;
		std::istringstream is(data, std::ios::binary);
		// command
		u8 cmd = readU8(is);
		if(cmd == 0) // update position
		{
			// do_interpolate
			bool do_interpolate = readU8(is);
			// pos
			m_position = readV3F1000(is);
			// velocity
			m_velocity = readV3F1000(is);
			// acceleration
			m_acceleration = readV3F1000(is);
			// yaw
			m_yaw = readF1000(is);
			// is_end_position (for interpolation)
			bool is_end_position = readU8(is);
			// update_interval
			float update_interval = readF1000(is);
			
			if(do_interpolate){
				if(!m_prop->physical)
					pos_translator.update(m_position, is_end_position, update_interval);
			} else {
				pos_translator.init(m_position);
			}
			updateNodePos();
		}
		else if(cmd == 1) // set texture modification
		{
			std::string mod = deSerializeString(is);
			updateTextures(mod);
		}
		else if(cmd == 2) // set sprite
		{
			v2s16 p = readV2S16(is);
			int num_frames = readU16(is);
			float framelength = readF1000(is);
			bool select_horiz_by_yawpitch = readU8(is);
			
			m_tx_basepos = p;
			m_anim_num_frames = num_frames;
			m_anim_framelength = framelength;
			m_tx_select_horiz_by_yawpitch = select_horiz_by_yawpitch;

			updateTexturePos();
		}
	}
};

// Prototype
LuaEntityCAO proto_LuaEntityCAO(NULL, NULL);

/*
	PlayerCAO
*/

class PlayerCAO : public ClientActiveObject
{
private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	scene::ITextSceneNode* m_text;
	std::string m_name;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
	bool m_is_local_player;
	LocalPlayer *m_local_player;
	float m_damage_visual_timer;
	bool m_dead;

public:
	PlayerCAO(IGameDef *gamedef, ClientEnvironment *env):
		ClientActiveObject(0, gamedef, env),
		m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS*2.0,BS/3.),
		m_node(NULL),
		m_text(NULL),
		m_position(v3f(0,10*BS,0)),
		m_yaw(0),
		m_is_local_player(false),
		m_local_player(NULL),
		m_damage_visual_timer(0),
		m_dead(false)
	{
		if(gamedef == NULL)
			ClientActiveObject::registerType(getType(), create);
	}

	void initialize(const std::string &data)
	{
		infostream<<"PlayerCAO: Got init data"<<std::endl;
		
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0)
			return;
		// name
		m_name = deSerializeString(is);
		// pos
		m_position = readV3F1000(is);
		// yaw
		m_yaw = readF1000(is);
		// dead
		m_dead = readU8(is);

		pos_translator.init(m_position);

		Player *player = m_env->getPlayer(m_name.c_str());
		if(player && player->isLocal()){
			m_is_local_player = true;
			m_local_player = (LocalPlayer*)player;
		}
	}

	~PlayerCAO()
	{
		if(m_node)
			m_node->remove();
	}

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
	{
		return new PlayerCAO(gamedef, env);
	}

	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_PLAYER;
	}
	core::aabbox3d<f32>* getSelectionBox()
	{
		if(m_is_local_player)
			return NULL;
		if(m_dead)
			return NULL;
		return &m_selection_box;
	}
	v3f getPosition()
	{
		return pos_translator.vect_show;
	}
		
	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
	{
		if(m_node != NULL)
			return;
		if(m_is_local_player)
			return;
		
		//video::IVideoDriver* driver = smgr->getVideoDriver();
		gui::IGUIEnvironment* gui = irr->getGUIEnvironment();
		
		scene::SMesh *mesh = new scene::SMesh();
		{ // Front
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
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		}
		{ // Back
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
			video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
			video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
			video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		}
		m_node = smgr->addMeshSceneNode(mesh, NULL);
		mesh->drop();
		// Set it to use the materials of the meshbuffers directly.
		// This is needed for changing the texture in the future
		m_node->setReadOnlyMaterials(true);
		updateNodePos();

		// Add a text node for showing the name
		std::wstring wname = narrow_to_wide(m_name);
		m_text = smgr->addTextSceneNode(gui->getBuiltInFont(),
				wname.c_str(), video::SColor(255,255,255,255), m_node);
		m_text->setPosition(v3f(0, (f32)BS*2.1, 0));
		
		updateTextures("");
		updateVisibility();
		updateNodePos();
	}

	void removeFromScene()
	{
		if(m_node == NULL)
			return;

		m_node->remove();
		m_node = NULL;
	}

	void updateLight(u8 light_at_pos)
	{
		if(m_node == NULL)
			return;
		
		u8 li = decode_light(light_at_pos);
		video::SColor color(255,li,li,li);
		setMeshColor(m_node->getMesh(), color);

		updateVisibility();
	}

	v3s16 getLightPosition()
	{
		return floatToInt(m_position+v3f(0,BS*1.5,0), BS);
	}

	void updateVisibility()
	{
		if(m_node == NULL)
			return;

		m_node->setVisible(!m_dead);
	}

	void updateNodePos()
	{
		if(m_node == NULL)
			return;

		m_node->setPosition(pos_translator.vect_show);

		v3f rot = m_node->getRotation();
		rot.Y = -m_yaw;
		m_node->setRotation(rot);
	}

	void step(float dtime, ClientEnvironment *env)
	{
		pos_translator.translate(dtime);
		updateVisibility();
		updateNodePos();

		if(m_damage_visual_timer > 0){
			m_damage_visual_timer -= dtime;
			if(m_damage_visual_timer <= 0){
				updateTextures("");
			}
		}
	}

	void processMessage(const std::string &data)
	{
		//infostream<<"PlayerCAO: Got message"<<std::endl;
		std::istringstream is(data, std::ios::binary);
		// command
		u8 cmd = readU8(is);
		if(cmd == 0) // update position
		{
			// pos
			m_position = readV3F1000(is);
			// yaw
			m_yaw = readF1000(is);

			pos_translator.update(m_position, false);

			updateNodePos();
		}
		else if(cmd == 1) // punched
		{
			// damage
			s16 damage = readS16(is);
			m_damage_visual_timer = 0.05;
			if(damage >= 2)
				m_damage_visual_timer += 0.05 * damage;
			updateTextures("^[brighten");
		}
		else if(cmd == 2) // died or respawned
		{
			m_dead = readU8(is);
			updateVisibility();
		}
	}

	void updateTextures(const std::string &mod)
	{
		if(!m_node)
			return;
		ITextureSource *tsrc = m_gamedef->tsrc();
		scene::IMesh *mesh = m_node->getMesh();
		if(mesh){
			{
				std::string tname = "player.png";
				tname += mod;
				scene::IMeshBuffer *buf = mesh->getMeshBuffer(0);
				buf->getMaterial().setTexture(0,
						tsrc->getTextureRaw(tname));
			}
			{
				std::string tname = "player_back.png";
				tname += mod;
				scene::IMeshBuffer *buf = mesh->getMeshBuffer(1);
				buf->getMaterial().setTexture(0,
						tsrc->getTextureRaw(tname));
			}
		}
	}
};

// Prototype
PlayerCAO proto_PlayerCAO(NULL, NULL);


