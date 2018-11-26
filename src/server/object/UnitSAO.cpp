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

#include "server.h"
#include "scripting_server.h"
#include "server/object/UnitSAO.h"

UnitSAO::UnitSAO(ServerEnvironment *env, v3f pos):
	ServerActiveObject(env, pos)
{
	// Initialize something to armor groups
	m_armor_groups["fleshy"] = 100;
}

ServerActiveObject *UnitSAO::getParent() const
{
	if (!m_attachment_parent_id)
		return nullptr;
	// Check if the parent still exists
	ServerActiveObject *obj = m_env->getActiveObject(m_attachment_parent_id);

	return obj;
}

void UnitSAO::setArmorGroups(const ItemGroupList &armor_groups)
{
	m_armor_groups = armor_groups;
	m_armor_groups_sent = false;
}

const ItemGroupList &UnitSAO::getArmorGroups()
{
	return m_armor_groups;
}

void UnitSAO::setAnimation(v2f frame_range, float frame_speed, float frame_blend, bool frame_loop)
{
	// store these so they can be updated to clients
	m_animation_range = frame_range;
	m_animation_speed = frame_speed;
	m_animation_blend = frame_blend;
	m_animation_loop = frame_loop;
	m_animation_sent = false;
}

void UnitSAO::getAnimation(v2f *frame_range, float *frame_speed, float *frame_blend, bool *frame_loop)
{
	*frame_range = m_animation_range;
	*frame_speed = m_animation_speed;
	*frame_blend = m_animation_blend;
	*frame_loop = m_animation_loop;
}

void UnitSAO::setAnimationSpeed(float frame_speed)
{
	m_animation_speed = frame_speed;
	m_animation_speed_sent = false;
}

void UnitSAO::setBonePosition(const std::string &bone, v3f position, v3f rotation)
{
	// store these so they can be updated to clients
	m_bone_position[bone] = core::vector2d<v3f>(position, rotation);
	m_bone_position_sent = false;
}

void UnitSAO::getBonePosition(const std::string &bone, v3f *position, v3f *rotation)
{
	*position = m_bone_position[bone].X;
	*rotation = m_bone_position[bone].Y;
}

void UnitSAO::setAttachment(int parent_id, const std::string &bone, v3f position, v3f rotation)
{
	// Attachments need to be handled on both the server and client.
	// If we just attach on the server, we can only copy the position of the parent. Attachments
	// are still sent to clients at an interval so players might see them lagging, plus we can't
	// read and attach to skeletal bones.
	// If we just attach on the client, the server still sees the child at its original location.
	// This breaks some things so we also give the server the most accurate representation
	// even if players only see the client changes.

	int old_parent = m_attachment_parent_id;
	m_attachment_parent_id = parent_id;
	m_attachment_bone = bone;
	m_attachment_position = position;
	m_attachment_rotation = rotation;
	m_attachment_sent = false;

	if (parent_id != old_parent) {
		onDetach(old_parent);
		onAttach(parent_id);
	}
}

void UnitSAO::getAttachment(int *parent_id, std::string *bone, v3f *position,
	v3f *rotation)
{
	*parent_id = m_attachment_parent_id;
	*bone = m_attachment_bone;
	*position = m_attachment_position;
	*rotation = m_attachment_rotation;
}

void UnitSAO::clearChildAttachments()
{
	for (int child_id : m_attachment_child_ids) {
		// Child can be NULL if it was deleted earlier
		if (ServerActiveObject *child = m_env->getActiveObject(child_id))
			child->setAttachment(0, "", v3f(0, 0, 0), v3f(0, 0, 0));
	}
	m_attachment_child_ids.clear();
}

void UnitSAO::clearParentAttachment()
{
	ServerActiveObject *parent = nullptr;
	if (m_attachment_parent_id) {
		parent = m_env->getActiveObject(m_attachment_parent_id);
		setAttachment(0, "", m_attachment_position, m_attachment_rotation);
	} else {
		setAttachment(0, "", v3f(0, 0, 0), v3f(0, 0, 0));
	}
	// Do it
	if (parent)
		parent->removeAttachmentChild(m_id);
}

void UnitSAO::addAttachmentChild(int child_id)
{
	m_attachment_child_ids.insert(child_id);
}

void UnitSAO::removeAttachmentChild(int child_id)
{
	m_attachment_child_ids.erase(child_id);
}

const std::unordered_set<int> &UnitSAO::getAttachmentChildIds()
{
	return m_attachment_child_ids;
}

void UnitSAO::onAttach(int parent_id)
{
	if (!parent_id)
		return;

	ServerActiveObject *parent = m_env->getActiveObject(parent_id);

	if (!parent || parent->isGone())
		return; // Do not try to notify soon gone parent

	if (parent->getType() == ACTIVEOBJECT_TYPE_LUAENTITY) {
		// Call parent's on_attach field
		m_env->getScriptIface()->luaentity_on_attach_child(parent_id, this);
	}
}

void UnitSAO::onDetach(int parent_id)
{
	if (!parent_id)
		return;

	ServerActiveObject *parent = m_env->getActiveObject(parent_id);
	if (getType() == ACTIVEOBJECT_TYPE_LUAENTITY)
		m_env->getScriptIface()->luaentity_on_detach(m_id, parent);

	if (!parent || parent->isGone())
		return; // Do not try to notify soon gone parent

	if (parent->getType() == ACTIVEOBJECT_TYPE_LUAENTITY)
		m_env->getScriptIface()->luaentity_on_detach_child(parent_id, this);
}

ObjectProperties* UnitSAO::accessObjectProperties()
{
	return &m_prop;
}

void UnitSAO::notifyObjectPropertiesModified()
{
	m_properties_sent = false;
}

