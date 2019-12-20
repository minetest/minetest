/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include <IBillboardSceneNode.h>
#include <ICameraSceneNode.h>
#include <ITextSceneNode.h>
#include <IMeshManipulator.h>
#include <IAnimatedMeshSceneNode.h>
#include "client/client.h"
#include "client/renderingengine.h"
#include "client/sound.h"
#include "client/tile.h"
#include "util/basic_macros.h"
#include "util/numeric.h" // For IntervalLimiter & setPitchYawRoll
#include "util/serialize.h"
#include "camera.h" // CameraModes
#include "collision.h"
#include "content_cso.h"
#include "environment.h"
#include "itemdef.h"
#include "localplayer.h"
#include "map.h"
#include "mesh.h"
#include "nodedef.h"
#include "serialization.h" // For decompressZlib
#include "settings.h"
#include "sound.h"
#include "tool.h"
#include "wieldmesh.h"
#include <algorithm>
#include <cmath>

class Settings;
struct ToolCapabilities;

std::unordered_map<u16, ClientActiveObject::Factory> ClientActiveObject::m_types;

template<typename T>
void SmoothTranslator<T>::init(T current)
{
	val_old = current;
	val_current = current;
	val_target = current;
	anim_time = 0;
	anim_time_counter = 0;
	aim_is_end = true;
}

template<typename T>
void SmoothTranslator<T>::update(T new_target, bool is_end_position, float update_interval)
{
	aim_is_end = is_end_position;
	val_old = val_current;
	val_target = new_target;
	if (update_interval > 0) {
		anim_time = update_interval;
	} else {
		if (anim_time < 0.001 || anim_time > 1.0)
			anim_time = anim_time_counter;
		else
			anim_time = anim_time * 0.9 + anim_time_counter * 0.1;
	}
	anim_time_counter = 0;
}

template<typename T>
void SmoothTranslator<T>::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;
	T val_diff = val_target - val_old;
	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
	val_current = val_old + val_diff * moveratio;
}

void SmoothTranslatorWrapped::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;
	f32 val_diff = std::abs(val_target - val_old);
	if (val_diff > 180.f)
		val_diff = 360.f - val_diff;

	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
	wrappedApproachShortest(val_current, val_target,
		val_diff * moveratio, 360.f);
}

void SmoothTranslatorWrappedv3f::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;

	v3f val_diff_v3f;
	val_diff_v3f.X = std::abs(val_target.X - val_old.X);
	val_diff_v3f.Y = std::abs(val_target.Y - val_old.Y);
	val_diff_v3f.Z = std::abs(val_target.Z - val_old.Z);

	if (val_diff_v3f.X > 180.f)
		val_diff_v3f.X = 360.f - val_diff_v3f.X;

	if (val_diff_v3f.Y > 180.f)
		val_diff_v3f.Y = 360.f - val_diff_v3f.Y;

	if (val_diff_v3f.Z > 180.f)
		val_diff_v3f.Z = 360.f - val_diff_v3f.Z;

	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
	wrappedApproachShortest(val_current.X, val_target.X,
		val_diff_v3f.X * moveratio, 360.f);

	wrappedApproachShortest(val_current.Y, val_target.Y,
		val_diff_v3f.Y * moveratio, 360.f);

	wrappedApproachShortest(val_current.Z, val_target.Z,
		val_diff_v3f.Z * moveratio, 360.f);
}

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
	TestCAO(Client *client, ClientEnvironment *env);
	virtual ~TestCAO() = default;

	ActiveObjectType getType() const
	{
		return ACTIVEOBJECT_TYPE_TEST;
	}

	static ClientActiveObject* create(Client *client, ClientEnvironment *env);

	void addToScene(ITextureSource *tsrc);
	void removeFromScene(bool permanent);
	void updateLight(u8 light_at_pos);
	v3s16 getLightPosition();
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	bool getCollisionBox(aabb3f *toset) const { return false; }
private:
	scene::IMeshSceneNode *m_node;
	v3f m_position;
};

// Prototype
TestCAO proto_TestCAO(NULL, NULL);

TestCAO::TestCAO(Client *client, ClientEnvironment *env):
	ClientActiveObject(0, client, env),
	m_node(NULL),
	m_position(v3f(0,10*BS,0))
{
	ClientActiveObject::registerType(getType(), create);
}

ClientActiveObject* TestCAO::create(Client *client, ClientEnvironment *env)
{
	return new TestCAO(client, env);
}

void TestCAO::addToScene(ITextureSource *tsrc)
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
	buf->getMaterial().setTexture(0, tsrc->getTextureForMesh("rat.png"));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = RenderingEngine::get_scene_manager()->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	updateNodePos();
}

