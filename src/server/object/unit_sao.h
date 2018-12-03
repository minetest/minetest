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

#pragma once

#include "serverobject.h"
#include "object_properties.h"

class UnitSAO: public ServerActiveObject
{
public:
	UnitSAO(ServerEnvironment *env, v3f pos);
	virtual ~UnitSAO() = default;

	void setRotation(v3f rotation) { m_rotation = rotation; }
	const v3f &getRotation() const { return m_rotation; }
	v3f getRadRotation() { return m_rotation * core::DEGTORAD; }

	// Deprecated
	f32 getRadYawDep() const { return (m_rotation.Y + 90.) * core::DEGTORAD; }

	s16 getHP() const { return m_hp; }
	// Use a function, if isDead can be defined by other conditions
	bool isDead() const { return m_hp == 0; }

	inline bool isAttached() const
	{ return getParent(); }

	void setArmorGroups(const ItemGroupList &armor_groups);
	const ItemGroupList &getArmorGroups();
	void setAnimation(v2f frame_range, float frame_speed, float frame_blend, bool frame_loop);
	void getAnimation(v2f *frame_range, float *frame_speed, float *frame_blend, bool *frame_loop);
	void setAnimationSpeed(float frame_speed);
	void setBonePosition(const std::string &bone, v3f position, v3f rotation);
	void getBonePosition(const std::string &bone, v3f *position, v3f *rotation);
	void setAttachment(int parent_id, const std::string &bone, v3f position, v3f rotation);
	void getAttachment(int *parent_id, std::string *bone, v3f *position, v3f *rotation);
	void clearChildAttachments();
	void clearParentAttachment();
	void addAttachmentChild(int child_id);
	void removeAttachmentChild(int child_id);
	const std::unordered_set<int> &getAttachmentChildIds();
	ServerActiveObject *getParent() const;
	ObjectProperties* accessObjectProperties();
	void notifyObjectPropertiesModified();
protected:
	s16 m_hp = -1;

	v3f m_rotation;

	bool m_properties_sent = true;
	ObjectProperties m_prop;

	ItemGroupList m_armor_groups;
	bool m_armor_groups_sent = false;

	v2f m_animation_range;
	float m_animation_speed = 0.0f;
	float m_animation_blend = 0.0f;
	bool m_animation_loop = true;
	bool m_animation_sent = false;
	bool m_animation_speed_sent = false;

	// Stores position and rotation for each bone name
	std::unordered_map<std::string, core::vector2d<v3f>> m_bone_position;
	bool m_bone_position_sent = false;

	int m_attachment_parent_id = 0;
	std::unordered_set<int> m_attachment_child_ids;
	std::string m_attachment_bone = "";
	v3f m_attachment_position;
	v3f m_attachment_rotation;
	bool m_attachment_sent = false;
private:
	void onAttach(int parent_id);
	void onDetach(int parent_id);
};
