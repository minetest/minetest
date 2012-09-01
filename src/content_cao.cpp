/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
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
#include "itemdef.h"
#include "tool.h"
#include "content_cso.h"
#include "sound.h"
#include "nodedef.h"
#include "localplayer.h"
#include "util/numeric.h" // For IntervalLimiter
#include "util/serialize.h"
#include "util/mathconstants.h"
#include "map.h"

class Settings;
struct ToolCapabilities;

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

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
	Other stuff
*/

static void setBillboardTextureMatrix(scene::IBillboardSceneNode *bill,
		float txs, float tys, int col, int row)
{
	video::SMaterial& material = bill->getMaterial(0);
	core::matrix4& matrix = material.getTextureMatrix(0);
	matrix.setTextureTranslate(txs*col, tys*row);
	matrix.setTextureScale(txs, tys);
}

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
	return floatToInt(m_position + v3f(0,0.5*BS,0), BS);
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
	GenericCAO
*/

#include "genericobject.h"

class GenericCAO : public ClientActiveObject
{
private:
	// Only set at initialization
	std::string m_name;
	bool m_is_player;
	bool m_is_local_player; // determined locally
	// Property-ish things
	ObjectProperties m_prop;
	//
	scene::ISceneManager *m_smgr;
	IrrlichtDevice *m_irr;
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_meshnode;
	scene::IBillboardSceneNode *m_spritenode;
	scene::ITextSceneNode* m_textnode;
	v3f m_position;
	v3f m_velocity;
	v3f m_acceleration;
	float m_yaw;
	s16 m_hp;
	SmoothTranslator pos_translator;
	// Spritesheet/animation stuff
	v2f m_tx_size;
	v2s16 m_tx_basepos;
	bool m_initial_tx_basepos_set;
	bool m_tx_select_horiz_by_yawpitch;
	int m_anim_frame;
	int m_anim_num_frames;
	float m_anim_framelength;
	float m_anim_timer;
	ItemGroupList m_armor_groups;
	float m_reset_textures_timer;
	bool m_visuals_expired;
	float m_step_distance_counter;
	u8 m_last_light;

public:
	GenericCAO(IGameDef *gamedef, ClientEnvironment *env):
		ClientActiveObject(0, gamedef, env),
		//
		m_is_player(false),
		m_is_local_player(false),
		//
		m_smgr(NULL),
		m_irr(NULL),
		m_selection_box(-BS/3.,-BS/3.,-BS/3., BS/3.,BS/3.,BS/3.),
		m_meshnode(NULL),
		m_spritenode(NULL),
		m_textnode(NULL),
		m_position(v3f(0,10*BS,0)),
		m_velocity(v3f(0,0,0)),
		m_acceleration(v3f(0,0,0)),
		m_yaw(0),
		m_hp(1),
		m_tx_size(1,1),
		m_tx_basepos(0,0),
		m_initial_tx_basepos_set(false),
		m_tx_select_horiz_by_yawpitch(false),
		m_anim_frame(0),
		m_anim_num_frames(1),
		m_anim_framelength(0.2),
		m_anim_timer(0),
		m_reset_textures_timer(-1),
		m_visuals_expired(false),
		m_step_distance_counter(0),
		m_last_light(255)
	{
		if(gamedef == NULL)
			ClientActiveObject::registerType(getType(), create);
	}

	void initialize(const std::string &data)
	{
		infostream<<"GenericCAO: Got init data"<<std::endl;
		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0){
			errorstream<<"GenericCAO: Unsupported init data version"
					<<std::endl;
			return;
		}
		m_name = deSerializeString(is);
		m_is_player = readU8(is);
		m_position = readV3F1000(is);
		m_yaw = readF1000(is);
		m_hp = readS16(is);
		
		int num_messages = readU8(is);
		for(int i=0; i<num_messages; i++){
			std::string message = deSerializeLongString(is);
			processMessage(message);
		}

		pos_translator.init(m_position);
		updateNodePos();
		