void TestCAO::removeFromScene(bool permanent)
{
	if (!m_node)
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
	if (!m_node)
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
	GenericCAO
*/

#include "genericobject.h"
#include "clientobject.h"

GenericCAO::GenericCAO(Client *client, ClientEnvironment *env):
		ClientActiveObject(0, client, env)
{
	if (client == NULL) {
		ClientActiveObject::registerType(getType(), create);
	} else {
		m_client = client;
	}
}

bool GenericCAO::getCollisionBox(aabb3f *toset) const
{
	if (m_prop.physical)
	{
		//update collision box
		toset->MinEdge = m_prop.collisionbox.MinEdge * BS;
		toset->MaxEdge = m_prop.collisionbox.MaxEdge * BS;

		toset->MinEdge += m_position;
		toset->MaxEdge += m_position;

		return true;
	}

	return false;
}

bool GenericCAO::collideWithObjects() const
{
	return m_prop.collideWithObjects;
}

void GenericCAO::initialize(const std::string &data)
{
	infostream<<"GenericCAO: Got init data"<<std::endl;
	processInitData(data);

	if (m_is_player) {
		// Check if it's the current player
		LocalPlayer *player = m_env->getLocalPlayer();
		if (player && strcmp(player->getName(), m_name.c_str()) == 0) {
			m_is_local_player = true;
			m_is_visible = false;
			player->setCAO(this);
		}
	}
}

void GenericCAO::processInitData(const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	const u8 version = readU8(is);

	if (version < 1) {
		errorstream << "GenericCAO: Unsupported init data version"
				<< std::endl;
		return;
	}

	// PROTOCOL_VERSION >= 37
	m_name = deSerializeString(is);
	m_is_player = readU8(is);
	m_id = readU16(is);
	m_position = readV3F32(is);
	m_rotation = readV3F32(is);
	m_hp = readU16(is);

	const u8 num_messages = readU8(is);

	for (int i = 0; i < num_messages; i++) {
		std::string message = deSerializeLongString(is);
		processMessage(message);
	}

	m_rotation = wrapDegrees_0_360_v3f(m_rotation);
	pos_translator.init(m_position);
	rot_translator.init(m_rotation);
	updateNodePos();
}

GenericCAO::~GenericCAO()
{
	removeFromScene(true);
}

bool GenericCAO::getSelectionBox(aabb3f *toset) const
{
	if (!m_prop.is_visible || !m_is_visible || m_is_local_player
			|| !m_prop.pointable) {
		return false;
	}
	*toset = m_selection_box;
	return true;
}

const v3f GenericCAO::getPosition() const
{
	if (!getParent())
		return pos_translator.val_current;

	// Calculate real position in world based on MatrixNode
	if (m_matrixnode) {
		v3s16 camera_offset = m_env->getCameraOffset();
		return m_matrixnode->getAbsolutePosition() +
				intToFloat(camera_offset, BS);
	}

	return m_position;
}

const bool GenericCAO::isImmortal()
{
	return itemgroup_get(getGroups(), "immortal");
}

scene::ISceneNode *GenericCAO::getSceneNode() const
{
	if (m_meshnode) {
		return m_meshnode;
	}

	if (m_animated_meshnode) {
		return m_animated_meshnode;
	}

	if (m_wield_meshnode) {
		return m_wield_meshnode;
	}

	if (m_spritenode) {
		return m_spritenode;
	}
	return NULL;
}

scene::IAnimatedMeshSceneNode *GenericCAO::getAnimatedMeshSceneNode() const
{
	return m_animated_meshnode;
}

void GenericCAO::setChildrenVisible(bool toset)
{
	for (u16 cao_id : m_attachment_child_ids) {
		GenericCAO *obj = m_env->getGenericCAO(cao_id);
		if (obj) {
			obj->setVisible(toset);
		}
	}
}

void GenericCAO::setAttachment(int parent_id, const std::string &bone, v3f position, v3f rotation)
{
	int old_parent = m_attachment_parent_id;
	m_attachment_parent_id = parent_id;
	m_attachment_bone = bone;
	m_attachment_position = position;
	m_attachment_rotation = rotation;

	ClientActiveObject *parent = m_env->getActiveObject(parent_id);

	if (parent_id != old_parent) {
		if (auto *o = m_env->getActiveObject(old_parent))
			o->removeAttachmentChild(m_id);
		if (parent)
			parent->addAttachmentChild(m_id);
	}

	updateAttachments();
}

void GenericCAO::getAttachment(int *parent_id, std::string *bone, v3f *position,
	v3f *rotation) const
{
	*parent_id = m_attachment_parent_id;
	*bone = m_attachment_bone;
	*position = m_attachment_position;
	*rotation = m_attachment_rotation;
}

void GenericCAO::clearChildAttachments()
{
	// Cannot use for-loop here: setAttachment() modifies 'm_attachment_child_ids'!
	while (!m_attachment_child_ids.empty()) {
		int child_id = *m_attachment_child_ids.begin();

		if (ClientActiveObject *child = m_env->getActiveObject(child_id))
			child->setAttachment(0, "", v3f(), v3f());

		removeAttachmentChild(child_id);
	}
}

void GenericCAO::clearParentAttachment()
{
	if (m_attachment_parent_id)
		setAttachment(0, "", m_attachment_position, m_attachment_rotation);
	else
		setAttachment(0, "", v3f(), v3f());
}

void GenericCAO::addAttachmentChild(int child_id)
{
	m_attachment_child_ids.insert(child_id);
}

void GenericCAO::removeAttachmentChild(int child_id)
{
	m_attachment_child_ids.erase(child_id);
}

ClientActiveObject* GenericCAO::getParent() const
{
	return m_attachment_parent_id ? m_env->getActiveObject(m_attachment_parent_id) :
			nullptr;
}

void GenericCAO::removeFromScene(bool permanent)
{
	// Should be true when removing the object permanently
	// and false when refreshing (eg: updating visuals)
	if (m_env && permanent) {
		// The client does not know whether this object does re-appear to
		// a later time, thus do not clear child attachments.

		clearParentAttachment();
	}

	if (m_meshnode) {
		m_meshnode->remove();
		m_meshnode->drop();
		m_meshnode = nullptr;
	} else if (m_animated_meshnode)	{
		m_animated_meshnode->remove();
		m_animated_meshnode->drop();
		m_animated_meshnode = nullptr;
	} else if (m_wield_meshnode) {
		m_wield_meshnode->remove();
		m_wield_meshnode->drop();
		m_wield_meshnode = nullptr;
	} else if (m_spritenode) {
		m_spritenode->remove();
		m_spritenode->drop();
		m_spritenode = nullptr;
	}

	if (m_matrixnode) {
		m_matrixnode->remove();
		m_matrixnode->drop();
		m_matrixnode = nullptr;
	}

	if (m_nametag) {
		m_client->getCamera()->removeNametag(m_nametag);
		m_nametag = nullptr;
	}
}

void GenericCAO::addToScene(ITextureSource *tsrc)
{
	m_smgr = RenderingEngine::get_scene_manager();

	if (getSceneNode() != NULL) {
		return;
	}

	m_visuals_expired = false;

	if (!m_prop.is_visible) {
		return;
	}

	video::E_MATERIAL_TYPE material_type = (m_prop.use_texture_alpha) ?
		video::EMT_TRANSPARENT_ALPHA_CHANNEL : video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

	auto grabMatrixNode = [this] {
		infostream << "GenericCAO::addToScene(): " << m_prop.visual << std::endl;
		m_matrixnode = RenderingEngine::get_scene_manager()->
				addDummyTransformationSceneNode();
		m_matrixnode->grab();
	};

	if (m_prop.visual == "sprite") {
		grabMatrixNode();
		m_spritenode = RenderingEngine::get_scene_manager()->addBillboardSceneNode(
				m_matrixnode, v2f(1, 1), v3f(0,0,0), -1);
		m_spritenode->grab();
		m_spritenode->setMaterialTexture(0,
				tsrc->getTextureForMesh("unknown_node.png"));
		m_spritenode->setMaterialFlag(video::EMF_LIGHTING, false);
		m_spritenode->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		m_spritenode->setMaterialType(material_type);
		m_spritenode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		u8 li = m_last_light;
		m_spritenode->setColor(video::SColor(255,li,li,li));
		m_spritenode->setSize(v2f(m_prop.visual_size.X,
				m_prop.visual_size.Y) * BS);
		{
			const float txs = 1.0 / 1;
			const float tys = 1.0 / 1;
			setBillboardTextureMatrix(m_spritenode,
					txs, tys, 0, 0);
		}
	} else if (m_prop.visual == "upright_sprite") {
		grabMatrixNode();
		scene::SMesh *mesh = new scene::SMesh();
		double dx = BS * m_prop.visual_size.X / 2;
		double dy = BS * m_prop.visual_size.Y / 2;
		u8 li = m_last_light;
		video::SColor c(255, li, li, li);

		{ // Front
			scene::IMeshBuffer *buf = new scene::SMeshBuffer();
			video::S3DVertex vertices[4] = {
				video::S3DVertex(-dx, -dy, 0, 0,0,0, c, 1,1),
				video::S3DVertex( dx, -dy, 0, 0,0,0, c, 0,1),
				video::S3DVertex( dx,  dy, 0, 0,0,0, c, 0,0),
				video::S3DVertex(-dx,  dy, 0, 0,0,0, c, 1,0),
			};
			if (m_is_player) {
				// Move minimal Y position to 0 (feet position)
				for (video::S3DVertex &vertex : vertices)
					vertex.Pos.Y += dy;
			}
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
			video::S3DVertex vertices[4] = {
				video::S3DVertex( dx,-dy, 0, 0,0,0, c, 1,1),
				video::S3DVertex(-dx,-dy, 0, 0,0,0, c, 0,1),
				video::S3DVertex(-dx, dy, 0, 0,0,0, c, 0,0),
				video::S3DVertex( dx, dy, 0, 0,0,0, c, 1,0),
			};
			if (m_is_player) {
				// Move minimal Y position to 0 (feet position)
				for (video::S3DVertex &vertex : vertices)
					vertex.Pos.Y += dy;
			}
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
		m_meshnode = RenderingEngine::get_scene_manager()->
			addMeshSceneNode(mesh, m_matrixnode);
		m_meshnode->grab();
		mesh->drop();
		// Set it to use the materials of the meshbuffers directly.
		// This is needed for changing the texture in the future
		m_meshnode->setReadOnlyMaterials(true);
	} else if (m_prop.visual == "cube") {
		grabMatrixNode();
		scene::IMesh *mesh = createCubeMesh(v3f(BS,BS,BS));
		m_meshnode = RenderingEngine::get_scene_manager()->
			addMeshSceneNode(mesh, m_matrixnode);
		m_meshnode->grab();
		mesh->drop();

		m_meshnode->setScale(m_prop.visual_size);
		u8 li = m_last_light;
		setMeshColor(m_meshnode->getMesh(), video::SColor(255,li,li,li));

		m_meshnode->setMaterialFlag(video::EMF_LIGHTING, false);
		m_meshnode->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		m_meshnode->setMaterialType(material_type);
		m_meshnode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
	} else if (m_prop.visual == "mesh") {
		grabMatrixNode();
		scene::IAnimatedMesh *mesh = m_client->getMesh(m_prop.mesh, true);
		if (mesh) {
			m_animated_meshnode = RenderingEngine::get_scene_manager()->
				addAnimatedMeshSceneNode(mesh, m_matrixnode);
			m_animated_meshnode->grab();
			mesh->drop(); // The scene node took hold of it
			m_animated_meshnode->animateJoints(); // Needed for some animations
			m_animated_meshnode->setScale(m_prop.visual_size);
			u8 li = m_last_light;

			// set vertex colors to ensure alpha is set
			setMeshColor(m_animated_meshnode->getMesh(), video::SColor(255,li,li,li));

			setAnimatedMeshColor(m_animated_meshnode, video::SColor(255,li,li,li));

			m_animated_meshnode->setMaterialFlag(video::EMF_LIGHTING, true);
			m_animated_meshnode->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
			m_animated_meshnode->setMaterialType(material_type);
			m_animated_meshnode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
			m_animated_meshnode->setMaterialFlag(video::EMF_BACK_FACE_CULLING,
				m_prop.backface_culling);
		} else
			errorstream<<"GenericCAO::addToScene(): Could not load mesh "<<m_prop.mesh<<std::endl;
	} else if (m_prop.visual == "wielditem" || m_prop.visual == "item") {
		grabMatrixNode();
		ItemStack item;
		if (m_prop.wield_item.empty()) {
			// Old format, only textures are specified.
			infostream << "textures: " << m_prop.textures.size() << std::endl;
			if (!m_prop.textures.empty()) {
				infostream << "textures[0]: " << m_prop.textures[0]
					<< std::endl;
				IItemDefManager *idef = m_client->idef();
				item = ItemStack(m_prop.textures[0], 1, 0, idef);
			}
		} else {
			infostream << "serialized form: " << m_prop.wield_item << std::endl;
			item.deSerialize(m_prop.wield_item, m_client->idef());
		}
		m_wield_meshnode = new WieldMeshSceneNode(
			RenderingEngine::get_scene_manager(), -1);
		m_wield_meshnode->setItem(item, m_client,
			(m_prop.visual == "wielditem"));

		m_wield_meshnode->setScale(m_prop.visual_size / 2.0f);
		u8 li = m_last_light;
		m_wield_meshnode->setColor(video::SColor(255, li, li, li));
	} else {
		infostream<<"GenericCAO::addToScene(): \""<<m_prop.visual
				<<"\" not supported"<<std::endl;
	}

	/* don't update while punch texture modifier is active */
	if (m_reset_textures_timer < 0)
		updateTextures(m_current_texture_modifier);

	scene::ISceneNode *node = getSceneNode();

	if (node && m_matrixnode)
		node->setParent(m_matrixnode);

	if (node && !m_prop.nametag.empty() && !m_is_local_player) {
		// Add nametag
		v3f pos;
		pos.Y = m_prop.selectionbox.MaxEdge.Y + 0.3f;
		m_nametag = m_client->getCamera()->addNametag(node,
			m_prop.nametag, m_prop.nametag_color,
			pos);
	}

	updateNodePos();
	updateAnimation();
	updateBonePosition();
	updateAttachments();
}

void GenericCAO::updateLight(u8 light_at_pos)
{
	// Don't update light of attached one
	if (getParent() != NULL) {
		return;
	}

	updateLightNoCheck(light_at_pos);

	// Update light of all children
	for (u16 i : m_attachment_child_ids) {
		ClientActiveObject *obj = m_env->getActiveObject(i);
		if (obj) {
			obj->updateLightNoCheck(light_at_pos);
		}
	}
}

void GenericCAO::updateLightNoCheck(u8 light_at_pos)
{
	if (m_glow < 0)
		return;

	u8 li = decode_light(light_at_pos + m_glow);
	if (li != m_last_light)	{
		m_last_light = li;
		video::SColor color(255,li,li,li);
		if (m_meshnode) {
			setMeshColor(m_meshnode->getMesh(), color);
		} else if (m_animated_meshnode) {
			setAnimatedMeshColor(m_animated_meshnode, color);
		} else if (m_wield_meshnode) {
			m_wield_meshnode->setColor(color);
		} else if (m_spritenode) {
			m_spritenode->setColor(color);
		}
	}
}

v3s16 GenericCAO::getLightPosition()
{
	if (m_is_player)
		return floatToInt(m_position + v3f(0, 0.5 * BS, 0), BS);

	return floatToInt(m_position, BS);
}

void GenericCAO::updateNodePos()
{
	if (getParent() != NULL)
		return;

	scene::ISceneNode *node = getSceneNode();

	if (node) {
		v3s16 camera_offset = m_env->getCameraOffset();
		v3f pos = pos_translator.val_current -
				intToFloat(camera_offset, BS);
		getPosRotMatrix().setTranslation(pos);
		if (node != m_spritenode) { // rotate if not a sprite
			v3f rot = m_is_local_player ? -m_rotation : -rot_translator.val_current;
			setPitchYawRoll(getPosRotMatrix(), rot);
		}
	}
}

void GenericCAO::step(float dtime, ClientEnvironment *env)
{
	// Handle model animations and update positions instantly to prevent lags
	if (m_is_local_player) {
		LocalPlayer *player = m_env->getLocalPlayer();
		m_position = player->getPosition();
		pos_translator.val_current = m_position;
		m_rotation.Y = wrapDegrees_0_360(player->getYaw());
		rot_translator.val_current = m_rotation;

		if (m_is_visible) {
			int old_anim = player->last_animation;
			float old_anim_speed = player->last_animation_speed;
			m_velocity = v3f(0,0,0);
			m_acceleration = v3f(0,0,0);
			const PlayerControl &controls = player->getPlayerControl();

			bool walking = false;
			if (controls.up || controls.down || controls.left || controls.right ||
					controls.forw_move_joystick_axis != 0.f ||
					controls.sidew_move_joystick_axis != 0.f)
				walking = true;

			f32 new_speed = player->local_animation_speed;
			v2s32 new_anim = v2s32(0,0);
			bool allow_update = false;

			// increase speed if using fast or flying fast
			if((g_settings->getBool("fast_move") &&
					m_client->checkLocalPrivilege("fast")) &&
					(controls.aux1 ||
					(!player->touching_ground &&
					g_settings->getBool("free_move") &&
					m_client->checkLocalPrivilege("fly"))))
					new_speed *= 1.5;
			// slowdown speed if sneeking
			if (controls.sneak && walking)
				new_speed /= 2;

			if (walking && (controls.LMB || controls.RMB)) {
				new_anim = player->local_animations[3];
				player->last_animation = WD_ANIM;
			} else if(walking) {
				new_anim = player->local_animations[1];
				player->last_animation = WALK_ANIM;
			} else if(controls.LMB || controls.RMB) {
				new_anim = player->local_animations[2];
				player->last_animation = DIG_ANIM;
			}

			// Apply animations if input detected and not attached
			// or set idle animation
			if ((new_anim.X + new_anim.Y) > 0 && !getParent()) {
				allow_update = true;
				m_animation_range = new_anim;
				m_animation_speed = new_speed;
				player->last_animation_speed = m_animation_speed;
			} else {
				player->last_animation = NO_ANIM;

				if (old_anim != NO_ANIM) {
					m_animation_range = player->local_animations[0];
					updateAnimation();
				}
			}

			// Update local player animations
			if ((player->last_animation != old_anim ||
				m_animation_speed != old_anim_speed) &&
				player->last_animation != NO_ANIM && allow_update)
					updateAnimation();

		}
	}

	if (m_visuals_expired && m_smgr) {
		m_visuals_expired = false;

		// Attachments, part 1: All attached objects must be unparented first,
		// or Irrlicht causes a segmentation fault
		for (u16 cao_id : m_attachment_child_ids) {
			ClientActiveObject *obj = m_env->getActiveObject(cao_id);
			if (obj) {
				scene::ISceneNode *child_node = obj->getSceneNode();
				// The node's parent is always an IDummyTraformationSceneNode,
				// so we need to reparent that one instead.
				if (child_node)
					child_node->getParent()->setParent(m_smgr->getRootSceneNode());
			}
		}

		removeFromScene(false);
		addToScene(m_client->tsrc());

		// Attachments, part 2: Now that the parent has been refreshed, put its attachments back
		for (u16 cao_id : m_attachment_child_ids) {
			ClientActiveObject *obj = m_env->getActiveObject(cao_id);
			if (obj)
				obj->updateAttachments();
		}
	}

	// Make sure m_is_visible is always applied
	scene::ISceneNode *node = getSceneNode();
	if (node)
		node->setVisible(m_is_visible);

	if(getParent() != NULL) // Attachments should be glued to their parent by Irrlicht
	{
		// Set these for later
		m_position = getPosition();
		m_velocity = v3f(0,0,0);
		m_acceleration = v3f(0,0,0);
		pos_translator.val_current = m_position;
		pos_translator.val_target = m_position;
	} else {
		rot_translator.translate(dtime);
		v3f lastpos = pos_translator.val_current;

		if(m_prop.physical)
		{
			aabb3f box = m_prop.collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			collisionMoveResult moveresult;
			f32 pos_max_d = BS*0.125; // Distance per iteration
			v3f p_pos = m_position;
			v3f p_velocity = m_velocity;
			moveresult = collisionMoveSimple(env,env->getGameDef(),
					pos_max_d, box, m_prop.stepheight, dtime,
					&p_pos, &p_velocity, m_acceleration,
					this, m_prop.collideWithObjects);
			// Apply results
			m_position = p_pos;
			m_velocity = p_velocity;

			bool is_end_position = moveresult.collides;
			pos_translator.update(m_position, is_end_position, dtime);
		} else {
			m_position += dtime * m_velocity + 0.5 * dtime * dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
			pos_translator.update(m_position, pos_translator.aim_is_end,
					pos_translator.anim_time);
		}
		pos_translator.translate(dtime);
		updateNodePos();

		float moved = lastpos.getDistanceFrom(pos_translator.val_current);
		m_step_distance_counter += moved;
		if (m_step_distance_counter > 1.5f * BS) {
			m_step_distance_counter = 0.0f;
			if (!m_is_local_player && m_prop.makes_footstep_sound) {
				const NodeDefManager *ndef = m_client->ndef();
				v3s16 p = floatToInt(getPosition() +
					v3f(0.0f, (m_prop.collisionbox.MinEdge.Y - 0.5f) * BS, 0.0f), BS);
				MapNode n = m_env->getMap().getNode(p);
				SimpleSoundSpec spec = ndef->get(n).sound_footstep;
				// Reduce footstep gain, as non-local-player footsteps are
				// somehow louder.
				spec.gain *= 0.6f;
				m_client->sound()->playSoundAt(spec, false, getPosition());
			}
		}
	}

	m_anim_timer += dtime;
	if(m_anim_timer >= m_anim_framelength)
	{
		m_anim_timer -= m_anim_framelength;
		m_anim_frame++;
		if(m_anim_frame >= m_anim_num_frames)
			m_anim_frame = 0;
	}

	updateTexturePos();

	if(m_reset_textures_timer >= 0)
	{
		m_reset_textures_timer -= dtime;
		if(m_reset_textures_timer <= 0) {
			m_reset_textures_timer = -1;
			updateTextures(m_previous_texture_modifier);
		}
	}
	if (!getParent() && std::fabs(m_prop.automatic_rotate) > 0.001) {
		m_rotation.Y += dtime * m_prop.automatic_rotate * 180 / M_PI;
		rot_translator.val_current = m_rotation;
		updateNodePos();
	}

	if (!getParent() && m_prop.automatic_face_movement_dir &&
			(fabs(m_velocity.Z) > 0.001 || fabs(m_velocity.X) > 0.001)) {
		float target_yaw = atan2(m_velocity.Z, m_velocity.X) * 180 / M_PI
				+ m_prop.automatic_face_movement_dir_offset;
		float max_rotation_per_sec =
				m_prop.automatic_face_movement_max_rotation_per_sec;

		if (max_rotation_per_sec > 0) {
			wrappedApproachShortest(m_rotation.Y, target_yaw,
				dtime * max_rotation_per_sec, 360.f);
		} else {
			// Negative values of max_rotation_per_sec mean disabled.
			m_rotation.Y = target_yaw;
		}

		rot_translator.val_current = m_rotation;
		updateNodePos();
	}
}

void GenericCAO::updateTexturePos()
{
	if(m_spritenode)
	{
		scene::ICameraSceneNode* camera =
				m_spritenode->getSceneManager()->getActiveCamera();
		if(!camera)
			return;
		v3f cam_to_entity = m_spritenode->getAbsolutePosition()
				- camera->getAbsolutePosition();
		cam_to_entity.normalize();

		int row = m_tx_basepos.Y;
		int col = m_tx_basepos.X;

		if (m_tx_select_horiz_by_yawpitch) {
			if (cam_to_entity.Y > 0.75)
				col += 5;
			else if (cam_to_entity.Y < -0.75)
				col += 4;
			else {
				float mob_dir =
						atan2(cam_to_entity.Z, cam_to_entity.X) / M_PI * 180.;
				float dir = mob_dir - m_rotation.Y;
				dir = wrapDegrees_180(dir);
				if (std::fabs(wrapDegrees_180(dir - 0)) <= 45.1f)
					col += 2;
				else if(std::fabs(wrapDegrees_180(dir - 90)) <= 45.1f)
					col += 3;
				else if(std::fabs(wrapDegrees_180(dir - 180)) <= 45.1f)
					col += 0;
				else if(std::fabs(wrapDegrees_180(dir + 90)) <= 45.1f)
					col += 1;
				else
					col += 4;
			}
		}

		// Animation goes downwards
		row += m_anim_frame;

		float txs = m_tx_size.X;
		float tys = m_tx_size.Y;
		setBillboardTextureMatrix(m_spritenode, txs, tys, col, row);
	}
}

// Do not pass by reference, see header.
void GenericCAO::updateTextures(std::string mod)
{
	ITextureSource *tsrc = m_client->tsrc();

	bool use_trilinear_filter = g_settings->getBool("trilinear_filter");
	bool use_bilinear_filter = g_settings->getBool("bilinear_filter");
	bool use_anisotropic_filter = g_settings->getBool("anisotropic_filter");

	m_previous_texture_modifier = m_current_texture_modifier;
	m_current_texture_modifier = mod;
	m_glow = m_prop.glow;

	video::E_MATERIAL_TYPE material_type = (m_prop.use_texture_alpha) ?
		video::EMT_TRANSPARENT_ALPHA_CHANNEL : video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

	if (m_spritenode) {
		if (m_prop.visual == "sprite") {
			std::string texturestring = "unknown_node.png";
			if (!m_prop.textures.empty())
				texturestring = m_prop.textures[0];
			texturestring += mod;
			m_spritenode->getMaterial(0).MaterialType = material_type;
			m_spritenode->getMaterial(0).MaterialTypeParam = 0.5f;
			m_spritenode->setMaterialTexture(0,
					tsrc->getTextureForMesh(texturestring));

			// This allows setting per-material colors. However, until a real lighting
			// system is added, the code below will have no effect. Once MineTest
			// has directional lighting, it should work automatically.
			if (!m_prop.colors.empty()) {
				m_spritenode->getMaterial(0).AmbientColor = m_prop.colors[0];
				m_spritenode->getMaterial(0).DiffuseColor = m_prop.colors[0];
				m_spritenode->getMaterial(0).SpecularColor = m_prop.colors[0];
			}

			m_spritenode->getMaterial(0).setFlag(video::EMF_TRILINEAR_FILTER, use_trilinear_filter);
			m_spritenode->getMaterial(0).setFlag(video::EMF_BILINEAR_FILTER, use_bilinear_filter);
			m_spritenode->getMaterial(0).setFlag(video::EMF_ANISOTROPIC_FILTER, use_anisotropic_filter);
		}
	}

	if (m_animated_meshnode) {
		if (m_prop.visual == "mesh") {
			for (u32 i = 0; i < m_prop.textures.size() &&
					i < m_animated_meshnode->getMaterialCount(); ++i) {
				std::string texturestring = m_prop.textures[i];
				if (texturestring.empty())
					continue; // Empty texture string means don't modify that material
				texturestring += mod;
				video::ITexture* texture = tsrc->getTextureForMesh(texturestring);
				if (!texture) {
					errorstream<<"GenericCAO::updateTextures(): Could not load texture "<<texturestring<<std::endl;
					continue;
				}

				// Set material flags and texture
				video::SMaterial& material = m_animated_meshnode->getMaterial(i);
				material.MaterialType = material_type;
				material.MaterialTypeParam = 0.5f;
				material.TextureLayer[0].Texture = texture;
				material.setFlag(video::EMF_LIGHTING, true);
				material.setFlag(video::EMF_BILINEAR_FILTER, false);
				material.setFlag(video::EMF_BACK_FACE_CULLING, m_prop.backface_culling);

				// don't filter low-res textures, makes them look blurry
				// player models have a res of 64
				const core::dimension2d<u32> &size = texture->getOriginalSize();
				const u32 res = std::min(size.Height, size.Width);
				use_trilinear_filter &= res > 64;
				use_bilinear_filter &= res > 64;

				m_animated_meshnode->getMaterial(i)
						.setFlag(video::EMF_TRILINEAR_FILTER, use_trilinear_filter);
				m_animated_meshnode->getMaterial(i)
						.setFlag(video::EMF_BILINEAR_FILTER, use_bilinear_filter);
				m_animated_meshnode->getMaterial(i)
						.setFlag(video::EMF_ANISOTROPIC_FILTER, use_anisotropic_filter);
			}
			for (u32 i = 0; i < m_prop.colors.size() &&
			i < m_animated_meshnode->getMaterialCount(); ++i)
			{
				// This allows setting per-material colors. However, until a real lighting
				// system is added, the code below will have no effect. Once MineTest
				// has directional lighting, it should work automatically.
				m_animated_meshnode->getMaterial(i).AmbientColor = m_prop.colors[i];
				m_animated_meshnode->getMaterial(i).DiffuseColor = m_prop.colors[i];
				m_animated_meshnode->getMaterial(i).SpecularColor = m_prop.colors[i];
			}
		}
	}
	if(m_meshnode)
	{
		if(m_prop.visual == "cube")
		{
			for (u32 i = 0; i < 6; ++i)
			{
				std::string texturestring = "unknown_node.png";
				if(m_prop.textures.size() > i)
					texturestring = m_prop.textures[i];
				texturestring += mod;


				// Set material flags and texture
				video::SMaterial& material = m_meshnode->getMaterial(i);
				material.MaterialType = material_type;
				material.MaterialTypeParam = 0.5f;
				material.setFlag(video::EMF_LIGHTING, false);
				material.setFlag(video::EMF_BILINEAR_FILTER, false);
				material.setTexture(0,
						tsrc->getTextureForMesh(texturestring));
				material.getTextureMatrix(0).makeIdentity();

				// This allows setting per-material colors. However, until a real lighting
				// system is added, the code below will have no effect. Once MineTest
				// has directional lighting, it should work automatically.
				if(m_prop.colors.size() > i)
				{
					m_meshnode->getMaterial(i).AmbientColor = m_prop.colors[i];
					m_meshnode->getMaterial(i).DiffuseColor = m_prop.colors[i];
					m_meshnode->getMaterial(i).SpecularColor = m_prop.colors[i];
				}

				m_meshnode->getMaterial(i).setFlag(video::EMF_TRILINEAR_FILTER, use_trilinear_filter);
				m_meshnode->getMaterial(i).setFlag(video::EMF_BILINEAR_FILTER, use_bilinear_filter);
				m_meshnode->getMaterial(i).setFlag(video::EMF_ANISOTROPIC_FILTER, use_anisotropic_filter);
			}
		} else if (m_prop.visual == "upright_sprite") {
			scene::IMesh *mesh = m_meshnode->getMesh();
			{
				std::string tname = "unknown_object.png";
				if (!m_prop.textures.empty())
					tname = m_prop.textures[0];
				tname += mod;
				scene::IMeshBuffer *buf = mesh->getMeshBuffer(0);
				buf->getMaterial().setTexture(0,
						tsrc->getTextureForMesh(tname));

				// This allows setting per-material colors. However, until a real lighting
				// system is added, the code below will have no effect. Once MineTest
				// has directional lighting, it should work automatically.
				if(!m_prop.colors.empty()) {
					buf->getMaterial().AmbientColor = m_prop.colors[0];
					buf->getMaterial().DiffuseColor = m_prop.colors[0];
					buf->getMaterial().SpecularColor = m_prop.colors[0];
				}

				buf->getMaterial().setFlag(video::EMF_TRILINEAR_FILTER, use_trilinear_filter);
				buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, use_bilinear_filter);
				buf->getMaterial().setFlag(video::EMF_ANISOTROPIC_FILTER, use_anisotropic_filter);
			}
			{
				std::string tname = "unknown_object.png";
				if (m_prop.textures.size() >= 2)
					tname = m_prop.textures[1];
				else if (!m_prop.textures.empty())
					tname = m_prop.textures[0];
				tname += mod;
				scene::IMeshBuffer *buf = mesh->getMeshBuffer(1);
				buf->getMaterial().setTexture(0,
						tsrc->getTextureForMesh(tname));

				// This allows setting per-material colors. However, until a real lighting
				// system is added, the code below will have no effect. Once MineTest
				// has directional lighting, it should work automatically.
				if (m_prop.colors.size() >= 2) {
					buf->getMaterial().AmbientColor = m_prop.colors[1];
					buf->getMaterial().DiffuseColor = m_prop.colors[1];
					buf->getMaterial().SpecularColor = m_prop.colors[1];
				} else if (!m_prop.colors.empty()) {
					buf->getMaterial().AmbientColor = m_prop.colors[0];
					buf->getMaterial().DiffuseColor = m_prop.colors[0];
					buf->getMaterial().SpecularColor = m_prop.colors[0];
				}

				buf->getMaterial().setFlag(video::EMF_TRILINEAR_FILTER, use_trilinear_filter);
				buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, use_bilinear_filter);
				buf->getMaterial().setFlag(video::EMF_ANISOTROPIC_FILTER, use_anisotropic_filter);
			}
			// Set mesh color (only if lighting is disabled)
			if (!m_prop.colors.empty() && m_glow < 0)
				setMeshColor(mesh, m_prop.colors[0]);
		}
	}
}

