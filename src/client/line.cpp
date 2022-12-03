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

#include "line.h"
#include "content_cao.h"
#include "client.h"
#include "clientmap.h"
#include "light.h"
#include "mapnode.h"
#include "nodedef.h"
#include "log.h"
#include "mapblock_mesh.h"

LineSceneNode::LineSceneNode(LineParams params, ClientEnvironment *cenv,
	scene::ISceneManager *mgr, s32 id)
: scene::ISceneNode(mgr->getRootSceneNode(), mgr, id), params(params), env(cenv)
{
	mat.MaterialType = video::EMT_ONETEXTURE_BLEND;
	mat.Lighting = false;
	mat.NormalizeNormals = true;
	mat.FogEnable = true;

	updateMaterial();

	mat.BlendOperation = video::EBO_ADD;
	mat.MaterialTypeParam = video::pack_textureBlendFunc(
		video::EBF_SRC_ALPHA,
		video::EBF_ONE_MINUS_SRC_ALPHA,
		video::EMFN_MODULATE_1X,
		video::EAS_TEXTURE | video::EAS_VERTEX_COLOR
	);

	current_animation_frame = 0;
	animation_time = 0.0f;

	mat.TextureLayers[0].TextureWrapU = (u8)video::ETC_REPEAT;
	mat.TextureLayers[0].TextureWrapV = (u8)video::ETC_REPEAT;
}

void LineSceneNode::updateEndsPositions()
{
	pos.first = calcPosition(params.attached_ids.first, params.pos.first);
	pos.second = calcPosition(params.attached_ids.second, params.pos.second);
}

void LineSceneNode::updateEndsLights()
{
	color.first = calcLight(pos.first, params.color.first);
	color.second = calcLight(pos.second, params.color.second);
}

v3f LineSceneNode::calcPosition(u16 id, v3f pos)
{
	GenericCAO* cao = env->getGenericCAO(id);

	if (cao)
		cao->getSceneNode()->getAbsoluteTransformation().transformVect(pos);
	else
		pos -= intToFloat(env->getCameraOffset(), BS);

	return pos;
}

video::SColor LineSceneNode::calcLight(v3f pos, video::SColor color)
{
	ClientMap &map = env->getClientMap();

	bool is_valid_pos;
	MapNode node = map.getNode(floatToInt(pos, BS), &is_valid_pos);

	u8 light = decode_light((is_valid_pos ? node.getLightBlend(env->getDayNightRatio(),
		env->getGameDef()->getNodeDefManager()->getLightingFlags(node)) :
		blend_light(env->getDayNightRatio(), LIGHT_SUN, 0)) + params.light_level);

	color.setRed(light * color.getRed() / 255);
	color.setGreen(light * color.getGreen() / 255);
	color.setBlue(light * color.getBlue() / 255);

	return color;
}

void LineSceneNode::updateAnimation(f32 dtime)
{
	if (params.animation.type == TAT_NONE)
		return;

	video::ITexture *tex = mat.getTexture(0);

	if (!tex)
		return;

	animation_time += dtime;

	v2u32 tex_size = tex->getSize();

	int frames_count, frame_length;
	params.animation.determineParams(tex_size, &frames_count, &frame_length, nullptr);
	f32 frame_length_f = frame_length / 1000.0f;

	u32 frames_delta = u32(animation_time / frame_length_f);

	current_animation_frame += frames_delta;

	if (current_animation_frame > (u32)frames_count)
		current_animation_frame -= (u32)frames_count;

	animation_time -= (frames_delta * frame_length_f);
}

void LineSceneNode::updateMaterial()
{
	mat.TextureLayers[0].Texture = env->getGameDef()->getTextureSource()->getTextureForMesh(params.texture);

	mat.TextureLayers[0].MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
	mat.TextureLayers[0].MagFilter = video::ETMAGF_NEAREST;

	mat.BackfaceCulling = !params.backface_culling;
}

