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

#ifndef CONTENT_CAO_HEADER
#define CONTENT_CAO_HEADER

#include <map>
#include "irrlichttypes_extrabloated.h"
#include "content_object.h"
#include "clientobject.h"
#include "object_properties.h"
#include "itemgroup.h"

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

	SmoothTranslator();

	void init(v3f vect);

	void sharpen();

	void update(v3f vect_new, bool is_end_position=false, float update_interval=-1);

	void translate(f32 dtime);

	bool is_moving();
};

class GenericCAO : public ClientActiveObject
{
private:
	// Only set at initialization
	std::string m_name;
	bool m_is_player;
	bool m_is_local_player;
	int m_id;
	// Property-ish things
	ObjectProperties m_prop;
	//
	scene::ISceneManager *m_smgr;
	IrrlichtDevice *m_irr;
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_meshnode;
	scene::IAnimatedMeshSceneNode *m_animated_meshnode;
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
	v2s32 m_animation_range;
	int m_animation_speed;
	int m_animation_blend;
	std::map<std::string, core::vector2d<v3f> > m_bone_position; // stores position and rotation for each bone name
	std::string m_attachment_bone;
	v3f m_attachment_position;
	v3f m_attachment_rotation;
	bool m_attached_to_local;
	int m_anim_frame;
	int m_anim_num_frames;
	float m_anim_framelength;
	float m_anim_timer;
	ItemGroupList m_armor_groups;
	float m_reset_textures_timer;
	bool m_visuals_expired;
	float m_step_distance_counter;
	u8 m_last_light;
	bool m_is_visible;

	std::vector<u16> m_children;

public:
	GenericCAO(IGameDef *gamedef, ClientEnvironment *env);

	~GenericCAO();

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
	{
		return new GenericCAO(gamedef, env);
	}

	inline u8 getType() const
	{
		return ACTIVEOBJECT_TYPE_GENERIC;
	}

	void initialize(const std::string &data);

	ClientActiveObject *getParent();

	bool getCollisionBox(aabb3f *toset);

	bool collideWithObjects();

	core::aabbox3d<f32>* getSelectionBox();

	v3f getPosition();

	scene::IMeshSceneNode *getMeshSceneNode();

	scene::IAnimatedMeshSceneNode *getAnimatedMeshSceneNode();

	scene::IBillboardSceneNode *getSpriteSceneNode();

	inline bool isPlayer() const
	{
		return m_is_player;
	}

	inline bool isLocalPlayer() const
	{
		return m_is_local_player;
	}

	inline bool isVisible() const
	{
		return m_is_visible;
	}

	inline void setVisible(bool toset)
	{
		m_is_visible = toset;
	}

	void setAttachments();

	void removeFromScene(bool permanent);

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr);

	inline void expireVisuals()
	{
		m_visuals_expired = true;
	}

	void updateLight(u8 light_at_pos);

	v3s16 getLightPosition();

	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void updateTexturePos();

	void updateTextures(const std::string &mod);

	void updateAnimation();

	void updateBonePosition();

	void updateAttachments();

	void processMessage(const std::string &data);

	bool directReportPunch(v3f dir, const ItemStack *punchitem=NULL,
			float time_from_last_punch=1000000);

	std::string debugInfoText();
};


#endif