void GenericCAO::updateAnimation()
{
	if (!m_animated_meshnode)
		return;

	if (m_animated_meshnode->getStartFrame() != m_animation_range.X ||
		m_animated_meshnode->getEndFrame() != m_animation_range.Y)
			m_animated_meshnode->setFrameLoop(m_animation_range.X, m_animation_range.Y);
	if (m_animated_meshnode->getAnimationSpeed() != m_animation_speed)
		m_animated_meshnode->setAnimationSpeed(m_animation_speed);
	m_animated_meshnode->setTransitionTime(m_animation_blend);
// Requires Irrlicht 1.8 or greater
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR >= 8) || IRRLICHT_VERSION_MAJOR > 1
	if (m_animated_meshnode->getLoopMode() != m_animation_loop)
		m_animated_meshnode->setLoopMode(m_animation_loop);
#endif
}

void GenericCAO::updateAnimationSpeed()
{
	if (!m_animated_meshnode)
		return;

	m_animated_meshnode->setAnimationSpeed(m_animation_speed);
}

void GenericCAO::updateBonePosition()
{
	if (m_bone_position.empty() || !m_animated_meshnode)
		return;

	m_animated_meshnode->setJointMode(irr::scene::EJUOR_CONTROL); // To write positions to the mesh on render
	for(std::unordered_map<std::string, core::vector2d<v3f>>::const_iterator
			ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii) {
		std::string bone_name = (*ii).first;
		v3f bone_pos = (*ii).second.X;
		v3f bone_rot = (*ii).second.Y;
		irr::scene::IBoneSceneNode* bone = m_animated_meshnode->getJointNode(bone_name.c_str());
		if(bone)
		{
			bone->setPosition(bone_pos);
			bone->setRotation(bone_rot);
		}
	}
}

