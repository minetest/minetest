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
#include <list>
#include "content_cao.h"
#include "luaentity_common.h"

class LuaEntityCAO : public ClientActiveObject
{
private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_meshnode;
	scene::IBillboardSceneNode *m_spritenode;
	v3f m_position;
	v3f m_velocity;
	v3f m_acceleration;
	float m_yaw;
	struct LuaEntityProperties *m_prop;
	SmoothTranslator pos_translator;
	// Spritesheet/animation stuff
	v2f m_tx_size;
	v2s16 m_tx_basepos;
	bool m_tx_select_horiz_by_yawpitch;
	int m_anim_frame;
	int m_anim_num_frames;
	float m_anim_framelength;
	float m_anim_timer;
public:

	inline u8 getType() const	{ return ACTIVEOBJECT_TYPE_LUAENTITY; }

	inline core::aabbox3d<f32>* getSelectionBox()
	{ return &m_selection_box;	}

	inline v3f getPosition()
	{ return pos_translator.vect_show;	}

	LuaEntityCAO(IGameDef *gamedef, ClientEnvironment *env):
		ClientActiveObject(0, gamedef, env),
		m_selection_box(-BS/3.,-BS/3.,-BS/3., BS/3.,BS/3.,BS/3.),
		m_meshnode(NULL),
		m_spritenode(NULL),
		m_position(v3f(0,10*BS,0)),
		m_velocity(v3f(0,0,0)),
		m_acceleration(v3f(0,0,0)),
		m_yaw(0),
		m_prop(new LuaEntityProperties),
		m_tx_size(1,1),
		m_tx_basepos(0,0),
		m_tx_select_horiz_by_yawpitch(false),
		m_anim_frame(0),
		m_anim_num_frames(1),
		m_anim_framelength(0.2),
		m_anim_timer(0)
	{
		if(gamedef == NULL)
			ClientActiveObject::registerType(LuaEntityCAO::getType(), LuaEntityCAO::create);

	}

	void initialize(const std::string &data)
	{
		infostream<<"LuaEntityCAO: Got init data"<<std::endl;

		std::istringstream is(data, std::ios::binary);
		// version
		u8 version = readU8(is);
		// check version
		if(version != 0)
			return;
		// pos
		m_position = readV3F1000(is);
		// yaw
		m_yaw = readF1000(is);
		// properties
		std::istringstream prop_is(deSerializeLongString(is), std::ios::binary);
		m_prop->deSerialize(prop_is);

		infostream<<"m_prop: "<<m_prop->dump()<<std::endl;

		m_selection_box = m_prop->collisionbox;
		m_selection_box.MinEdge *= BS;
		m_selection_box.MaxEdge *= BS;

		pos_translator.init(m_position);

		m_tx_size.X = 1.0 / m_prop->spritediv.X;
		m_tx_size.Y = 1.0 / m_prop->spritediv.Y;
		m_tx_basepos.X = m_tx_size.X * m_prop->initial_sprite_basepos.X;
		m_tx_basepos.Y = m_tx_size.Y * m_prop->initial_sprite_basepos.Y;

		updateNodePos();
	}

