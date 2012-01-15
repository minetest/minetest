/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "content_cao.h"

/*
	TestCAO
*/

class TestCAO : public ClientActiveObject
{
public:

	TestCAO(IGameDef *gamedef, ClientEnvironment *env):
		ClientActiveObject(0, gamedef, env),
		m_node(NULL),
		m_position(v3f(0,10*BS,0))
	{
		ClientActiveObject::registerType(getType(), create);
	}

	~TestCAO()
	{
	}

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
	{
		return new TestCAO(gamedef, env);
	}

	inline u8 getType() const
	{	return ACTIVEOBJECT_TYPE_TEST;	}

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
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

	void removeFromScene()
	{
		if(m_node == NULL)
			return;

		m_node->remove();
		m_node = NULL;
	}

	void updateLight(u8 light_at_pos)
	{
	}

	v3s16 getLightPosition()
	{
		return floatToInt(m_position, BS);
	}

	void updateNodePos()
	{
		if(m_node == NULL)
			return;

		m_node->setPosition(m_position);
		//m_node->setRotation(v3f(0, 45, 0));
	}

	void step(float dtime, ClientEnvironment *env)
	{
		if(m_node)
		{
			v3f rot = m_node->getRotation();
			//infostream<<"dtime="<<dtime<<", rot.Y="<<rot.Y<<std::endl;
			rot.Y += dtime * 180;
			m_node->setRotation(rot);
		}
	}

	void processMessage(const std::string &data)
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
private:
	scene::IMeshSceneNode *m_node;
	v3f m_position;
};

// Prototype
TestCAO proto_TestCAO(NULL, NULL);