void GenericCAO::updateAttachments()
{
	ClientActiveObject *parent = getParent();

	m_attached_to_local = parent && parent->isLocalPlayer();

	/*
	Following cases exist:
		m_attachment_parent_id == 0 && !parent
			This object is not attached
		m_attachment_parent_id != 0 && parent
			This object is attached
		m_attachment_parent_id != 0 && !parent
			This object will be attached as soon the parent is known
		m_attachment_parent_id == 0 && parent
			Impossible case
	*/

	if (!parent) { // Detach or don't attach
		if (m_matrixnode) {
			v3f old_pos = getPosition();

			m_matrixnode->setParent(m_smgr->getRootSceneNode());
			getPosRotMatrix().setTranslation(old_pos);
			m_matrixnode->updateAbsolutePosition();
		}
	}
	else // Attach
	{
		scene::ISceneNode *parent_node = parent->getSceneNode();
		scene::IAnimatedMeshSceneNode *parent_animated_mesh_node =
				parent->getAnimatedMeshSceneNode();
		if (parent_animated_mesh_node && !m_attachment_bone.empty()) {
			parent_node = parent_animated_mesh_node->getJointNode(m_attachment_bone.c_str());
		}

		if (m_matrixnode && parent_node) {
			m_matrixnode->setParent(parent_node);
			getPosRotMatrix().setTranslation(m_attachment_position);
			//setPitchYawRoll(getPosRotMatrix(), m_attachment_rotation);
			// use Irrlicht eulers instead
			getPosRotMatrix().setRotationDegrees(m_attachment_rotation);
			m_matrixnode->updateAbsolutePosition();
		}
	}
}

