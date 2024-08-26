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

#include "irr_aabb3d.h"
#include "irr_v3d.h"
#include <quaternion.h>
#include <string>
#include <unordered_map>


enum ActiveObjectType {
	ACTIVEOBJECT_TYPE_INVALID = 0,
	ACTIVEOBJECT_TYPE_TEST = 1,
// Obsolete stuff
// ACTIVEOBJECT_TYPE_ITEM = 2,
// ACTIVEOBJECT_TYPE_RAT = 3,
// ACTIVEOBJECT_TYPE_OERKKI1 = 4,
// ACTIVEOBJECT_TYPE_FIREFLY = 5,
// ACTIVEOBJECT_TYPE_MOBV2 = 6,
// End obsolete stuff
	ACTIVEOBJECT_TYPE_LUAENTITY = 7,
// Special type, not stored as a static object
	ACTIVEOBJECT_TYPE_PLAYER = 100,
// Special type, only exists as CAO
	ACTIVEOBJECT_TYPE_GENERIC = 101,
};
// Other types are defined in content_object.h

struct ActiveObjectMessage
{
	ActiveObjectMessage(u16 id_, bool reliable_=true, std::string_view data_ = "") :
		id(id_),
		reliable(reliable_),
		datastring(data_)
	{}

	u16 id;
	bool reliable;
	std::string datastring;
};

enum ActiveObjectCommand {
	AO_CMD_SET_PROPERTIES,
	AO_CMD_UPDATE_POSITION,
	AO_CMD_SET_TEXTURE_MOD,
	AO_CMD_SET_SPRITE,
	AO_CMD_PUNCHED,
	AO_CMD_UPDATE_ARMOR_GROUPS,
	AO_CMD_SET_ANIMATION,
	AO_CMD_SET_BONE_POSITION,
	AO_CMD_ATTACH_TO,
	AO_CMD_SET_PHYSICS_OVERRIDE,
	AO_CMD_OBSOLETE1,
	// ^ UPDATE_NAMETAG_ATTRIBUTES deprecated since 0.4.14, removed in 5.3.0
	AO_CMD_SPAWN_INFANT,
	AO_CMD_SET_ANIMATION_SPEED
};

struct BoneOverride
{
	struct PositionProperty
	{
		v3f previous;
		v3f vector;
		bool absolute = false;
		f32 interp_timer = 0;
	} position;

	v3f getPosition(v3f anim_pos) const {
		f32 progress = dtime_passed / position.interp_timer;
		if (progress > 1.0f || position.interp_timer == 0.0f)
			progress = 1.0f;
		return position.vector.getInterpolated(position.previous, progress)
				+ (position.absolute ? v3f() : anim_pos);
	}

	struct RotationProperty
	{
		core::quaternion previous;
		core::quaternion next;
		// Redundantly store the euler angles serverside
		// so that we can return them in the appropriate getters
		v3f next_radians;
		bool absolute = false;
		f32 interp_timer = 0;
	} rotation;

	v3f getRotationEulerDeg(v3f anim_rot_euler) const {
		core::quaternion rot;

		f32 progress = dtime_passed / rotation.interp_timer;
		if (progress > 1.0f || rotation.interp_timer == 0.0f)
			progress = 1.0f;
		rot.slerp(rotation.previous, rotation.next, progress);
		if (!rotation.absolute) {
			core::quaternion anim_rot(anim_rot_euler * core::DEGTORAD);
			rot = rot * anim_rot; // first rotate by anim. bone rot., then rot.
		}

		v3f rot_euler;
		rot.toEuler(rot_euler);
		return rot_euler * core::RADTODEG;
	}

	struct ScaleProperty
	{
		v3f previous;
		v3f vector{1, 1, 1};
		bool absolute = false;
		f32 interp_timer = 0;
	} scale;

	v3f getScale(v3f anim_scale) const {
		f32 progress = dtime_passed / scale.interp_timer;
		if (progress > 1.0f || scale.interp_timer == 0.0f)
			progress = 1.0f;
		return scale.vector.getInterpolated(scale.previous, progress)
				* (scale.absolute ? v3f(1) : anim_scale);
	}

	f32 dtime_passed = 0;

	bool isIdentity() const
	{
		return !position.absolute && position.vector == v3f()
				&& !rotation.absolute && rotation.next == core::quaternion()
				&& !scale.absolute && scale.vector == v3f(1);
	}
};

typedef std::unordered_map<std::string, BoneOverride> BoneOverrideMap;


/*
	Parent class for ServerActiveObject and ClientActiveObject
*/
class ActiveObject
{
public:
	ActiveObject(u16 id):
		m_id(id)
	{
	}

	u16 getId() const
	{
		return m_id;
	}

	void setId(u16 id)
	{
		m_id = id;
	}

	virtual ActiveObjectType getType() const = 0;


	/*!
	 * Returns the collision box of the object.
	 * This box is translated by the object's
	 * location.
	 * The box's coordinates are world coordinates.
	 * @returns true if the object has a collision box.
	 */
	virtual bool getCollisionBox(aabb3f *toset) const = 0;


	/*!
	 * Returns the selection box of the object.
	 * This box is not translated when the
	 * object moves.
	 * The box's coordinates are world coordinates.
	 * @returns true if the object has a selection box.
	 */
	virtual bool getSelectionBox(aabb3f *toset) const = 0;


	virtual bool collideWithObjects() const = 0;


	virtual void setAttachment(int parent_id, const std::string &bone, v3f position,
			v3f rotation, bool force_visible) {}
	virtual void getAttachment(int *parent_id, std::string *bone, v3f *position,
			v3f *rotation, bool *force_visible) const {}
	virtual void clearChildAttachments() {}
	virtual void clearParentAttachment() {}
	virtual void addAttachmentChild(int child_id) {}
	virtual void removeAttachmentChild(int child_id) {}
protected:
	u16 m_id; // 0 is invalid, "no id"
};
