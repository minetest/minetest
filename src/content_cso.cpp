/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "content_cso.h"
#include <IBillboardSceneNode.h>
#include "tile.h"
#include "environment.h"
#include "gamedef.h"
#include "log.h"
#include "map.h"

/*
static void setBillboardTextureMatrix(scene::IBillboardSceneNode *bill,
		float txs, float tys, int col, int row)
{
	video::SMaterial& material = bill->getMaterial(0);
	core::matrix4& matrix = material.getTextureMatrix(0);
	matrix.setTextureTranslate(txs*col, tys*row);
	matrix.setTextureScale(txs, tys);
}
*/

class SmokePuffCSO: public ClientSimpleObject
{
	float m_age;
	scene::IBillboardSceneNode *m_spritenode;
public:
	SmokePuffCSO(scene::ISceneManager *smgr,
			ClientEnvironment *env, v3f pos, v2f size):
		m_age(0),
		m_spritenode(NULL)
	{
		infostream<<"SmokePuffCSO: constructing"<<std::endl;
		m_spritenode = smgr->addBillboardSceneNode(
				NULL, v2f(1,1), pos, -1);
		m_spritenode->setMaterialTexture(0,
				env->getGameDef()->tsrc()->getTexture("smoke_puff.png"));
		m_spritenode->setMaterialFlag(video::EMF_LIGHTING, false);
		m_spritenode->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		//m_spritenode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
		m_spritenode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);
		m_spritenode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		m_spritenode->setColor(video::SColor(255,0,0,0));
		m_spritenode->setVisible(true);
		m_spritenode->setSize(size);
		/* Update brightness */
		u8 light = 64;
		try{
			MapNode n = env->getMap().getNode(floatToInt(pos, BS));
			light = decode_light(n.getLightBlend(env->getDayNightRatio(),
					env->getGameDef()->ndef()));
		}
		catch(InvalidPositionException &e){}
		video::SColor color(255,light,light,light);
		m_spritenode->setColor(color);
	}
	virtual ~SmokePuffCSO()
	{
		infostream<<"SmokePuffCSO: destructing"<<std::endl;
		m_spritenode->remove();
	}
	void step(float dtime)
	{
		m_age += dtime;
		if(m_age > 1.0){
			m_to_be_removed = true;
		}
	}
};

ClientSimpleObject* createSmokePuff(scene::ISceneManager *smgr,
		ClientEnvironment *env, v3f pos, v2f size)
{
	return new SmokePuffCSO(smgr, env, pos, size);
}