void GenericCAO::processMessage(const std::string &data)
{
	//infostream<<"GenericCAO: Got message"<<std::endl;
	std::istringstream is(data, std::ios::binary);
	// command
	u8 cmd = readU8(is);
	if (cmd == GENERIC_CMD_SET_PROPERTIES) {
		m_prop = gob_read_set_properties(is);

		m_selection_box = m_prop.selectionbox;
		m_selection_box.MinEdge *= BS;
		m_selection_box.MaxEdge *= BS;

		m_tx_size.X = 1.0 / m_prop.spritediv.X;
		m_tx_size.Y = 1.0 / m_prop.spritediv.Y;

		if(!m_initial_tx_basepos_set){
			m_initial_tx_basepos_set = true;
			m_tx_basepos = m_prop.initial_sprite_basepos;
		}
		if (m_is_local_player) {
			LocalPlayer *player = m_env->getLocalPlayer();
			player->makes_footstep_sound = m_prop.makes_footstep_sound;
			aabb3f collision_box = m_prop.collisionbox;
			collision_box.MinEdge *= BS;
			collision_box.MaxEdge *= BS;
			player->setCollisionbox(collision_box);
			player->setEyeHeight(m_prop.eye_height);
			player->setZoomFOV(m_prop.zoom_fov);
		}

		if ((m_is_player && !m_is_local_player) && m_prop.nametag.empty())
			m_prop.nametag = m_name;

		expireVisuals();
	} else if (cmd == GENERIC_CMD_UPDATE_POSITION) {
		// Not sent by the server if this object is an attachment.
		// We might however get here if the server notices the object being detached before the client.
		m_position = readV3F32(is);
		m_velocity = readV3F32(is);
		m_acceleration = readV3F32(is);

		if (std::fabs(m_prop.automatic_rotate) < 0.001f)
			m_rotation = readV3F32(is);
		else
			readV3F32(is);

		m_rotation = wrapDegrees_0_360_v3f(m_rotation);
		bool do_interpolate = readU8(is);
		bool is_end_position = readU8(is);
		float update_interval = readF32(is);

		// Place us a bit higher if we're physical, to not sink into
		// the ground due to sucky collision detection...
		if(m_prop.physical)
			m_position += v3f(0,0.002,0);

		if(getParent() != NULL) // Just in case
			return;

		if(do_interpolate)
		{
			if(!m_prop.physical)
				pos_translator.update(m_position, is_end_position, update_interval);
		} else {
			pos_translator.init(m_position);
		}
		rot_translator.update(m_rotation, false, update_interval);
		updateNodePos();
	} else if (cmd == GENERIC_CMD_SET_TEXTURE_MOD) {
		std::string mod = deSerializeString(is);

		// immediatly reset a engine issued texture modifier if a mod sends a different one
		if (m_reset_textures_timer > 0) {
			m_reset_textures_timer = -1;
			updateTextures(m_previous_texture_modifier);
		}
		updateTextures(mod);
	} else if (cmd == GENERIC_CMD_SET_SPRITE) {
		v2s16 p = readV2S16(is);
		int num_frames = readU16(is);
		float framelength = readF32(is);
		bool select_horiz_by_yawpitch = readU8(is);

		m_tx_basepos = p;
		m_anim_num_frames = num_frames;
		m_anim_framelength = framelength;
		m_tx_select_horiz_by_yawpitch = select_horiz_by_yawpitch;

		updateTexturePos();
	} else if (cmd == GENERIC_CMD_SET_PHYSICS_OVERRIDE) {
		float override_speed = readF32(is);
		float override_jump = readF32(is);
		float override_gravity = readF32(is);
		// these are sent inverted so we get true when the server sends nothing
		bool sneak = !readU8(is);
		bool sneak_glitch = !readU8(is);
		bool new_move = !readU8(is);


		if(m_is_local_player)
		{
			LocalPlayer *player = m_env->getLocalPlayer();
			player->physics_override_speed = override_speed;
			player->physics_override_jump = override_jump;
			player->physics_override_gravity = override_gravity;
			player->physics_override_sneak = sneak;
			player->physics_override_sneak_glitch = sneak_glitch;
			player->physics_override_new_move = new_move;
		}
	} else if (cmd == GENERIC_CMD_SET_ANIMATION) {
		// TODO: change frames send as v2s32 value
		v2f range = readV2F32(is);
		if (!m_is_local_player) {
			m_animation_range = v2s32((s32)range.X, (s32)range.Y);
			m_animation_speed = readF32(is);
			m_animation_blend = readF32(is);
			// these are sent inverted so we get true when the server sends nothing
			m_animation_loop = !readU8(is);
			updateAnimation();
		} else {
			LocalPlayer *player = m_env->getLocalPlayer();
			if(player->last_animation == NO_ANIM)
			{
				m_animation_range = v2s32((s32)range.X, (s32)range.Y);
				m_animation_speed = readF32(is);
				m_animation_blend = readF32(is);
				// these are sent inverted so we get true when the server sends nothing
				m_animation_loop = !readU8(is);
			}
			// update animation only if local animations present
			// and received animation is unknown (except idle animation)
			bool is_known = false;
			for (int i = 1;i<4;i++)
			{
				if(m_animation_range.Y == player->local_animations[i].Y)
					is_known = true;
			}
			if(!is_known ||
					(player->local_animations[1].Y + player->local_animations[2].Y < 1))
			{
					updateAnimation();
			}
		}
	} else if (cmd == GENERIC_CMD_SET_ANIMATION_SPEED) {
		m_animation_speed = readF32(is);
		updateAnimationSpeed();
	} else if (cmd == GENERIC_CMD_SET_BONE_POSITION) {
		std::string bone = deSerializeString(is);
		v3f position = readV3F32(is);
		v3f rotation = readV3F32(is);
		m_bone_position[bone] = core::vector2d<v3f>(position, rotation);

		updateBonePosition();
	} else if (cmd == GENERIC_CMD_ATTACH_TO) {
		u16 parent_id = readS16(is);
		std::string bone = deSerializeString(is);
		v3f position = readV3F32(is);
		v3f rotation = readV3F32(is);

		setAttachment(parent_id, bone, position, rotation);

		// localplayer itself can't be attached to localplayer
		if (!m_is_local_player)
			m_is_visible = !m_attached_to_local;
	} else if (cmd == GENERIC_CMD_PUNCHED) {
		u16 result_hp = readU16(is);

		// Use this instead of the send damage to not interfere with prediction
		s32 damage = (s32)m_hp - (s32)result_hp;

		m_hp = result_hp;

		if (m_is_local_player)
			m_env->getLocalPlayer()->hp = m_hp;

		if (damage > 0)
		{
			if (m_hp == 0)
			{
				// TODO: Execute defined fast response
				// As there is no definition, make a smoke puff
				ClientSimpleObject *simple = createSmokePuff(
						m_smgr, m_env, m_position,
						v2f(m_prop.visual_size.X, m_prop.visual_size.Y) * BS);
				m_env->addSimpleObject(simple);
			} else if (m_reset_textures_timer < 0) {
				// TODO: Execute defined fast response
				// Flashing shall suffice as there is no definition
				m_reset_textures_timer = 0.05;
				if(damage >= 2)
					m_reset_textures_timer += 0.05 * damage;
				updateTextures(m_current_texture_modifier + "^[brighten");
			}
		}

		if (m_hp == 0) {
			// Same as 'Server::DiePlayer'
			clearParentAttachment();
			// Same as 'ObjectRef::l_remove'
			if (!m_is_player)
				clearChildAttachments();
		}
	} else if (cmd == GENERIC_CMD_UPDATE_ARMOR_GROUPS) {
		m_armor_groups.clear();
		int armor_groups_size = readU16(is);
		for(int i=0; i<armor_groups_size; i++)
		{
			std::string name = deSerializeString(is);
			int rating = readS16(is);
			m_armor_groups[name] = rating;
		}
	} else if (cmd == GENERIC_CMD_UPDATE_NAMETAG_ATTRIBUTES) {
		// Deprecated, for backwards compatibility only.
		readU8(is); // version
		m_prop.nametag_color = readARGB8(is);
		if (m_nametag != NULL) {
			m_nametag->nametag_color = m_prop.nametag_color;
			v3f pos;
			pos.Y = m_prop.collisionbox.MaxEdge.Y + 0.3f;
			m_nametag->nametag_pos = pos;
		}
	} else if (cmd == GENERIC_CMD_SPAWN_INFANT) {
		u16 child_id = readU16(is);
		u8 type = readU8(is); // maybe this will be useful later
		(void)type;

		addAttachmentChild(child_id);
	} else {
		warningstream << FUNCTION_NAME
			<< ": unknown command or outdated client \""
			<< +cmd << "\"" << std::endl;
	}
}

