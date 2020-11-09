
#include "line.h"
#include "log.h"
#include <cmath>

using namespace irr;

static void generateVertexPair( video::S3DVertex *verts, LineVertex from,
	LineVertex to, v3f camera_pos, float tcx, float tcy, bool wrap)
{
	v3f pos = from.getScenePos();
	v3f other = to.getScenePos();
	v3f tangent = other - pos;
	v3f eye = (camera_pos - pos).normalize();
	v3f bitangent = tangent.crossProduct(eye).normalize();
	v3f left = pos + bitangent * from.width / 2;
	v3f right = pos - bitangent * from.width / 2;

	if (wrap)
		tcx *= pos.getDistanceFrom(other) / to.width;

	verts[0] = video::S3DVertex(left.X, left.Y, left.Z,
								eye.X, eye.Y, eye.Z,
								from.color, tcx, tcy);
	verts[1] = video::S3DVertex(right.X, right.Y, right.Z,
								eye.X, eye.Y, eye.Z,
								from.color, tcx, 1 - tcy);
}

static const u16 quadFan[4] = {0, 3, 2, 1};
static const u16 lineSegment[4] = {0, 1};

void LineSceneNode::render()
{
	if ((mat.MaterialType == video::EMT_SOLID) ==
		(SceneManager->getSceneNodeRenderPass() == scene::ESNRP_TRANSPARENT))
		return;

	video::IVideoDriver *driver = SceneManager->getVideoDriver();
	if (!driver)
		return;

	auto camera = SceneManager->getActiveCamera();
	if (!camera)
		return;
	v3f camera_pos = camera->getPosition();

	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	mat.Thickness = src_point.width;

	if ( alphaMode == 0 )
		mat.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	else if ( alphaMode == 1 )
		mat.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	else if ( alphaMode == 2 )
		mat.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;

	driver->setMaterial(mat);

	float aspect = 1;
	float texWidth = 1;

	video::ITexture *tex = mat.getTexture(0);
	if (tex) {
		auto dim = tex->getSize();
		texWidth = dim.Width;
		aspect = (float)dim.Height / (float)dim.Width;
	}

	if (simple)
	{
		video::S3DVertex verts[2];
		v3f srcp = src_point.getScenePos();
		v3f srcn = (camera_pos - srcp).normalize();
		v3f dstp = dst_point.getScenePos();
		v3f dstn = (camera_pos - dstp).normalize();
		float tcx = 1;
		if (wrap_texture) {
			tcx = srcp.getDistanceFrom(dstp) * aspect / src_point.width;
		}
		verts[0] = video::S3DVertex(srcp.X, srcp.Y, srcp.Z,
									srcn.X, srcn.Y, srcn.Z,
									src_point.color, 0, 0.5);
		verts[1] = video::S3DVertex(dstp.X, dstp.Y, dstp.Z,
									dstn.X, dstn.Y, dstn.Z,
									dst_point.color, tcx, 0.5);

		driver->drawVertexPrimitiveList(verts, 2, lineSegment, 1,
										video::EVT_STANDARD, scene::EPT_LINES);
	}
	else
	{
		video::S3DVertex verts[4];
		video::S3DVertex *pverts = verts;

		generateVertexPair(pverts, src_point, dst_point, camera_pos, 0, 0, wrap_texture);
		generateVertexPair(pverts + 2, dst_point, src_point, camera_pos, aspect, 1, wrap_texture);

		driver->drawIndexedTriangleFan(verts, 4, quadFan, 2);
	}
}

void LineSceneNode::OnRegisterSceneNode()
{
	setAutomaticCulling(scene::EAC_OFF);
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
	}

	ISceneNode::OnRegisterSceneNode();
}

void LineSceneNode::OnAnimate(u32 t)
{
	recalculateBounds();
}