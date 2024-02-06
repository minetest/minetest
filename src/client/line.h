/*
Minetest
Copyright (C) 2023 Andrey2470T, AndreyT <andreyt2203@gmail.com>

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

#include "line_params.h"
#include <S3DVertex.h>
#include <SMaterial.h>
#include <ISceneNode.h>
#include "clientenvironment.h"
#include <ISceneManager.h>
#include <unordered_map>
#include "clientevent.h"
#include "log.h"
#include "../threading/mutex_auto_lock.h"

class LineSceneNode;

class LineSceneNodeManager
{
public:
	LineSceneNodeManager(ClientEnvironment* env)
	: m_env(env)
	{
	}

	~LineSceneNodeManager();

	void add3DLineToList(u32 id, LineSceneNode *line_node);
	void remove3DLineFromList(u32 id);
	void handle3DLineEvent(ClientEventType event_type, u32 id, LineParams *params);
	void step(f32 dtime);

private:
	ClientEnvironment *m_env;
	std::unordered_map<u32, LineSceneNode*> m_line_scenenode_list;
	std::mutex m_line_scenenode_list_lock;
};

class LineSceneNode : public scene::ISceneNode
{
public:
	// The aabbox of the line. Actually not used.
	core::aabbox3df bounds;

	// The properties of the line.
	LineParams params;

	// Actual positions and colors of the ends.
	std::pair<v3f, v3f> pos;
	std::pair<video::SColor, video::SColor> color;

	// Only one material is used.
	video::SMaterial mat;

	ClientEnvironment *env;

	u32 current_animation_frame;
	f32 animation_time;

	virtual void render();
	virtual void OnRegisterSceneNode();
	virtual void OnAnimate(u32 t)
	{
	}

	virtual const core::aabbox3df &getBoundingBox() const
	{
		return bounds;
	}

	virtual u32 getMaterialCount() const
	{
		return 1;
	}

	virtual const video::SMaterial &getMaterial(u32 i = 0) const
	{
		return mat;
	}

	// Reloads the texture, updates the clamp mode and backface culling flag.
	void updateMaterial();

	LineSceneNode(
		LineParams params,
		ClientEnvironment* cenv,
		scene::ISceneManager* mgr, s32 id
	);

private:
	// Calculates a new position of the line end with current 'pos' position.
	// If the end is attached to an object, 'pos' is relative.
	v3f calcPosition(u16 attach_id, v3f pos);

	// Calculates a new color of the line end based on the light level in the given 'pos' position
	// and the light level emitted by the line.
	video::SColor calcLight(v3f pos, video::SColor color);

	// Updates both ends positions.
	void updateEndsPositions();
	// Updates both ends colors.
	void updateEndsLights();
	// Update animation frame.
	void updateAnimation(f32 dtime);

	// Draws a rectangular line in the 3D space.
	// If 'vertical' = true, the line will be perpendicular to the horizontal plane.
	void draw3DLineStrip(bool vertical = false) const;

	// Calculates vertices of a circle in the 3D space.
	// Helper method for 'drawCylindricShape()'.
	void calc3DCircle(bool first, video::S3DVertex *verts, u16 *inds, u32 verts_c) const;

	// Draws a cylinder in the 3D space.
	// It draws two rounds with 20 vertices each and cylindrical surface between them.
	void drawCylindricShape() const;

	friend void LineSceneNodeManager::step(f32 dtime);
};