/* \pre punchitem != NULL
 */
bool GenericCAO::directReportPunch(v3f dir, const ItemStack *punchitem,
		float time_from_last_punch)
{
	assert(punchitem);	// pre-condition
	const ToolCapabilities *toolcap =
			&punchitem->getToolCapabilities(m_client->idef());
	PunchDamageResult result = getPunchDamage(
			m_armor_groups,
			toolcap,
			punchitem,
			time_from_last_punch);

	if(result.did_punch && result.damage != 0)
	{
		if(result.damage < m_hp)
		{
			m_hp -= result.damage;
		} else {
			m_hp = 0;
			// TODO: Execute defined fast response
			// As there is no definition, make a smoke puff
			ClientSimpleObject *simple = createSmokePuff(
					m_smgr, m_env, m_position,
					v2f(m_prop.visual_size.X, m_prop.visual_size.Y) * BS);
			m_env->addSimpleObject(simple);
		}
		// TODO: Execute defined fast response
		// Flashing shall suffice as there is no definition
		if (m_reset_textures_timer < 0) {
			m_reset_textures_timer = 0.05;
			if (result.damage >= 2)
				m_reset_textures_timer += 0.05 * result.damage;
			updateTextures(m_current_texture_modifier + "^[brighten");
		}
	}

	return false;
}

std::string GenericCAO::debugInfoText()
{
	std::ostringstream os(std::ios::binary);
	os<<"GenericCAO hp="<<m_hp<<"\n";
	os<<"armor={";
	for(ItemGroupList::const_iterator i = m_armor_groups.begin();
			i != m_armor_groups.end(); ++i)
	{
		os<<i->first<<"="<<i->second<<", ";
	}
	os<<"}";
	return os.str();
}

// Prototype
GenericCAO proto_GenericCAO(NULL, NULL);