void LineSceneNode::draw3DLineStrip(bool vertical) const
{
	f32 dist = pos.first.getDistanceFrom(pos.second);

	v3f positions[4];

	if (!vertical) {
		positions[0] = v3f(-params.width / 2, 0.0f, 0.0f); // LOWER LEFT
		positions[1] = v3f(params.width / 2, 0.0f, 0.0f); // LOWER RIGHT
		positions[2] = v3f(params.width / 2, 0.0f, dist); // UPPER LEFT
		positions[3] = v3f(-params.width / 2, 0.0f, dist); // UPPER RIGHT
	}
	else {
		positions[0] = v3f(0.0f, params.width / 2, 0.0f); // LOWER LEFT
		positions[1] = v3f(0.0f, -params.width / 2, 0.0f); // LOWER RIGHT
		positions[2] = v3f(0.0f, -params.width / 2, dist); // UPPER LEFT
		positions[3] = v3f(0.0f, params.width / 2, dist); // UPPER RIGHT
	}

	v3f dir = pos.second - pos.first;
	dir.normalize();

	v3f rotations = dir.getHorizontalAngle();

	core::matrix4 rot_mat, axis_rot_mat, trans_mat, res_mat;

	rot_mat.setRotationRadians(v3f(core::degToRad(rotations.X), core::degToRad(rotations.Y), 0.0f));
	axis_rot_mat.setRotationAxisRadians(params.axis_angle, dir);
	trans_mat.setTranslation(pos.first);

	res_mat = trans_mat * axis_rot_mat * rot_mat;

	res_mat.transformVect(positions[0]);
	res_mat.transformVect(positions[1]);
	res_mat.transformVect(positions[2]);
	res_mat.transformVect(positions[3]);

	core::plane3df line_plane(positions[3], positions[2], positions[1]);

	video::ITexture *tex = mat.getTexture(0);

	v2f ll_corner_coords(0.0f, 0.0f);
	v2f ur_corner_coords(1.0f, 1.0f);

	if (tex) {
		core::dimension2du tex_size = tex->getSize();

		if (params.animation.type != TAT_NONE) {
			v2u32 frame_size;
			params.animation.determineParams(tex_size, nullptr, nullptr, &frame_size);

			f32 aspect = (f32)frame_size.Y / frame_size.X;
			f32 tcoords_u = dist * aspect / params.width;


			ll_corner_coords = params.animation.getTextureCoords(tex_size, current_animation_frame);
			ur_corner_coords.X = ll_corner_coords.X + (f32)frame_size.X / tex_size.Width * tcoords_u;
			ur_corner_coords.Y = ll_corner_coords.Y + (f32)frame_size.Y / tex_size.Height;
		}
		else {
			f32 aspect = (f32)tex_size.Height / tex_size.Width;
			f32 tcoords_u = dist * aspect / params.width;

			ur_corner_coords.X = tcoords_u;
		}
	}
	video::S3DVertex vertices[4] = {
		video::S3DVertex(positions[0], line_plane.Normal, color.first, ll_corner_coords),
		video::S3DVertex(positions[1], line_plane.Normal, color.first, v2f(ll_corner_coords.X, ur_corner_coords.Y)),
		video::S3DVertex(positions[2], line_plane.Normal, color.second, ur_corner_coords),
		video::S3DVertex(positions[3], line_plane.Normal, color.second, v2f(ur_corner_coords.X, ll_corner_coords.Y))
	};

	video::IVideoDriver *vdrv = SceneManager->getVideoDriver();

	vdrv->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	vdrv->setMaterial(mat);

	u16 indices[] = {0, 1, 2, 2, 3, 0};

	vdrv->drawIndexedTriangleList(vertices, 4, indices, 2);
}

void LineSceneNode::calc3DCircle(bool first, video::S3DVertex *verts, u16 *inds, u32 verts_c) const
{
	const v3f &center_pos = first ? pos.first : pos.second;
	v3f normal(0.0f, 0.0f, first ? -1.0f : 1.0f);
	video::SColor col = first ? color.first : color.second;

	// Unfinished code of the future animation support for this type:
	// https://gist.github.com/Andrey2470T/5a2b4af215f65306dfbaeae1a47e132e

	v2f tcoords(0.5f, 0.5f);

	verts[0] = video::S3DVertex(center_pos, normal, col, tcoords);

	tcoords.X = -0.5f;
	tcoords.Y = 0.0f;

	v3f radius(-params.width / 2, 0.0f, 0.0f);

	f32 rot_steps = 360.0f / (verts_c - 2);

	inds[0] = 0;

	core::matrix4 trans_mat, rot_mat;
	trans_mat.setTranslation(center_pos);

	v3f dist = pos.second - pos.first;

	v3f rotations = dist.getHorizontalAngle();
	rot_mat.setRotationRadians(v3f(core::degToRad(rotations.X), core::degToRad(rotations.Y), 0.0f));

	core::matrix4 res_mat = trans_mat * rot_mat;

	for (u32 i = 0; i < verts_c - 1; i++) {
		v3f rotated_radius = radius;
		rotated_radius.rotateXYBy(rot_steps * i);
		res_mat.transformVect(rotated_radius);

		v2f rotated_tcoords = tcoords;
		rotated_tcoords.rotateBy(-rot_steps * i);
		rotated_tcoords += v2f(0.5f, 0.5f);

		verts[i + 1] = video::S3DVertex(
			rotated_radius,
			normal,
			col,
			rotated_tcoords
		);

		inds[i + 1] = i + 1;
	}
}