	~LuaEntityCAO()
	{
		delete m_prop;
	}

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
	{
		return new LuaEntityCAO(gamedef, env);
	}


	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr)
	{
		if(m_meshnode != NULL || m_spritenode != NULL)
			return;

		//video::IVideoDriver* driver = smgr->getVideoDriver();

		if(m_prop->visual == "sprite"){
			infostream<<"LuaEntityCAO::addToScene(): single_sprite"<<std::endl;
			m_spritenode = smgr->addBillboardSceneNode(
					NULL, v2f(1, 1), v3f(0,0,0), -1);
			m_spritenode->setMaterialTexture(0,
					tsrc->getTextureRaw("unknown_block.png"));
			m_spritenode->setMaterialFlag(video::EMF_LIGHTING, false);
			m_spritenode->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
			m_spritenode->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
			m_spritenode->setMaterialFlag(video::EMF_FOG_ENABLE, true);
			m_spritenode->setColor(video::SColor(255,0,0,0));
			m_spritenode->setVisible(false); /* Set visible when brightness is known */
			m_spritenode->setSize(m_prop->visual_size*BS);
			{
				const float txs = 1.0 / 1;
				const float tys = 1.0 / 1;
				setBillboardTextureMatrix(m_spritenode,
						txs, tys, 0, 0);
			}
		} else if(m_prop->visual == "cube"){
			infostream<<"LuaEntityCAO::addToScene(): cube"<<std::endl;
			scene::IMesh *mesh = createCubeMesh(v3f(BS,BS,BS));
			m_meshnode = smgr->addMeshSceneNode(mesh, NULL);
			mesh->drop();
			
			m_meshnode->setScale(v3f(1));
			// Will be shown when we know the brightness
			m_meshnode->setVisible(false);
		} else if (m_prop->visual == "plant") {
			infostream<<"LuaEntityCAO::addToScene(): plant"<<std::endl;
			scene::IMesh *mesh = createPlantMesh(v3f(BS,BS,BS));
			m_meshnode = smgr->addMeshSceneNode(mesh, NULL);
			mesh->drop();

			m_meshnode->setScale(v3f(1));
			// Will be shown when we know the brightness
			m_meshnode->setVisible(false);
		} else if (m_prop->visual == "cube_disorted") {
			infostream<<"LuaEntityCAO::addToScene(): irregular_cube"<<std::endl;
			scene::IMesh *mesh = createCubeMesh(v3f(BS,BS,BS), m_prop->collisionbox);
			m_meshnode = smgr->addMeshSceneNode(mesh, NULL);
			mesh->drop();

			m_meshnode->setScale(v3f(1));
			// Will be shown when we know the brightness
			m_meshnode->setVisible(false);
		} else {
			infostream<<"LuaEntityCAO::addToScene(): \""<<m_prop->visual
					<<"\" not supported"<<std::endl;
		}
		updateTextures("");
		updateNodePos();
	}

	void removeFromScene()
	{
		if(m_meshnode){
			m_meshnode->remove();
			m_meshnode = NULL;
		}
		if(m_spritenode){
			m_spritenode->remove();
			m_spritenode = NULL;
		}
	}

	void updateLight(u8 light_at_pos)
	{
		u8 li = decode_light(light_at_pos);
		video::SColor color(255,li,li,li);
		if(m_meshnode){
			setMeshColor(m_meshnode->getMesh(), color);
			m_meshnode->setVisible(true);
		}
		if(m_spritenode){
			m_spritenode->setColor(color);
			m_spritenode->setVisible(true);
		}
	}

	v3s16 getLightPosition()
	{
		return floatToInt(m_position, BS);
	}

	void updateNodePos()
	{
		if(m_meshnode){
			m_meshnode->setPosition(pos_translator.vect_show);
		}
		if(m_spritenode){
			m_spritenode->setPosition(pos_translator.vect_show);
		}
	}

	void step(float dtime, ClientEnvironment *env)
	{
		if(m_prop->physical){
			core::aabbox3d<f32> box = m_prop->collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			collisionMoveResult moveresult;
			f32 pos_max_d = BS*0.25; // Distance per iteration
			v3f p_pos = m_position;
			v3f p_velocity = m_velocity;
			IGameDef *gamedef = env->getGameDef();
			moveresult = collisionMovePrecise(&env->getMap(), gamedef,
					pos_max_d, box, dtime, p_pos, p_velocity);
			// Apply results
			m_position = p_pos;
			m_velocity = p_velocity;

			bool is_end_position = moveresult.collides;
			pos_translator.update(m_position, is_end_position, dtime);
			pos_translator.translate(dtime);
			updateNodePos();

			m_velocity += dtime * m_acceleration;
		} else {
			m_position += dtime * m_velocity + 0.5 * dtime * dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
			pos_translator.update(m_position, pos_translator.aim_is_end, pos_translator.anim_time);
			pos_translator.translate(dtime);
			updateNodePos();
		}

		m_anim_timer += dtime;
		if(m_anim_timer >= m_anim_framelength){
			m_anim_timer -= m_anim_framelength;
			m_anim_frame++;
			if(m_anim_frame >= m_anim_num_frames)
				m_anim_frame = 0;
		}

		updateTexturePos();
	}

	void updateTexturePos()
	{
		if(m_spritenode){
			scene::ICameraSceneNode* camera =
					m_spritenode->getSceneManager()->getActiveCamera();
			if(!camera)
				return;
			v3f cam_to_entity = m_spritenode->getAbsolutePosition()
					- camera->getAbsolutePosition();
			cam_to_entity.normalize();

			int row = m_tx_basepos.Y;
			int col = m_tx_basepos.X;

			if(m_tx_select_horiz_by_yawpitch)
			{
				if(cam_to_entity.Y > 0.75)
					col += 5;
				else if(cam_to_entity.Y < -0.75)
					col += 4;
				else{
					float mob_dir = atan2(cam_to_entity.Z, cam_to_entity.X) / PI * 180.;
					float dir = mob_dir - m_yaw;
					dir = wrapDegrees_180(dir);
					//infostream<<"id="<<m_id<<" dir="<<dir<<std::endl;
					if(fabs(wrapDegrees_180(dir - 0)) <= 45.1)
						col += 2;
					else if(fabs(wrapDegrees_180(dir - 90)) <= 45.1)
						col += 3;
					else if(fabs(wrapDegrees_180(dir - 180)) <= 45.1)
						col += 0;
					else if(fabs(wrapDegrees_180(dir + 90)) <= 45.1)
						col += 1;
					else
						col += 4;
				}
			}

			// Animation goes downwards
			row += m_anim_frame;

			float txs = m_tx_size.X;
			float tys = m_tx_size.Y;
			setBillboardTextureMatrix(m_spritenode,
					txs, tys, col, row);
		}
	}

	void updateTextures(const std::string &mod)
	{
		ITextureSource *tsrc = m_gamedef->tsrc();

		if(m_spritenode){
			std::string texturestring = "unknown_block.png";
			if(m_prop->textures.size() >= 1)
				texturestring = m_prop->textures[0];
			texturestring += mod;
			m_spritenode->setMaterialTexture(0,
					tsrc->getTextureRaw(texturestring));
		}
		if(m_meshnode){
			for (u32 i = 0; i < 6; ++i)
			{
				std::string texturestring = "unknown_block.png";
				if(m_prop->textures.size() > i)
					texturestring = m_prop->textures[i];
				texturestring += mod;
				AtlasPointer ap = tsrc->getTexture(texturestring);

				// Get the tile texture and atlas transformation
				video::ITexture* atlas = ap.atlas;
				v2f pos = ap.pos;
				v2f size = ap.size;

				// Set material flags and texture
				video::SMaterial& material = m_meshnode->getMaterial(i);
				material.setFlag(video::EMF_LIGHTING, false);
				material.setFlag(video::EMF_BILINEAR_FILTER, false);
				material.setTexture(0, atlas);
				material.getTextureMatrix(0).setTextureTranslate(pos.X, pos.Y);
				material.getTextureMatrix(0).setTextureScale(size.X, size.Y);
			}
		}
	}

	void processMessage(const std::string &data)
	{
		std::istringstream is(data, std::ios::binary);
		// command
		u8 cmd = readU8(is);
		if(cmd == 0) // update position
		{
			// do_interpolate
			bool do_interpolate = readU8(is);
			// pos
			m_position = readV3F1000(is);
			// velocity
			m_velocity = readV3F1000(is);
			// acceleration
			m_acceleration = readV3F1000(is);
			// yaw
			m_yaw = readF1000(is);
			// is_end_position (for interpolation)
			bool is_end_position = readU8(is);
			// update_interval
			float update_interval = readF1000(is);

			if(do_interpolate){
				if(!m_prop->physical)
					pos_translator.update(m_position, is_end_position, update_interval);
			} else {
				pos_translator.init(m_position);
			}
			updateNodePos();
		}
		else if(cmd == 1) // set texture modification
		{
			std::string mod = deSerializeString(is);
			updateTextures(mod);
		}
		else if(cmd == 2) // set sprite
		{
			v2s16 p = readV2S16(is);
			int num_frames = readU16(is);
			float framelength = readF1000(is);
			bool select_horiz_by_yawpitch = readU8(is);

			m_tx_basepos = p;
			m_anim_num_frames = num_frames;
			m_anim_framelength = framelength;
			m_tx_select_horiz_by_yawpitch = select_horiz_by_yawpitch;

			updateTexturePos();
		}
	}
};

// Prototype
LuaEntityCAO proto_LuaEntityCAO(NULL, NULL);