		if(m_is_player){
			Player *player = m_env->getPlayer(m_name.c_str());
			if(player && player->isLocal()){
				m_is_local_player = true;
			}
		}
	}

	~GenericCAO()
	{
	}

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
	{
		return new GenericCAO(gamedef, env);
	}

	u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_GENERIC;
	}
	core::aabbox3d<f32>* getSelectionBox()
	{
		if(!m_prop.is_visible || m_is_local_player)
			return NULL;
		return &m_selection_box;
	}
	v3f getPosition()
	{
		return pos_translator.vect_show;
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

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
	{
		m_smgr = smgr;
		m_irr = irr;

		if(m_meshnode != NULL || m_spritenode != NULL)
			return;
		
		m_visuals_expired = false;

		if(!m_prop.is_visible || m_is_local_player)
			return;
	
		//video::IVideoDriver* driver = smgr->getVideoDriver();

		if(m_prop.visual == "sprite"){
			infostream<<"GenericCAO::addToScene(): single_sprite"<<std::endl;
			m_spritenode = smgr->addBillboardSceneNode(
					NULL, v2f(1, 1), v3f(0,0,0), -1);
			m_spritenode->setMaterialTexture(0,
					tsrc->getTextureRaw("unknown_block.png"));
			m_spritenode->setMaterialFlag(video::EMF_LIGHTING, false);
			m_spritenode->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
			m_spritenode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
			m_spritenode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
			u8 li = m_last_light;
			m_spritenode->setColor(video::SColor(255,li,li,li));
			m_spritenode->setSize(m_prop.visual_size*BS);
			{
				const float txs = 1.0 / 1;
				const float tys = 1.0 / 1;
				setBillboardTextureMatrix(m_spritenode,
						txs, tys, 0, 0);
			}
		}
		else if(m_prop.visual == "upright_sprite")
		{
			scene::SMesh *mesh = new scene::SMesh();
			double dx = BS*m_prop.visual_size.X/2;
			double dy = BS*m_prop.visual_size.Y/2;
			{ // Front
			scene::IMeshBuffer *buf = new scene::SMeshBuffer();
			u8 li = m_last_light;
			video::SColor c(255,li,li,li);
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-dx,-dy,0, 0,0,0, c, 0,1),
				video::S3DVertex(dx,-dy,0, 0,0,0, c, 1,1),
				video::S3DVertex(dx,dy,0, 0,0,0, c, 1,0),
				video::S3DVertex(-dx,dy,0, 0,0,0, c, 0,0),
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
			u8 li = m_last_light;
			video::SColor c(255,li,li,li);
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(dx,-dy,0, 0,0,0, c, 1,1),
				video::S3DVertex(-dx,-dy,0, 0,0,0, c, 0,1),
				video::S3DVertex(-dx,dy,0, 0,0,0, c, 0,0),
				video::S3DVertex(dx,dy,0, 0,0,0, c, 1,0),
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
			m_meshnode = smgr->addMeshSceneNode(mesh, NULL);
			mesh->drop();
			// Set it to use the materials of the meshbuffers directly.
			// This is needed for changing the texture in the future
			m_meshnode->setReadOnlyMaterials(true);
		}
		else if(m_prop.visual == "cube"){
			infostream<<"GenericCAO::addToScene(): cube"<<std::endl;
			scene::IMesh *mesh = createCubeMesh(v3f(BS,BS,BS));
			m_meshnode = smgr->addMeshSceneNode(mesh, NULL);
			mesh->drop();
			
			m_meshnode->setScale(v3f(m_prop.visual_size.X,
					m_prop.visual_size.Y,
					m_prop.visual_size.X));
			u8 li = m_last_light;
			setMeshColor(m_meshnode->getMesh(), video::SColor(255,li,li,li));
		} else if(m_prop.visual == "wielditem"){
			infostream<<"GenericCAO::addToScene(): node"<<std::endl;
			infostream<<"textures: "<<m_prop.textures.size()<<std::endl;
			if(m_prop.textures.size() >= 1){
				infostream<<"textures[0]: "<<m_prop.textures[0]<<std::endl;
				IItemDefManager *idef = m_gamedef->idef();
				ItemStack item(m_prop.textures[0], 1, 0, "", idef);
				scene::IMesh *mesh = item.getDefinition(idef).wield_mesh;
				m_meshnode = smgr->addMeshSceneNode(mesh, NULL);
				
				m_meshnode->setScale(v3f(m_prop.visual_size.X/2,
						m_prop.visual_size.Y/2,
						m_prop.visual_size.X/2));
				u8 li = m_last_light;
				setMeshColor(m_meshnode->getMesh(), video::SColor(255,li,li,li));
			}
		} else {
			infostream<<"GenericCAO::addToScene(): \""<<m_prop.visual
					<<"\" not supported"<<std::endl;
		}
		updateTextures("");
		
		scene::ISceneNode *node = NULL;
		if(m_spritenode)
			node = m_spritenode;
		else if(m_meshnode)
			node = m_meshnode;
		if(node && m_is_player && !m_is_local_player){
			// Add a text node for showing the name
			gui::IGUIEnvironment* gui = irr->getGUIEnvironment();
			std::wstring wname = narrow_to_wide(m_name);
			m_textnode = smgr->addTextSceneNode(gui->getBuiltInFont(),
					wname.c_str(), video::SColor(255,255,255,255), node);
			m_textnode->setPosition(v3f(0, BS*1.1, 0));
		}
		
		updateNodePos();
	}

	void expireVisuals()
	{
		m_visuals_expired = true;
	}
		
	void updateLight(u8 light_at_pos)
	{
		bool is_visible = (m_hp != 0);
		u8 li = decode_light(light_at_pos);
		m_last_light = li;
		video::SColor color(255,li,li,li);
		if(m_meshnode){
			setMeshColor(m_meshnode->getMesh(), color);
			m_meshnode->setVisible(is_visible);
		}
		if(m_spritenode){
			m_spritenode->setColor(color);
			m_spritenode->setVisible(is_visible);
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
			v3f rot = m_meshnode->getRotation();
			rot.Y = -m_yaw;
			m_meshnode->setRotation(rot);
		}
		if(m_spritenode){
			m_spritenode->setPosition(pos_translator.vect_show);
		}
	}

	void step(float dtime, ClientEnvironment *env)
	{
		v3f lastpos = pos_translator.vect_show;

		if(m_visuals_expired && m_smgr && m_irr){
			m_visuals_expired = false;
			removeFromScene();
			addToScene(m_smgr, m_gamedef->tsrc(), m_irr);
		}

		if(m_prop.physical){
			core::aabbox3d<f32> box = m_prop.collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			collisionMoveResult moveresult;
			f32 pos_max_d = BS*0.125; // Distance per iteration
			f32 stepheight = 0;
			v3f p_pos = m_position;
			v3f p_velocity = m_velocity;
			v3f p_acceleration = m_acceleration;
			IGameDef *gamedef = env->getGameDef();
			moveresult = collisionMoveSimple(&env->getMap(), gamedef,
					pos_max_d, box, stepheight, dtime,
					p_pos, p_velocity, p_acceleration);
			// Apply results
			m_position = p_pos;
			m_velocity = p_velocity;
			m_acceleration = p_acceleration;
			
			bool is_end_position = moveresult.collides;
			pos_translator.update(m_position, is_end_position, dtime);
			pos_translator.translate(dtime);
			updateNodePos();
		} else {
			m_position += dtime * m_velocity + 0.5 * dtime * dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
			pos_translator.update(m_position, pos_translator.aim_is_end, pos_translator.anim_time);
			pos_translator.translate(dtime);
			updateNodePos();
		}

		float moved = lastpos.getDistanceFrom(pos_translator.vect_show);
		m_step_distance_counter += moved;
		if(m_step_distance_counter > 1.5*BS){
			m_step_distance_counter = 0;
			if(!m_is_local_player && m_prop.makes_footstep_sound){
				INodeDefManager *ndef = m_gamedef->ndef();
				v3s16 p = floatToInt(getPosition() + v3f(0,
						(m_prop.collisionbox.MinEdge.Y-0.5)*BS, 0), BS);
				MapNode n = m_env->getMap().getNodeNoEx(p);
				SimpleSoundSpec spec = ndef->get(n).sound_footstep;
				m_gamedef->sound()->playSoundAt(spec, false, getPosition());
			}
		}

		m_anim_timer += dtime;
		if(m_anim_timer >= m_anim_framelength){
			m_anim_timer -= m_anim_framelength;
			m_anim_frame++;
			if(m_anim_frame >= m_anim_num_frames)
				m_anim_frame = 0;
		}

		updateTexturePos();

		if(m_reset_textures_timer >= 0){
			m_reset_textures_timer -= dtime;
			if(m_reset_textures_timer <= 0){
				m_reset_textures_timer = -1;
				updateTextures("");
			}
		}
		if(fabs(m_prop.automatic_rotate) > 0.001){
			m_yaw += dtime * m_prop.automatic_rotate * 180 / M_PI;
			updateNodePos();
		}
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
					float mob_dir = atan2(cam_to_entity.Z, cam_to_entity.X) / M_PI * 180.;
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

		if(m_spritenode)
		{
			if(m_prop.visual == "sprite")
			{
				std::string texturestring = "unknown_block.png";
				if(m_prop.textures.size() >= 1)
					texturestring = m_prop.textures[0];
				texturestring += mod;
				m_spritenode->setMaterialTexture(0,
						tsrc->getTextureRaw(texturestring));
			}
		}
		if(m_meshnode)
		{
			if(m_prop.visual == "cube")
			{
				for (u32 i = 0; i < 6; ++i)
				{
					std::string texturestring = "unknown_block.png";
					if(m_prop.textures.size() > i)
						texturestring = m_prop.textures[i];
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
			else if(m_prop.visual == "upright_sprite")
			{
				scene::IMesh *mesh = m_meshnode->getMesh();
				{
					std::string tname = "unknown_object.png";
					if(m_prop.textures.size() >= 1)
						tname = m_prop.textures[0];
					tname += mod;
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(0);
					buf->getMaterial().setTexture(0,
							tsrc->getTextureRaw(tname));
				}
				{
					std::string tname = "unknown_object.png";
					if(m_prop.textures.size() >= 2)
						tname = m_prop.textures[1];
					else if(m_prop.textures.size() >= 1)
						tname = m_prop.textures[0];
					tname += mod;
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(1);
					buf->getMaterial().setTexture(0,
							tsrc->getTextureRaw(tname));
				}
			}
		}
	}

	void processMessage(const std::string &data)
	{
		//infostream<<"GenericCAO: Got message"<<std::endl;
		std::istringstream is(data, std::ios::binary);
		// command
		u8 cmd = readU8(is);
		if(cmd == GENERIC_CMD_SET_PROPERTIES)
		{
			m_prop = gob_read_set_properties(is);

			m_selection_box = m_prop.collisionbox;
			m_selection_box.MinEdge *= BS;
			m_selection_box.MaxEdge *= BS;
				
			m_tx_size.X = 1.0 / m_prop.spritediv.X;
			m_tx_size.Y = 1.0 / m_prop.spritediv.Y;

			if(!m_initial_tx_basepos_set){
				m_initial_tx_basepos_set = true;
				m_tx_basepos = m_prop.initial_sprite_basepos;
			}
			
			expireVisuals();
		}
		else if(cmd == GENERIC_CMD_UPDATE_POSITION)
		{
			m_position = readV3F1000(is);
			m_velocity = readV3F1000(is);
			m_acceleration = readV3F1000(is);
			if(fabs(m_prop.automatic_rotate) < 0.001)
				m_yaw = readF1000(is);
			bool do_interpolate = readU8(is);
			bool is_end_position = readU8(is);
			float update_interval = readF1000(is);

			// Place us a bit higher if we're physical, to not sink into
			// the ground due to sucky collision detection...
			if(m_prop.physical)
				m_position += v3f(0,0.002,0);
			
			if(do_interpolate){
				if(!m_prop.physical)
					pos_translator.update(m_position, is_end_position, update_interval);
			} else {
				pos_translator.init(m_position);
			}
			updateNodePos();
		}
		else if(cmd == GENERIC_CMD_SET_TEXTURE_MOD)
		{
			std::string mod = deSerializeString(is);
			updateTextures(mod);
		}
		else if(cmd == GENERIC_CMD_SET_SPRITE)
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
		else if(cmd == GENERIC_CMD_PUNCHED)
		{
			/*s16 damage =*/ readS16(is);
			s16 result_hp = readS16(is);
			
			m_hp = result_hp;
		}
		else if(cmd == GENERIC_CMD_UPDATE_ARMOR_GROUPS)
		{
			m_armor_groups.clear();
			int armor_groups_size = readU16(is);
			for(int i=0; i<armor_groups_size; i++){
				std::string name = deSerializeString(is);
				int rating = readS16(is);
				m_armor_groups[name] = rating;
			}
		}
	}
	
	bool directReportPunch(v3f dir, const ItemStack *punchitem=NULL,
			float time_from_last_punch=1000000)
	{
		assert(punchitem);
		const ToolCapabilities *toolcap =
				&punchitem->getToolCapabilities(m_gamedef->idef());
		PunchDamageResult result = getPunchDamage(
				m_armor_groups,
				toolcap,
				punchitem,
				time_from_last_punch);

		if(result.did_punch && result.damage != 0)
		{
			if(result.damage < m_hp){
				m_hp -= result.damage;
			} else {
				m_hp = 0;
				// TODO: Execute defined fast response
				// As there is no definition, make a smoke puff
				ClientSimpleObject *simple = createSmokePuff(
						m_smgr, m_env, m_position,
						m_prop.visual_size * BS);
				m_env->addSimpleObject(simple);
			}
			// TODO: Execute defined fast response
			// Flashing shall suffice as there is no definition
			m_reset_textures_timer = 0.05;
			if(result.damage >= 2)
				m_reset_textures_timer += 0.05 * result.damage;
			updateTextures("^[brighten");
		}
		
		return false;
	}
	
	std::string debugInfoText()
	{
		std::ostringstream os(std::ios::binary);
		os<<"GenericCAO hp="<<m_hp<<"\n";
		os<<"armor={";
		for(ItemGroupList::const_iterator i = m_armor_groups.begin();
				i != m_armor_groups.end(); i++){
			os<<i->first<<"="<<i->second<<", ";
		}
		os<<"}";
		return os.str();
	}
};

// Prototype
GenericCAO proto_GenericCAO(NULL, NULL);