void LineSceneNode::drawCylindricShape() const
{
	const u32 circle_verts_c = 22; // including the center and matching vertices at the seam
	std::vector<video::S3DVertex> st_circle_verts(circle_verts_c);
	std::vector<u16> st_circle_inds(circle_verts_c);

	calc3DCircle(true, st_circle_verts.data(), st_circle_inds.data(), circle_verts_c);

	std::vector<video::S3DVertex> end_circle_verts(circle_verts_c);
	std::vector<u16> end_circle_inds(circle_verts_c);

	calc3DCircle(false, end_circle_verts.data(), end_circle_inds.data(), circle_verts_c);

	const u32 cyl_verts_c = (circle_verts_c - 1) * 2; // count of vertices on both circles + matching vertices
	std::vector<video::S3DVertex> cylinder_verts;
	std::vector<u16> cylinder_inds;

	video::ITexture *tex = mat.getTexture(0);

	f32 tcoords_u = 0.0f;
	f32 tcoords_v = 0.0f;

	f32 dist;
	f32 cyl_length;
	f32 aspect;

	if (tex) {
		core::dimension2du tex_size = mat.getTexture(0)->getSize();

		aspect = (f32)tex_size.Height / tex_size.Width;

		cyl_length = (circle_verts_c - 2) * st_circle_verts[1].Pos.getDistanceFrom(st_circle_verts[2].Pos);

		dist = st_circle_verts[0].Pos.getDistanceFrom(end_circle_verts[0].Pos);
		tcoords_u = dist * aspect / cyl_length;
		tcoords_v = 1.0f;
	}

	for (u16 i = 1; i <= cyl_verts_c / 2; i++) {
		video::S3DVertex vert1 = st_circle_verts[i];
		video::S3DVertex vert2 = end_circle_verts[i];

		static v3f normal;

		normal = core::plane3df(
			i == 1 ? st_circle_verts[circle_verts_c - 2].Pos : st_circle_verts[i - 1].Pos,
			i == 1 ? end_circle_verts[circle_verts_c - 2].Pos : end_circle_verts[i - 1].Pos,
			i == circle_verts_c - 1 ? end_circle_verts[2].Pos : end_circle_verts[i + 1].Pos
		).Normal;

		vert1.Normal = normal;
		vert2.Normal = normal;

		tcoords_v = tex ? 1.0f - (i - 1) * (1.0f / (circle_verts_c - 2)) : tcoords_v;

		vert1.TCoords = tex ? v2f(0.0f, tcoords_v) : v2f(0.0f, 0.0f);
		vert2.TCoords = tex ? v2f(tcoords_u, tcoords_v) : v2f(0.0f, 0.0f);

		cylinder_verts.push_back(vert1);
		cylinder_verts.push_back(vert2);
	}

	for (u16 i = 0; i < cyl_verts_c; i += 2) {
		cylinder_inds.push_back(i);
		cylinder_inds.push_back(i + 1);
	}

	video::IVideoDriver *vdrv = SceneManager->getVideoDriver();

	vdrv->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	vdrv->setMaterial(mat);

	vdrv->drawIndexedTriangleFan(st_circle_verts.data(), circle_verts_c, st_circle_inds.data(), circle_verts_c - 2);

	vdrv->drawIndexedTriangleFan(end_circle_verts.data(), circle_verts_c, end_circle_inds.data(), circle_verts_c - 2);

	vdrv->drawVertexPrimitiveList(
		cylinder_verts.data(),
		cyl_verts_c,
		cylinder_inds.data(),
		cyl_verts_c - 2,
		video::EVT_STANDARD,
		scene::EPT_TRIANGLE_STRIP,
		video::EIT_16BIT
	);
}

void LineSceneNode::render()
{
	switch(params.type) {
		case LineType::FLAT:
			draw3DLineStrip();
			break;
		case LineType::DIHEDRAL:
			draw3DLineStrip();
			draw3DLineStrip(true);
			break;
		case LineType::CYLINDRIC:
			drawCylindricShape();
			break;
		default:
			break;
	}
}

void LineSceneNode::OnRegisterSceneNode()
{
	setAutomaticCulling(scene::EAC_OFF);
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);

	ISceneNode::OnRegisterSceneNode();
}

LineSceneNodeManager::~LineSceneNodeManager()
{
	MutexAutoLock lock(m_line_scenenode_list_lock);

	for (auto &p : m_line_scenenode_list)
		p.second->remove();
}

