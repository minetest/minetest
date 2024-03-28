/*
Minetest
Copyright (C) 2022-2023 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>
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

#include "client/client.h"
#include <SViewFrustum.h>
#include <ICameraSceneNode.h>

class UniversalCamera : public scene::ICameraSceneNode
{
public:
	UniversalCamera(ClientEnvironment *env, video::IVideoDriver *driver, scene::ISceneNode *parent,
			scene::ISceneManager *smgr, s32 id);
	~UniversalCamera() = default;

	virtual void OnRegisterSceneNode();
	virtual void OnAnimate(u32 time);
	virtual bool OnEvent(const SEvent &event);

	virtual void render();
	virtual void updateMatrices() {}
	virtual void bindTargetAndRotation(bool) {}
	virtual bool getTargetAndRotationBinding() const { return false; }

	virtual bool isInputReceiverEnabled() const { return true; }
	virtual void setInputReceiverEnabled(bool) {}

	virtual void buildProjectionMatrix();
	virtual void buildViewArea();

	virtual const scene::SViewFrustum *getViewFrustum() const { return &m_view_area; }
	virtual scene::ESCENE_NODE_TYPE getType() const { return scene::ESNT_CAMERA; }
	virtual const core::rect<f32> &getViewPortF32() const { return m_viewport; }
	virtual const aabb3f &getBoundingBox() const { return m_view_area.getBoundingBox(); }
	virtual const v3f &getUpVector() const { return m_up_vector; }
	virtual const v3f &getTarget() const { return m_target; }
	virtual const v3f &getRotation() const { return scene::ISceneNode::getRotation(); }
	virtual f32 getAspectRatio() const { return m_aspect; }
	virtual f32 getNearValue() const { return m_znear; }
	virtual f32 getFarValue() const { return m_zfar; }
	virtual f32 getFOV() const { return m_fov; }

	virtual void setUpVector(const v3f &pos) { m_up_vector = pos; }
	virtual void setProjectionMatrix(const core::matrix4 &, bool) {}
	virtual void setViewMatrixAffector(const core::matrix4 &) {}
	virtual void setRotation(const v3f &rot) { scene::ISceneNode::setRotation(rot); }
	virtual void setFOV(f32) {}

	virtual void setViewPort(const core::rectf &&vp)
	{
		m_mouse_pos.set(0,0);
		m_viewport = vp;
	}
	virtual void setTarget(const v3f &pos)
	{
		m_target = pos;
		updatePositionState();
	}
	virtual void setNearValue(f32 f)
	{
		m_znear = f;
		buildProjectionMatrix();
	}
	virtual void setFarValue(f32 f)
	{
		m_zfar = f;
		buildProjectionMatrix();
	}
	virtual void setAspectRatio(f32 f)
	{
		m_aspect = f;
		buildProjectionMatrix();
	}
	virtual void setProjectionMatrix(const core::matrix4 &proj)
	{
		m_view_area.getTransform(video::ETS_PROJECTION) = proj;
	}

	virtual const core::matrix4 &getViewMatrixAffector() const
	{
		return m_view_matrix_affector;
	}
	virtual const core::matrix4 &getProjectionMatrix() const
	{
		return m_view_area.getTransform(video::ETS_PROJECTION);
	}
	virtual const core::matrix4 &getViewMatrix() const
	{
		return m_view_area.getTransform(video::ETS_VIEW);
	}
	const core::recti getViewPort() const
	{
		/*
		 *  Redundant code but necessary to mitigate some data sync issues
		 *  between the camera render loop and the rendering pipeline.
		 */

		const auto &screensize = m_driver->getScreenSize();

		return core::recti(
			screensize.Width  * m_viewport.UpperLeftCorner.X,
			screensize.Height * m_viewport.UpperLeftCorner.Y,
			screensize.Width  * m_viewport.LowerRightCorner.X,
			screensize.Height * m_viewport.LowerRightCorner.Y
		);
	}

	inline void setCameraOffset(const v3s16 &offset)
	{
		m_camera_offset = v3f(offset.X, offset.Y, offset.Z) * BS;
	}

	inline const std::string &getRenderTextureName() const { return m_render_texture_name; }
	inline f32 getRenderTextureAspectRatio() const { return m_render_texture_aspect_ratio; }
	inline void setRenderTexture(const std::string &name, f32 aspect_ratio)
	{
		m_render_texture_name = name;
		m_render_texture_aspect_ratio = aspect_ratio;
	}

	void setPosition(const v3f &pos, bool interpolation = false, f32 speed = 0.f);
	void setRotation(const v3f &rot, bool interpolation = false, f32 speed = 0.f);
	void setFOV(f32 fov, bool interpolation = false, f32 speed = 0.f);
	void setZoom(f32 zoom, bool interpolation = false, f32 speed = 0.f);

	void step();
	void attachTo(ClientActiveObject *object, bool follow = true);
	inline void detach() { m_parent_id = -1; }

private:
	void interpolate();
	void manipulate(u32 time);
	void updateDeltaTime(u32 time);
	void updatePositionState();

	void doZoom();
	v3f doRotate(v3f &rotate);
	void doTranslate();

	void updatePos(const v3f &rot);

	inline bool haveParent() const { return m_parent_id != -1; }
	inline void setTargetOffset(const v3f &parent_pos)
	{
		m_target_offset = m_target - parent_pos;
	}
	inline void rotateTarget(v3f &target, const v3f &rot)
	{
		target.rotateXYBy(-rot.X, m_pos);
		target.rotateXZBy(-rot.Y, m_pos);
	}

	ClientEnvironment    *m_env    { nullptr };
	video::IVideoDriver  *m_driver { nullptr };
	scene::ISceneManager *m_smgr   { nullptr };

	std::string m_render_texture_name;

	core::dimension2du m_screensize;
	core::rectf m_viewport;

	scene::SViewFrustum m_view_area;
	core::matrix4 m_view_matrix_affector; // Not used but needed

	v3f m_pos;
	v3f m_rot;
	v3f m_target;

	v3f m_target_offset;
	v3f m_camera_offset;
	v3f m_up_vector { 0.f, 1.f, 0.f };

	v2f m_mouse_pos;
	v2f m_rotate_start;

	f32 m_aspect { 0.f };
	f32 m_fov    { core::PI / 2.5f };
	f32 m_znear  { 1.f };
	f32 m_zfar   { 1e5f };
	f32 m_roll   { 0.f };

	f32 m_render_texture_aspect_ratio { 1.f };

	f32 m_zoom     { 0.f };
	f32 m_new_zoom { 0.f };
	f32 m_zoom_min { 0.f };
	f32 m_zoom_max { 1000.f };

	u32 m_last_time;
	u32 m_dtime;

	u16 m_undersampling = MYMAX(g_settings->getU16("undersampling"), 1);

	int m_parent_id { -1 };

	bool m_parent_follow { false };
	bool m_mouse_pressed { false };

	enum Zoom { None, In, Out };

	struct {
		u8 zooming         { Zoom::None };
		bool rotating      { false };
		bool translating   { false };
		bool interpolating { false };
	} m_action;

	struct {
		f32 fov { 0.f };
		f32 zoom { 0.f };

		core::quaternion q_rotation;
		v3f rotation_counter;
		v3f rotation;
		v3f target;

		v3f translation;
		v3f start_translation;

		struct {
			f32 fov       { 0.f };
			f32 zoom      { 0.f };
			f32 rotate    { 0.f };
			f32 translate { 0.f };
		} speed;

		struct {
			f32 rotate    { 0.f };
			f32 translate { 0.f };
		} timer;
	} m_interp;
};