void LineSceneNodeManager::add3DLineToList(u32 id, LineSceneNode *line_node)
{
	MutexAutoLock lock(m_line_scenenode_list_lock);

	m_line_scenenode_list.insert({id, line_node});
}

void LineSceneNodeManager::remove3DLineFromList(u32 id)
{
	MutexAutoLock lock(m_line_scenenode_list_lock);

	if (!m_line_scenenode_list[id])
		return;

	m_line_scenenode_list[id]->remove();
	m_line_scenenode_list.erase(id);
}

void LineSceneNodeManager::handle3DLineEvent(ClientEventType event_type, u32 id, LineParams *params)
{
	switch (event_type) {
		case CE_ADD_3DLINE:
		{
			LineSceneNode *line = new LineSceneNode(
				*params,
				m_env,
				m_env->getGameDef()->getSceneManager(),
				id
			);

			add3DLineToList(id, line);
			break;
		}
		case CE_CHANGE_3DLINE_PROPERTIES:
		{
			// If the client enviroment doesn't have the line with such id, skip it
			// One of reasons may be joining a new player whose the lines nodes map is empty
			if (!m_line_scenenode_list[id])
				break;

			auto& line_params = m_line_scenenode_list[id]->params;

			for (u16 prop = 0; prop < LineParams::LP_COUNT; prop++) {
				if (!params->last_changed_props[prop])
					continue;

				switch ((LineParams::LineProperty)prop) {
					case LineParams::LP_TYPE:
						line_params.type = params->type;
						break;
					case LineParams::LP_START_POS:
						line_params.pos.first = params->pos.first;
						break;
					case LineParams::LP_END_POS:
						line_params.pos.second = params->pos.second;
						break;
					case LineParams::LP_START_COLOR:
						line_params.color.first = params->color.first;
						break;
					case LineParams::LP_END_COLOR:
						line_params.color.second = params->color.second;
						break;
					case LineParams::LP_START_ATTACH_TO:
						line_params.attached_ids.first = params->attached_ids.first;
						break;
					case LineParams::LP_END_ATTACH_TO:
						line_params.attached_ids.second = params->attached_ids.second;
						break;
					case LineParams::LP_WIDTH:
						line_params.width = params->width;
						break;
					case LineParams::LP_TEXTURE:
						line_params.texture = params->texture;
						m_line_scenenode_list[id]->updateMaterial();
						break;
					case LineParams::LP_PLAYERNAME:
						line_params.playername = params->playername;
						break;
					case LineParams::LP_ANIMATION:
						line_params.animation = params->animation;
						break;
					case LineParams::LP_BACKFACE_CULLING:
						line_params.backface_culling = params->backface_culling;
						m_line_scenenode_list[id]->updateMaterial();
						break;
					case LineParams::LP_LIGHT_LEVEL:
						line_params.light_level = params->light_level;
						break;
					case LineParams::LP_AXIS_ANGLE:
						line_params.axis_angle = params->axis_angle;
						break;
					default:
						break;
				}
			}
			break;
		}
		case CE_REMOVE_3DLINE:
		{
			remove3DLineFromList(id);
			break;
		}
		default:
			break;
	}

	delete params;
}

void LineSceneNodeManager::step(f32 dtime)
{
	MutexAutoLock lock(m_line_scenenode_list_lock);

	const float radius = g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE * BS;

	v3f ppos = m_env->getLocalPlayer()->getPosition();

	for (auto line : m_line_scenenode_list) {
		auto &pos = line.second->pos;

		v3f cam_offset = intToFloat(m_env->getCameraOffset(), BS);
		bool is_far_from_start_pos = ppos.getDistanceFromSQ(pos.first+cam_offset) > radius * radius;
		bool is_far_from_end_pos = ppos.getDistanceFromSQ(pos.second+cam_offset) > radius * radius;

		bool attachments_invalid = false;

		u16 first_attach_id = line.second->params.attached_ids.first;
		u16 second_attach_id = line.second->params.attached_ids.second;

		if (first_attach_id && !m_env->getGenericCAO(first_attach_id))
			attachments_invalid = true;

		if (second_attach_id && !m_env->getGenericCAO(second_attach_id))
			attachments_invalid = true;

		// The line gets to be invisible if distance to any of its ends exceeds 'max_block_send_distance' setting.
		if (is_far_from_start_pos || is_far_from_end_pos || attachments_invalid)
			line.second->setVisible(false);
		else
			line.second->setVisible(true);

		line.second->updateAnimation(dtime);

		line.second->updateEndsPositions();

		line.second->updateEndsLights();
	}
}
